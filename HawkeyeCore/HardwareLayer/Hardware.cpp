#include "stdafx.h"

#include <future>

#include "AuditLog.hpp"
#include "BCILed.hpp"
#include "BeckmanCamera.hpp"
#include "CameraSim.hpp"
#include "CellCounterFactory.h"
#include "DataConversion.hpp"
#include "EnumConversion.hpp"
#include "ErrorCode.hpp"
#include "FocusControllerSim.hpp"
#include "GetAsStrFunctions.hpp"
#include "Hardware.hpp"
#include "HawkeyeDirectory.hpp"
#include "InstrumentConfig.hpp"
#include "MotorStatusDLL.hpp"
#include "OmicronLed.hpp"
#include "ReagentController.hpp"
#include "ReagentControllerSim.hpp"
#include "SyringePump.hpp"
#include "SyringePumpSim.hpp"
#include "StageController.hpp"
#include "StageControllerSim.hpp"
#include "UserList.hpp"
#include "VoltageMeasurement.hpp"
#include "VoltageMeasurementSim.hpp"

static const char MODULENAME[] = "Hardware";

#define FIRMWARE_TRANSFER_COMPLETE      100
#define FIRMWARE_APP_IDENTIFIER         1	// "1" - Identifier for application firmware

// *UpdateSystemStatus* is called every 500ms.
// The voltages are read every three seconds.
static const uint8_t VoltageUpdateInterval = 6;
static uint8_t voltageUpdateIntervalCount = VoltageUpdateInterval;	// Force the initial update of the voltages.

const std::map<Hardware::InitializationSequence, std::string>
EnumConversion<Hardware::InitializationSequence>::enumStrings<Hardware::InitializationSequence>::data =
{
	{ Hardware::InitializationSequence::Start, std::string("Start") },
	{ Hardware::InitializationSequence::FirmwareUpdate, std::string("FirmwareUpdate") },
	{ Hardware::InitializationSequence::Probe, std::string ("Probe") },
	{ Hardware::InitializationSequence::Stage, std::string("Stage") },
	{ Hardware::InitializationSequence::Camera, std::string("Camera") },
	{ Hardware::InitializationSequence::EEPROM, std::string("EEPROM") },
	{ Hardware::InitializationSequence::FocusController, std::string("FocusController") },
	{ Hardware::InitializationSequence::BrightfieldLed, std::string("BrightfieldLed") },
	{ Hardware::InitializationSequence::ReagentController, std::string("ReagentController") },
	{ Hardware::InitializationSequence::Syringe, std::string("Syringe") },
	{ Hardware::InitializationSequence::VoltageMeasurement, std::string("VoltageMeasurement") },
	{ Hardware::InitializationSequence::Complete, std::string("Complete") },
	{ Hardware::InitializationSequence::Error, std::string("Error") },
};

//*****************************************************************************
Hardware::Hardware ()
	: inited_(false)
	, stopSystemStatusUpdateTimer_(false)
	, isFirmwareUpdated_(false)
	, needFirmwareUpdate_ (false)
	, isFirmwareUpdateFailed_(false)
	, carrierType_(eCarrierType::eCarousel)
{
}

//*****************************************************************************
Hardware::~Hardware()
{
	stopSystemStatusUpdateTimer_ = true;
	systemStatusUpdateTimer_->cancel();
	
	// There are in the opposite order of their creation.
	pVoltageMeasurement_.reset();
	pSyringePump_.reset();
	pReagentController_.reset();
	pFocusController_.reset();
	pEEPROMController_.reset();
	pCamera_.reset();
	pCBOService_.reset();
}

//*****************************************************************************
void Hardware::initialize (
	std::shared_ptr<boost::asio::io_context> pUpstream,
	InitializationState initState,
	std::function<void (bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

#if 0 // for *ErrorCode* testing...
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026001: " + ErrorCode (0x00026001).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026002: " + ErrorCode (0x00026002).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026003: " + ErrorCode (0x00026003).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026004: " + ErrorCode (0x00026004).getAsString ());

	Logger::L().Log (MODULENAME, severity_level::error, "0x00026101: " + ErrorCode (0x00026101).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026110: " + ErrorCode (0x00026110).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026120: " + ErrorCode (0x00026120).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026130: " + ErrorCode (0x00026130).getAsString ());

	Logger::L().Log (MODULENAME, severity_level::error, "0x00026201: " + ErrorCode (0x00026201).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026210: " + ErrorCode (0x00026210).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026214: " + ErrorCode (0x00026214).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026220: " + ErrorCode (0x00026220).getAsString ());

	Logger::L().Log (MODULENAME, severity_level::error, "0x00026301: " + ErrorCode (0x00026301).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026305: " + ErrorCode (0x00026305).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026308: " + ErrorCode (0x00026308).getAsString ());
	Logger::L().Log (MODULENAME, severity_level::error, "0x00026310: " + ErrorCode (0x00026310).getAsString ());
#endif

	initializationState_ = initState;
	pCBOService_ = std::make_shared<CBOService>(pUpstream);
	stopSystemStatusUpdateTimer_ = false;
	systemStatusUpdateTimer_ = std::make_shared <boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());

	if (inited_)
	{
		Logger::L ().Log (MODULENAME, severity_level::debug1, "Hardware already initialized");
		pCBOService_->enqueueExternal (callback, inited_);
		return;
	}

	const auto initializeComplete = [this, callback](bool status) -> void
	{
		// Reset the "initializationState_" if no error has occurred
		if (initializationState_ == InitializationState::eInitializationInProgress)
		{
			initializationState_ = boost::none;
		}

		pCBOService_->enqueueExternal (callback, status);

		inited_ = status;
		if (inited_)
		{
			this->setInitialSystemStatus ();
		}
	};

	const auto cboInitializeComplete = [this, initializeComplete](bool status) -> void
	{
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance (instrument_error::controller_general_connectionerror, instrument_error::cntrlr_general_instance::none, instrument_error::severity_level::error));
			setNextInitializationSequence (initializeComplete, InitializationSequence::Error);
			return;
		}

		if (HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard)
		{
			Logger::L ().Log (MODULENAME, severity_level::debug1, "Initialize: <with hardware>");

			const uint32_t fwVersion = pCBOService_->CBO()->GetFirmwareVersionUint32();
			if ((fwVersion & FW_IDENTIFIER_MASK) != FW_IDENTIFIER_APPLICATION)
			{
				needFirmwareUpdate_ = true;
			}
		}
		else
		{
			Logger::L ().Log (MODULENAME, severity_level::debug1, "Initialize: <without controller board>");
		}

		// Create all the hardware module objects before starting the initialization
		if (!createHardwareModules())
		{
			setNextInitializationSequence (initializeComplete, InitializationSequence::Error);
			return;
		}

		setNextInitializationSequence (initializeComplete, InitializationSequence::FirmwareUpdate);
	};

	// This must occur here, otherwise the wrong io_context (io_service) is assigned to pCBOService.
	pCBOService_->SetCBO (std::make_shared<ControllerBoardOperation>(pCBOService_->getInternalIos(), CNTLR_SN_A_STR, CNTLR_SN_B_STR));

	if (HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard)
	{
		pCBOService_->Initialize (cboInitializeComplete);
	}
	else
	{
		pCBOService_->enqueueInternal (std::bind(cboInitializeComplete, true));
	}
}

