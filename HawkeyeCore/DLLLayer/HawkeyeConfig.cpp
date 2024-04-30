#include "stdafx.h"

#include "AppConfig.hpp"
#include "DBif_Api.h"
#include "FileSystemUtilities.hpp"
#include "HawkeyeAssert.hpp"
#include "HawkeyeConfig.hpp"

#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "ImageAnalysisParameters.hpp"
#include "InstrumentConfig.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "HawkeyeConfig";

static const std::string BRIGHTFIELD = "BRIGHTFIELD";
static const std::string TOP_1 = "TOP_1";
static const std::string BOTTOM_1 = "BOTTOM_1";
static const std::string TOP_2 = "TOP_2";
static const std::string BOTTOM_2 = "BOTTOM_2";
static const int TwoAmps = 1000000;

//*****************************************************************************
template <typename T>
static bool onValueChange (const PropertyField<T>& value)
{
	InstrumentConfig::Instance().Set();
	
	return true;
}

//*****************************************************************************
HawkeyeConfig::LedType HawkeyeConfig::positionToLedType (std::string position) {
	
	if (position == BRIGHTFIELD) {
		return HawkeyeConfig::LedType::LED_BrightField;
	} else if (position == TOP_1) {
		return HawkeyeConfig::LedType::LED_TOP1;
	} else if (position == BOTTOM_1) {
		return HawkeyeConfig::LedType::LED_BOTTOM1;
	} else if (position == TOP_2) {
		return HawkeyeConfig::LedType::LED_TOP2;
	} else if (position == BOTTOM_2) {
		return HawkeyeConfig::LedType::LED_BOTTOM2;
	} else {
		HAWKEYE_ASSERT (MODULENAME, false);
	}
}

//*****************************************************************************
HawkeyeConfig::LedConfig_t& HawkeyeConfig::getLedConfigByPositionName (std::string position) {

	return config_.leds.ledmap[positionToLedType (position)];
}

