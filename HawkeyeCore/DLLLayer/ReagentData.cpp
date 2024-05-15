#include "stdafx.h"

#include <algorithm>
#include <sys/timeb.h>

#include "ReagentData_p.hpp"
#include "ReagentData.hpp"

#include "CellHealthReagents.hpp"
#include "HawkeyeConfig.hpp"
#include "ReagentControllerBase.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "ReagentData";

static std::shared_ptr<ReagentDataPrivate> impl;

//*****************************************************************************
void ReagentData::Initialize()
{
	if (impl)
	{
		return;
	}

	impl = std::make_shared<ReagentDataPrivate>();
}

//*****************************************************************************
std::vector<AnalysisDefinitionDLL>& ReagentData::GetAnalysisDefinitions()
{
	return impl->analysisDefinitions;
}

//*****************************************************************************
std::vector<ReagentCleaningInstruct_t>& ReagentData::GetCleaningInstructions()
{
	return impl->cleaningInstructions;
}

//*****************************************************************************
std::vector<ReagentContainerStateDLL>& ReagentData::GetReagentContainerStates()
{
	return impl->reagentContainerStates;
}

//*****************************************************************************
std::vector<ReagentInfoRecordDLL>& ReagentData::GetReagentInfoRecords()
{
	return impl->reagentInfoRecords;
}

//*****************************************************************************
std::map<uint16_t, ReagentDefinitionDLL>& ReagentData::GetReagentDefinitions()
{
	return impl->reagentDefinitions;
}

//*****************************************************************************
std::map<size_t, std::vector<ReagentValveMap_t>>& ReagentData::GetReagentValveMaps()
{
	return impl->reagentValveMaps;
}

//*****************************************************************************
// Effective Reagent Expiration date in days
//*****************************************************************************
static uint32_t getContainerEffectiveExp (const RfidTag_t& rfid_tag)
{
	uint32_t effective_exp_date = 0;

	const boost::gregorian::date today = boost::gregorian::day_clock::local_day();
	const boost::gregorian::date local_epoch = boost::gregorian::from_simple_string("1970-01-01");
	const boost::gregorian::days now_days = today - local_epoch;

	// If pack is not yet in service, then expiration is the general expiration date
	if (rfid_tag.inServiceDate == 0)
		return rfid_tag.packExp;

	const auto service_expiration = rfid_tag.inServiceDate + rfid_tag.serviceLife;
		
	// Important Condition:  if the user tries to cheat the system by rolling back the system date, this check will be executed.
	if (rfid_tag.inServiceDate > static_cast<uint32_t>(now_days.days()))
		return 0;

	return std::min (service_expiration, rfid_tag.packExp);
}

//*****************************************************************************
static ReagentContainerPosition getReagentContainerPosition (const RfidTag_t& rfidTag)
{
	auto position = ReagentControllerBase::GetReagentContainerPosition(rfidTag);
	switch (position)
	{
		case ReagentControllerBase::ReagentPosition::eMainBay:
			return ReagentContainerPosition::eMainBay_1;
		case ReagentControllerBase::ReagentPosition::eDoorLeftBay:
			return ReagentContainerPosition::eDoorLeft_2;
		case ReagentControllerBase::ReagentPosition::eDoorRightBay:
			return ReagentContainerPosition::eDoorRight_3;
		default:
			return ReagentContainerPosition::eUnknown;
	}
}

//*****************************************************************************
static instrument_error::reagent_pack_instance getReagentPackInstance(
	const ReagentContainerPosition& position)
{
	switch (position)
	{
		case ReagentContainerPosition::eMainBay_1:
			return instrument_error::reagent_pack_instance::main_bay;
		case ReagentContainerPosition::eDoorRight_3:
			return instrument_error::reagent_pack_instance::door_right;
		case ReagentContainerPosition::eDoorLeft_2:
			return instrument_error::reagent_pack_instance::door_left;
		default:
			return instrument_error::reagent_pack_instance::general;
	}
}

//*****************************************************************************
bool ReagentData::isPackUsable() const
{
	// Internally we don't need to check whether pack is primed or not.
	if (isExpired() || isEmpty())
	{
		return false;
	}

	return true;
}

//*****************************************************************************
bool ReagentData::isExpired() const
{
	return impl->isExpired;
}