bool Hardware::isConfiguredHWPresent()
{
	//First see if we can initialize with the hardware config we were given.
	auto brightfield = HawkeyeConfig::Instance().get().brightfieldLedType;
	auto cameraType = HawkeyeConfig::Instance().get().cameraType;

	auto camera = createCamera(cameraType);
	auto led = createBrightFieldLed(brightfield, pCBOService_);

	Logger::L().Log(MODULENAME, severity_level::normal, "isConfiguredHWPresent : brightfieldLedType=" + std::to_string(HawkeyeConfig::Instance().get().brightfieldLedType) + ".");
	Logger::L().Log(MODULENAME, severity_level::normal, "isConfiguredHWPresent : cameraType=" + std::to_string(HawkeyeConfig::Instance().get().cameraType) + ".");

	auto cameraIsPresent = camera->isPresent();
	auto ledIsPresent = led->IsPresent();

	bool configValid = false;
	for (auto testConfig : HawkeyeConfig::OpticalHardwarePairings)
	{
		if (testConfig.hardware.camera == cameraType && testConfig.hardware.brightfieldLed == brightfield)
		{
			configValid = true;
		}
	}

	if (configValid && cameraIsPresent && ledIsPresent)
	{
		//Init'ed ok. Return without doing anything else.
		return true;
	}

	if (!configValid)
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "isConfiguredHWPresent: configValid=" + std::to_string(configValid) + ".");
	}

	if (!cameraIsPresent)
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "isConfiguredHWPresent: cameraIsPresent=" + std::to_string(cameraIsPresent) + ".");
	}

	if (!ledIsPresent)
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "isConfiguredHWPresent: ledIsPresent=" + std::to_string(ledIsPresent) + ".");
	}

	return false;

}

bool Hardware::detectHardware()
{

	//First see if we can initialize with the hardware config we were given.
	auto brightfield = HawkeyeConfig::Instance().get().brightfieldLedType;

	bool foundConfig = false;
	HawkeyeConfig::OpticalHardwareTypePairing foundConfigType;
	bool isOmicronPresent = false;

	if(brightfield == HawkeyeConfig::BrightfieldLedType::Omicron)
	{
		auto led = createBrightFieldLed(brightfield, pCBOService_);
		isOmicronPresent = led->IsPresent();

	}
	//Go through all valid hardware configs
	Logger::L().Log(MODULENAME, severity_level::normal, "detectHardware: isOmicronPresent=" + std::to_string(isOmicronPresent) + ".");

	Logger::L().Log(MODULENAME, severity_level::normal, "detectHardware: Iterating through optical pairings....");
	for (auto attempt : HawkeyeConfig::OpticalHardwarePairings)
	{
		//Create this particular camera and led pairing.
		auto camera = createCamera(attempt.hardware.camera);
		auto led = createBrightFieldLed(attempt.hardware.brightfieldLed, pCBOService_);

		auto cameraOkInThisConfig = camera->isPresent();
		auto ledOkInThisConfig = led->IsPresent() &&
			!(isOmicronPresent &&
				attempt.hardware.brightfieldLed == HawkeyeConfig::BrightfieldLedType::BCI);//Make sure we don't accidentally detect a BCI as omicron
		if(cameraOkInThisConfig && ledOkInThisConfig)
		{
			//This particular pairing seems valid.
			foundConfig = true;
			foundConfigType = attempt;
		}

		Logger::L().Log(MODULENAME, severity_level::normal, "detectHardware: cameraOkInThisConfig=" + std::to_string(cameraOkInThisConfig) + ".");
		Logger::L().Log(MODULENAME, severity_level::normal, "detectHardware: ledOkInThisConfig=" + std::to_string(ledOkInThisConfig) + ".");
	}

	if(foundConfig)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "detectHardware: found Config, setting optical hardware configuration...");
		//Save the config we found
		InstrumentConfig::Instance().setOpticalHardwareConfig(foundConfigType.type);
	}

	return foundConfig;
}

//*****************************************************************************
void Hardware::StopHardwareOperations()
{ 
	pCBOService_->CBO()->HaltOperation();
	if (pCamera_ != nullptr)
	{
		pCamera_->shutdown();
	}
	stopSystemStatusUpdateTimer_ = true;
	systemStatusUpdateTimer_->cancel();
	initializationState_ = boost::none;
	//pCBOService_->StopServices();
}

std::shared_ptr<LedBase> Hardware::createBrightFieldLed(int16_t led, std::shared_ptr<CBOService>& cbo)
{
	if (led == HawkeyeConfig::BrightfieldLedType::Omicron) {
		return std::make_shared<OmicronLed>(cbo);
	}
	else if (led == HawkeyeConfig::BrightfieldLedType::BCI) {
		return std::make_shared<BCILed>(cbo);
	}
	Logger::L().Log(MODULENAME, severity_level::critical, "Unknown brightfield LED specified: " +
		std::to_string(HawkeyeConfig::Instance().get().brightfieldLedType) + " Initialization Stopped");
	return nullptr;
	
}

std::shared_ptr<iCamera> Hardware::createCamera(int16_t camera)
{
	if (camera == HawkeyeConfig::CameraType::Allied)
	{
		return std::make_shared<Camera_Allied>();
	}

	return std::make_shared<Camera_Basler>(); //By default make a Basler camera. If it's not basler, it'll later fail initializaiton.
}


