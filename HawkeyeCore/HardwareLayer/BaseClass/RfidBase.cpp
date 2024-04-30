#include "stdafx.h"

#include <iostream>

#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "HawkeyeConfig.hpp"
#include "Logger.hpp"
#include "MemoryStreamBuf.hpp"
#include "Registers.hpp"
#include "RfidBase.hpp"

static const char MODULENAME[] = "RfidBase";

#ifdef TEST_REPLACING_REAGENT_PACK
	constexpr uint16_t RemainingReagentPackUses = 1;
	static uint16_t remainingReagentPackUses = RemainingReagentPackUses;
#endif

//*****************************************************************************
RfidBase::RfidBase (std::shared_ptr<CBOService> pCBOService)
	: isSim_(false)
	, pCBOService_(std::move(pCBOService))
{
}

//*****************************************************************************
RfidBase::~RfidBase()
{
	//reagent_remaining_offsets_.clear();
}

#ifdef TEST_REPLACING_REAGENT_PACK
//*****************************************************************************
void decrementRemainingReagentPackUses()
{
	remainingReagentPackUses--;
}

//*****************************************************************************
void resetRemainingReagentPackUses()
{
	remainingReagentPackUses = RemainingReagentPackUses;
}
#endif

//*****************************************************************************
void RfidBase::initialize (std::function<void(bool)> callback) {

	pCBOService_->enqueueExternal (callback, true);
}

//*****************************************************************************
void RfidBase::scan (std::function<void(bool)> callback) {

	pCBOService_->enqueueExternal (callback, true);
}

//*****************************************************************************
void RfidBase::read (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback) {

	pCBOService_->enqueueExternal (callback, true, std::make_shared<std::vector<RfidTag_t>>());
}

//*****************************************************************************
void RfidBase::setTime (std::function<void(bool)> callback) {
	pCBOService_->enqueueExternal (callback, true);
}

//*****************************************************************************
void RfidBase::setValveMap (std::function<void(bool)> callback, ReagentValveMap_t valveMap) {
	pCBOService_->enqueueExternal (callback, true);
}

