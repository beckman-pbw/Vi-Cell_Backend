#include "stdafx.h"

#include "AppConfig.hpp"
#include "CalibrationHistoryDLL.hpp"
#include "DataImporter.hpp"
#include "FileSystemUtilities.hpp"
#include "InstrumentConfig.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "ImportExportConfiguration.hpp"
#include "QualityControlsDLL.hpp"
#include "ReagentPack.hpp"
#include "SignaturesDLL.hpp"

static AppConfig appCfg_;

// This code upgrades v1.2 configuration INFO files to v1.3.

//*****************************************************************************
bool importAnalysisInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importAnalysisInfoFile: <enter>");

	DataImporter::Log ("Importing Analysis info file...");

	const std::string infoFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::Analysis);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	// Load the existing Analysis.info file.
	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "config");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read Analysis info file...", severity_level::error);
		return false;
	}

	AnalysisDefinitionsDLL::Import (pConfig_.get());

	Logger::L().Log (MODULENAME, severity_level::debug1, "importAnalysisInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importCalibrationHistoryInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importCalibrationHistoryInfoFile: <enter>");

	DataImporter::Log ("Importing CalibrationHistory info file...");

	std::string infoFile = HawkeyeDirectory::Instance().getFilePath (HawkeyeDirectory::FileType::CalibrationHistory);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	// CalibrationHistory.info does not contain the "config" tag.
	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "calibration");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read CalibrationHistory info file, config tag is missing", severity_level::error);
		return false;
	}

	CalibrationHistoryDLL::Instance().Import (pConfig_.get());

	Logger::L().Log (MODULENAME, severity_level::debug1, "importCalibrationHistoryInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importCelltypesInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importCelltypesInfoFile: <enter>");

	DataImporter::Log ("Importing CellType info file...");

	const std::string infoFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::CellType);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "config");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read CellTypes info file...", severity_level::error);
		return false;
	}

	CellTypesDLL::Import (pConfig_.get());

	Logger::L().Log (MODULENAME, severity_level::debug1, "importCelltypesInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importControllerCalInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importControllerCalInfoFile: <enter>");

	DataImporter::Log ("Importing ControllerCal info file...");

	const std::string infoFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::ControllerCal);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "config");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read ControllerCal info file...", severity_level::error);
		return false;
	}

	HawkeyeConfig::ImportControllerCalInfoFile (pConfig_->get_child ("motor_controllers"));

	Logger::L().Log (MODULENAME, severity_level::debug1, "importControllerCalInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importQualityControlInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importQualityControlInfoFile: <enter>");

	DataImporter::Log ("Importing QualityControl info file...");

	std::string infoFile = HawkeyeDirectory::Instance().getFilePath (HawkeyeDirectory::FileType::QualityControl);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "config");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read QualityControl info file...", severity_level::error);
		return false;
	}

	QualityControlsDLL::Import (pConfig_->get_child ("quality_controls"));

	Logger::L().Log (MODULENAME, severity_level::debug1, "importQualityControlInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importSignatureInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importSignatureInfoFile: <enter>");

	DataImporter::Log ("Importing Signatures info file...");

	std::string infoFile = HawkeyeDirectory::Instance().getFilePath (HawkeyeDirectory::FileType::Signatures);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "config");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read Signature info file...", severity_level::error);
		return false;
	}

	SignaturesDLL::Import (pConfig_->get_child ("signature_definitions"));

	Logger::L().Log (MODULENAME, severity_level::debug1, "importSignatureInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importUserlistInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importUserlistInfoFile: <enter>");

	DataImporter::Log ("Importing UserList file...");

	std::string infoFile = HawkeyeDirectory::Instance().getFilePath (HawkeyeDirectory::FileType::UserList);

	ConfigUtils::ClearCachedConfigFile (infoFile);

	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "UsersGoHere");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read UserList info file...", severity_level::error);
		return false;
	}

	UserList::Import (pConfig_->get_child (USERLIST_NODE));

	Logger::L().Log (MODULENAME, severity_level::debug1, "importUserlistInfoFile: <exit>");

	return true;
}

