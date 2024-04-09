#include "stdafx.h"

#include "ReagentPack_p.hpp"
#include "ReagentPack.hpp"

#include "AppConfig.hpp"
#include "AuditLog.hpp"
#include "CellHealthReagents.hpp"
#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include <DBif_Api.h>
#include "FileSystemUtilities.hpp"
#include "GetAsStrFunctions.hpp"
#ifndef DATAIMPORTER
#include "Hardware.hpp"
#endif
#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeServices.hpp"
#include "Logger.hpp"
#include "Reagent.hpp"
#include "ReagentCommon.hpp"
#include "ReagentData.hpp"
#include "ReagentDLL.hpp"
#include "ReagentLoadWorkflow.hpp"
#include "ReagentUnloadWorkflow.hpp"
#ifndef DATAIMPORTER
#include "Rfid.hpp"
#endif
#include "SetIndentation.hpp"
#include "SyringePumpPort.hpp"
#include "SystemErrors.hpp"
#include "UserList.hpp"
#include "Workflow.hpp"
#include "WorkflowController.hpp"

static const char MODULENAME[] = "ReagentPack";

static int consecReadErrors_ = 0;
static const int MAX_CONSEC_READ_FAILS = 3;

static bool stopContinuousReading = false;
static std::map<uint16_t, ReagentDefinitionDLL> cachedReagentDefinitions_;
static std::unique_ptr<ReagentPackPrivate> impl; // Variables related to the functioning of the ReagentPack class.
static std::shared_ptr<boost::asio::deadline_timer> readRfidTagsTimer_;

//*****************************************************************************
ReagentPack::ReagentPack()
{
}

//*****************************************************************************
ReagentPack::~ReagentPack()
{
	stopContinuousReading = true;
	if (readRfidTagsTimer_)
	{
		readRfidTagsTimer_->cancel();
	}
}