//*****************************************************************************
bool Hardware::createHardwareModules()
{
	std::shared_ptr<LedBase> pLed;

	if (HawkeyeConfig::Instance().get().hardwareConfig.camera)
	{
		if(HawkeyeConfig::Instance().get().cameraType == HawkeyeConfig::CameraType::UnknownCamera ||
			HawkeyeConfig::Instance().get().brightfieldLedType == HawkeyeConfig::BrightfieldLedType::UnknownLed)
		{
			Logger::L().Log(MODULENAME, severity_level::critical, "Unknown camera or led specified, LED: " +
				std::to_string(HawkeyeConfig::Instance().get().brightfieldLedType) + " camera: " + 
				std::to_string(HawkeyeConfig::Instance().get().cameraType));
		}

		//Check if we have omicron. Set the LED to omicron if so. This has to be done at the start.
		auto led = createBrightFieldLed(HawkeyeConfig::BrightfieldLedType::Omicron, pCBOService_);
		if (led->IsPresent())
		{
			InstrumentConfig::Instance().setLedType(HawkeyeConfig::BrightfieldLedType::Omicron);
		}
		if (!isConfiguredHWPresent())
		{
			auto detectedHw = detectHardware();
			if (!detectedHw)
			{
				Logger::L().Log(MODULENAME, severity_level::critical, "Unknown hardware config, LED: " +
					std::to_string(HawkeyeConfig::Instance().get().brightfieldLedType) + " camera: " +
					std::to_string(HawkeyeConfig::Instance().get().cameraType));
			}
		}
		pCamera_ = std::make_shared<BeckmanCamera>(pCBOService_);
	}
	else
	{
		pCamera_ = std::make_shared<CameraSim>(pCBOService_);
	}
	
	pEEPROMController_ = std::make_shared<EEPROMController>(pCBOService_);
	if (HawkeyeConfig::Instance().get().hardwareConfig.focusController)
	{
		pFocusController_ = std::make_shared<FocusController>(pCBOService_);
	}
	else
	{
		pFocusController_ = std::make_shared<FocusControllerSim>(pCBOService_);
	}
	
	if (HawkeyeConfig::Instance().get().hardwareConfig.led)
	{
		auto ledType = HawkeyeConfig::Instance().get().brightfieldLedType;
		pLed = createBrightFieldLed(ledType, pCBOService_);
		if (pLed == nullptr) return false;
	}
	else
	{
		pLed = std::make_shared<LedBase>(pCBOService_);
	}
	
	if (HawkeyeConfig::Instance().get().hardwareConfig.reagentController)
	{
		pReagentController_ = std::make_shared<ReagentController>(pCBOService_);
	}
	else
	{
		pReagentController_ = std::make_shared<ReagentControllerSim>(pCBOService_);
	}
	
	if (HawkeyeConfig::Instance().get().hardwareConfig.stageController)
	{
		pStageController_ = std::make_shared<StageController>(pCBOService_);
	}
	else
	{
		pStageController_ = std::make_shared<StageControllerSim>(pCBOService_);
	}
	
	if (HawkeyeConfig::Instance().get().hardwareConfig.syringePump)
	{
		pSyringePump_ = std::make_shared<SyringePump>(pCBOService_);
	}
	else
	{
		pSyringePump_ = std::make_shared<SyringePumpSim>(pCBOService_);
	}

	if (HawkeyeConfig::Instance().get().hardwareConfig.voltageMeasurement)
	{
		pVoltageMeasurement_ = std::make_shared<VoltageMeasurement>(pCBOService_);
	}
	else
	{
		pVoltageMeasurement_ = std::make_shared<VoltageMeasurementSim>(pCBOService_);
	}

//TODO: Hunter, add other LEDs based on the contents of HawkeyeStatic.info.
	ledObjects_[HawkeyeConfig::LedType::LED_BrightField] = pLed;

	std::string log_string = { };

	if (!pCamera_)
		log_string.append("CAMERA  ");
	else if (!pEEPROMController_)
		log_string.append("EEPROM  ");
	else if (!pFocusController_)
		log_string.append("FOCUS CONTROLLER  ");
	else if (!getLedSharedPtr(HawkeyeConfig::LedType::LED_BrightField))
		log_string.append("BF LED  ");
	else if (!pSyringePump_)
		log_string.append("SYRINGE PUMP  ");
	else if (!pReagentController_)
		log_string.append("REAGENT CONTROLLER  ");
	else if (!pStageController_)
		log_string.append("STAGE CONTROLLER  ");
	else if(!pVoltageMeasurement_)
		log_string.append("VOLTAGE MEASUREMENT  ");

	if (!log_string.empty()) 		
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "Failed to Instantiate: " + log_string + "Initialization Stopped");
		return false;
	}

	return true;
}
//*****************************************************************************
void Hardware::internalInitializationProcess (std::function<void(bool)> callback, InitializationSequence sequence)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitializationProcess : " + EnumConversion<InitializationSequence>::enumToString(sequence));

	switch (sequence)
	{
		case InitializationSequence::FirmwareUpdate:
		{
			if (!HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard)
			{
				setNextInitializationSequence (callback, InitializationSequence::Probe);
				break;
			}

			std::string binFileFullPath;
			std::string hashFileFullPath;
			std::string binFileName;
			std::string hashFileName;

			if (getFirmware(binFileFullPath, hashFileFullPath, binFileName, hashFileName))
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("updating firmware with \n bin file : %s\n hash file : %s") % binFileFullPath % hashFileFullPath));
				updateFirmware ([this, callback, binFileFullPath, hashFileFullPath, binFileName, hashFileName](bool status) -> void
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitializationProcess:: updateFirmware " + std::string(status ? "success" : "failure"));
					if (status)
					{
						auto fw_downloader = gefirmwareDownloader();
						auto prev_fw_ver_str = fw_downloader->GetFirmwareVersionString(fw_downloader->GetPreviousFirmwareVersion());
						auto curr_fw_ver_str = fw_downloader->GetFirmwareVersionString(fw_downloader->GetCurrentFirmwareVersion());
						AuditLogger::L().Log (generateAuditWriteData(
							"",
							audit_event_type::evt_firmwareupdate,
							"Previous: " + prev_fw_ver_str + "\nCurrent: " + curr_fw_ver_str));

						needFirmwareUpdate_ = false;
						isFirmwareUpdated_ = true;
						archiveFirmware(binFileFullPath, hashFileFullPath, binFileName, hashFileName);
						setNextInitializationSequence (callback, InitializationSequence::Probe);
					}
					else
					{
						auto fw_downloader = gefirmwareDownloader();
						auto curr_fw_ver_str = fw_downloader->GetFirmwareVersionString(fw_downloader->GetCurrentFirmwareVersion());
						AuditLogger::L().Log (generateAuditWriteData(
							"",
							audit_event_type::evt_firmwareupdate, 
							"Update failure!\nCurrent: " + curr_fw_ver_str));

						isFirmwareUpdateFailed_ = true;
						initializationState_ = InitializationState::eFirmwareUpdateFailed;
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::controller_general_firmwareupdate,
							instrument_error::cntrlr_general_instance::none, 
							instrument_error::severity_level::error));
						// If firmware download failed, consider it as "Error"
						setNextInitializationSequence (callback, InitializationSequence::Error);
					}
				}, binFileFullPath, hashFileFullPath);
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitializationProcess:: Firmware binaries not found, firmware not updated");

				if (needFirmwareUpdate_)
				{
					initializationState_ = InitializationState::eFirmwareUpdateFailed;
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::controller_general_firmwareupdate, 
						instrument_error::cntrlr_general_instance::none, 
						instrument_error::severity_level::error));
					setNextInitializationSequence (callback, InitializationSequence::Error);
				}
				else
				{
					setNextInitializationSequence (callback, InitializationSequence::Probe);
				}
			}
			break;
		}

		case InitializationSequence::Probe:
		{
			//	// Make sure the probe is up.
			getStageController()->ProbeUp ([this, callback](bool status)
			{
				if (!status)
				{
					Logger::L ().Log (MODULENAME, severity_level::error, "InitializationSequence::Probe: failed to move the probe up");
					setNextInitializationSequence (callback, InitializationSequence::Error);
				}

				setNextInitializationSequence (callback, InitializationSequence::Stage);
			});
			break;
		}

		case InitializationSequence::Stage:
		{
			t_pPTree cfgTree;
			pStageController_->Init ("", cfgTree, true, HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::MotorControl),
				[this, callback](bool status, bool tubeFoundDuringInit)
					{
						InitializationSequence nextSeq = InitializationSequence::Camera;
						int32_t angle = NoEjectAngle;
						bool holding = EJECT_HOLDING_CURRENT_OFF;
						if ( HawkeyeConfig::Instance().get().automationEnabled && !pStageController_->IsCarouselPresent() )
						{
							angle = DfltAutomationEjectAngle;
							holding = EJECT_HOLDING_CURRENT_ON;
						}

						if ( !status )
						{
							if ( tubeFoundDuringInit )
							{
								Logger::L().Log( MODULENAME, severity_level::error, "InitializationSequence::Stage: failed to initialize StageController, carousel tube is found when initializing" );
								ReportSystemError::Instance().ReportError( BuildErrorInstance(
									instrument_error::motion_sampledeck_tubedetected,
									instrument_error::motion_sample_deck_instances::carousel,
									instrument_error::severity_level::warning ) );
								initializationState_ = InitializationState::eInitializationStopped_CarosuelTubeDetected;
							}
							else
							{
								Logger::L().Log( MODULENAME, severity_level::error, "InitializationSequence::Stage: failed to initialize StageController" );
								ReportSystemError::Instance().ReportError( BuildErrorInstance(
									instrument_error::motion_sampledeck_initerror,
									instrument_error::motion_sample_deck_instances::general,
									instrument_error::severity_level::error ) );
							}
							nextSeq = InitializationSequence::Error;
							holding = EJECT_HOLDING_CURRENT_OFF;
						}

						pStageController_->EjectStage( [ this, callback, status, nextSeq, holding ]( bool ejectStatus )
							{
								if ( !ejectStatus )
								{
									Logger::L().Log( MODULENAME, severity_level::error, "internalInitializationProcess::Stage: Failed to eject the stage" );
									if ( status )		// previous status was OK; add system error for this... conversely, don't add another error for this if a previous fail...
									{
										ReportSystemError::Instance().ReportError( BuildErrorInstance(
											instrument_error::motion_sampledeck_ejectfail,
											instrument_error::motion_sample_deck_instances::general,
											instrument_error::severity_level::error ) );
									}
									setNextInitializationSequence( callback, InitializationSequence::Error );
									return;
								}

								pStageController_->SetStageProfile( [ this, status, callback, nextSeq ]( bool disableOk ) -> void
								{
									if ( !disableOk )
									{
										Logger::L().Log( MODULENAME, severity_level::error, "internalInitializationProcess::Stage: Failed to disable holding current" );
										setNextInitializationSequence( callback, InitializationSequence::Error );
										return;
									}
									setNextInitializationSequence( callback, nextSeq );		// error sequence may have been passed in... otherwise go to 'Camera'
								}, holding );
								return;
							}, angle );
						return;
					}
			);
			break;
		}

		case InitializationSequence::Camera:
		{
			HAWKEYE_ASSERT (MODULENAME, pCamera_);

			pCamera_->initialize([this, callback](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess Camera: failed to initialize camera");
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_camera_initerror, 
						instrument_error::severity_level::error));
					setNextInitializationSequence (callback, InitializationSequence::Error);
					return;
				}
				setNextInitializationSequence (callback, InitializationSequence::EEPROM);
			});
			break;
		}

		case InitializationSequence::EEPROM:
		{
			HAWKEYE_ASSERT (MODULENAME, pEEPROMController_);

			if (!HawkeyeConfig::Instance().get().hardwareConfig.eePromController)
			{
				setNextInitializationSequence (callback, InitializationSequence::FocusController);
				break;
			}

			pEEPROMController_->ReadDataToCache([this, callback](bool status) {
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess EEPROMData: failed to read EEPROM data from controller board");
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::controller_general_fw_operation,
						instrument_error::cntrlr_general_instance::none, 
						instrument_error::severity_level::error));
					/* initializationState = eeprom_failed */;
					setNextInitializationSequence (callback, InitializationSequence::Error);
					return;
				}

				setNextInitializationSequence (callback, InitializationSequence::FocusController);
			});

			break;
		}

		case InitializationSequence::FocusController:
		{
			HAWKEYE_ASSERT (MODULENAME, pFocusController_);

			setNextInitializationSequence (callback, InitializationSequence::BrightfieldLed);
			break;
		}

		case InitializationSequence::BrightfieldLed:
		{
			const auto pLed = getLedSharedPtr(HawkeyeConfig::LedType::LED_BrightField);

			HAWKEYE_ASSERT (MODULENAME, pLed);

			pLed->initialize(HawkeyeConfig::LedType::LED_BrightField, [this, callback](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess Led: Failed to initialize LED!");
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_initerror, 
						instrument_error::imaging_led_instances::brightfield, 
						instrument_error::severity_level::error));
					setNextInitializationSequence (callback, InitializationSequence::Error);
					return;
				}
				setNextInitializationSequence (callback, InitializationSequence::Syringe);
			});
			return;
		}

		case InitializationSequence::Syringe:
		{
			HAWKEYE_ASSERT (MODULENAME, pSyringePump_);

			pSyringePump_->initialize ([this, callback](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess SyringePump: Failed to configure and initialize syringe pump!");
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::fluidics_syringepump_initerror, 
						instrument_error::fluidics_syringepump_instances::pump_control, 
						instrument_error::severity_level::error));
					setNextInitializationSequence (callback, InitializationSequence::Error);
					return;
				}
				setNextInitializationSequence (callback, InitializationSequence::ReagentController);
			});

			break;
		}

		case InitializationSequence::ReagentController:
		{	
			HAWKEYE_ASSERT (MODULENAME, pReagentController_);

			pReagentController_->Initialize ([this, callback](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess ReagentController: failed to initialize ReagentController");
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::motion_motor_initerror,
						instrument_error::motion_motor_instances::reagent_probe, 
						instrument_error::severity_level::error));
					setNextInitializationSequence (callback, InitializationSequence::Error);
					return;
				}

				setNextInitializationSequence (callback, InitializationSequence::VoltageMeasurement);
			}, HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::MotorControl), true);
			break;
		}

		case InitializationSequence::VoltageMeasurement:
		{
			HAWKEYE_ASSERT (MODULENAME, pVoltageMeasurement_);

			if (!pVoltageMeasurement_->Initialize())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess VoltageMeasurement: Failed to initialize voltage measurement!");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::controller_general_hardware_health, 
					instrument_error::cntrlr_general_instance::none, 
					instrument_error::severity_level::error));
				setNextInitializationSequence (callback, InitializationSequence::Error);
				return;
			}
			setNextInitializationSequence (callback, InitializationSequence::Complete);
			break;
		}

		case InitializationSequence::Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitializationProcess: hw_init_complete");
			pCBOService_->enqueueExternal (callback, true);
			break;
		}

		case InitializationSequence::Error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess: hw_init_error");
			pCBOService_->enqueueExternal (callback, false);
			break;
		}

		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "internalInitializationProcess: Unknown/Default state");
			pCBOService_->enqueueExternal (callback, false);
			break;
		}

	} // End "switch (sequence)"

}