//*****************************************************************************
// Cannot make *RfAppdataRegisters& rfidData* const because *memory_streambuf*
// fails to compile.
//*****************************************************************************
void RfidBase::parseTag (size_t tagNum, RfAppdataRegisters& rfidData, RfidTag_t& rfidTag)
{
	uint16_t byte_offset=0;
	std::stringstream ss;

	static std::string last_parse_content = "";
	static std::string last_sim_content = "";

	Logger::L().Log (MODULENAME, severity_level::debug3, "parseTag: <enter>");

	MemoryStreamBuf tagBuf (reinterpret_cast<char *>(rfidData.ParamMap), sizeof(rfidData.ParamMap));
	std::istream tagStream(&tagBuf);

	//NOTE: hack to get around missing values in simultation files.
	if (isSim_) {
		for (auto i = 0; i < sizeof(rfidTag.tagSN); i++) {
			rfidTag.tagSN[i] = i + 1;
		}
	} else {
		memcpy (rfidTag.tagSN, rfidData.Status.TagSN, sizeof(rfidData.Status.TagSN));
	}

	ss << boost::str(boost::format("tag[%d] SN: 0x%02X%02X%02X%02X%02X")
		% tagNum
		% int(rfidTag.tagSN[0])
		% int(rfidTag.tagSN[1])
		% int(rfidTag.tagSN[2])
		% int(rfidTag.tagSN[3])
		% int(rfidTag.tagSN[4]));

	tagStream.read (rfidTag.packPn, 7);
	byte_offset += 7;
	ss << std::endl << "PackPn: " << rfidTag.packPn << std::endl;

	tagStream.read ((char*)&rfidTag.packExp, sizeof(uint32_t));
	byte_offset += sizeof(uint32_t);
	ss << "PackExp: " << rfidTag.packExp << std::endl;

	tagStream.read ((char*)&rfidTag.serviceLife, sizeof(uint16_t));
	byte_offset += sizeof(uint16_t);
	ss << "ServiceLife: " << rfidTag.serviceLife << std::endl;

	tagStream.read ((char*)&rfidTag.inServiceDate, sizeof(uint32_t));
	byte_offset += sizeof(uint32_t);
	ss << "InServiceDate: " << rfidTag.inServiceDate << std::endl;

	tagStream.read ((char*)&rfidTag.packLotNum, sizeof(uint32_t));
	byte_offset += sizeof(uint32_t);
	ss << "PackLotNum: " << rfidTag.packLotNum << std::endl;

	if (isSim_) 
	{
		// Change expiration dates to valid ones if needed
		if (HawkeyeConfig::Instance().get().rfidTagSimulation_SetValidTagData)
		{
			boost::gregorian::date today = boost::gregorian::day_clock::local_day();
			boost::gregorian::date local_epoch = boost::gregorian::from_simple_string("1970-01-01");
			boost::gregorian::days now_days = today - local_epoch;

			rfidTag.inServiceDate = now_days.days();
			rfidTag.packExp = rfidTag.inServiceDate + 30;
			rfidTag.packLotNum = 654321;
			rfidTag.serviceLife = 30;
		}

		std::stringstream ss2;
		ss2 << "PackExp: " << rfidTag.packExp << std::endl;
		ss2 << "ServiceLife: " << rfidTag.serviceLife << std::endl;
		ss2 << "InServiceDate: " << rfidTag.inServiceDate << std::endl;
		ss2 << "PackLotNum: " << rfidTag.packLotNum << std::endl;

		if (last_sim_content != ss2.str())
		{
			last_sim_content = ss2.str();
			Logger::L().Log (MODULENAME, severity_level::debug1, "Simulation Values:\n" + last_sim_content);
		}
	}

	tagStream.read ((char*)&rfidTag.numCleaners, sizeof(uint8_t));
	byte_offset += sizeof(uint8_t);
	ss << "NumCleaners: " << (int)rfidTag.numCleaners << std::endl;

	rfidTag.cleaners = new Cleaner_t[rfidTag.numCleaners];
	rfidTag.remainingReagentOffsets.clear();

	for (uint8_t i = 0; i < rfidTag.numCleaners; i++)
	{
		tagStream.read((char*)&rfidTag.cleaners[i].index, sizeof(uint16_t));
		byte_offset += sizeof(uint16_t);

		char ch;
		std::string temp;
		while ((ch = static_cast<char>(tagStream.get())) != '\0')
		{
			temp += ch;
		}

		// Fix some legacy bad names we don't want to see:
		if (temp == "Disinfectant")
			temp = "Conditioning Solution";

		DataConversion::convertToCharPointer(rfidTag.cleaners[i].desc, temp);

		byte_offset += uint32_t(temp.size());
		++byte_offset;
		tagStream.read((char*)&rfidTag.cleaners[i].totalUses, sizeof(uint16_t));
		byte_offset += sizeof(uint16_t);

		tagStream.read ((char*)&rfidTag.cleaners[i].remainingUses, sizeof(uint16_t));
		rfidTag.remainingReagentOffsets.push_back(static_cast<uint16_t>(byte_offset));

		byte_offset += sizeof(uint16_t);
		
		tagStream.read(rfidTag.cleaners[i].partNum, 7);
		byte_offset += 7;

		ss << "\tCleaner[" << (int)i << "].Index: " << rfidTag.cleaners[i].index << std::endl;
		if (rfidTag.cleaners[i].desc != nullptr) {
			ss << "\tCleaner[" << (int) i << "].Desc: " << rfidTag.cleaners[i].desc << std::endl;
		} else {
			ss << "\tCleaner[" << (int) i << "].Desc: <NULL>" << std::endl;
		}
		ss << "\tCleaner[" << (int)i << "].TotalUses: " << rfidTag.cleaners[i].totalUses << std::endl;
		ss << "\tCleaner[" << (int)i << "].RemainingUses: " << rfidTag.cleaners[i].remainingUses << std::endl;
		ss << "\tCleaner[" << (int)i << "].PartNum: " << rfidTag.cleaners[i].partNum << std::endl;

	} // End "for (uint8_t i = 0; i < rfidTag.numCleaners; i++)"
	
	tagStream.read ((char*)&rfidTag.hasReagent, sizeof(uint8_t));
	byte_offset += sizeof(uint8_t);
	ss << "HasReagent: " << (int)rfidTag.hasReagent << std::endl;

	if (rfidTag.hasReagent)
	{
		tagStream.read((char*)&rfidTag.reagent.index, sizeof(uint16_t));
		byte_offset += sizeof(uint16_t);

		char ch;
		std::string temp;
		while ((ch = static_cast<char>(tagStream.get())) != '\0')
		{
			temp += ch;
		}
		DataConversion::convertToCharPointer(rfidTag.reagent.desc, temp);

		byte_offset += uint32_t(temp.size());
		++byte_offset;
		tagStream.read((char*)&rfidTag.reagent.cycles, sizeof(uint8_t));
		byte_offset += sizeof(uint8_t);

		tagStream.read((char*)&rfidTag.reagent.totalUses, sizeof(uint16_t));
		byte_offset += sizeof(uint16_t);

		tagStream.read ((char*)&rfidTag.reagent.remainingUses, sizeof(uint16_t));
		rfidTag.remainingReagentOffsets.push_back(byte_offset);

		byte_offset += sizeof(uint16_t);

#ifdef TEST_REPLACING_REAGENT_PACK		
		rfidTag.reagent.remainingUses = remainingReagentPackUses;
#endif
		
		//NOTE: hack to get around missing values in simulation files.
		if (isSim_ && HawkeyeConfig::Instance().get().rfidTagSimulation_SetValidTagData) {
			rfidTag.reagent.totalUses = 300;
#ifndef TEST_REPLACING_REAGENT_PACK
			rfidTag.reagent.remainingUses = 300;
#endif

//NOTE: for testing dynamic update of UI...
			//static uint16_t remainingUses=300;
			//remainingUses--;
			//
			//if (remainingUses == 0) {
			//	remainingUses = rfidTag.reagent.totalUses;
			//}
			//
			//rfidTag.reagent.remainingUses = remainingUses;			
		}


		tagStream.read(rfidTag.reagent.partNum, 7);
		byte_offset += sizeof(rfidTag.reagent.partNum);

		tagStream.read((char*)&rfidTag.reagent.numCleaningInstructions, sizeof(uint8_t));
		byte_offset += sizeof(uint8_t);

		if (rfidTag.reagent.numCleaningInstructions > 0)
		{
			rfidTag.reagent.cleaning_instructions = new CleaningSequenceStep_t[rfidTag.reagent.numCleaningInstructions];

			for (uint8_t j = 0; j < rfidTag.reagent.numCleaningInstructions; j++)
			{
				tagStream.read((char*)&rfidTag.reagent.cleaning_instructions[j], sizeof(CleaningSequenceStep_t));
			}
		}
		byte_offset += sizeof(CleaningSequenceStep_t) * rfidTag.reagent.numCleaningInstructions;

		ss << "\tReagent.index: " << rfidTag.reagent.index << std::endl;
		if (rfidTag.reagent.desc != nullptr)
			ss << "\tReagent.desc: " << rfidTag.reagent.desc << std::endl;
		else
			ss << "\tReagent.desc: <NULL>" << std::endl;
		ss << "\tReagent.cycles: " << (int)rfidTag.reagent.cycles << std::endl;
		ss << "\tReagent.totalUses: " << rfidTag.reagent.totalUses << std::endl;
		ss << "\tReagent.remainingUses: " << rfidTag.reagent.remainingUses << std::endl;
		ss << "\tReagent.partNum: " << rfidTag.reagent.partNum << std::endl;
		ss << "\tReagent.numCleaningInstructions: " << (int)rfidTag.reagent.numCleaningInstructions << std::endl;
	
	} // End "if (rfidTag.hasReagent)"


	tagStream.read((char*)&rfidTag.numAnalyses, sizeof(uint8_t));
	byte_offset += sizeof(uint8_t);
	ss << "NumAnalyses: " << (int)rfidTag.numAnalyses << std::endl;

	rfidTag.analyses = new Analysis_t[rfidTag.numAnalyses];
	for (uint8_t i = 0; i < rfidTag.numAnalyses; i++)
	{
		tagStream.read((char*)&rfidTag.analyses[i].index, sizeof(uint16_t));
		byte_offset += sizeof(uint16_t);
		ss << "\tAnalysis[" << (int)i << "].Index: " << rfidTag.analyses[i].index << std::endl;

		char ch;
		std::string temp;
		while ((ch = static_cast<char>(tagStream.get())) != '\0')
		{
			temp += ch;
		}
		DataConversion::convertToCharPointer(rfidTag.analyses[i].desc, temp);

		byte_offset += uint32_t(temp.size());
		++byte_offset;

		if (rfidTag.analyses[i].desc != nullptr) {
			ss << "\tAnalysis[" << (int)i << "].Desc: " << rfidTag.analyses[i].desc << std::endl;
		} else {
			ss << "\tAnalysis[" << (int)i << "].Desc: <NULL>" << std::endl;
		}

		tagStream.read((char*)&rfidTag.analyses[i].numReagents, sizeof(uint8_t));
		byte_offset += sizeof(uint8_t);
		ss << "\tAnalysis[" << (int)i << "].NumReagents: " << (int)rfidTag.analyses[i].numReagents << std::endl;
		if (rfidTag.analyses[i].numReagents > 0)
		{
			rfidTag.analyses[i].reagentIndices = new uint16_t[rfidTag.analyses[i].numReagents];

			tagStream.read((char*)rfidTag.analyses[i].reagentIndices, sizeof(uint16_t)*rfidTag.analyses[i].numReagents);
			byte_offset += sizeof(uint16_t)*rfidTag.analyses[i].numReagents;

			for (uint8_t j = 0; j < rfidTag.analyses[i].numReagents; j++)
			{
				ss << "\t\tAnalysis[" << (int)i << "].ReagentIndex[" << (int)j << "]: " << rfidTag.analyses[i].reagentIndices[j] << std::endl;
			}
		}
		else
		{
			rfidTag.analyses[i].reagentIndices = nullptr;
		}

		tagStream.read((char*)&rfidTag.analyses[i].numFL_Illuminators, sizeof(uint8_t));
		byte_offset += sizeof(uint8_t);

		ss << "\tNumFL_Illuminators: " << (int)rfidTag.analyses[i].numFL_Illuminators << std::endl;
		if (rfidTag.analyses[i].numFL_Illuminators > 0)
		{
			rfidTag.analyses[i].illuminators = new FL_IlluminationSettings[rfidTag.analyses[i].numFL_Illuminators];

			for (uint8_t j = 0; j < rfidTag.analyses[i].numFL_Illuminators; j++)
			{
				tagStream.read((char*)rfidTag.analyses[i].illuminators, sizeof(FL_IlluminationSettings)*rfidTag.analyses[i].numFL_Illuminators);
				byte_offset += sizeof(FL_IlluminationSettings)*rfidTag.analyses[i].numFL_Illuminators;
				ss << "\t\tIlluminator[" << (int)j << "].illuminator_wavelength_nm: " << rfidTag.analyses[i].illuminators[j].illuminator_wavelength_nm << std::endl;
				ss << "\t\tIlluminator[" << (int)j << "].emission_wavelength_nm: " << rfidTag.analyses[i].illuminators[j].emission_wavelength_nm << std::endl;
				ss << "\t\tIlluminator[" << (int)j << "].exposure_time_ms: " << rfidTag.analyses[i].illuminators[j].exposure_time_ms << std::endl;
			}
		}
		else
		{
			rfidTag.analyses[i].illuminators = nullptr;
		}

		tagStream.read((char*)&rfidTag.analyses[i].populationParameterKey.key, sizeof(uint16_t));
		byte_offset += sizeof(uint16_t);
		ss << "\tPopulationParameterKey.key: " << (int)rfidTag.analyses[i].populationParameterKey.key << std::endl;

		if (rfidTag.analyses[i].populationParameterKey.key > 0 && rfidTag.analyses[i].populationParameterKey.key != 0xFFFF)
		{
			tagStream.read((char*)&rfidTag.analyses[i].populationParameterKey.subKey, sizeof(Parameter_t) - sizeof(uint16_t));
			byte_offset += sizeof(Parameter_t) - sizeof(uint16_t);

			ss << "\t\tPopulationParameterKey.subKey: " << (int)rfidTag.analyses[i].populationParameterKey.subKey << std::endl;
			ss << "\t\tPopulationParameterKey.thresholdValue: " << (int)rfidTag.analyses[i].populationParameterKey.thresholdValue << std::endl;
			ss << "\t\tPopulationParameterKey.aboveThreshold: " << (int)rfidTag.analyses[i].populationParameterKey.aboveThreshold << std::endl;
		}
		else
		{
			ss << "\tNo PopulationParameterKey Found" << std::endl;
		}

		tagStream.read((char*)&rfidTag.analyses[i].numParameters, sizeof(uint8_t));
		byte_offset += sizeof(uint8_t);

		ss << "\tNumParameters: " << (int)rfidTag.analyses[i].numParameters << std::endl;
		if (rfidTag.analyses[i].numParameters > 0)
		{
			rfidTag.analyses[i].parameters = new Parameter_t[rfidTag.analyses[i].numParameters];

			tagStream.read((char*)rfidTag.analyses[i].parameters, sizeof(Parameter_t)*rfidTag.analyses[i].numParameters);
			byte_offset += sizeof(Parameter_t)*rfidTag.analyses[i].numParameters;

			for (uint8_t j = 0; j < rfidTag.analyses[i].numParameters; j++)
			{
				ss << "\t\tParameter[" << (int)j << "].key: " << rfidTag.analyses[i].parameters[j].key << std::endl;
				ss << "\t\tParameter[" << (int)j << "].subKey: " << rfidTag.analyses[i].parameters[j].subKey << std::endl;
				ss << "\t\tParameter[" << (int)j << "].thresholdValue: " << rfidTag.analyses[i].parameters[j].thresholdValue << std::endl;
				ss << "\t\tParameter[" << (int)j << "].aboveThreshold: " << (int)rfidTag.analyses[i].parameters[j].aboveThreshold << std::endl;
			}
		}
		else
		{
			rfidTag.analyses[i].parameters = nullptr;
		}
	}

//TODO: for Vi-Cell
// No need to log this for CellHealth since there is no real reagent pack.
	//if (last_parse_content != ss.str())
	//{
	//	last_parse_content = ss.str();
	//	Logger::L().Log (MODULENAME, severity_level::debug1, "Tag Contents\n" + last_parse_content);
	//}
	
	Logger::L().Log (MODULENAME, severity_level::debug3, "parseTag: <exit>");
}