//*****************************************************************************
// Only for upgrading v1.2...
// The assumption is that the DB is empty.
//*****************************************************************************
void ReagentPack::Import (boost::property_tree::ptree& ptParent) {

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <enter>");

	auto reagentPack = ptParent.equal_range("reagent_container");
	for (auto it = reagentPack.first; it != reagentPack.second; ++it)
	{
		std::string tagSn = it->second.get<std::string>("tag_sn");

		std::string reformattedTagSn = boost::str(boost::format("%02x:%02x:%02x:%02x:%02x")
			% tagSn.substr(0, 2)
			% tagSn.substr(2, 2)
			% tagSn.substr(4, 2)
			% tagSn.substr(6, 2)
			% tagSn.substr(8, 2)
		);

		boost::to_upper (reformattedTagSn);

//TODO: save to test with next week...
		//saveCurrentPack (reformattedTagSn);

		DBApi::DB_ReagentTypeRecord dbReagentRecord = {};
		dbReagentRecord.ContainerTagSn = reformattedTagSn;
		dbReagentRecord.Current = true;

		DBApi::eQueryResult dbStatus = DBApi::DbAddReagentInfo (dbReagentRecord);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Import: <exit, DbAddReagentInfo failed, status: %ld>") % (int32_t)dbStatus));
			return;
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <exit>");
}


#ifndef DATAIMPORTER
//*****************************************************************************
std::map<uint16_t, ReagentDefinitionDLL> getReagentDefinitions()
{
	return cachedReagentDefinitions_;
}

//*****************************************************************************
std::string formatTagIdentifier (const uint8_t identifier[8])
{
	auto tagSn = boost::str(boost::format("%02x:%02x:%02x:%02x:%02x")
		% int(identifier[0])
		% int(identifier[1])
		% int(identifier[2])
		% int(identifier[3])
		% int(identifier[4])
	);

	boost::to_upper (tagSn);

	return tagSn;
}

//*****************************************************************************
std::string stringifyReagentContainerState (ReagentContainerStateDLL rcs, uint8_t tabCount = 0)
{
	const std::string in_service_date_str = ChronoUtilities::ConvertToDateString (rcs.in_service_date);
	const std::string exp_date_str = ChronoUtilities::ConvertToDateString (rcs.exp_date);

	return
		SetIndentation (tabCount, false) + "Reagent container ID: " + formatTagIdentifier(rcs.identifier) +
		SetIndentation (tabCount + 1) + "In-service date: " + in_service_date_str +
		SetIndentation (tabCount + 1) + "Expiration date: " + exp_date_str +
		SetIndentation (tabCount + 1) + "Lot information: " + rcs.lot_information +
		SetIndentation (tabCount + 1) + boost::str (boost::format ("Events remaining: %d") % rcs.events_remaining);
}

//*****************************************************************************
static void reagentDefinitionToDLL (ReagentDefinitionDLL& to, const ReagentDefinition& from)
{
	to.reagent_index = from.reagent_index;
	to.label = std::string (from.label);
	to.mixing_cycles = from.mixing_cycles;
}

//*****************************************************************************
void reagentDefinitionFromDLL (ReagentDefinition& to, const ReagentDefinitionDLL& from) {

	to.reagent_index = from.reagent_index;
	strncpy_s (to.label, sizeof (to.label), from.label.c_str (), from.label.size ());
	to.mixing_cycles = from.mixing_cycles;
}

//*****************************************************************************
static void reagentStateToDLL (ReagentStateDLL& to, const ReagentState& from) {

	to.reagent_index = from.reagent_index;
	DataConversion::convertToStandardString (to.lot_information, from.lot_information);
	to.events_possible = from.events_possible;
	to.events_remaining = from.events_remaining;
	to.valve_location = static_cast<SyringePumpPort::Port>(from.valve_location);
}

//*****************************************************************************
void reagentStateFromDLL (ReagentState& to, const ReagentStateDLL& from) {

	to.reagent_index = from.reagent_index;
	DataConversion::convertToCharPointer (to.lot_information, from.lot_information);
	to.events_possible = from.events_possible;
	to.events_remaining = from.events_remaining;
	to.valve_location = static_cast<uint8_t>(from.valve_location);
}

//*****************************************************************************
static void reagentContainerStateToDLL (ReagentContainerStateDLL& to, const ReagentContainerState& from) {

	memcpy_s (to.identifier, sizeof (to.identifier), from.identifier, sizeof (to.identifier));
	DataConversion::convertToStandardString (to.bci_part_number, from.bci_part_number);
	to.in_service_date = from.in_service_date;
	to.exp_date = from.exp_date;

	DataConversion::convertToStandardString (to.lot_information, from.lot_information);
	to.status = from.status;
	to.events_remaining = from.events_remaining;
	to.position = from.position;

	to.reagent_states.clear();
	for (uint8_t i = 0; i < from.num_reagents; i++) {
		ReagentStateDLL rs;
		reagentStateToDLL (rs, from.reagent_states[i]);
		to.reagent_states.push_back (rs);
	}
}

//*****************************************************************************
void reagentContainerStateFromDLL (ReagentContainerState& to, const ReagentContainerStateDLL& from) {

	memcpy_s (to.identifier, sizeof (to.identifier), from.identifier, sizeof (to.identifier));

	DataConversion::convertToCharPointer (to.bci_part_number, from.bci_part_number);

	to.in_service_date = (uint32_t)from.in_service_date;
	to.exp_date = (uint32_t)from.exp_date;

	DataConversion::convertToCharPointer (to.lot_information, from.lot_information);

	to.status = from.status;

	to.events_remaining = from.events_remaining;
	to.position = from.position;

	to.num_reagents = static_cast<uint8_t>(from.reagent_states.size ());
	if (to.num_reagents > 0)
	{
		to.reagent_states = new ReagentState[to.num_reagents];
		for (uint8_t i = 0; i < to.num_reagents; i++)
		{
			reagentStateFromDLL (to.reagent_states[i], from.reagent_states[i]);
		}
	}
	else
	{
		to.reagent_states = nullptr;
	}
}

//*****************************************************************************
static void reagentContainerUnloadOptionToDLL (ReagentContainerUnloadOptionDLL& to, ReagentContainerUnloadOption& from)
{
	//TODO: need to investigate for Hunter, we might not need container id.
	//location_id = *reinterpret_cast<const uint8_t*> (container_opt.container_id);
	to.location_id = 0;
	to.container_action = from.container_action;
	memcpy_s (to.container_id, sizeof (to.container_id), from.container_id, sizeof (from.container_id));
}

//*****************************************************************************
static std::string GetTagSnAsString()
{
	std::vector<ReagentContainerStateDLL> states = impl->reagentData.GetReagentContainerStates();
	if (states.size() == 0) {
		Logger::L().Log(MODULENAME, severity_level::warning, "GetTagSnAsString: <exit, no reagent state found>");
		return "";
	}
	if ((states[0].identifier[0] == 0) && (states[0].identifier[1] == 0) &&
		(states[0].identifier[2] == 0) && (states[0].identifier[3] == 0) &&
		(states[0].identifier[4] == 0))
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "GetTagSnAsString: <exit, no reagent tag found>");
		return "";
	}

	return formatTagIdentifier(states[0].identifier);
}