//*****************************************************************************
void Hardware::setNextInitializationSequence (std::function<void(bool)> callback, InitializationSequence nextSequence) {

	pCBOService_->enqueueInternal (std::bind (&Hardware::internalInitializationProcess, this, callback, nextSequence));
}

//*****************************************************************************
bool Hardware::getFirmware (std::string & binFileFullPath, std::string & hashFileFullPath, std::string & binFileName, std::string & hashFileName) const
{
	const std::string firmwareDir = HawkeyeDirectory::Instance().getFirmwareDir();
	const boost::filesystem::path pp(firmwareDir);
	boost::system::error_code ec;
	bool foundBin = false;
	bool foundHash = false;

	const bool exists = boost::filesystem::exists(pp, ec);
	if ((!exists) || (ec && ec != boost::system::errc::no_such_file_or_directory)) {
		return false;
	}

	const bool is_dir = boost::filesystem::is_directory(pp, ec);
	if ((ec) || (!is_dir)) {
		return false;
	}

	boost::filesystem::directory_iterator it(pp);
	boost::filesystem::directory_iterator endit;

	while ((it != endit) && (!(foundBin && foundHash)))
	{
		if (boost::filesystem::is_regular_file(*it) && it->path().extension() == ".bin")
		{
			binFileName = it->path().filename().string();
			binFileFullPath = boost::str(boost::format("%s\\%s") % firmwareDir % binFileName);
			foundBin = true;
		}

		if (boost::filesystem::is_regular_file(*it) && it->path().extension() == ".txt")
		{
			hashFileName = it->path().filename().string();
			hashFileFullPath = boost::str(boost::format("%s\\%s") % firmwareDir % hashFileName);
			foundHash = true;
		}
		++it;
	}

	if (!(foundBin && foundHash))
	{
		return false;
	}

	return true;
}