//*****************************************************************************
bool HawkeyeConfig::Initialize()
{
	if (isLoaded_) {
		Logger::L().Log (MODULENAME, severity_level::debug2, "Hawkeye configuration is already loaded.");
		return isLoaded_;
	}

	// Get InstrumentConfig by reference so that the config fields can be updated.
	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();

	//*************************************************************************************
	//NOTE: THE ORDER OF THIS CODE *MUST* MATCH THE ORDER IN THE *HawkeyeConfig.hpp* FILE.
	//*************************************************************************************

	Logger::L().Log (MODULENAME, severity_level::normal, "*** Static Configuration Parameters ***");

	// Static fields.
	{
		std::string type;
		switch (static_cast<InstrumentType>(instConfig.InstrumentType)) {
		default:
		case UnknownInstrument:
			type = "UNKNOWN";
			return isLoaded_;
		case ViCELL_BLU_Instrument:
			type = "Vi-CELL BLU";
			break;
		case ViCELL_FL_Instrument:
			type = "Vi-CELL FL";
			break;
		case CellHealth_ScienceModule:
			type = "CellHealth Science Module";
			break;
		}
	}

	// *** Static fields ***
	config_.autoFocusCoarseStartingPosition.init (&instConfig.AF_Settings.coarse_start);
	config_.autoFocusCoarseEndingPosition.init (&instConfig.AF_Settings.coarse_end);
	config_.autoFocusCoarseStep.init (&instConfig.AF_Settings.coarse_step);
	config_.autoFocusFineRange.init (&instConfig.AF_Settings.fine_range);
	config_.autoFocusFineStep.init (&instConfig.AF_Settings.fine_step);
	config_.autoFocusSharpnessLowThreshold.init (&instConfig.AF_Settings.sharpness_low_threshold);
//TODO: is save_image for autofocus still used...
	config_.abiMaxImageCount.init (&instConfig.AbiMaxImageCount);
	config_.manualSampleNudgeVolume.init (&instConfig.SampleNudgeVolume);
	config_.manualSampleNudgeSpeed.init (&instConfig.SampleNudgeSpeed);
	config_.flowCellDepthExperimentalConstant.init (&instConfig.FlowCellDepthConstant);
	config_.rfidTagSimulation_SetValidTagData.init (&instConfig.RfidSim.set_valid_tag_data);
	config_.rfidTagSimulation_TotalTags.init (&instConfig.RfidSim.total_tags);
	config_.rfidTagSimulationFile_MainBay.init (&instConfig.RfidSim.main_bay_file);
	config_.rfidTagSimulationFile_RightDoor.init (&instConfig.RfidSim.door_right_file);
	config_.rfidTagSimulationFile_LeftDoor.init (&instConfig.RfidSim.door_left_file);
	config_.generateLegacyCellCountingData.init (&instConfig.LegacyData);
	config_.nightlyCleanMinutesFromMidnight.init (&instConfig.NightlyCleanOffset);

	// These fields are not available in the DB.
	cameraTriggerTimeout_ms_ = 500;
	cameraImageCaptureTimeout_ms_ = 1000;
	
	config_.cameraType.init (&instConfig.CameraType);
	config_.cameraTriggerTimeout_ms.init (&cameraTriggerTimeout_ms_);
	config_.cameraImageCaptureTimeout_ms.init (&cameraImageCaptureTimeout_ms_);

	{
		if (!ImageAnalysisParameters::Instance().Initialize())
		{
			Logger::L().Log(MODULENAME, severity_level::error, "Initialize: <exit, ImageAnalysisParameters::Initialize failed>");
			return false;
		}

		DBApi::DB_ImageAnalysisParamRecord& imageAnalysisParams = ImageAnalysisParameters::Instance().Get();

		config_.cellCounting.algorithmMode.init (&imageAnalysisParams.AlgorithmMode);
		config_.cellCounting.backgroundIntensityTolerance.init (&imageAnalysisParams.BkgdIntensityTolerance);
		config_.cellCounting.bubbleMinimumSpotAreaPercentage.init (&imageAnalysisParams.BubbleMinSpotAreaPercent);
		config_.cellCounting.bubbleMinimumSpotAverageBrightness.init (&imageAnalysisParams.BubbleMinSpotAvgBrightness);
		config_.cellCounting.bubbleMode.init (&imageAnalysisParams.BubbleMode);
		config_.cellCounting.bubbleRejectionImgAreaPercentage.init (&imageAnalysisParams.BubbleRejectImageAreaPercent);
		config_.cellCounting.cellSpotBrightnessExclusionThreshold.init (&imageAnalysisParams.CellSpotBrightnessExclusionThreshold);
		config_.cellCounting.centerSpotMinimumIntensityLimit.init (&imageAnalysisParams.CenterSpotMinIntensityLimit);
		config_.cellCounting.concentrationImageControlCount.init (&imageAnalysisParams.ConcImageControlCount);

		DeclusterSetting declusterSetting = { };

		// Create the empty decluster *off* settings, this is not used.  It is for offsetting the indices for the other entries.
		config_.cellCounting.declusterSettings.push_back (declusterSetting);

		declusterSetting.accumulatorThreshold.init (&imageAnalysisParams.DeclusterAccumulatorThreshLow);
		declusterSetting.minimumDistanceThreshold.init (&imageAnalysisParams.DeclusterMinDistanceThreshLow);
		config_.cellCounting.declusterSettings.push_back (declusterSetting);

		declusterSetting.accumulatorThreshold.init (&imageAnalysisParams.DeclusterAccumulatorThreshMed);
		declusterSetting.minimumDistanceThreshold.init (&imageAnalysisParams.DeclusterMinDistanceThreshMed);
		config_.cellCounting.declusterSettings.push_back (declusterSetting);

		declusterSetting.accumulatorThreshold.init (&imageAnalysisParams.DeclusterAccumulatorThreshHigh);
		declusterSetting.minimumDistanceThreshold.init (&imageAnalysisParams.DeclusterMinDistanceThreshHigh);
		config_.cellCounting.declusterSettings.push_back (declusterSetting);

		config_.cellCounting.fovDepthMM.init (&imageAnalysisParams.FovDepthMM);
		config_.cellCounting.hotPixelEliminationMode.init (&imageAnalysisParams.HotPixelEliminationMode);
		config_.cellCounting.nominalBackgroundLevel.init (&imageAnalysisParams.NominalBkgdLevel);
		config_.cellCounting.peakIntensitySelectionAreaLimit.init (&imageAnalysisParams.PeakIntensitySelectionAreaLimit);
		config_.cellCounting.pixelFovMM.init (&imageAnalysisParams.PixelFovMM);
		config_.cellCounting.roiXCoordinate.init (&imageAnalysisParams.ROI_Xcoords);
		config_.cellCounting.roiYCoordinate.init (&imageAnalysisParams.ROI_Ycoords);
		config_.cellCounting.smallParticleSizingCorrection.init (&imageAnalysisParams.SmallParticleSizingCorrection);
		config_.cellCounting.subpeakAnalysisMode.init (&imageAnalysisParams.SubPeakAnalysisMode);
	}

	// *** Dynamic fields ***
	config_.instrumentType.init (&instConfig.InstrumentType, onValueChange<int16_t>);
	config_.brightfieldLedType.init(&instConfig.BrightFieldLedType, onValueChange<int16_t>);
	config_.instrumentSerialNumber.init (&instConfig.InstrumentSNStr, onValueChange<std::string>);
	config_.previousFocusPosition.init (&instConfig.FocusPosition, onValueChange<int32_t>);
	config_.flowCellDepth.init (&instConfig.FlowCellDepth, onValueChange<float>);
	config_.securityType.init (&instConfig.SecurityMode, onValueChange<int16_t>);
	config_.totalSamplesProcessed.init (&instConfig.TotalSamplesProcessed, onValueChange<int32_t>);
	config_.discardTrayCapacity.init (&instConfig.DiscardTrayCapacity, onValueChange<int16_t>);
	config_.inactivityTimeout_mins.init (&instConfig.InactivityTimeout, onValueChange<int16_t>);
	config_.passwordExpiration.init (&instConfig.PasswordExpiration, onValueChange<int16_t>);
	config_.lastNightCleanTime.init (&instConfig.LastNightlyClean, onValueChange<system_TP>);

	// CellHealth always has automation enabled (there is no Carousel or Plate).
	instConfig.AutomationEnabled = true;
	instConfig.ACupEnabled = true;
	instConfig.AutomationInstalled = true;
	
	config_.automationInstalled.init (&instConfig.AutomationInstalled, onValueChange<bool>);
	config_.automationEnabled.init (&instConfig.AutomationEnabled, onValueChange<bool>);
	config_.acupEnabled.init (&instConfig.ACupEnabled, onValueChange<bool>);
	config_.automationPort.init (&instConfig.AutomationPort, onValueChange<int32_t>);
	config_.isSimulatorCarousel.init (&instConfig.CarouselSimulator, onValueChange<bool>);
	config_.normalShutdown.init (&instConfig.NormalShutdown, onValueChange<bool>);

	config_.nextAnalysisDefIndex.init (&instConfig.NextAnalysisDefIndex, onValueChange<int16_t>);
	config_.nextBCICellTypeIndex.init (&instConfig.NextBCICellTypeIndex, onValueChange<int32_t>);

	config_.activeDirectory.server.init (&instConfig.AD_Settings.servername, onValueChange<std::string>);
	config_.activeDirectory.ipPort.init (&instConfig.AD_Settings.port_number, onValueChange<int32_t>);
	config_.activeDirectory.domain.init (&instConfig.AD_Settings.base_dn, onValueChange<std::string>);
	config_.activeDirectory.enabled.init (&instConfig.AD_Settings.enabled, onValueChange<bool>);

	config_.smtp.server.init (&instConfig.Email_Settings.server_addr, onValueChange<std::string>);
	config_.smtp.ipPort.init (&instConfig.Email_Settings.port_number, onValueChange<int32_t>);
	config_.smtp.username.init (&instConfig.Email_Settings.username, onValueChange<std::string>);
	config_.smtp.password.init (&instConfig.Email_Settings.pwd_hash, onValueChange<std::string>);
	config_.smtp.authEnabled.init (&instConfig.Email_Settings.authenticate, onValueChange<bool>);

	config_.motorCal.carouselThetaHomeOffset.init (&instConfig.CarouselThetaHomeOffset, onValueChange<int32_t>);
	config_.motorCal.carouselRadiusOffset.init (&instConfig.CarouselRadiusOffset, onValueChange<int32_t>);
	config_.motorCal.plateThetaHomeOffset.init (&instConfig.PlateThetaHomeOffset, onValueChange<int32_t>);
	config_.motorCal.plateThetaCalPos.init (&instConfig.PlateThetaCalPos, onValueChange<int32_t>);
	config_.motorCal.plateRadiusCenterPos.init (&instConfig.PlateRadiusCenterPos, onValueChange<int32_t>);

	config_.runOptions.sampleSetName.init (&instConfig.RunOptions.sample_set_name, onValueChange<std::string>);
	config_.runOptions.sampleName.init (&instConfig.RunOptions.sample_name, onValueChange<std::string>);
	config_.runOptions.saveImageCount.init (&instConfig.RunOptions.save_image_count, onValueChange<int16_t>);
	config_.runOptions.saveNthImage.init (&instConfig.RunOptions.save_nth_image, onValueChange<int16_t>);
	config_.runOptions.resultsExport.init (&instConfig.RunOptions.results_export, onValueChange<bool>);
	config_.runOptions.resultsExportFolder.init (&instConfig.RunOptions.results_export_folder, onValueChange<std::string>);
	config_.runOptions.appendResultsExport.init (&instConfig.RunOptions.append_results_export, onValueChange<bool>);
	config_.runOptions.appendResultsExportFolder.init (&instConfig.RunOptions.append_results_export_folder, onValueChange<std::string>);
	config_.runOptions.resultFilename.init (&instConfig.RunOptions.result_filename, onValueChange<std::string>);
	config_.runOptions.resultsFolder.init (&instConfig.RunOptions.results_folder, onValueChange<std::string>);
	config_.runOptions.autoExportPdf.init (&instConfig.RunOptions.auto_export_pdf, onValueChange<bool>);
	config_.runOptions.csvFolder.init (&instConfig.RunOptions.csv_folder, onValueChange<std::string>);
	config_.runOptions.washType.init (&instConfig.RunOptions.wash_type, onValueChange<int16_t>);
	config_.runOptions.dilution.init (&instConfig.RunOptions.dilution, onValueChange<int16_t>);
	config_.runOptions.bpqcCellTypeIndex.init (&instConfig.RunOptions.bpqc_cell_type_index, onValueChange<uint32_t>);

	DBApi::DB_IlluminatorRecord illuminator = {};
	auto foundIlluminator = false;
	//Find the relevant illuminator_info
	auto illuminatorInfo =
		std::find_if(instConfig.IlluminatorsInfoList.begin(), instConfig.IlluminatorsInfoList.end(), [](DBApi::illuminator_info info)
			{
				return info.type == static_cast<int16_t>(HawkeyeConfig::LedType::LED_BrightField);
			});
	if(illuminatorInfo != instConfig.IlluminatorsInfoList.end())
	{
		//Found the illuminator record. Now find the config for it.
		auto result = DBApi::DbFindIlluminatorsByIndex(illuminator, illuminatorInfo->index);
		if(result == DBApi::eQueryResult::QueryOk)
		{
			foundIlluminator = true;
		}
	}
	if (!foundIlluminator)
	{
		//For some reason we didn't find the illuminator config. Add a default.
		illuminator.IlluminatorIndex = static_cast<int16_t>(LedType::LED_BrightField);
		illuminator.IlluminatorType = config_.brightfieldLedType;
		illuminator.IlluminatorNameStr = "BRIGHTFIELD";
		illuminator.PositionNum = static_cast<int16_t>(LedType::LED_BrightField);
		illuminator.Tolerance = 0.1f;
		illuminator.MaxVoltage = 1500000;
		illuminator.IlluminatorWavelength = 999;
		illuminator.EmissionWavelength = 0;
		//TODO: this is not used from the DB: illuminator.ExposureTimeMs.
		// The value is specified by the appropriate script file.
		illuminator.PercentPower = 20;
		illuminator.SimmerVoltage = 0;
		illuminator.Ltcd = 100;
		illuminator.Ctld = 100;
		illuminator.FeedbackDiode = 1;

		// This does not have to be done as the BrightField LED is in the DB template by default.
		DBApi::eQueryResult dbStatus = DBApi::DbAddIlluminator(illuminator);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			if (dbStatus != DBApi::eQueryResult::InsertObjectExists)
			{
				Logger::L().Log(MODULENAME, severity_level::error,
					boost::str(boost::format("Initialize: <exit, DbAddIlluminator failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_readerror,
					instrument_error::instrument_storage_instance::instrument_configuration,
					instrument_error::severity_level::warning));
				return false;
			}

			Logger::L().Log(MODULENAME, severity_level::warning,
				boost::str(boost::format("Initialize: <%s illuminator already exists>")
					% illuminator.IlluminatorNameStr));
		}
	}

	//Add in how long the camera is open past the trigger width
	illuminator.Ctld += ExposureOffset;
	
	LedConfig_t ledConfig;
	ledConfig.FromDbStyle (illuminator);
	config_.leds.ledmap.insert (std::pair<LedType, LedConfig_t>(LedType::LED_BrightField, ledConfig));

	omicronTolerance_ = 0.1f;
	config_.leds.omicronTolerance.init (&omicronTolerance_);
	bciTolerance_ = 0.1f;
	config_.leds.bciTolerance.init (&bciTolerance_);
	// BCI LED 500000uV -> 1Amp, max current is 2A which is 1000000uV
	maxBCILedVoltage_uv_ = TwoAmps;
	config_.leds.maxBCILedVoltage_uv.init (&maxBCILedVoltage_uv_);

	Logger::L().Log (MODULENAME, severity_level::normal, "initialize: " + config_.ToStr());

	isLoaded_ = true;

	return isLoaded_;
}