//*****************************************************************************
bool importHawkeyeDynamicStaticInfoFiles(bool setSerNo)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importHawkeyeDynamicStaticInfoFiles: <enter>");

	{
		DataImporter::Log ("Importing HawkeyeDynamic info file...");

		const std::string infoFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::HawkeyeDynamic);

		ConfigUtils::ClearCachedConfigFile (infoFile);

		// Load the existing Analysis.info file.
		t_opPTree pConfig_ = appCfg_.initwithtag (infoFile, "config.DLL");
		if (!pConfig_) {
			DataImporter::Log ("Unable to read HawkeyeDynamic info file...", severity_level::error);
			return false;
		}

		HawkeyeConfig::ImportHawkeyeDynamicInfoFile (pConfig_.get(), setSerNo);
	}

	{
		DataImporter::Log ("Importing HawkeyeStatic info file...");

		const std::string infoFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::HawkeyeStatic);

		ConfigUtils::ClearCachedConfigFile (infoFile);

		// Load the existing Analysis.info file.
		t_opPTree pConfig_ = appCfg_.initwithtag (infoFile, "config.DLL");
		if (!pConfig_) {
			DataImporter::Log ("Unable to read Analysis info file...", severity_level::error);
			return false;
		}

		HawkeyeConfig::ImportHawkeyeStaticInfoFile (pConfig_.get());
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "importHawkeyeDynamicStaticInfoFiles: <exit>");

	return true;
}

//*****************************************************************************
bool importReagentPackInfoFile()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "importReagentPackInfoFile: <enter>");

	DataImporter::Log ("Importing ReagentPack info file...");

	std::string infoFile = HawkeyeDirectory::Instance().getFilePath (HawkeyeDirectory::FileType::ReagentPack);

	std::string encryptedFileName;
	bool success = HDA_GetEncryptedFileName (infoFile, encryptedFileName);
	if (success && !FileSystemUtilities::FileExists(encryptedFileName))
	{
		DataImporter::Log ("ReagentPack info file not found, assuming no reagent pack was ever installed...");
		return true;
	}	
	
	ConfigUtils::ClearCachedConfigFile (infoFile);

	t_opPTree pConfig_ = appCfg_.initwithtag(infoFile, "config");
	if (!pConfig_) {
		DataImporter::Log ("Unable to read ReagentPack info file...", severity_level::error);
		return false;
	}

	ReagentPack::Instance().Import (pConfig_->get_child ("reagent_containers"));

	Logger::L().Log (MODULENAME, severity_level::debug1, "importReagentPackInfoFile: <exit>");

	return true;
}

//*****************************************************************************
// v1.2...
//*****************************************************************************
bool DataImporter::ImportConfiguration(bool setSerNo)
{
	/*
		Files to import:
			Analysis             (v1.3) Done
			CalibrationHistory   (v1.3) Done
			CellType             (v1.3) Done
			EEPROM               (not needed) Dead
			HawkeyeDynamic       (v1.3) Done
			HawkeyeStatic        (v1.3) Done
			QualityControl       (v1.3) Done
			ReagentPack          (v1.3)
			Reagents             (n/a)  v1.2 config does not contain enough info to be useful.
			SignatureDefinitions (v1.3) Done
			UserList             (v1.3) Done
			Scripts              (v1.4)
			UI XMLDB             (v1.4)
	*/

	if (!importReagentPackInfoFile())
	{
		DataImporter::Log("Unable to import ReagentPack info file...", severity_level::error);
		return false;
	}
	if (!importHawkeyeDynamicStaticInfoFiles(setSerNo))
	{
		DataImporter::Log("Unable to import HawkeyeDynamic or HawkeyeStatic info files...", severity_level::error);
		return false;
	}
	if (!importControllerCalInfoFile())
	{
		DataImporter::Log("Unable to import Motor registration info file...", severity_level::error);
		return false;
	}
	if (!importAnalysisInfoFile())
	{
		DataImporter::Log("Unable to import Analysis info file...", severity_level::error);
		return false;
	}
	if (!importCelltypesInfoFile())
	{
		DataImporter::Log("Unable to import CellTypes info file...", severity_level::error);
		return false;
	}
	if (!importCalibrationHistoryInfoFile())
	{
		DataImporter::Log("Unable to import CalibrationHistory info file...", severity_level::error);
		return false;
	}
	if (!importQualityControlInfoFile())
	{
		DataImporter::Log("Unable to import QualityControl info file...", severity_level::error);
		return false;
	}
	if (!importSignatureInfoFile())
	{
		DataImporter::Log("Unable to import Signature info file...", severity_level::error);
		return false;
	}
	if (!importUserlistInfoFile())
	{
		DataImporter::Log("Unable to import UserList info file...", severity_level::error);
		return false;
	}

	return true;
}

//*****************************************************************************
bool DataImporter::ImportConfigurationForOfflineAnalysis()
{
	boost::property_tree::ptree& metadata = GetMetaData();
//TODO: test for valid read...

	boost::property_tree::ptree& instNode = metadata.get_child (INSTRUMENT_NODE);
	const boost::optional<uint16_t> t_uint = instNode.get_optional<uint16_t>(INSTRUMENT_TYPE_NODE);
	if (!t_uint)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Importing ViCell_BLU data is not allowed.");
		return false;
	}

	auto he = ImportExportConfiguration::ImportConfigurationForOfflineAnalysis (instNode);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ImportConfigurationForOfflineAnalysis: <exit, failed to set the configuration tree>");
		return false;
	}

	return true;
}
