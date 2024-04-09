#include "stdafx.h"

#include "AnalysisDefinitionsDLL.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "ImportExportConfiguration.hpp"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"
#include "PtreeConversionHelper.hpp"
#include "QualityControlsDLL.hpp"
#include "SignaturesDLL.hpp"
#include "SystemErrors.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "ImportExportConfiguration";

static std::string generalInformationStr_;

typedef struct
{
	boost::optional<int16_t> instrumentType;
	boost::optional<std::string> version;
	boost::optional<std::string> serialNumber;
	boost::optional<std::string> username;
	boost::optional<system_TP>   exportTime;
} GeneralInformation_t;

static GeneralInformation_t generalInformation_;

//*****************************************************************************
const std::vector<std::pair<std::string, boost::property_tree::ptree>> ImportExportConfiguration::getExportConfigMap()
{
	std::vector<std::pair<std::string, boost::property_tree::ptree>> exportMap = {};

	exportMap.push_back (AnalysisDefinitionsDLL::Export());
	exportMap.push_back (CellTypesDLL::Export());
	exportMap.push_back (QualityControlsDLL::Export());
	exportMap.push_back (SignaturesDLL::Export());
	exportMap.push_back (UserList::Instance().Export());
	exportMap.push_back (InstrumentConfig::Export());
	//NOTE: reagents don't need to be exported...

	return exportMap;
}

//*****************************************************************************
std::vector <std::pair<std::string, ImportExportConfiguration::ImportFn*>> ImportExportConfiguration::getImportConfigMap()
{
	std::vector <std::pair<std::string, ImportFn*>> importMap = {};

	importMap.push_back (std::make_pair("analyses", AnalysisDefinitionsDLL::Import));
	importMap.push_back (std::make_pair("cell_types", CellTypesDLL::Import));
	importMap.push_back (std::make_pair("quality_controls", QualityControlsDLL::Import));
	importMap.push_back (std::make_pair("signature_definitions", SignaturesDLL::Import));
	importMap.push_back (std::make_pair(USERLIST_NODE, UserList::Import));
	//NOTE: reagents are not exported...

	// This is only valid for importing an exported v1.3 configuration.
	importMap.push_back (std::make_pair(INSTRUMENT_NODE, InstrumentConfig::Import));

	return importMap;
}

//*****************************************************************************
bool ImportExportConfiguration::ImportGeneralInformation (boost::property_tree::ptree& giTree)
{
	generalInformation_.instrumentType = giTree.get_optional<int16_t>("InstrumentType");
	if (!generalInformation_.instrumentType)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Importing ViCell_BLU data is not allowed.");
		return false;
	}

	generalInformation_.version = giTree.get_optional<std::string>("Version");
	if (!generalInformation_.version)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportGeneralInformation: Failed to find version tag");
		return false;
	}

	generalInformation_.serialNumber = giTree.get_optional<std::string>("SerialNumber");
	if (!generalInformation_.serialNumber)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportGeneralInformation: Failed to find serial number tag");
		return false;
	}

	generalInformation_.username = giTree.get_optional<std::string>("UserName");
	if (!generalInformation_.username)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportGeneralInformation: Failed to find user name tag");
		return false;
	}

	boost::optional<uint64_t> timestamp = giTree.get_optional<uint64_t>("ExportTime");
	if (!timestamp)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportGeneralInformation: Failed to find export time tag");
		return false;
	}

	generalInformation_.exportTime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(*timestamp);

	generalInformationStr_ += "\nVersion : " + generalInformation_.version.get();
	generalInformationStr_ += "\nSerialNumber : " + generalInformation_.serialNumber.get();
	generalInformationStr_ += "\nUserName : " + generalInformation_.username.get();
	generalInformationStr_ += "\nExportTime : " + ChronoUtilities::ConvertToString(generalInformation_.exportTime.get());

	return true;
}

//*****************************************************************************
HawkeyeError ImportExportConfiguration::getExportConfigurationData (std::string username, boost::property_tree::ptree& pTreeToExport)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "getExportConfigurationData: <enter>");

	pTreeToExport = boost::property_tree::ptree();

	pt::ptree configurationNode = {};	

	configurationNode.add (INSTRUMENT_TYPE_NODE, InstrumentConfig::Instance().Get().InstrumentType);
	configurationNode.add (VERSION_NODE, InstrumentConfig::GetDLLVersion());
	configurationNode.add (SERIAL_NUMBER_NODE, HawkeyeConfig::Instance().get().instrumentSerialNumber.get());

	configurationNode.add ("UserName", username);

	// Add current time (as seconds from epoch) in UTC.
	auto time = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime());
	configurationNode.add ("ExportTime", time);

	for (const auto& pair : getExportConfigMap())
	{
		configurationNode.add_child (pair.first, pair.second);
	}

	pTreeToExport.add_child (CONFIGURATION_NODE, configurationNode);

	Logger::L().Log (MODULENAME, severity_level::debug1, "getExportConfigurationData: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError ImportExportConfiguration::setImportConfigurationData (const boost::property_tree::ptree& pImportTree)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "setImportConfigurationData: <enter>");

	boost::property_tree::ptree root;

	try
	{
		root = pImportTree.get_child (CONFIGURATION_NODE);
	}
	catch (...)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "setImportConfigurationData: <exit, failed to find root tag: " + CONFIGURATION_NODE + ">");
		return HawkeyeError::eStorageFault;
	}

	if (!ImportGeneralInformation (root))
	{
		return HawkeyeError::eNotSupported;
	}

	HawkeyeError he = HawkeyeError::eSuccess;

	for (const auto& item : getImportConfigMap())
	{
		try
		{
			auto pConfigTree = root.get_child (item.first);
			item.second (pConfigTree);
		}
		catch (...)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, 
				boost::str(boost::format("setImportConfigurationData: <exit, failed to find <%s> configuration tag in import tree>") % item.first));
			he = HawkeyeError::eEntryInvalid;
			break;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "setImportConfigurationData: <exit>");

	return he;
}

//*****************************************************************************
HawkeyeError ImportExportConfiguration::ImportConfigurationForOfflineAnalysis (const boost::property_tree::ptree& pImportTree)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "ImportConfigurationForOfflineAnalysis: <enter>");

	boost::property_tree::ptree root;

	try
	{
		root = pImportTree.get_child (CONFIGURATION_NODE);
	}
	catch (...)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "ImportConfigurationForOfflineAnalysis: <exit, failed to find root tag: " + CONFIGURATION_NODE + ">");
		return HawkeyeError::eInvalidArgs;
	}

	HawkeyeError he = HawkeyeError::eSuccess;
	for (const auto& item : getImportConfigMap())
	{
		try
		{
			auto pConfigTree = root.get_child (item.first);
			item.second (pConfigTree);
		}
		catch (...)
		{
			if (item.first == INSTRUMENT_NODE)
			{
				Logger::L().Log (MODULENAME, severity_level::notification,
					"Import: <exit, \"Instrument\" tag is only valid for importing configuration, not data and configuration");
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::critical,
					boost::str(boost::format("ImportConfigurationForOfflineAnalysis: <exit, failed to find <%s> configuration tag in import tree>") % item.first));
				he = HawkeyeError::eInvalidArgs;
				break;
			}
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "ImportConfigurationForOfflineAnalysis: <exit>");

	return he;
}

//*****************************************************************************
std::string ImportExportConfiguration::GetGeneralInformation()
{
	return generalInformationStr_;
}