//*****************************************************************************
void ReagentPack::saveCurrentPack (const std::string tagSn)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "saveCurrentPack: <enter>");

	std::string tagNum = tagSn;

	boost::to_upper (tagNum);

	try
	{
		if (tagNum.length() == 0)
			return;

		static std::vector<DBApi::DB_ReagentTypeRecord> allPacks = {};
		DBApi::eQueryResult qr = DBApi::DbGetReagentInfoList(allPacks);
		if (qr == DBApi::eQueryResult::QueryOk)
		{
			for (auto rec = allPacks.begin(); rec != allPacks.end(); ++rec)
			{
				if ((rec->Current) && (rec->ContainerTagSn != tagNum))
				{
					rec->Current = false;
					DBApi::eQueryResult qr = DBApi::DbModifyReagentInfo(*rec);
					if (qr != DBApi::eQueryResult::QueryOk)
					{
						Logger::L().Log(MODULENAME, severity_level::error, "saveCurrentPack: failed to set current to false");
					}
				}
			}
		}

		std::vector<ReagentContainerStateDLL> states = impl->reagentData.GetReagentContainerStates();

		// Find the record with the given serial number
		DBApi::DB_ReagentTypeRecord rrec = {};
		bool found = false;
		for (auto rec = allPacks.begin(); rec != allPacks.end(); ++rec)
		{
			if (rec->ContainerTagSn == tagNum)
			{

				rrec = *rec;
				found = true;
				break;
			}
		}

		if (!found)
		{
			rrec.ContainerTagSn = tagNum;
		}
		rrec.LotNumStr = states[0].lot_information;
		rrec.Current = true;
		rrec.PackPartNumStr = std::string(states[0].bci_part_number);
		rrec.LotExpirationDate = states[0].exp_date;
		rrec.InServiceDate = states[0].in_service_date;
		rrec.InServiceExpirationLength = 90;
		rrec.Protected = false;

		for (auto rd = cachedReagentDefinitions_.begin(); rd != cachedReagentDefinitions_.end(); ++rd)
		{
			if (std::find(rrec.ReagentIndexList.begin(), rrec.ReagentIndexList.end(), rd->second.reagent_index) == rrec.ReagentIndexList.end())
			{
				rrec.ReagentIndexList.push_back(rd->second.reagent_index);
				rrec.ReagentNamesList.push_back(rd->second.label);
				rrec.MixingCyclesList.push_back(rd->second.mixing_cycles);
			}
		}

		Logger::L().Log(MODULENAME, severity_level::debug1, "Saving tag: " + tagNum);

		if (!found)
		{
			DBApi::eQueryResult qr = DBApi::DbAddReagentInfo(rrec);
			if (qr != DBApi::eQueryResult::QueryOk)
				Logger::L().Log(MODULENAME, severity_level::error, "saveCurrentPack: failed to add record");
		}
		else
		{
			DBApi::eQueryResult qr = DBApi::DbModifyReagentInfo(rrec);
			if (qr != DBApi::eQueryResult::QueryOk)
				Logger::L().Log(MODULENAME, severity_level::error, "saveCurrentPack: failed to modify record");
		}

		Logger::L().Log(MODULENAME, severity_level::debug1, "saveCurrentPack: <exit>");
	}
	catch (std::exception ex)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "saveCurrentPack: <exit - exception>");
	}
}

//*****************************************************************************
void ReagentPack::setValveMapForContainer (size_t rcsIndex, size_t valveMapIndex, std::function<void (bool)> callback) const
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "setValveMapForContainer: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onComplete = [=](bool success)
	{
		std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };

		std::map<size_t, std::vector<ReagentValveMap_t>>& valveMaps = impl->reagentData.GetReagentValveMaps();

		if (success)
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "setValveMapForContainer: <exit, success>");
			size_t newValveMapIndex = (valveMapIndex + 1);

			if (newValveMapIndex >= impl->reagentData.GetReagentValveMaps()[rcsIndex].size())
			{
				impl->pReagentPackServices->enqueueInternal (callback, true);
				return;
			}
			setValveMapForContainer (rcsIndex, newValveMapIndex, callback);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::error, "setValveMapForContainer: <exit, Failure>");

		if (valveMaps.find(rcsIndex) != valveMaps.end() && valveMapIndex < valveMaps[rcsIndex].size())
		{
			auto tagIndex = valveMaps[rcsIndex].at (valveMapIndex).parameter.tag_index;
			Logger::L().Log (MODULENAME, severity_level::error, "setValveMapForContainer: failed to set the valve map for tag: index : " + std::to_string (tagIndex));
		}

		impl->pReagentPackServices->enqueueInternal (callback, false);
	};


	impl->pReagentPackServices->enqueueInternal ([=]() -> void
		{
			std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };

			std::map<size_t, std::vector<ReagentValveMap_t>>& valveMaps = impl->reagentData.GetReagentValveMaps();

			if (valveMaps.find(rcsIndex) == valveMaps.end())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setValveMapForContainer: invalid rcs index : " + std::to_string (rcsIndex));
				onComplete (false);
				return;
			}

			if (valveMapIndex >= valveMaps[rcsIndex].size())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setValveMapForContainer: invalid valve map index : " + std::to_string (valveMapIndex));
				onComplete (false);
				return;
			}

			auto currentValveMap = valveMaps[rcsIndex].at(valveMapIndex);
			Hardware::Instance().getReagentController()->setValveMap (onComplete, currentValveMap);
		});
}

