#include "stdafx.h"

#include "HawkeyeConfig.hpp"
#include "Logger.hpp"
#include "RfidSim.hpp"

#include "HawkeyeDirectory.hpp"
#include "CellHealthReagents.hpp"
#include "ReagentControllerBase.hpp"

static const char MODULENAME[] = "RfidSim";

const int kREAD_TIME = 300;
const int kRAND_TIME = 150;

/**
 ***************************************************************************
 * \brief Block for a minimum amount of time and optionally add a random amount
 * \param min_ms - minimum number of milli-seconds to block for
 * \param rand_ms - a uniformly random amount of time up to this value is added to the delay
 */
static void Delay(uint32_t min_ms, uint32_t rand_ms)
{
	const int kSLEEP_TIME = 10;

	auto delayT = min_ms;
	if (delayT < kSLEEP_TIME)
		delayT = kSLEEP_TIME;
	if (rand_ms > 0)
		delayT += (rand() % rand_ms);

	// We want this to be a busy wait
	// this code in intentionally ineffecient 
	while (delayT > 0)
	{
		Sleep(kSLEEP_TIME);
		if (delayT <= kSLEEP_TIME)
			delayT = 0;
		else
			delayT -= kSLEEP_TIME;
	}
}



//*****************************************************************************
RfidSim::RfidSim (std::shared_ptr<CBOService> pCBOService)
	: RfidBase (pCBOService)
{
	isSim_ = true;
	srand(static_cast<unsigned int>(time(NULL)));
}

//*****************************************************************************
RfidSim::~RfidSim()
{
}

//*****************************************************************************
void RfidSim::read (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback)
{
	RfidTag_t rfidTag = {0};

	//NOTE: CellHealth does not have a Reagent Pack like ViCellBLU.
	// This code is set to simulation mode.
	// This delay was causing timing issues in image capture.
	//Delay(kREAD_TIME, kRAND_TIME);

	std::shared_ptr<std::vector<RfidTag_t>> rfidTags = std::make_shared<std::vector<RfidTag_t>>();

	//collect the filenames tag binary files
	std::vector<std::string> rfid_files;
	rfid_files.push_back(HawkeyeConfig::Instance().get().rfidTagSimulationFile_MainBay);//is ok if we collect an empty string
	//NOTE: these were for fluorescence which is obsolete.
	//rfid_files.push_back(HawkeyeConfig::Instance().get().rfidTagSimulationFile_RightDoor);
	//rfid_files.push_back(HawkeyeConfig::Instance().get().rfidTagSimulationFile_LeftDoor);
	
	for (size_t i=0; i<HawkeyeConfig::Instance().get().rfidTagSimulation_TotalTags; i++)
	{
		//std::string filePath = HawkeyeDirectory::Instance().getInstrumentDir(false);
		std::string filePath = boost::str (boost::format ("%s\\Software\\%s") % HawkeyeDirectory::Instance().getInstrumentDir() % rfid_files[i]);
		std::ifstream rfidStream(filePath, std::ios::in | std::fstream::binary);

		RfAppdataRegisters rfidData;
		rfidData.Status.TagSN[4] = static_cast<uint8_t>(i + 1);
		if (rfidStream.is_open())
		{
			rfidStream.seekg (0, rfidStream.end);
			std::streampos bufLen = rfidStream.tellg();
			rfidStream.seekg (0, rfidStream.beg);
			Logger::L().Log (MODULENAME, severity_level::debug2, "read " + std::to_string(bufLen) + " bytes");
			rfidStream.read ((char*)&rfidData.ParamMap, sizeof(RfAppdataRegisters));

			parseTag (i, rfidData, rfidTag);

			// For Cell Health Module: corrct the data read from the disk to match up with the current database values
			if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
			{
				if (rfidTag.hasReagent)
				{
					int vol = 0;
					CellHealthReagents::GetVolume(CellHealthReagents::FluidType::TrypanBlue, vol);
					rfidTag.reagent.remainingUses = ReagentControllerBase::ConvertFluidVolumetoUsageCount(SyringePumpPort::Reagent_1, vol, true);
				}
				for (uint8_t i = 0; i < rfidTag.numCleaners; i++)
				{
					int vol = 0;
					CellHealthReagents::FluidType CHRFT;
					SyringePumpPort::Port SPP;
					switch (i)
					{
					case 0:
						CHRFT = CellHealthReagents::FluidType::Cleaner;
						SPP = SyringePumpPort::Cleaner_1;
						break;
					case 1:
						CHRFT = CellHealthReagents::FluidType::ConditioningSolution;
						SPP = SyringePumpPort::Cleaner_2;
						break;
					case 2:
						CHRFT = CellHealthReagents::FluidType::Buffer;
						SPP = SyringePumpPort::Cleaner_3;
						break;
					default:
						continue;
					}
					CellHealthReagents::GetVolume(CHRFT, vol);
					rfidTag.cleaners[i].remainingUses = ReagentControllerBase::ConvertFluidVolumetoUsageCount(SPP, vol, true);
				}
			}

			rfidData.Status.AuthStatus = 1;
			rfidData.Status.ProgramStatus = 1;
			rfidData.Status.ValidationStatus = 1;
		}
		else
		{
			//not finding a tag binary file is a valid test case.
			Logger::L().Log (MODULENAME, severity_level::critical, boost::str(boost::format("Failed to open RFID tag file \"%s\"") % rfid_files[i].c_str() ));
			rfidData.Status.AuthStatus = 0;
			rfidData.Status.ProgramStatus = 0;
			rfidData.Status.ValidationStatus = 0;
		}

		rfidTags.get()->push_back (rfidTag);
	}

	pCBOService_->enqueueInternal (callback, true, rfidTags);
}