//*****************************************************************************
HawkeyeConfig::HawkeyeConfig_t& HawkeyeConfig::get() {

	return config_;
}

//*****************************************************************************
void HawkeyeConfig::setHardwareForCHM()
{
	config_.hardwareConfig.camera = true;
	config_.hardwareConfig.eePromController = true;
	config_.hardwareConfig.focusController = true;
	config_.hardwareConfig.led = true;
	config_.hardwareConfig.syringePump = true;
	config_.hardwareConfig.reagentController = false;
	config_.hardwareConfig.stageController = false;
	config_.hardwareConfig.voltageMeasurement = true;
	config_.hardwareConfig.controllerBoard = true;
	config_.hardwareConfig.discardTray = false;
}

//*****************************************************************************
void HawkeyeConfig::setHardwareForViCell()
{
	config_.hardwareConfig.camera = true;
	config_.hardwareConfig.eePromController = true;
	config_.hardwareConfig.focusController = true;
	config_.hardwareConfig.led = true;
	config_.hardwareConfig.syringePump = true;
	config_.hardwareConfig.reagentController = true;
	config_.hardwareConfig.stageController = true;
	config_.hardwareConfig.voltageMeasurement = true;
	config_.hardwareConfig.controllerBoard = true;
	config_.hardwareConfig.discardTray = true;
}