//*****************************************************************************
void ReagentPack::setValveMapInternal (size_t rcsIndex, std::function<void (bool)> callback) const
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "setValveMapInternal: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };

	auto& valveMaps = impl->reagentData.GetReagentValveMaps();

	auto onComplete = [=](bool status) -> void
	{
		if (status)
		{
			Logger::L().Log (MODULENAME, severity_level::debug2, "setValveMapInternal: <exit, success>");
			size_t newRcsIndex = (rcsIndex + 1);
			if (valveMaps.find (newRcsIndex) == valveMaps.end())
			{
				impl->pReagentPackServices->enqueueInternal (callback, true);
				return;
			}
			setValveMapInternal (newRcsIndex, callback);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::error, "setValveMapInternal: <exit, Failure>");

		std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };

		auto& rcs = impl->reagentData.GetReagentContainerStates();
		if (rcsIndex < rcs.size())
		{
			rcs[rcsIndex].status = ReagentContainerStatus::eFaulted;
			Logger::L().Log (MODULENAME, severity_level::error, "setValveMapInternal : Failed to set the valve map for pack number : " + rcs[rcsIndex].bci_part_number);
		}
		impl->pReagentPackServices->enqueueInternal (callback, false);
	};

	impl->pReagentPackServices->enqueueInternal ([=]()
		{
			if (valveMaps.find(rcsIndex) == valveMaps.end())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setValveMapInternal : Invalid rcs index : " + std::to_string (rcsIndex));
				onComplete (false);
				return;
			}

			setValveMapForContainer (rcsIndex, 0, onComplete);
		});
}

//*****************************************************************************
void ReagentPack::setValveMap (std::function<void (bool)> callback) const
{
	if (impl->isValveMapSet.load())	// Only allow "setValveMap" to be called once.
	{
		impl->pReagentPackServices->enqueueInternal (callback, true);
		return;
	}

	auto onSetValveMapComplete = [this, callback](bool status) -> void
	{
		impl->isValveMapSet = status;
		impl->isValveMapSetInProgress = false;
		impl->pReagentPackServices->enqueueInternal (callback, status);
	};

	impl->isValveMapSetInProgress = true;
	impl->pReagentPackServices->enqueueInternal ([this, onSetValveMapComplete]()
		{
			setValveMapInternal (0, onSetValveMapComplete);
		});
}

//*****************************************************************************
void ReagentPack::initializeInternal (std::function<void (bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeInternal <enter>");
	SystemStatus::Instance().getData().remainingReagentPackUses = CellHealthReagents::GetRemainingReagentUses();

	readRfidTagsTimer_ = std::make_shared <boost::asio::deadline_timer> (impl->pReagentPackServices->getInternalIosRef());

	auto onReadRfidTagsComplete = [this, callback](bool status) -> void {
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "initializeInternal: RFID read failed");
			// During initialization return *true* regardless of whether there is a reagent pack installed.
			impl->pReagentPackServices->enqueueExternal (callback, true);
			return;
		}

		//NOTE: Any failure with the reagent pack is acceptable during initialization, as user always have 
		// the provision to replace the pack once the system is up and running.
		// BUT, Failure of the reagent hardware is always false condition.

		// Check if the current reagent pack is the same as the last time the instrument was powered up.
		if (!verify_current_pack())
		{
			Logger::L().Log(MODULENAME, severity_level::debug1, "initializeInternal : New/Unknown Reagent pack found");
			impl->isPackInitialized = false;
			SystemStatus::Instance().getData().remainingReagentPackUses = 0;
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::reagent_pack_invalid,
				instrument_error::reagent_pack_instance::main_bay,
				instrument_error::severity_level::warning));


			impl->reagentData.SetToInvalid();
			impl->pReagentPackServices->enqueueExternal (callback, true);
			return;
		}

		if (!impl->reagentData.isPackUsable())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "initializeInternal : Reagent pack is unusable");
			impl->isPackInitialized = false;
			SystemStatus::Instance().getData().remainingReagentPackUses = 0;
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::reagent_pack_invalid,
				instrument_error::reagent_pack_instance::main_bay,
				instrument_error::severity_level::warning));

			impl->reagentData.SetToInvalid();
			impl->pReagentPackServices->enqueueExternal (callback, true);
			return;
		}

		auto onArmDownComplete = [this, callback](bool status) -> void {
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "initializeInternal : Reagent arm down error");

			}
			else
			{
				boost::system::error_code ec;
				stopContinuousReading = false;
				continuouslyReadRfidTags (ec);
				Logger::L().Log (MODULENAME, severity_level::debug2, "initializeInternal <exit>");
			}

			impl->pReagentPackServices->enqueueExternal (callback, status);
		};

		// Reagent pack should go through load sequence to prime the lines and be ready for the real operation.
		// We may expect the reagent arm may not be all the way down; yet, we don't expect it ot be raised all the 
		// way up to home if completed reagent pack load sequence.
		if (!Hardware::Instance().getReagentController ()->IsDown() && (!Hardware::Instance().getReagentController()->IsHome()))
		{
			impl->pReagentPackServices->enqueueInternal ([this, callback, onArmDownComplete]() -> void
				{
					Hardware::Instance().getReagentController ()->ArmDown (onArmDownComplete);
				});
			return;
		}

		onArmDownComplete (true);
	};

	impl->pReagentPackServices->enqueueInternal (std::bind (&ReagentPack::scanRfidTags, this, [this, callback, onReadRfidTagsComplete](bool status) -> void {
		if (status)
		{
			impl->pReagentPackServices->enqueueInternal (std::bind (&ReagentPack::readRfidTags, this, onReadRfidTagsComplete));
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "initializeReagentPack: RFID scan failed");
			// During initialization return *true* regardless of whether there is a reagent pack installed.
			impl->pReagentPackServices->enqueueExternal (callback, true);
		}
		}));
}