//*****************************************************************************
void Hardware::archiveFirmware(
	const std::string & binFileFullPath, const std::string& hashFileFullPath,
	const std::string& binFileName, const std::string& hashFileName) const
{
	const boost::filesystem::path fwArchivePath(HawkeyeDirectory::Instance().getFirmwareArchiveDir());

	try 
	{
		if (boost::filesystem::exists (fwArchivePath))
		{
			boost::filesystem::remove_all (fwArchivePath);
		}

		boost::filesystem::create_directory (fwArchivePath);
		
		boost::filesystem::path binSrcPath (binFileFullPath);
		boost::filesystem::path binDstPath (HawkeyeDirectory::Instance().getFirmwareArchiveDir() + "\\" + binFileName);
		Logger::L().Log (MODULENAME, severity_level::debug1, "archiveFirmware: bin destination file   : " + binDstPath.generic_string());
		const boost::filesystem::path hashSrcPath (hashFileFullPath);
		const boost::filesystem::path hashDstPath (HawkeyeDirectory::Instance().getFirmwareArchiveDir() + "\\" + hashFileName);
		Logger::L().Log (MODULENAME, severity_level::debug1, "archiveFirmware: hash destination file   : " + hashDstPath.generic_string());

		// Copy the files from "bin" directory to "bin\\archive" directory
		boost::filesystem::copy (binSrcPath, binDstPath);
		boost::filesystem::copy (hashSrcPath, hashDstPath);

		// Deletes the files from "bin" directory
		boost::filesystem::remove(binSrcPath);
		boost::filesystem::remove(hashSrcPath);
	}
	catch(boost::filesystem::filesystem_error const & ec)
	{
		const boost::system::error_code errorCode = ec.code();
		Logger::L().Log (MODULENAME, severity_level::error, "archiveFirmware: Failed to archive firmware binary. error = " + errorCode.message());
		
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::controller_general_firmwareupdate, 
			instrument_error::cntrlr_general_instance::none, 
			instrument_error::severity_level::notification));
	}
}

//*****************************************************************************
void Hardware::updateFirmware(std::function<void(bool)> fwUpdateComplteCallback, const std::string& binFileFullPath, const std::string& hashFileFullPath)
{
	Logger::L().Log (MODULENAME, severity_level::normal, "initialize:: updateFirmware:: eFirmwareUpdateInProgress");

	HAWKEYE_ASSERT (MODULENAME, fwUpdateComplteCallback);
	
	initializationState_ = InitializationState::eFirmwareUpdateInProgress;

	pFirmwareDownload_ = std::make_shared<FirmwareDownload>(pCBOService_);
	if (!pFirmwareDownload_)
	{
		pCBOService_->enqueueExternal (fwUpdateComplteCallback, false);
		return;
	}
	
	pFirmwareDownload_->SetFirmwareProgressCallback([this, fwUpdateComplteCallback](bool finished, int16_t total) -> void
	{
		static int16_t prevTotal = 0;

		if (total > prevTotal)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format("Firmware update : %d ") % total));
			prevTotal = total;
		}

		if (finished)
		{
			prevTotal = 0;

			if (total == FIRMWARE_TRANSFER_COMPLETE)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "Firmware update completed successfully. <setting initialize state to InProgress>");
				initializationState_ = InitializationState::eInitializationInProgress;
				pCBOService_->enqueueExternal (fwUpdateComplteCallback, true);
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "Firmware update failed.");
				pCBOService_->enqueueExternal (fwUpdateComplteCallback, false);
				return;
			}
		}

	});

	pCBOService_->enqueueInternal([this, binFileFullPath, hashFileFullPath, fwUpdateComplteCallback]() -> void
	{
		boost::system::error_code ec;
		const bool initOk = pFirmwareDownload_->InitiateFirmwareUpdate(binFileFullPath, hashFileFullPath, ec);
		if (!initOk || ec)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "updateFirmware : Failed to initiate firmware download.");
			pCBOService_->enqueueExternal (std::bind(fwUpdateComplteCallback, false));
		}
	});
}