//*****************************************************************************
void HawkeyeConfig::setHardwareForSimulation()
{
	config_.hardwareConfig.camera = false;
	config_.hardwareConfig.eePromController = false;
	config_.hardwareConfig.focusController = false;
	config_.hardwareConfig.led = false;
	config_.hardwareConfig.syringePump = false;
	config_.hardwareConfig.reagentController = false;
	config_.hardwareConfig.stageController = false;
	config_.hardwareConfig.voltageMeasurement = false;
	config_.hardwareConfig.controllerBoard = false;
	config_.hardwareConfig.discardTray = false;
}

//*****************************************************************************
bool HawkeyeConfig::ConfigFileExists()
{
	std::string encryptedFileName;
	bool isValidFilename = HDA_GetEncryptedFileName (HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::HawkeyeConfig), encryptedFileName);
	if (isValidFilename && FileSystemUtilities::FileExists(encryptedFileName))
	{
		return true;
	}
	else
	{
		return false;
	}
}

//*****************************************************************************
bool HawkeyeConfig::isValidInstrumentType()
{
	return config_.instrumentType != InstrumentType::UnknownInstrument;
}

//*****************************************************************************
bool HawkeyeConfig::isValidInstrumentType(const InstrumentType& type)
{
	return isValidInstrumentType() && config_.instrumentType == type;
}