//*****************************************************************************
// returns true is the reagent pack is expired or has less time remaining than required
// returns the effective days left on the pack through the passed reference parameter;
// expired packs return 0;
// all other conditions return (item.exp_date - now_days.days() + 1) since the
// last day will evaluate to 0 remaining days using item.exp_date - now_days.days()
//*****************************************************************************
bool ReagentData::isNearExpiration (bool reportError, uint32_t &daysRemaining, uint64_t minutesRequired) const
//// TODO
/*
THIS FUNCTION MAY NOT BE IMPLEMENTED RIGHT FOR HUNTER
*/
////
{
	if (isExpired() || impl->reagentContainerStates.empty())
	{
		daysRemaining = 0;
		return true;
	}

	boost::gregorian::date today = boost::gregorian::day_clock::local_day();
	boost::gregorian::date local_epoch = boost::gregorian::from_simple_string("1970-01-01");
	boost::gregorian::days now_days = today - local_epoch;

	struct _timeb utm;
	_ftime_s(&utm);

	// calculate the number of minutes left until midnight, accounting for timezone and daylight saving time offsets
	uint64_t nowMins = utm.time / 60;	// UTC minutes since epoch

	nowMins -= utm.timezone;			// timezone offset is signed minutes
	nowMins += utm.dstflag > 0 ? 60 : 0;

	uint64_t dayMinsLeft = 1440 - (nowMins % 1440);

	int64_t daysLeft = INT64_MAX;

	for (const auto& item : impl->reagentContainerStates)
	{
		int64_t remaining = static_cast<int64_t>(item.exp_date) - now_days.days();

		if (remaining < 0)
		{
			if (reportError)
			{
				auto reagent_pack_instance = getReagentPackInstance(item.position);
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::reagent_pack_expired, 
					reagent_pack_instance, 
					instrument_error::severity_level::warning));
				Logger::L().Log (MODULENAME, severity_level::warning, "isNearExpiration: Reagent pack is expired");
			}
			daysRemaining = 0;
			return true;
		}

		else if (remaining == 0)
		{
			daysLeft = 0;
			if (dayMinsLeft < minutesRequired)
			{
				auto reagent_pack_instance = getReagentPackInstance(item.position);
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::reagent_pack_expired, 
					reagent_pack_instance, 
					instrument_error::severity_level::warning));
				Logger::L().Log (MODULENAME, severity_level::warning, "isNearExpiration: Reagent remaining time less than required, remaining: " + std::to_string(dayMinsLeft) + "  required: " + std::to_string(minutesRequired));
				daysRemaining = 0;
				return true;
			}
			break;
		}
		else
		{
			if (remaining < daysLeft)
			{
				daysLeft = remaining;
			}
		}
	}

	if (daysLeft == INT64_MAX)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "isNearExpiration: Unable to determine remaing reagent time!");
		daysLeft = 0;
	}
	else if (daysLeft == 0)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "isNearExpiration: Reagent pack last day!");
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "isNearExpiration: Reagent remaining days: " + std::to_string(daysLeft + 1));
	}

	daysRemaining = static_cast<uint32_t>(daysLeft+1);

	return false;
}

//*****************************************************************************
bool ReagentData::isEmpty() const
{
	return impl->isEmpty;
}