//*****************************************************************************
void ReagentPack::Initialize (std::shared_ptr<boost::asio::io_context> upstream, std::function<void (bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "initialize: <enter>");


	// Only allow initializing once.
	if (impl)
	{
		return;
	}

	impl = std::make_unique<ReagentPackPrivate>();
	impl->pReagentPackServices = std::make_shared<HawkeyeServices>(upstream, "ReagentPack_Thread");
	impl->reagentData.Initialize();

	cachedReagentDefinitions_.clear();

	ReagentDefinitionDLL rd = {};
	rd.reagent_index = 1; rd.label = "Cleaning Agent"; rd.mixing_cycles = 1;
	cachedReagentDefinitions_.emplace(rd.reagent_index, rd);
	rd.reagent_index = 2; rd.label = "Conditioning Solution"; rd.mixing_cycles = 1;
	cachedReagentDefinitions_.emplace(rd.reagent_index, rd);
	rd.reagent_index = 3; rd.label = "Buffer Solution"; rd.mixing_cycles = 1;
	cachedReagentDefinitions_.emplace(rd.reagent_index, rd);
	rd.reagent_index = 4; rd.label = "Trypan Blue"; rd.mixing_cycles = 3;
	cachedReagentDefinitions_.emplace(rd.reagent_index, rd);

	impl->pReagentPackServices->enqueueInternal (std::bind (&ReagentPack::initializeInternal, this, [this, callback](bool status) -> void
		{
			if (status)
			{
				impl->pReagentPackServices->enqueueInternal (callback, status);
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "initializeReagentPack: RFID scan failed");

				// During initialization return *true* regardless of whether there is a reagent pack installed.
				impl->pReagentPackServices->enqueueExternal (callback, true);
			}
		}));
}

//*****************************************************************************
void ReagentPack::onLoadComplete (std::string loggedInUsername, HawkeyeError he, reagent_load_complete_callback_DLL onComplete)
{
	if (!WorkflowController::Instance().isOfType (Workflow::Type::ReagentLoad))
	{
		HAWKEYE_ASSERT (MODULENAME, false);
		return;
	}

	auto currentSequence = WorkflowController::Instance().getCurrentWorkflowOpState<ReagentLoadSequence>();
	if (he == HawkeyeError::eSuccess)
	{
		readRfidTags ([this, loggedInUsername, onComplete, currentSequence](bool status) -> void
			{
				if (impl->reagentData.isPackUsable())
				{
					std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };

					// Save this pack's info - used on next power up:
					// If the saved serial number matches the found serial number - adjust proboes down if required. 
					// If the serial number on next boot does not match, require a Re-Loading of the Reagent Pack. 
					// 
					saveCurrentPack (GetTagSnAsString());

					AuditLogger::L().Log (generateAuditWriteData(
						loggedInUsername,
						audit_event_type::evt_reagentload,
						stringifyReagentContainerState (impl->reagentData.GetReagentContainerStates().back())));

					Logger::L().Log (MODULENAME, severity_level::debug1,
						"onReagentPackLoadComplete :success with current state : " +
						std::string (getReagentPackLoadStatusAsStr (currentSequence)));
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "onReagentPackLoadComplete : Failed to read RFID tag");
				}

				boost::system::error_code ec;
				stopContinuousReading = false;
				continuouslyReadRfidTags (ec);

				impl->pReagentPackServices->enqueueExternal (onComplete, currentSequence);
			});
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "onReagentPackLoadComplete: failure " + std::string (getReagentPackLoadStatusAsStr (currentSequence)));
	Logger::L().Log (MODULENAME, severity_level::error, "Load Reagent State Machine Error Occurred.");

	if (currentSequence == eLFailure_ReagentInvalid)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_reagentinvalid,
			getReagentPackLoadStatusAsStr (currentSequence)));
	}

	if (currentSequence == eLFailure_ReagentEmpty || currentSequence == eLFailure_ReagentExpired)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_reagentunusable,
			getReagentPackLoadStatusAsStr (currentSequence)));
	}

	// If pack is present (but is invalid) then consider pack is initialized but not usable
	if (Hardware::Instance().getReagentController ()->IsPackInstalled())
	{
		ScanAndReadRfidTags ([this, onComplete, currentSequence](bool)
			{
				impl->pReagentPackServices->enqueueExternal (onComplete, currentSequence);
			});
		return;
	}

	impl->pReagentPackServices->enqueueExternal (onComplete, currentSequence);
}