//*****************************************************************************
void HawkeyeConfig::SetDatabaseConfig (DatabaseConfig_t& dbConfig)
{
	LoadDeveloperConfigFile (dbConfig);

	config_.database.ipAddr.init (&dbConfig.ipAddr, onValueChange<std::string>);
	config_.database.ipPort.init (&dbConfig.ipPort, onValueChange<std::string>);
	config_.database.dbName.init (&dbConfig.dbName, onValueChange<std::string>);
}

//*****************************************************************************
bool HawkeyeConfig::LoadDeveloperConfigFile (DatabaseConfig_t& dbConfig)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "LoadConfigFile: <enter>");

	AppConfig appCfg_ = {};
	DatabaseConfig_t devDbConfig = {};
	
	std::string infoFile = HawkeyeDirectory::Instance().getFilePath (HawkeyeDirectory::FileType::HawkeyeConfig);
	t_opPTree pConfig_ = appCfg_.initwithtag (infoFile, "config.database", true);
	if (!pConfig_)
	{
		return false;
	}

	try
	{
		devDbConfig.ipAddr = pConfig_->get<std::string>("ip_addr");
		devDbConfig.ipPort = pConfig_->get<std::string>("ip_port");
		devDbConfig.dbName = pConfig_->get<std::string>("db_name");
	}
	catch (...)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "LoadConfigFile: <exit, unable to read developer database configuratoin>");
		return false;
	}

	dbConfig = devDbConfig;

	Logger::L().Log (MODULENAME, severity_level::debug1, "LoadConfigFile: <exit>");

	return true;
}