//*****************************************************************************
void ReagentData::decodeRfidTag (const RfidTag_t& rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "decodeRfidTag: <enter>");

	impl->reagentContainerStates.emplace_back (readReagentContainerState(rfidTag));
	const auto& rcs = impl->reagentContainerStates.back();
	for (size_t index = 0; index < rcs.reagent_states.size(); index++)
	{
		ReagentValveMap_t reagentValveMap;

		reagentValveMap.parameter.valve_number = SyringePumpPort::ToPhysicalPort(
			SyringePumpPort::Port(rcs.reagent_states[index].valve_location));
		reagentValveMap.parameter.tag_index = rfidTag.tagIndex;
		reagentValveMap.parameter.parameter_index = rfidTag.remainingReagentOffsets.at(index);
		reagentValveMap.parameter.parameter_length = sizeof(rcs.reagent_states[index].events_remaining);

		impl->reagentValveMaps[impl->reagentContainerStates.size() - 1].emplace_back (reagentValveMap);
	}

	uint8_t cleanser_count = rfidTag.numCleaners & 0x0F;  // lower nibble gives the cleansers information
	if (cleanser_count != 0)
	{
		auto cleanerDefinitions = readCleanerDefinitions(rfidTag);
		for (const auto& item : cleanerDefinitions)
		{
			if (impl->reagentDefinitions.find(item.reagent_index) == impl->reagentDefinitions.end())
			{
				impl->reagentDefinitions[item.reagent_index] = item;
			}
		}
	}

	if (rfidTag.hasReagent)
	{
		append (impl->cleaningInstructions, std::move(readCleaningInstructions(rfidTag)));

		auto reagentDefinitions = readReagentDefinitions(rfidTag);
		for (const auto& item : reagentDefinitions)
		{
			if (impl->reagentDefinitions.find(item.reagent_index) == impl->reagentDefinitions.end())
			{
				impl->reagentDefinitions[item.reagent_index] = item;
			}
		}
	}

	append (impl->analysisDefinitions, std::move(readAnalysisDefinitions(rfidTag)));
	append (impl->reagentInfoRecords, std::move(readReagentInfo(rfidTag)));

	Logger::L().Log (MODULENAME, severity_level::debug2, "decodeRfidTag: <exit>");
}

//*****************************************************************************
uint16_t ReagentData::getContainerRemainingEvents (const RfidTag_t& rfid_tag)
{
	uint16_t remaining_events = std::numeric_limits<uint16_t>::max();
	std::vector<Cleaner_t> cleaners = DataConversion::create_vector_from_Clist (rfid_tag.cleaners, rfid_tag.numCleaners);

	for (const auto& cleaner : cleaners)
	{
		// Container events remaining is the minimum remaining events of the reagents.
		remaining_events = std::min (cleaner.remainingUses, remaining_events);
	}

	if (rfid_tag.hasReagent)
	{
		remaining_events = std::min (rfid_tag.reagent.remainingUses, remaining_events);
	}

	return remaining_events;
}