//*****************************************************************************
// This must be done by a logged-in user for traceability, but no special permissions are necessary.
//*****************************************************************************
void ReagentPack::Load(
	ReagentPack::reagent_load_status_callback_DLL onLoadStatusChange,
	ReagentPack::reagent_load_complete_callback_DLL onComplete,
	std::function<void (HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Load: <enter>");

	HAWKEYE_ASSERT (MODULENAME, onLoadStatusChange);
	HAWKEYE_ASSERT (MODULENAME, onComplete);

#ifdef TEST_REPLACING_REAGENT_PACK
	void resetRemainingReagentPackUses();
	resetRemainingReagentPackUses();
#endif

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "LoadReagentPack: Not Permitted Workflow busy!");
		impl->pReagentPackServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	std::function<void (std::function<void (bool)>)> scanAndReadRfidTagsLambda = std::bind (&ReagentPack::ScanAndReadRfidTags, this, std::placeholders::_1);
	std::function<std::tuple<bool, bool> (void)>isReagentPackValidLambda = [this]()
	{
		bool isExpired = impl->reagentData.isExpired();
		bool isEmpty = impl->reagentData.isEmpty();

		return std::make_tuple (isExpired, isEmpty);
	};

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	auto workflow = std::make_shared<ReagentLoadWorkflow> (scanAndReadRfidTagsLambda, isReagentPackValidLambda, onLoadStatusChange);
	workflow->registerCompletionHandler ([this, loggedInUsername, onComplete](HawkeyeError he)
		{
			onLoadComplete (loggedInUsername, he, onComplete);
		});

	WorkflowController::Instance().set_load_execute_async ([this, callback](HawkeyeError success)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "LoadReagentPack: <exit>");
			impl->pReagentPackServices->enqueueExternal (callback, success);

		}, workflow, Workflow::Type::ReagentLoad);
}

//*****************************************************************************
void ReagentPack::onUnloadComplete (std::string loggedInUsername, HawkeyeError he, reagent_unload_complete_callback_DLL onUnloadComplete)
{
	if (!WorkflowController::Instance().isOfType (Workflow::Type::ReagentUnload))
	{
		HAWKEYE_ASSERT (MODULENAME, false);
		return;
	}

	auto currentSequence = WorkflowController::Instance().getCurrentWorkflowOpState<ReagentUnloadSequence>();
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_reagentunload,
			"Success"));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onReagentPackUnloadComplete: unload operation failed!");
	}

	impl->pReagentPackServices->enqueueExternal (onUnloadComplete, currentSequence);
}

//*****************************************************************************
void ReagentPack::Unload (
	ReagentContainerUnloadOption* UnloadActions,
	uint16_t nContainers,
	reagent_unload_status_callback_DLL onStatusChange,
	reagent_unload_complete_callback_DLL onComplete,
	std::function<void (HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "UnloadReagentPack: <enter>");

	HAWKEYE_ASSERT (MODULENAME, onComplete);
	HAWKEYE_ASSERT (MODULENAME, onStatusChange);

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "UnloadReagentPack: Not Permitted Workflow busy!");
		impl->pReagentPackServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	// Stop continuously reading the RFID tag(s).
	stopContinuousReading = true;
	if (readRfidTagsTimer_)
	{
		readRfidTagsTimer_->cancel();
	}

	if (!Hardware::Instance().getReagentController()->isRfidSim())
	{
		// - At this point, regardless of outcome
		// Ensure all records are marked as inactive 
		// 
		std::vector<DBApi::DB_ReagentTypeRecord> allPacks = {};
		DBApi::eQueryResult qr = DBApi::DbGetReagentInfoList(allPacks);
		if (qr != DBApi::eQueryResult::QueryOk) {
			Logger::L().Log(MODULENAME, severity_level::debug1, "Unload: no DB recs");
		}

		for (int j = 0; j < allPacks.size(); j++)
		{
			if (allPacks[j].Current)
			{
				allPacks[j].Current = false;
				DBApi::eQueryResult qr = DBApi::DbModifyReagentInfo(allPacks[j]);
				if (DBApi::eQueryResult::QueryOk != qr)
				{
					Logger::L().Log(MODULENAME, severity_level::error, "Unload: failed to update DB record!");
				}
			}
		}
	}

	/**
	* Specific request possible for EACH of the 'N' possible reagent containers present in the system.
	* The user may want to drain the expired crap in Door Left, but purge the lines to Door Right
	* and then leave the Cleaning Kit in the main bay alone.
	*/

	std::vector<ReagentContainerUnloadOptionDLL> unloadContainersOptions;
	unloadContainersOptions.clear ();
	if (UnloadActions != nullptr && nContainers > 0)
	{
		//TODO: need to investigate for Hunter, we might not need container id.
		//location_id = *reinterpret_cast<const uint8_t*> (container_opt.container_id);

		for (auto i = 0; i < nContainers; i++)
		{
			ReagentContainerUnloadOptionDLL item;
			reagentContainerUnloadOptionToDLL (item, UnloadActions[i]);
			unloadContainersOptions.push_back (item);
		}
	}
	else
	{
		//return HawkeyeError::eInvalidArgs;
		ReagentContainerUnloadOptionDLL null_option = ReagentContainerUnloadOptionDLL();
		unloadContainersOptions.push_back (null_option);
	}

	SystemStatus::Instance().getData().remainingReagentPackUses = 0;

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	auto workflow = std::make_shared<ReagentUnloadWorkflow> (unloadContainersOptions, onStatusChange);
	workflow->registerCompletionHandler ([this, loggedInUsername, onComplete](HawkeyeError he) -> void
		{
			onUnloadComplete (loggedInUsername, he, onComplete);
		});

	WorkflowController::Instance().set_load_execute_async ([this, callback](HawkeyeError success)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "UnloadReagentPack: <exit>");
			impl->pReagentPackServices->enqueueExternal (callback, success);

		}, workflow, Workflow::Type::ReagentUnload);
}