//*****************************************************************************
// For v1.2 data...
//*****************************************************************************
void HawkeyeConfig::ImportControllerCalInfoFile (boost::property_tree::ptree& ptConfig)
{
	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();

	try
	{
		boost::property_tree::ptree& ptCarouselConfig = ptConfig.get_child ("carousel_controller");
		boost::property_tree::ptree& ptControllerConfig = ptCarouselConfig.get_child ("controller_config");
		instConfig.CarouselThetaHomeOffset = ptControllerConfig.get<int32_t>("CarouselThetaHomePosOffset");
		instConfig.CarouselRadiusOffset = ptControllerConfig.get<int32_t>("CarouselRadiusOffset");

		boost::property_tree::ptree& ptPlateConfig = ptConfig.get_child ("plate_controller");
		ptControllerConfig = ptPlateConfig.get_child ("controller_config");
		instConfig.PlateThetaHomeOffset = ptControllerConfig.get<int32_t>("PlateThetaHomePosOffset");
		instConfig.PlateThetaCalPos = ptControllerConfig.get<int32_t>("PlateThetaCalPos");
		instConfig.PlateRadiusCenterPos = ptControllerConfig.get<int32_t>("PlateRadiusCenterPos");
	}
	catch (...)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, 
			"Failed to read ControllerCal.einfo file, the content tags have no value(s)");
	}

	InstrumentConfig::Instance().Set();
}