//*****************************************************************************
ReagentContainerStateDLL ReagentData::readReagentContainerState (const RfidTag_t& rfidTag)
{
//// TODO
/*
THIS FUNCTION MAY NOT BE IMPLEMENTED RIGHT FOR HUNTER
*/
////
	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentContainerState: <enter>");

	ReagentContainerStateDLL rcs = {};
	rcs.status = ReagentContainerStatus::eInvalid;

	//NOTE: Since for CellHealth there is no reagent pack always consider it okay..
	// The "events_remaining" will ensure that no samples are run w/o reagents.
	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		rcs.events_remaining = CellHealthReagents::GetRemainingReagentUses();
		impl->isEmpty = false;
	}
	else
	{
		if (rcs.events_remaining == 0)
		{
			auto reagent_pack_instance = getReagentPackInstance(rcs.position);
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::reagent_pack_empty,
				reagent_pack_instance,
				instrument_error::severity_level::warning));
			Logger::L().Log (MODULENAME, severity_level::warning, "isEmpty: Reagent pack is empty");

			impl->isEmpty = true;
		}
		else
		{
			impl->isEmpty = false;
		}
	}

	// Populate the reagent pack definitions from the RFID tag data.
	memcpy(rcs.identifier, rfidTag.tagSN, sizeof(rfidTag.tagSN));
	rcs.bci_part_number = rfidTag.packPn;
	rcs.exp_date = getContainerEffectiveExp(rfidTag);
	rcs.in_service_date = rfidTag.inServiceDate;
	rcs.lot_information = boost::str(boost::format("%d") % rfidTag.packLotNum);

	uint8_t cleanser_count = rfidTag.numCleaners & 0x0F;  // lower nibble gives the cleansers information
	if (cleanser_count != 0)
	{
		append(rcs.reagent_states, std::move(readCleanerStates(rfidTag)));
	}

	if (rfidTag.hasReagent)
	{
		append(rcs.reagent_states, std::move(readReagentStates(rfidTag)));
	}

	// Update the container position here
	rcs.position = getReagentContainerPosition(rfidTag);

	// Now update the Reagent Container Status based on expiration and Reagent container Position
	// TODO shall we do this check in the beginning and return false if expired?

	boost::gregorian::date today = boost::gregorian::day_clock::local_day();
	boost::gregorian::date local_epoch = boost::gregorian::from_simple_string("1970-01-01");
	boost::gregorian::days now_days = today - local_epoch;

	auto now_in_days = now_days.days();
	auto msg = boost::str (boost::format("isExpired:\nexpiration_in_days: %5d\n       now_in_days: %5d\n         days_left: %5d")
		% std::to_string(rcs.exp_date)
		% std::to_string(now_in_days)
		% std::to_string(static_cast<int32_t>(rcs.exp_date) - static_cast<int32_t>(now_in_days)));

	static std::map<std::size_t, std::string> log_msg_cache;
	if (log_msg_cache[0] != msg)
	{
		log_msg_cache[0] = msg;
		Logger::L().Log (MODULENAME, severity_level::normal, msg);
	}

	impl->isExpired = static_cast<uint32_t>(now_days.days()) > getContainerEffectiveExp(rfidTag);
	if (impl->isExpired || rcs.position == ReagentContainerPosition::eUnknown)
	{
		rcs.events_remaining = 0;

		auto reagent_pack_instance = getReagentPackInstance(rcs.position);
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_expired,
			reagent_pack_instance,
			instrument_error::severity_level::warning));
		Logger::L().Log (MODULENAME, severity_level::warning, "isExpired: Reagent pack is expired");

		Logger::L().Log (MODULENAME, severity_level::warning,
			boost::str (boost::format ("Identifier: 0x%02X%02X%02X%02X%02X")
				% int(rcs.identifier[0])
				% int(rcs.identifier[1])
				% int(rcs.identifier[2])
				% int(rcs.identifier[3])
				% int(rcs.identifier[4])));

		std::string str;
		for (auto& v : rcs.reagent_states)
		{
			if (v.events_remaining == 0)
			{
				str += boost::str (boost::format ("\n   reagent_index: %d\nevents_remaining: %d")
					% v.reagent_index
					% v.events_remaining);
			}
		}

		Logger::L().Log (MODULENAME, severity_level::warning, str);
	}

	if (!impl->isEmpty && !impl->isExpired)
	{
		rcs.status = ReagentContainerStatus::eOK;
	}
	
	for (size_t index = 0; index < rcs.reagent_states.size(); index++)
	{
		const auto& item = rcs.reagent_states[index];
		if (item.valve_location == SyringePumpPort::Port::InvalidPort)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "readReagentContainerState: invalid valve location for reagent status with index: " + std::to_string(index));
			rcs.status = ReagentContainerStatus::eInvalid;
			break;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentContainerState: <exit>");

	return rcs;
}