///*****************************************************************************
// This is only called during initialization and when changing the reagent pack.
//*****************************************************************************
void ReagentPack::scanRfidTags (std::function<void (bool)> callback) const
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "scanRfidTags: <enter>");

	if (!Hardware::Instance().getReagentController ()->IsPackInstalled()) {
		Logger::L().Log (MODULENAME, severity_level::warning, "scanRfidTags: no reagent pack installed");
		ReportSystemError::Instance().ReportError (BuildErrorInstance (
			instrument_error::reagent_pack_nopack,
			instrument_error::reagent_pack_instance::general,
			Hardware::Instance().isInited() ? instrument_error::severity_level::warning : instrument_error::severity_level::notification));
		impl->pReagentPackServices->enqueueInternal (callback, false);
		return;
	}

	if (!Hardware::Instance().getReagentController()->IsDoorClosed ()) {
		Logger::L().Log (MODULENAME, severity_level::warning, "scanRfidTags: door is not closed");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::reagent_door,
			Hardware::Instance().isInited() ? instrument_error::severity_level::warning : instrument_error::severity_level::notification));
		impl->pReagentPackServices->enqueueInternal (callback, false);
		return;
	}

	Hardware::Instance().getReagentController()->scanRfidTags ([this, callback](bool status) -> void {
		if (!status) {
			Logger::L().Log (MODULENAME, severity_level::warning, "scanRfidTags: RFID scan failed");
		}

		impl->areRfidTagsScanned = status;
		impl->isValveMapSet = false;

		Logger::L().Log (MODULENAME, severity_level::normal, "RFID tag(s) scanned");

		Logger::L().Log (MODULENAME, severity_level::debug2, "scanRfidTags: <exit>");

		impl->pReagentPackServices->enqueueInternal (callback, status);
		});
}

//*****************************************************************************
void ReagentPack::readRfidTags (std::function<void (bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "readRfidTags: <enter>");

	if (!impl->areRfidTagsScanned)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "readRfidTags: <exit, tags not scanned>");
		impl->pReagentPackServices->enqueueInternal (callback, false);
		return;
	}

	// Return true without reading RFID tags if valve map are being set
	if (impl->isValveMapSetInProgress.load())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "readRfidTags: <exit, setting valve map currently>");
		impl->pReagentPackServices->enqueueInternal (callback, true);
		return;
	}


	Hardware::Instance().getReagentController()->readRfidTags ([this, callback](std::shared_ptr<std::vector<RfidTag_t>> rfidTags) -> void
		{
			bool readFail = false;
			if (!rfidTags)
			{	
				readFail = true;;
			} else 	if (rfidTags->size() == 0)
			{
				readFail = true;;
			}
			if (readFail) {
				impl->isPackInitialized = false;
				if (consecReadErrors_++ < MAX_CONSEC_READ_FAILS)
				{
					Logger::L().Log(MODULENAME, severity_level::error, "readRfidTags: <exit, read tag error - less than max consec read errors>");
					impl->pReagentPackServices->enqueueInternal(callback, true);
				}
				else
				{
					Logger::L().Log(MODULENAME, severity_level::critical, "readRfidTags: <exit, several consecutive reag tag errors -- bad read or shutting down?>");
					impl->pReagentPackServices->enqueueInternal(callback, false);
				}
				return;
			}
			consecReadErrors_ = 0;

			std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
			//NOTE: save for debugging...
			//Logger::L().Log (MODULENAME, severity_level::debug1, "readRfidTags 1: <locking mutex>");

			impl->reagentData.Decode(*rfidTags);
			{
				// Find and add any missing reagent definitions and cache/write them in info file
				if (impl->reagentData.addMissingReagentDefinitions (cachedReagentDefinitions_))
				{
					cachedReagentDefinitions_.clear();
					cachedReagentDefinitions_ = impl->reagentData.GetReagentDefinitions();
				}

				//TODO: currently this only supports Scout...  need to refactor for Hunter...

				
				std::vector<ReagentContainerStateDLL> states = impl->reagentData.GetReagentContainerStates();
				if (states.size() > 0) {
					SystemStatus::Instance().getData().remainingReagentPackUses = states[0].events_remaining;
				}			

				if (impl->reagentData.isPackUsable())
				{
					Logger::L().Log (MODULENAME, severity_level::debug2, "readRfidTags: <exit>");
					impl->isPackInitialized = true;
					impl->pReagentPackServices->enqueueInternal (std::bind (&ReagentPack::setValveMap, this, callback));
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "readRfidTags: <exit, pack is unusable>");
					impl->isPackInitialized = false;
					impl->pReagentPackServices->enqueueInternal (callback, false);
				}
			}
		});
}