//*****************************************************************************
// For v1.2 data...
//*****************************************************************************
void HawkeyeConfig::ImportHawkeyeDynamicInfoFile (boost::property_tree::ptree& ptConfig, bool setSerNo)
{
	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();
	if (setSerNo)
		instConfig.InstrumentSNStr = ptConfig.get<std::string>("instrument_serial_number");

	instConfig.FocusPosition   = ptConfig.get<int32_t>("previous_focus_position");
	instConfig.FlowCellDepth   = ptConfig.get<float>("flow_cell_depth");
	instConfig.SecurityMode    = ptConfig.get<int16_t>("is_security");
	instConfig.TotalSamplesProcessed = ptConfig.get<int32_t>("total_samples_processed");
	instConfig.DiscardTrayCapacity   = ptConfig.get<int16_t>("discard_tray_capacity");
	instConfig.InactivityTimeout     = ptConfig.get<int16_t>("inactivity_timeout_mins");
	instConfig.PasswordExpiration = ptConfig.get<int16_t>("password_expiration");
	uint32_t expTime = ptConfig.get<uint32_t>("last_night_clean_time");
	instConfig.LastNightlyClean = ChronoUtilities::ConvertToTimePoint<std::chrono::hours>(expTime);
	instConfig.NormalShutdown = ptConfig.get<bool>("normal_shutdown");
	
	InstrumentConfig::Instance().Set();
}