//*****************************************************************************
std::vector<ReagentDefinitionDLL> ReagentData::readCleanerDefinitions (const RfidTag_t& rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readCleanerDefinitions: <enter>");
	std::vector<ReagentDefinitionDLL> definitionList;
	definitionList.reserve(rfidTag.numCleaners);

	for (uint8_t index = 0; index < rfidTag.numCleaners; index++)
	{
		ReagentDefinitionDLL rd = {};
		rd.reagent_index = rfidTag.cleaners[index].index;
		rd.label = rfidTag.cleaners[index].desc;
		rd.mixing_cycles = 1;
		definitionList.emplace_back(rd);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readCleanerDefinitions: <exit>");
	return definitionList;
}

//*****************************************************************************
std::vector<ReagentStateDLL> ReagentData::readCleanerStates (const RfidTag_t& rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readCleanerStates: <enter>");
	std::vector<ReagentStateDLL> stateList;
	stateList.reserve(rfidTag.numCleaners);

	for (uint8_t index = 0; index < rfidTag.numCleaners; index++)
	{
		ReagentStateDLL rs = {};
		rs.reagent_index = rfidTag.cleaners[index].index;
		rs.lot_information = rfidTag.cleaners[index].partNum;
		rs.events_possible = rfidTag.cleaners[index].totalUses;
		rs.events_remaining = rfidTag.cleaners[index].remainingUses;
		rs.valve_location = ReagentControllerBase::convertToSyringePort(
			ReagentControllerBase::GetReagentPosition(rfidTag, index, true));

		stateList.emplace_back(rs);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readCleanerStates: <exit>");
	return stateList;
}

//*****************************************************************************
std::vector<ReagentCleaningInstruct_t> ReagentData::readCleaningInstructions (const RfidTag_t & rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readCleaningInstructions: <enter>");
	std::vector<ReagentCleaningInstruct_t> cleanInstructionList;

	if(rfidTag.hasReagent)
	{
		cleanInstructionList.reserve(rfidTag.reagent.numCleaningInstructions);

		for (uint8_t i = 0; i < rfidTag.reagent.numCleaningInstructions; i++)
		{
			ReagentCleaningInstruct_t clean = {};
			clean.cleaning_index = rfidTag.reagent.cleaning_instructions[i].cleaner_index;
			clean.bitvalue = rfidTag.reagent.cleaning_instructions[i].cleaning_instruction;
			
			cleanInstructionList.emplace_back(clean);
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readCleaningInstructions: <exit>");
	return cleanInstructionList;
}

//*****************************************************************************
std::vector<ReagentDefinitionDLL> ReagentData::readReagentDefinitions (const RfidTag_t & rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentDefinitions: <enter>");

	std::vector<ReagentDefinitionDLL> definitionList;

	if (rfidTag.hasReagent)
	{
		ReagentDefinitionDLL definition = {};
		definition.reagent_index = rfidTag.reagent.index;
		definition.label = rfidTag.reagent.desc;
		definition.mixing_cycles = rfidTag.reagent.cycles;

		definitionList.emplace_back(definition);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentDefinitions: <exit>");
	return definitionList;
}

//*****************************************************************************
std::vector<ReagentStateDLL> ReagentData::readReagentStates (const RfidTag_t& rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentStates: <enter>");

	std::vector<ReagentStateDLL> stateList;

	if (rfidTag.hasReagent)
	{
		ReagentStateDLL state = {};
		uint8_t reagent_location = rfidTag.numCleaners >> 4;
		state.reagent_index = rfidTag.reagent.index;
		state.lot_information = rfidTag.reagent.partNum;
		state.events_possible = rfidTag.reagent.totalUses;
		state.events_remaining = rfidTag.reagent.remainingUses;
		state.valve_location = ReagentControllerBase::convertToSyringePort(
			ReagentControllerBase::GetReagentPosition(rfidTag, reagent_location, false));

		stateList.emplace_back(state);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentStates: <exit>");
	return stateList;
}

//*****************************************************************************
std::vector<AnalysisDefinitionDLL> ReagentData::readAnalysisDefinitions (const RfidTag_t & rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readAnalysisDefinitions: <enter>");

	std::vector<AnalysisDefinitionDLL> analysisDefList;
	analysisDefList.reserve(rfidTag.numAnalyses);

	for (uint8_t i = 0; i < rfidTag.numAnalyses; i++)
	{
		AnalysisDefinitionDLL ad = {};

		ad.analysis_index = rfidTag.analyses[i].index;
		ad.label = rfidTag.analyses[i].desc;

		ad.reagent_indices.reserve(rfidTag.analyses[i].numReagents);
		for (uint8_t j = 0; j < rfidTag.analyses[i].numReagents; j++)
		{
			ad.reagent_indices.emplace_back(rfidTag.analyses[i].reagentIndices[j]);
		}

		ad.fl_illuminators.reserve(rfidTag.analyses[i].numFL_Illuminators);
		for (uint8_t j = 0; j < rfidTag.analyses[i].numFL_Illuminators; j++)
		{
			FL_IlluminationSettings flis;
			flis.exposure_time_ms = rfidTag.analyses[i].illuminators[j].exposure_time_ms;
			flis.illuminator_wavelength_nm = rfidTag.analyses[i].illuminators[j].illuminator_wavelength_nm;
			flis.emission_wavelength_nm = rfidTag.analyses[i].illuminators[j].emission_wavelength_nm;

			ad.fl_illuminators.emplace_back(flis);
		}

		AnalysisParameterDLL pop_ap;
		pop_ap.characteristic.key = rfidTag.analyses[i].populationParameterKey.key;
		pop_ap.characteristic.s_key = rfidTag.analyses[i].populationParameterKey.subKey;
		pop_ap.characteristic.s_s_key = 0;
		pop_ap.threshold_value = rfidTag.analyses[i].populationParameterKey.thresholdValue;
		pop_ap.above_threshold = rfidTag.analyses[i].populationParameterKey.aboveThreshold == 0 ? false : true;
		ad.population_parameter = pop_ap;

		ad.analysis_parameters.reserve(rfidTag.analyses[i].numParameters);
		for (uint8_t j = 0; j < rfidTag.analyses[i].numParameters; j++)
		{
			AnalysisParameterDLL ap;
			ap.characteristic.key = rfidTag.analyses[i].parameters[j].key;
			ap.characteristic.s_key = rfidTag.analyses[i].parameters[j].subKey;
			ap.threshold_value = rfidTag.analyses[i].parameters[j].thresholdValue;
			ap.above_threshold = (rfidTag.analyses[i].parameters[j].aboveThreshold == 0 ? false : true);

			ad.analysis_parameters.emplace_back(ap);
		}

		analysisDefList.emplace_back(ad);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readAnalysisDefinitions: <exit>");
	return analysisDefList;
}

//*****************************************************************************
std::vector<ReagentInfoRecordDLL> ReagentData::readReagentInfo (const RfidTag_t & rfidTag)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentInfo: <enter>");

	auto ToReagentInfoRecord = [](const RfidTag_t&rtag, const char* desc)->ReagentInfoRecordDLL
	{
		ReagentInfoRecordDLL rir_temp;

		rir_temp.pack_number = rtag.packPn;
		rir_temp.lot_number = rtag.packLotNum;
		rir_temp.reagent_label = std::string(desc);

		rir_temp.expiration_date = rtag.packExp;
		rir_temp.in_service_date = rtag.inServiceDate;
		rir_temp.effective_expiration_date = getContainerEffectiveExp(rtag);

		return rir_temp;
	};

	std::vector<ReagentInfoRecordDLL> reagentinfoList;
	const auto size = rfidTag.numCleaners + (rfidTag.hasReagent ? 1 : 0);
	reagentinfoList.reserve(size);

	// For Cleaning agents.
	for (uint8_t count = 0; count < rfidTag.numCleaners; count++)
	{
		reagentinfoList.emplace_back(ToReagentInfoRecord(rfidTag, rfidTag.cleaners[count].desc));
	}

	// For Reagent.
	if (rfidTag.hasReagent)
	{
		reagentinfoList.emplace_back(ToReagentInfoRecord(rfidTag, rfidTag.reagent.desc));
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "readReagentInfo: <exit>");
	return reagentinfoList;
}

//*****************************************************************************
void ReagentData::Decode (const std::vector<RfidTag_t>& rfidTags)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Decode: <enter>");

	if (rfidTags.empty())
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_rfid_rfiderror, 
			instrument_error::severity_level::warning));
		return;
	}

	clear();

	for (auto& v : rfidTags)
	{
		decodeRfidTag (v);
	}
}

//*****************************************************************************
void ReagentData::SetToInvalid()
{
	if (!impl) return;

	if (impl->reagentContainerStates.empty()) return;
	
	impl->reagentContainerStates[0].events_remaining = 0;
	impl->reagentContainerStates[0].status = ReagentContainerStatus::eInvalid;
	memset(impl->reagentContainerStates[0].identifier, 0, sizeof(ReagentContainerStateDLL::identifier));
}

//*****************************************************************************
void ReagentData::clear()
{
//	std::cout << "clear() called" << std::endl;
	impl->clear();
}

//*****************************************************************************
bool ReagentData::addMissingReagentDefinitions(std::map<uint16_t, ReagentDefinitionDLL>& input)
{
	bool missingEntryFound = false;

	for (const auto& item : input)
	{
		// Found entry in "input" which is not present in current "reagentDefinitionsDLL".
		if (impl->reagentDefinitions.find(item.first) == impl->reagentDefinitions.end())
		{
			missingEntryFound = true;
			impl->reagentDefinitions[item.first] = item.second;
		}
	}

	for (const auto& item : impl->reagentDefinitions)
	{
		// Found entry in "reagentDefinitionsDLL" which is not present in current "input".
		if (input.find(item.first) == input.end())
		{
			missingEntryFound = true;
			input[item.first] = item.second;
		}
	}

	return missingEntryFound;
}