//*****************************************************************************
SamplePositionDLL Hardware::getCarrierPosition() const
{
	auto tempPos = pStageController_->GetStagePosition();
	return tempPos;
}

/// <summary>
/// Gets the default carrier position asynchronously
/// For Plate - Default is "A-1"
/// For Carousel - Default is any location with tube present or Invalid position
/// </summary>
/// <remarks>
/// This method will in initialize the carrier present if not already initialized
/// </remarks>
/// <param name="positionCb">Callback to update caller with carrier position</param>
void Hardware::getDefaultCarrierPosition (std::function<void(SamplePositionDLL)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// Lambda to update host with carrier position
	auto updateHost = [this, callback](SamplePositionDLL pos) -> void
	{
		pCBOService_->enqueueExternal ([callback, pos]() {
			callback (pos);
		});
	};
	
	if (getStageController()->IsCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultCarrierPosition: Carousel is detected");

		pCBOService_->enqueueExternal ([this, updateHost]() -> void
		{
			// Select carousel if not already initialized
			this->getStageController()->SelectStage([=](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultCarrierPosition: Unable to select carousel controller");
					updateHost(SamplePositionDLL());
					return;
				}

				getStageController()->FindFirstTube([=](bool)
				{
					updateHost(getStageController()->GetStagePosition());
				});

			}, eCarrierType::eCarousel);
		});
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultCarrierPosition: Plate is detected");

		pCBOService_->enqueueExternal ([this, updateHost]() -> void
		{
			// Select carousel if not already initialized
			this->getStageController()->SelectStage([=](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "getDefaultCarrierPosition: Unable to select plate controller");
					updateHost(SamplePositionDLL());
					return;
				}

				// Set the default position of plate (i.e. A1)
				SamplePositionDLL position = SamplePositionDLL();
				position.setRowColumn(eStageRows::PlateRowA, 1);
				updateHost(position);

			}, eCarrierType::ePlate_96);

		});
	}
}

//*****************************************************************************
HawkeyeError Hardware::SaveFocusPositionToConfig(const int32_t& pos)
{
	HawkeyeConfig::Instance().get().previousFocusPosition = pos;
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
boost::optional<InitializationState> Hardware::getInitializationState()
{
	return initializationState_;
}

//*****************************************************************************
void Hardware::RestoreFocusPositionFromConfig(std::function<void(HawkeyeError, int32_t)> onComplete, bool moveMotor, sm_RestoreFocusFromConfig state)
{
	switch (state)
	{
		case rffc_Start:
		{
			
			if (!moveMotor)
			{
				pCBOService_->enqueueExternal ([this, onComplete]()-> void
				{
					auto pos = HawkeyeConfig::Instance().get().previousFocusPosition;
					onComplete(HawkeyeError::eSuccess, pos);
				});
				return;
			}

			// Caller must initialize the focus controller.
			if (getFocusController()->IsInitialized() == false)
			{
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::published_errors::motion_motor_initerror,
					instrument_error::motion_motor_instances::focus,
					instrument_error::severity_level::error));

				pCBOService_->enqueueExternal ([this, onComplete]()-> void
				{
					auto pos = HawkeyeConfig::Instance().get().previousFocusPosition;
					onComplete(HawkeyeError::eHardwareFault, pos);
				});
				return;
			}
			
			pCBOService_->enqueueInternal ([this, onComplete, moveMotor]()->void 
				{ RestoreFocusPositionFromConfig(onComplete, moveMotor, rffc_Home); });
			break;
		}
		case rffc_Home:
		{
			auto homeComplete = [this, onComplete, moveMotor](bool homeStatus) -> void
			{
				if (!homeStatus)
				{
					pCBOService_->enqueueExternal ([this, onComplete]()-> void
					{
						auto pos = HawkeyeConfig::Instance().get().previousFocusPosition;
						onComplete(HawkeyeError::eHardwareFault, pos);
					});
					return;
				}

				pCBOService_->enqueueInternal([this, onComplete, moveMotor]()->void
					{ RestoreFocusPositionFromConfig (onComplete, moveMotor, rffc_SetPosition); });
			};

			getFocusController()->FocusHome(homeComplete);
			break;
		}
		case rffc_SetPosition:
		{
			auto pos = HawkeyeConfig::Instance().get().previousFocusPosition;

			auto setComplete = [this, onComplete](bool setStatus, int32_t endPos)->void
			{
				HawkeyeError he = setStatus ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault;
				pCBOService_->enqueueExternal ([this, onComplete, he, endPos]()-> void
				{
					onComplete(he, endPos);
				});
				
			};

			// Home the focus motor and set focus motor position to actual "currentFocusPosition"
			getFocusController()->SetPosition(setComplete, pos);
			break;
		}
	}
}

//*****************************************************************************
eCarrierType Hardware::getCarrier()
{
	eCarrierType carrierType = eCarrierType::eUnknown;
	if (pStageController_->IsCarouselPresent())
	{
		carrierType = eCarrierType::eCarousel;
	}
	else
	{
		carrierType = eCarrierType::ePlate_96;
	}

	if (carrierType != carrierType_)
	{
		Logger::L().Log (MODULENAME, severity_level::notification, boost::str (boost::format("getCarrier: carrier Mismatch <stored:%s> <actual:%s>") 
			% getCarrierTypeAsStr(carrierType_) % getCarrierTypeAsStr(carrierType)));
	}

	carrierType_ = carrierType;
	
	return carrierType_;
}

//*****************************************************************************
std::shared_ptr<CameraBase>& Hardware::getCamera() {

	HAWKEYE_ASSERT (MODULENAME, pCamera_);
	return pCamera_;
}

//*****************************************************************************
std::shared_ptr<FocusControllerBase>& Hardware::getFocusController() {

	HAWKEYE_ASSERT (MODULENAME, pFocusController_);
	return pFocusController_;
}

//*****************************************************************************
std::shared_ptr<LedBase> Hardware::getLed (HawkeyeConfig::LedType ledType) {

	return getLedSharedPtr(ledType);
}

//*****************************************************************************
std::shared_ptr<EEPROMController>& Hardware::getEEPROMController()
{
	HAWKEYE_ASSERT (MODULENAME, pEEPROMController_);
	return pEEPROMController_;
}

std::shared_ptr<FirmwareDownload>& Hardware::gefirmwareDownloader()
{
	HAWKEYE_ASSERT (MODULENAME, pFirmwareDownload_);
	return pFirmwareDownload_;
}