//*****************************************************************************
// For v1.2 data...
//*****************************************************************************
void HawkeyeConfig::ImportHawkeyeStaticInfoFile (boost::property_tree::ptree& ptConfig)
{
	DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();

	instConfig.InstrumentType = ptConfig.get<int16_t>("instrument_type");

	instConfig.AF_Settings.coarse_start = ptConfig.get<int32_t>("auto_focus_coarse_starting_position");
	instConfig.AF_Settings.coarse_end   = ptConfig.get<int32_t>("auto_focus_coarse_ending_position");
	instConfig.AF_Settings.coarse_step  = ptConfig.get<int16_t>("auto_focus_coarse_step");
	instConfig.AF_Settings.fine_range   = ptConfig.get<int32_t>("auto_focus_fine_range");
	instConfig.AF_Settings.fine_step    = ptConfig.get<int16_t>("auto_focus_fine_step");
	instConfig.AF_Settings.sharpness_low_threshold = ptConfig.get<int16_t>("auto_focus_sharpness_low_threshold");

	instConfig.AbiMaxImageCount  = ptConfig.get<int16_t>("abi_max_image_count");
	instConfig.SampleNudgeVolume = ptConfig.get<int16_t>("manual_sample_nudge_volume");
	instConfig.SampleNudgeSpeed  = ptConfig.get<int16_t>("manual_sample_nudge_speed");
	instConfig.FlowCellDepthConstant = ptConfig.get<float>("flowcelldepth_experimental_constant");
	
	instConfig.RfidSim.set_valid_tag_data = ptConfig.get<bool>("rfid_tag_simulation_set_valid_tagdata");
	instConfig.RfidSim.total_tags      = ptConfig.get<int16_t>("rfid_tag_simulation_total_tags");
	instConfig.RfidSim.main_bay_file   = ptConfig.get<std::string>("rfid_tag_simulation_file_main_bay");
	instConfig.RfidSim.door_left_file  = ptConfig.get<std::string>("rfid_tag_simulation_file_left_door");
	instConfig.RfidSim.door_right_file = ptConfig.get<std::string>("rfid_tag_simulation_file_right_door");

	instConfig.LegacyData = ptConfig.get<bool>("generate_legacy_cellcounting_data");
	instConfig.CarouselSimulator  = ptConfig.get<bool>("is_carousel_simulator");
	instConfig.NightlyCleanOffset = ptConfig.get<int16_t>("nightly_clean_minutes_from_midnight");

	InstrumentConfig::Instance().Set();

	// Do not need to import the Brightfield LED configuration.
	// It is in the DB by default.

	boost::property_tree::ptree& ptConfig2 = ptConfig.get_child ("cell_counting");

	DBApi::DB_ImageAnalysisParamRecord& iapRecord = ImageAnalysisParameters::Instance().Get();

	iapRecord.AlgorithmMode = ptConfig2.get<int32_t>("algorithm_mode");
	iapRecord.BubbleMode = ptConfig2.get<bool>("bubble_mode");
	iapRecord.SubPeakAnalysisMode = ptConfig2.get<bool>("subpeak_analysis_mode");
	iapRecord.ROI_Xcoords = ptConfig2.get<int32_t>("roi_x_coordinate");
	iapRecord.ROI_Ycoords = ptConfig2.get<int32_t>("roi_y_coordinate");
	iapRecord.DeclusterAccumulatorThreshLow = ptConfig2.get<int32_t>("decluster_accumulator_threshold_low");
	iapRecord.DeclusterMinDistanceThreshLow = ptConfig2.get<int32_t>("decluster_minimum_distance_threshold_low");
	iapRecord.DeclusterAccumulatorThreshMed = ptConfig2.get<int32_t>("decluster_accumulator_threshold_medium");
	iapRecord.DeclusterMinDistanceThreshMed = ptConfig2.get<int32_t>("decluster_minimum_distance_threshold_medium");
	iapRecord.DeclusterAccumulatorThreshHigh = ptConfig2.get<int32_t>("decluster_accumulator_threshold_high");
	iapRecord.DeclusterMinDistanceThreshHigh = ptConfig2.get<int32_t>("decluster_minimum_distance_threshold_high");
	iapRecord.FovDepthMM = ptConfig2.get<double>("fov_depth_mm");
	iapRecord.PixelFovMM = ptConfig2.get<double>("pixel_fov_mm");
	iapRecord.ConcImageControlCount = ptConfig2.get<int32_t>("concentration_image_control_count");
	iapRecord.BubbleMinSpotAreaPercent = ptConfig2.get<float>("bubble_minimum_spot_area_percentage");
	iapRecord.BubbleMinSpotAvgBrightness = ptConfig2.get<float>("bubble_minimum_spot_average_brightness");
	iapRecord.BubbleRejectImageAreaPercent = ptConfig2.get<float>("bubble_rejection_image_area_percentage");
	iapRecord.NominalBkgdLevel = ptConfig2.get<double>("nominal_background_level");
	iapRecord.BkgdIntensityTolerance = ptConfig2.get<double>("background_intensity_tolerance");
	iapRecord.CenterSpotMinIntensityLimit = ptConfig2.get<float>("center_spot_minimum_intensity_limit");
	iapRecord.PeakIntensitySelectionAreaLimit = ptConfig2.get<double>("peak_intensity_selection_area_limit");
	iapRecord.CellSpotBrightnessExclusionThreshold = ptConfig2.get<float>("cell_spot_brightness_exclusion_threshold");
	iapRecord.HotPixelEliminationMode = ptConfig2.get<double>("hot_pixel_elimination_mode");
	iapRecord.SmallParticleSizingCorrection = ptConfig2.get<double>("small_particle_sizing_correction");

	ImageAnalysisParameters::Instance().Set();
}