//*****************************************************************************
void ReagentPack::continuouslyReadRfidTags (boost::system::error_code ec)
{
	if (stopContinuousReading)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "continuouslyReadRfidTags: shutdown");
		return;
	}

	if (!impl->areRfidTagsScanned)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "continuouslyReadRfidTags: RFID tags are not scanned!");
		return;
	}

	auto onReadRfidTagsComplete = [this](bool status) -> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "continuouslyReadRfidTags: RFID read failed");
			return;
		}

// Removed to only read once as CellHealth does not have a reagent pack.
// Only read once to ensure that the rest of the reagent pack code does not report errors.
		readRfidTagsTimer_->expires_from_now (boost::posix_time::milliseconds(2000));
		readRfidTagsTimer_->async_wait ([this](boost::system::error_code ec)->void
			{
				SystemStatus::Instance().getData().remainingReagentPackUses = CellHealthReagents::GetRemainingReagentUses();
				continuouslyReadRfidTags (ec);
			});
	};

	impl->pReagentPackServices->enqueueInternal (std::bind (&ReagentPack::readRfidTags, this, onReadRfidTagsComplete));
}

//*****************************************************************************
void ReagentPack::ScanAndReadRfidTags (std::function<void (bool)> callback) {

	Logger::L().Log (MODULENAME, severity_level::debug1, "ScanAndReadRfidTags: <enter>");

	scanRfidTags ([this, callback](bool status) -> void
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "ScanAndReadRfidTags: <exit>");
			if (!status) {
				Logger::L().Log (MODULENAME, severity_level::error, "ScanAndReadRfidTags: RFID scan failed");
				impl->pReagentPackServices->enqueueInternal (callback, status);
				return;
			}

			readRfidTags (callback);
		});
}

//*****************************************************************************
bool ReagentPack::verify_current_pack()
{
	if (!HawkeyeConfig::Instance().get().withHardware) {
		return true;
	}

	const std::string tag_sn_str = GetTagSnAsString();
	Logger::L().Log(MODULENAME, severity_level::debug2, "verify_current_pack: <tag_sn_str: " + tag_sn_str);
	if (tag_sn_str.length() == 0)
		return false;

	std::vector<DBApi::DB_ReagentTypeRecord> allPacks = {};
	DBApi::eQueryResult qr = DBApi::DbGetReagentInfoList(allPacks);
	if (qr != DBApi::eQueryResult::QueryOk) {
		Logger::L().Log(MODULENAME, severity_level::debug1, "verify_current_pack: no DB recs <exit>");
		return false;
	}

	for (auto it = allPacks.begin(); it != allPacks.end(); ++it)
	{
		Logger::L().Log(MODULENAME, severity_level::debug2, "verify_current_pack: <it: " + it->ContainerTagSn);

		if (!it->Current)
			continue;
		
		if (it->ContainerTagSn == tag_sn_str) {
			return true;
		}
	}

	return false;
}

//*****************************************************************************
bool ReagentPack::isPackUsable() const
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };

	if (!impl)
	{
		return false;
	}

	return impl->reagentData.isPackUsable();
}

//*****************************************************************************
bool ReagentPack::isNearExpiration (bool reportError, uint32_t& daysRemaining, uint64_t minutesRequired) const
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	return impl->reagentData.isNearExpiration (reportError, daysRemaining, minutesRequired);
}

//*****************************************************************************
std::vector<AnalysisDefinitionDLL>& ReagentPack::GetAnalysisDefinitions()
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	return impl->reagentData.GetAnalysisDefinitions();
}

//*****************************************************************************
std::vector<ReagentCleaningInstruct_t>& ReagentPack::GetCleaningInstructions()
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	return impl->reagentData.GetCleaningInstructions();
}

//*****************************************************************************
std::vector<ReagentContainerStateDLL>& ReagentPack::GetReagentContainerStates()
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	auto rcs = impl->reagentData.GetReagentContainerStates();

	if (stopContinuousReading && rcs.size() > 0)
	{
		impl->reagentData.SetToInvalid();
	}

	return impl->reagentData.GetReagentContainerStates();
}

//*****************************************************************************
std::vector<ReagentInfoRecordDLL>& ReagentPack::GetReagentInfoRecords()
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	return impl->reagentData.GetReagentInfoRecords();
}

//*****************************************************************************
std::map<uint16_t, ReagentDefinitionDLL>& ReagentPack::GetReagentDefinitions()
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	return impl->reagentData.GetReagentDefinitions();
}

//*****************************************************************************
bool ReagentPack::isEmpty()
{
	std::lock_guard<std::mutex> guard{ impl->reagentDataMutex };
	return impl->reagentData.isEmpty();
}

//*****************************************************************************
bool ReagentPack::isPackInitialized() const
{
	return impl->isPackInitialized;
}

//*****************************************************************************
void ReagentPack::Stop()
{
	stopContinuousReading = true;
}

#endif