//*****************************************************************************
std::shared_ptr<ReagentControllerBase>& Hardware::getReagentController() {

	HAWKEYE_ASSERT (MODULENAME, pReagentController_);
	return pReagentController_;
}

//*****************************************************************************
std::shared_ptr<SyringePumpBase>& Hardware::getSyringePump() {

	HAWKEYE_ASSERT (MODULENAME, pSyringePump_);
	return pSyringePump_;
}

std::shared_ptr<StageControllerBase>& Hardware::getStageController()
{
	HAWKEYE_ASSERT (MODULENAME, pStageController_);
	return pStageController_;
}

//*****************************************************************************
void Hardware::setInitialSystemStatus()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "setInitialSystemStatus: <enter>");

	SystemStatusData& systemStatusData = SystemStatus::Instance().getData();

	// Initiate the timer immediately (almost after 10 milliseconds)
	systemStatusUpdateTimer_->expires_from_now (boost::posix_time::milliseconds(10));
	systemStatusUpdateTimer_->async_wait(std::bind (&Hardware::UpdateSystemStatus, this, std::placeholders::_1));

	std::string cellcountingVersion = boost::str (boost::format ("%d.%d.%d")
								   % CellCounter::CellCounterFactory::GetDLLMajorVersion()
								   % CellCounter::CellCounterFactory::GetDLLMinorVersion()
								   % CellCounter::CellCounterFactory::GetDLLReleaseNumber());

	auto& systemVersion = SystemStatus::Instance().systemVersion;
	DataConversion::convertToCharArray(systemVersion.img_analysis_version, sizeof(systemVersion.img_analysis_version), cellcountingVersion);
	DataConversion::convertToCharArray(systemVersion.system_serial_number, sizeof(systemVersion.system_serial_number), HawkeyeConfig::Instance().get().instrumentSerialNumber);
	if (HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard)
	{
		DataConversion::convertToCharArray (systemVersion.controller_firmware_version, sizeof(systemVersion.controller_firmware_version), pCBOService_->CBO()->GetFirmwareVersion());

	}
	else
	{
		DataConversion::convertToCharArray(systemVersion.controller_firmware_version, sizeof(systemVersion.controller_firmware_version), "N/A");
	}
	
	if (HawkeyeConfig::Instance().get().hardwareConfig.camera)
	{
		DataConversion::convertToCharArray(systemVersion.camera_firmware_version, sizeof(systemVersion.camera_firmware_version), pCamera_->getVersion());
	}
	else
	{
		DataConversion::convertToCharArray(systemVersion.camera_firmware_version, sizeof(systemVersion.camera_firmware_version), "N/A");
	}
	
	if (HawkeyeConfig::Instance().get().hardwareConfig.syringePump)
	{
		DataConversion::convertToCharArray(systemVersion.syringepump_firmware_version, sizeof(systemVersion.syringepump_firmware_version), pSyringePump_->getVersion());
	}
	else
	{
		DataConversion::convertToCharArray(systemVersion.syringepump_firmware_version, sizeof(systemVersion.syringepump_firmware_version), "N/A");
	}


	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str (boost::format ("\nControllerBoard: %s\n         Camera: %s\n  ImageAnalysis: %s\n    SyringePump: %s\n   SerialNumber: %s")
			% std::string(systemVersion.controller_firmware_version)
			% std::string(systemVersion.camera_firmware_version)
			% std::string(systemVersion.img_analysis_version)
			% std::string(systemVersion.syringepump_firmware_version)
			% std::string(systemVersion.system_serial_number)
		));

	boost::system::error_code ec;
	UpdateSystemStatus(ec); // Lets get the first hand status of all the system parameters to report, this will also trigger periodic update timer.

	Logger::L().Log (MODULENAME, severity_level::debug2, "setInitialSystemStatus: <enter>");
}

//*****************************************************************************
static void cbVoltageRead (const VoltageMeasurement::VoltageMeasurements_t& voltages)
{
	SystemStatusData& systemStatusData = SystemStatus::Instance().getData ();

	systemStatusData.voltage_neg_3V = voltages.neg_v_3;
	systemStatusData.voltage_3_3V = voltages.v_33;
	systemStatusData.voltage_5V_Sensor = voltages.P5Vsen;
	systemStatusData.voltage_5V_Circuit = voltages.v_5;
	systemStatusData.voltage_12V = voltages.v_12;
	systemStatusData.voltage_24V = voltages.v_24;
	systemStatusData.temperature_ControllerBoard = voltages.boardTemp;
	systemStatusData.temperature_CPU = voltages.cpuTemp;
	systemStatusData.temperature_OpticalCase = voltages.extTemp;
}

