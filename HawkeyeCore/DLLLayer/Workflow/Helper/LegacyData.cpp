#include "stdafx.h"

#include "FileSystemUtilities.hpp"
#include "LegacyData.hpp"

static std::string MODULENAME = "LegacyData";

#define WQH_TYPECOUNT 5
static std::string dataTypes[WQH_TYPECOUNT]= { "CamBF", "CamFL1", "CamFL2", "CamFL3", "CamFL4" };
static std::string dataPaths[WQH_TYPECOUNT];
static std::string dataPath;

//*****************************************************************************
bool createLegacyImageDataDirectories (size_t imageCnt, std::string imgDirPath)
{
	size_t numDir = 0;

	switch (HawkeyeConfig::Instance().get().instrumentType)
	{
		case HawkeyeConfig::InstrumentType::ViCELL_BLU_Instrument:
		case HawkeyeConfig::InstrumentType::CellHealth_ScienceModule:
			numDir = 1;	// Only Bright field
			break;
		case HawkeyeConfig::InstrumentType::ViCELL_FL_Instrument:
			numDir = WQH_TYPECOUNT;	// Bright field + 4 Fluorescent channel
			break;
		default:
			return false;
	}

	for (size_t i = 0; i < numDir; i++)
	{
		std::string dirPath = boost::str(boost::format("%s\\%d\\%s")
										 % imgDirPath
										 % imageCnt
										 % dataTypes[i]);

		if (!FileSystemUtilities::CreateDirectories(dirPath))
		{
			return false;
		}

		dataPaths[i] = dirPath;
	}
	return true;
}

//*****************************************************************************
bool getLegacyDataPath (HawkeyeConfig::LedType ledType, std::string& dataPath)
{
	auto index = static_cast<uint32_t>(ledType) - 1;

	if (index >= WQH_TYPECOUNT)
	{
		return false;
	}

	dataPath = dataPaths[index];

	return true;
}

//*****************************************************************************
bool getLegacyDataType (HawkeyeConfig::LedType ledType, std::string& dataType)
{
	auto index = static_cast<uint32_t>(ledType) - 1;

	if (index >= WQH_TYPECOUNT)
	{
		return false;
	}

	dataType = dataTypes[index];

	return true;
}