//*****************************************************************************
void Hardware::UpdateSystemStatus (const boost::system::error_code& ec) {

	Logger::L().Log (MODULENAME, severity_level::debug2, "UpdateSystemStatus: <enter>");

	if (!stopSystemStatusUpdateTimer_) {
		if (ec == boost::asio::error::operation_aborted) {
			return;
		}

		if (!pCamera_->isBusy())
		{
			SignalStatus signalStatus = pCBOService_->CBO()->GetSignalStatus();

			SystemStatusData& systemStatusData = SystemStatus::Instance().getData();

			if (signalStatus.isSet(SignalStatus::CarouselPresent))
			{
				systemStatusData.sensor_carousel_detect = ssStateActive;
				systemStatusData.sensor_carousel_tube_detect = (signalStatus.isSet(SignalStatus::CarouselTube) ? ssStateActive : ssStateInactive);
			}
			else
			{
				systemStatusData.sensor_carousel_detect = ssStateInactive;
				systemStatusData.sensor_carousel_tube_detect = ssStateInactive;
			}
			systemStatusData.sensor_reagent_pack_door_closed = (signalStatus.isSet(SignalStatus::ReagentDoorClosed) ? ssStateActive : ssStateInactive);
			systemStatusData.sensor_reagent_pack_in_place = (signalStatus.isSet(SignalStatus::ReagentPackInstalled) ? ssStateActive : ssStateInactive);

			systemStatusData.sensor_radiusmotor_home = (signalStatus.isSet(SignalStatus::RadiusMotorHome) ? ssStateActive : ssStateInactive);
			systemStatusData.sensor_thetamotor_home = (signalStatus.isSet(SignalStatus::ThetaMotorHome) ? ssStateActive : ssStateInactive);
			systemStatusData.sensor_probemotor_home = (signalStatus.isSet(SignalStatus::ProbeMotorHome) ? ssStateActive : ssStateInactive);
			systemStatusData.sensor_focusmotor_home = (signalStatus.isSet(SignalStatus::FocusMotorHome) ? ssStateActive : ssStateInactive);
			if (pReagentController_) {
				systemStatusData.sensor_reagentmotor_upper = (pReagentController_->IsUp() ? ssStateActive : ssStateInactive);
				systemStatusData.sensor_reagentmotor_lower = (pReagentController_->IsDown() ? ssStateActive : ssStateInactive);
			}
			systemStatusData.sensor_flopticsmotor1_home = (signalStatus.isSet(SignalStatus::Rack1MotorHome) ? ssStateActive : ssStateInactive);
			systemStatusData.sensor_flopticsmotor2_home = (signalStatus.isSet(SignalStatus::Rack2MotorHome) ? ssStateActive : ssStateInactive);

			if (carrierType_ != eCarrierType::eUnknown)
			{
				if (pStageController_) {
					pStageController_->GetMotorStatus (systemStatusData.motor_Theta, systemStatusData.motor_Radius, systemStatusData.motor_Probe);
				}
				if (pReagentController_) {
					pReagentController_->GetMotorStatus (systemStatusData.motor_Reagent);
				}
				if (pFocusController_) {
					pFocusController_->GetMotorStatus (systemStatusData.motor_Focus);
				}
			}
			else
			{
				MotorStatusDLL temp = { }; // Initialize temporary motor status with default values			

				temp.ToCStyle (systemStatusData.motor_Radius);
				temp.ToCStyle (systemStatusData.motor_Theta);
				temp.ToCStyle (systemStatusData.motor_Probe);
				temp.ToCStyle (systemStatusData.motor_Reagent);
				temp.ToCStyle (systemStatusData.motor_Focus);
			}

			if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::InstrumentType::ViCELL_FL_Instrument) {

//TODO: incomplete...
//sysStat.motor_FLRack1 = rack1Motor->GetMotorStatus();
//sysStat.motor_FLRack2 = rack2Motor->GetMotorStatus();

			} else {
				MotorStatusDLL temp = { }; // Initialize temporary motor status with default values

				temp.ToCStyle (systemStatusData.motor_FLRack1);
				temp.ToCStyle (systemStatusData.motor_FLRack2);
			}

			// User may manually move the stage, so fetch the correct/latest stage position every time
			SamplePositionDLL currentPos = {};
			if (pStageController_)
			{
				currentPos = pStageController_->GetStagePosition();
				if (!currentPos.isValid() && pStageController_->IsCarouselPresent()) {
					currentPos.setRowColumn (eStageRows::CarouselRow, static_cast<uint8_t>(pStageController_->GetNearestTubePosition()));
				}
			}
			systemStatusData.sampleStageLocation = currentPos.getSamplePosition();

			if (pFocusController_) {
				systemStatusData.focus_DefinedFocusPosition = pFocusController_->Position();
			}

			const bool isFocusMotorHomed = (systemStatusData.motor_Focus.flags & eMotorFlags::mfHomed) != 0; // Check if motor has been homed or not.
			if (!isFocusMotorHomed)
			{
				// If focus motor is not homed yet that means the "focusController" has no idea about the focus motor
				// position. When asked the position initially (just after system restart it will return the the position
				// as "0" and on doing relative move it will return position as "moveStep" (number of steps focus motor was 
				// asked to move either up or down).

				// "HawkeyeConfig::Instance().get().previousFocusPosition" stores the previous known focus position.

				// Actual focus motor position when motor not homed = currentFocusPos + HawkeyeConfig::Instance().get().previousFocusPosition;

				// If focus motor is not homed then get the stored focus position and add it to current focus position
				systemStatusData.focus_DefinedFocusPosition += HawkeyeConfig::Instance().get().previousFocusPosition;
			}
			systemStatusData.focus_IsFocused = (systemStatusData.focus_DefinedFocusPosition != 0 ? true : false);

			// The valve position is from the DLL point of view, not from the syringe pump point of view.
			SyringePumpPort valvePosition;
			pSyringePump_->getValve (valvePosition);
			systemStatusData.syringeValvePosition = valvePosition.get();

			uint32_t position;
			pSyringePump_->getPosition (position);
			systemStatusData.syringePosition = position;

			systemStatusData.brightfieldLedPercentPower = getLed(HawkeyeConfig::LedType::LED_BrightField)->getConfig()->percentPower;

			// The voltages are read every six times through to limit the number of messages to the ControllerBoard.
			if (voltageUpdateIntervalCount == VoltageUpdateInterval)
			{
				voltageUpdateIntervalCount = 0;
				pVoltageMeasurement_->ReadVoltages (std::bind (&cbVoltageRead, std::placeholders::_1));
			}
			else
			{
				voltageUpdateIntervalCount++;
			}

			//NOTE: *last_calibrated_date_concentration* and *last_calibrated_date_size* are set in 
			// GetSystemStatus so that the Hardware object does not have to know about HawkeyeLogicImpl.

			systemStatusData.sample_tube_disposal_remaining_capacity = HawkeyeConfig::Instance().get().discardTrayCapacity;

			systemStatusData.system_total_sample_count = static_cast<uint32_t>(HawkeyeConfig::Instance().get().totalSamplesProcessed);


			//TODO: set these values as appropriate...
			systemStatusData.is_standalone_mode = false;

			//Logger::L ().Log (MODULENAME, severity_level::debug1,
			//	boost::str (boost::format ("Hardware::UpdateSystemStatus health: %d") % systemStatusData.health));

			// NOTE : active_error_count, active_error_codes and health are updated in GetSystemStatus() so that 
			// any errors occurred after the last status update are not missed out.

			systemStatusUpdateTimer_->expires_from_now (boost::posix_time::milliseconds (1000));
			systemStatusUpdateTimer_->async_wait (std::bind (&Hardware::UpdateSystemStatus, this, std::placeholders::_1));
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "UpdateSystemStatus: <exit>");
}

//*****************************************************************************
bool Hardware::isFirmwareUpdated () const
{	
	return isFirmwareUpdated_;
}

//*****************************************************************************
bool Hardware::isFirmwareUpdateFailed() const
{
	return isFirmwareUpdateFailed_;
}
//*****************************************************************************
std::map<HawkeyeConfig::LedType, std::shared_ptr<LedBase>>& Hardware::getLedObjectsMap()
{
	return ledObjects_;
}

//*****************************************************************************
std::shared_ptr<LedBase> Hardware::getLedSharedPtr (HawkeyeConfig::LedType ledType)
{
	const auto led = ledObjects_.find(ledType);

	HAWKEYE_ASSERT (MODULENAME, led != ledObjects_.end ());
	HAWKEYE_ASSERT (MODULENAME, led->second);

	return led->second;
}

bool  Hardware::IsControllerBoardFaulted()
{
	if (pCBOService_)
		return pCBOService_->CBO()->GetCommsFaulted();

	return false;
}
