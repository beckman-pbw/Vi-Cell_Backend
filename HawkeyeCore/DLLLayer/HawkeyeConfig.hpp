#pragma once

#include <stdint.h>
#include <map>
#include <sstream>
#include <vector>

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include "ChronoUtilities.hpp"
#include "DBif_Api.h"
#include "ImageAnalysisParameters.hpp"
#include "PropertyField.hpp"
#include "SecurityHelpers.hpp"

enum class OpticalHardwareConfig
{
	UNKNOWN = 0,
	OMICRON_BASLER = 1,
	BECKMAN_BASLER = 2,
	BECKMAN_ALLIED = 3
};

class HawkeyeConfig
{
public:
	static HawkeyeConfig& Instance()
	{
		static HawkeyeConfig instance;
		return instance;
	}

	HawkeyeConfig()
		: isLoaded_(false)
	{
	}

	typedef enum : int16_t
	{
		UnknownInstrument = 0,
		ViCELL_BLU_Instrument,
		ViCELL_FL_Instrument,
		CellHealth_ScienceModule,
	} InstrumentType;

	typedef enum : int16_t
	{
		UnknownCamera = 0,
		Basler = 1,
		Allied = 2
	} CameraType;

	typedef enum {
		UnknownLed = 0,
		Omicron = 1,
		BCI = 2,
	} BrightfieldLedType;

	struct OpticalHardwareTuple
	{
		CameraType camera;
		BrightfieldLedType brightfieldLed;
	};
	struct OpticalHardwareTypePairing
	{
		OpticalHardwareConfig type;
		OpticalHardwareTuple hardware;
	};

	static constexpr OpticalHardwareTypePairing OpticalHardwarePairings[] =
	{
		{OpticalHardwareConfig::OMICRON_BASLER, {Basler,Omicron}},
		{OpticalHardwareConfig::BECKMAN_BASLER, {Basler, BCI}},
		{OpticalHardwareConfig::BECKMAN_ALLIED, {Allied, BCI}}
	};

	//Old Basler cameras' exposure time was set by trigger width, yet the exposure isn't
	//totally the width, it's the width + some offset. That offset is here and comes
	//from the tables at https://docs.baslerweb.com/exposure-time#table-1 and
	//https://docs.baslerweb.com/exposure-mode. Other cameras should do a timed
	//exposure based on the given exposure time + this offset to match the old
	//Baslers.
	static constexpr uint32_t ExposureOffset = 24;

	// The LEDs are split between two racks with two LEDs in each rack.
	//NOTE: LedType here should really be LedPosition, refactor this in the near future...
	enum class LedType : uint8_t {
		LED_Unknown = 0,
		LED_BrightField,
		LED_TOP1,     // Rack 1, Top
		LED_BOTTOM1,  // Rack 1, Bottom
		LED_TOP2,     // Rack 2, Top
		LED_BOTTOM2,  // Rack 2, Bottom
	};

	typedef struct
	{
		std::string ipAddr;
		std::string ipPort;
		std::string dbName;
	} DatabaseConfig_t;

	// This structure is only used for loading the configuration.
	// When an LED object is created, the appropriate LedConfig from here
	// is copied to the newly created LED object.
	typedef struct LedConfig_t
	{
		std::string ToStr()
		{
			return boost::str (boost::format (
				"\n\tname: %s\n"
				"\tilluminator_wavelength: %d\n"
				"\temission_wavelength: %d\n"
				"\tpercent_power: %5.1f%%\n"
				"\tsimmer_current_voltage: %d\n"
				"\tltcd: %d\n"
				"\tctld: %d\n"
				"\tfeedbackPhotodiode: %d\n")
				% name
				% illuminatorWavelength
				% emissionWavelength
				% percentPower
				% simmerCurrentVoltage
				% ltcd
				% ctld
				% feedbackPhotodiode);
		}

		void FromDbStyle (const DBApi::DB_IlluminatorRecord& dbIlluminatorRecord)
		{
			name = dbIlluminatorRecord.IlluminatorNameStr;
			position = static_cast<LedType>(dbIlluminatorRecord.PositionNum);
			illuminatorWavelength = dbIlluminatorRecord.IlluminatorWavelength;
			emissionWavelength = dbIlluminatorRecord.EmissionWavelength;
			percentPower = dbIlluminatorRecord.PercentPower;
			simmerCurrentVoltage = dbIlluminatorRecord.SimmerVoltage;
			ltcd = dbIlluminatorRecord.Ltcd;
			ctld = dbIlluminatorRecord.Ctld;
			feedbackPhotodiode = dbIlluminatorRecord.FeedbackDiode;
		}

		std::string    name;
		LedType        position;
		uint16_t	   illuminatorWavelength;  // nm
		uint16_t	   emissionWavelength;     // nm
		float          percentPower;           //
		uint32_t	   simmerCurrentVoltage;   // uVolt
		uint32_t	   ltcd;                   // uSec
		uint32_t	   ctld;                   // uSec
		uint32_t	   feedbackPhotodiode;     // 1 or 2
	} LedConfig_t;

	typedef std::map<LedType, LedConfig_t> LedMap_t;

	typedef struct Leds {
		std::string ToStr()
		{
			std::stringstream ss;

			ss << boost::str (boost::format(
				"*** LED Configuration ***"
				"\n\tomicronTolerance: %f\n"
				"\tbciTolerance: %f\n"
				"\tBCI uV: %f\n"
				"\tBCI Amps: %f\n")
				% omicronTolerance
				% bciTolerance
				% maxBCILedVoltage_uv
				% (maxBCILedVoltage_uv / 500000.0));

			// Currently only supporting the BrightField LED.
			ss << ledmap[LedType::LED_BrightField].ToStr();

			return ss.str();
		}

		PropertyFieldReadOnly<float>    omicronTolerance;
		PropertyFieldReadOnly<float>    bciTolerance;
		PropertyFieldReadOnly<uint32_t> maxBCILedVoltage_uv;
		LedMap_t ledmap;
	} Leds_t;

	typedef struct DeclusterSetting {
		std::string ToStr (int idx)
		{
			return boost::str (boost::format(
				"\n\t\t[%d]: accumulatorThreshold: %d\n"
				"\t\t[%d]: minimumDistanceThreshold: %d")
				% idx
				% accumulatorThreshold
				% idx
				% minimumDistanceThreshold);
		}

		PropertyFieldReadOnly<int32_t> accumulatorThreshold;
		PropertyFieldReadOnly<int32_t> minimumDistanceThreshold;
	} DeclusterSetting_t;

//TODO: is this needed???
	typedef struct Loggers
	{

	} Loggers_t;

	typedef struct RunOptions
	{
		PropertyField<std::string> sampleSetName;
		PropertyField<std::string> sampleName;
		PropertyField<int16_t>     saveImageCount;
		PropertyField<int16_t>     saveNthImage;
		PropertyField<bool>        resultsExport;
		PropertyField<std::string> resultsExportFolder;
		PropertyField<bool>        appendResultsExport;
		PropertyField<std::string> appendResultsExportFolder;
		PropertyField<std::string> resultFilename;
		PropertyField<std::string> resultsFolder;
		PropertyField<bool>        autoExportPdf;
		PropertyField<std::string> csvFolder;
		PropertyField<int16_t>     washType;
		PropertyField<int16_t>     dilution;
		PropertyField<uint32_t>    bpqcCellTypeIndex;
//TODO: future, analysisIndex...
	} RunOptions_t;

	typedef struct MotorConfig
	{
		PropertyField<float> carouselThetaHomeOffset;
		PropertyField<float> carouselRadiusOffset;
		PropertyField<float> plateThetaHomePosOffset;
		PropertyField<float> plateThetaCalPos;
		PropertyField<float> plateRadiusCenterPos;
	} MotorConfig_t;

	typedef struct CellCounting
	{
		std::string ToStr()
		{
			std::stringstream ss;
			ss << boost::str (boost::format(
				"*** CellCounting Configuration ***"
				"\n\talgorithmMode: %s\n"
				"\tbackgroundIntensityTolerance: %f\n"
				"\tbubbleMinimumSpotAreaPercentage: %f\n"
				"\tbubbleMinimumSpotAverageBrightness: %f\n"
				"\tbubbleMode: %d\n"
				"\tbubbleRejectionImgAreaPercentage: %f\n"
				"\tcellSpotBrightnessExclusionThreshold: %f\n"
				"\tcenterSpotMinimumIntensityLimit: %f\n"
				"\tconcentrationImageControlCount: %d\n"
				"\tfovDepthMM: %f\n"
				"\thotPixelEliminationMode: %f\n"
				"\tnominalBackgroundLevel: %f\n"
				"\tpeakIntensitySelectionAreaLimit: %f\n"
				"\tpixelFovMM: %f\n"
				"\troiXCoordinate: %d\n"
				"\troiYCoordinate: %d\n"
				"\tsmallParticleSizingCorrection: %f\n"
				"\tsubpeakAnalysisMode: %d\n")
				% (algorithmMode ? "true" : "false")
				% backgroundIntensityTolerance
				% bubbleMinimumSpotAreaPercentage
				% bubbleMinimumSpotAverageBrightness
				% (bubbleMode ? "true" : "false")
				% bubbleRejectionImgAreaPercentage
				% cellSpotBrightnessExclusionThreshold
				% centerSpotMinimumIntensityLimit
				% concentrationImageControlCount
				% fovDepthMM
				% hotPixelEliminationMode
				% nominalBackgroundLevel
				% peakIntensitySelectionAreaLimit
				% pixelFovMM
				% roiXCoordinate
				% roiYCoordinate
				% smallParticleSizingCorrection
				% (subpeakAnalysisMode ? "true" : "false"));

			ss << "\tDeclusterSettings";

			for (int idx=1; idx < declusterSettings.size(); idx++)
			{
				ss << declusterSettings[idx].ToStr (idx);
			}

			ss << std::endl;

			return ss.str();
		}

		PropertyFieldReadOnly<int32_t>  algorithmMode;
		PropertyFieldReadOnly<double> backgroundIntensityTolerance;
		PropertyFieldReadOnly<float>  bubbleMinimumSpotAreaPercentage;
		PropertyFieldReadOnly<float>  bubbleMinimumSpotAverageBrightness;
		PropertyFieldReadOnly<bool>   bubbleMode;
		PropertyFieldReadOnly<float>  bubbleRejectionImgAreaPercentage;
		PropertyFieldReadOnly<double> cellSpotBrightnessExclusionThreshold;
		PropertyFieldReadOnly<double> centerSpotMinimumIntensityLimit;
		PropertyFieldReadOnly<int32_t> concentrationImageControlCount;
		std::vector<DeclusterSetting_t> declusterSettings;
		PropertyFieldReadOnly<double> fovDepthMM;
		PropertyFieldReadOnly<double> hotPixelEliminationMode;
		PropertyFieldReadOnly<double> nominalBackgroundLevel;
		PropertyFieldReadOnly<double> peakIntensitySelectionAreaLimit;
		PropertyFieldReadOnly<double> pixelFovMM;
		PropertyFieldReadOnly<int32_t> roiXCoordinate;
		PropertyFieldReadOnly<int32_t> roiYCoordinate;
		PropertyFieldReadOnly<double> smallParticleSizingCorrection;
		PropertyFieldReadOnly<bool>   subpeakAnalysisMode;
	} CellCounting_t;

	typedef struct Database
	{
		std::string ToStr()
		{
			std::stringstream ss;
			ss << boost::str (boost::format(
				"*** Database Configuration ***"
				"\n\tIP Address: %s\n"
				"\tIP Port: %s\n"
				"\tDatabase Name: %s\n"
			)
				% ipAddr.get()
				% ipPort.get()
				% dbName.get()
			);

			return ss.str();
		}

		PropertyField<std::string> ipAddr;
		PropertyField<std::string> ipPort;
		PropertyField<std::string> dbName;
	} Database_t;

	typedef struct ActiveDirectory
	{
		std::string ToStr()
		{
			std::stringstream ss;
			ss << boost::str (boost::format(
				"*** ActiveDirectory Configuration ***"
				"\n\tServer: %s\n"
				"\tIP Port: %d\n"
				"\tDomain: %s\n"
				"\tEnable: %s\n"
			)
				% server.get()
				% ipPort.get()
				% domain.get()
				% (enabled.get() ? "true" : "false")
			);

			return ss.str();
		}

		PropertyField<std::string> server;
		PropertyField<int32_t>     ipPort;
		PropertyField<std::string> domain;
//TODO: "enabled" field is not currently used by the Backend code.
		PropertyField<bool>        enabled;
	} ActiveDirectory_t;

	typedef struct Smtp
	{
		std::string ToStr()
		{
			std::stringstream ss;

			ss << boost::str (boost::format(
				"*** SMTP Configuration ***"
				"\n\tServer: %s\n"
				"\tIP Port: %d\n"
#ifdef _DEBUG
				"\tUsername: %s\n"
				"\tPassword: %s\n"
#else
				"\tUsername: not displayed\n"
				"\tPassword: not displayed\n"
#endif
				"\tEnable: %s\n"
			)
				% server.get()
				% ipPort
#ifdef _DEBUG
				% username.get()
				% password.get()
#endif
				% (authEnabled ? "true" : "false")
			);

			return ss.str();
		}

		PropertyField<std::string> server;
		PropertyField<int32_t>     ipPort;
		PropertyField<std::string> username;
		PropertyField<std::string> password;
		PropertyField<bool>        authEnabled;
	} Smtp_t;

	typedef struct MotorCalibration {
		std::string ToStr()
		{
			std::stringstream ss;

			ss << boost::str (boost::format(
				"*** Motor Calibration ***"
				"\n\tcarouselThetaHomeOffset: %d\n"
				"\t     carouselRadiusOffset: %d\n"
				"\t     plateThetaHomeOffset: %d\n"
				"\t         plateThetaCalPos: %d\n"
				"\t     plateRadiusCenterPos: %d\n")
				% carouselThetaHomeOffset
				% carouselRadiusOffset
				% plateThetaHomeOffset
				% plateThetaCalPos
				% plateRadiusCenterPos);

			return ss.str();
		}

		PropertyField<int32_t> carouselThetaHomeOffset;
		PropertyField<int32_t> carouselRadiusOffset;
		PropertyField<int32_t> plateThetaHomeOffset;
		PropertyField<int32_t> plateThetaCalPos;
		PropertyField<int32_t> plateRadiusCenterPos;
	} MotorCalibration_t;

	// a collection of flags that represent if each piece of hardware is present
	typedef struct HardwareConfig
	{
		bool camera;
		bool eePromController;
		bool focusController;
		bool led;
		bool syringePump;
		bool reagentController;
		bool stageController;
		bool voltageMeasurement;
		// used in places where previously the hardware flag was used as "is in simulation"
		// rather than used for specific hardware components
		bool controllerBoard;
		bool discardTray;
	} HardwareConfig_t;
	
//*************************************************************************************
//NOTE: THE ORDER OF THIS CODE *MUST* MATCH THE ORDER IN THE *HawkeyeConfig.cpp* FILE.
//*************************************************************************************
	typedef struct HawkeyeConfigData
	{
		std::string ToStr()
		{
			std::stringstream ss;

			std::string instrumentTypeStr;
			switch (instrumentType)
			{
				case ViCELL_BLU_Instrument: instrumentTypeStr = "ViCELL BLU"; break;
				case ViCELL_FL_Instrument: instrumentTypeStr = "ViCELL FL"; break;
				case CellHealth_ScienceModule: instrumentTypeStr = "CellHealth Science Module"; break;
				default: instrumentTypeStr = "Unknown"; break;
			}
			
			ss << boost::str (boost::format (
				"\n*** General Dynamic Configuration ***"
				"\n\tinstrumentType: %s\n"
				"\tbrightfieldLedType: %d\n"
				"\tinstrumentSerialNumber: %s\n"
				"\tpreviousFocusPosition: %d\n"
				"\tflowCellDepth: %f\n"
				"\tsecurityType: %s\n"
				"\ttotalSamplesProcessed: %d\n"
				"\tdiscardTrayCapacity: %d\n"
				"\tpasswordExpiration: %d\n"
				"\tlastNightCleanTime: %s\n"
				"\tnormalShutdown: %d\n"
				"\tnextAnalysisDefIndex: %d\n"
				"\tnextBCICellTypeIndex: %d\n"
			)
				% instrumentTypeStr
				% (brightfieldLedType == BrightfieldLedType::Omicron ? "Omicron" : "BCI")
				% instrumentSerialNumber.get()
				% previousFocusPosition
				% flowCellDepth
				% (securityType == eSECURITYTYPE::eNoSecurity ? "None" : securityType == eSECURITYTYPE::eLocalSecurity ? "Local" : "ActiveDirectory")
				% totalSamplesProcessed
				% discardTrayCapacity
				% passwordExpiration
				% ChronoUtilities::ConvertToString (lastNightCleanTime, "%Y-%m-%d %H:%M:%S")
				% normalShutdown
				% nextAnalysisDefIndex
				% nextBCICellTypeIndex
			);

			ss << boost::str (boost::format (
				"*** General Static Configuration ***\n"
				"\tHardware\n"
				"\t\tCamera: %s\n"
				"\t\tEEPROM Controller: %s\n"
				"\t\tFocus Controller: %s\n"
				"\t\tLED: %s\n"
				"\t\tSyringe Pump: %s\n"
				"\t\tReagent Controller: %s\n"
				"\t\tStage Controller: %s\n"
				"\t\tVoltage Measurement: %s\n"
				"\t\tController Board: %s\n"
				"\t\tDiscard Tray: %s\n"
				"\tautoFocusCoarseStartingPosition: %d\n"
				"\tautoFocusCoarseEndingPosition: %d\n"
				"\tautoFocusCoarseStep: %d\n"
				"\tautoFocusFineRange: %d\n"
				"\tautoFocusFineStep: %d\n"
				"\tautoFocusSharpnessLowThreshold: %d\n"
				"\tabiMaxImageCount: %d\n"
				"\tmanualSampleNudgeVolume: %d\n"
				"\tmanualSampleNudgeSpeed: %d\n"
				"\tflowCellDepthExperimentalConstant: %d\n"
				"\trfidTagSimulation_SetValidTagData: %d\n"
				"\trfidTagSimulation_TotalTags: %d\n"
				"\trfidTagSimulationFile_MainBay: %d\n"
				"\trfidTagSimulationFile_RightDoor: %d\n"
				"\trfidTagSimulationFile_LeftDoor: %d\n"
				"\tgenerateLegacyCellCountingData: %d\n"
				"\tisSimulatorCarousel: %d\n"
				"\tnightlyCleanMinutesFromMidnight: %d\n"
				"\tcameraTriggerTimeout_ms: %d\n"
				"\tcameraImageCaptureTimeout_ms: %d\n"
				"\tcameraType: %d\n"
			)
				% (hardwareConfig.camera ? "true" : "false")
				% (hardwareConfig.eePromController ? "true" : "false")
				% (hardwareConfig.focusController ? "true" : "false")
				% (hardwareConfig.led ? "true" : "false")
				% (hardwareConfig.syringePump ? "true" : "false")
				% (hardwareConfig.reagentController ? "true" : "false")
				% (hardwareConfig.stageController ? "true" : "false")
				% (hardwareConfig.voltageMeasurement ? "true" : "false")
				% (hardwareConfig.controllerBoard ? "true" : "false")
				% (hardwareConfig.discardTray ? "true" : "false")
				% autoFocusCoarseStartingPosition.get()
				% autoFocusCoarseEndingPosition.get()
				% autoFocusCoarseStep.get()
				% autoFocusFineRange.get()
				% autoFocusFineStep.get()
				% autoFocusSharpnessLowThreshold.get()
				% abiMaxImageCount.get()
				% manualSampleNudgeVolume.get()
				% manualSampleNudgeSpeed.get()
				% flowCellDepthExperimentalConstant.get()
				% rfidTagSimulation_SetValidTagData.get()
				% rfidTagSimulation_TotalTags.get()
				% rfidTagSimulationFile_MainBay.get()
				% rfidTagSimulationFile_RightDoor.get()
				% rfidTagSimulationFile_LeftDoor.get()
				% generateLegacyCellCountingData.get()
				% isSimulatorCarousel.get()
				% nightlyCleanMinutesFromMidnight.get()
				% cameraTriggerTimeout_ms.get()
				% cameraImageCaptureTimeout_ms.get()
				% (cameraType.get() == 1 ? "Basler" : "Unknown")
			);

			ss << cellCounting.ToStr();

			ss << leds.ToStr();

			ss << database.ToStr();

			ss << activeDirectory.ToStr();

			ss << smtp.ToStr();

			return ss.str();
		}

		// Static configuration fields, read from HawkeyeStatic.info.
		bool                               withHardware;
		PropertyFieldReadOnly<int32_t>     autoFocusCoarseStartingPosition;
		PropertyFieldReadOnly<int32_t>     autoFocusCoarseEndingPosition;
		PropertyFieldReadOnly<int16_t>     autoFocusCoarseStep;
		PropertyFieldReadOnly<int32_t>     autoFocusFineRange;
		PropertyFieldReadOnly<int16_t>     autoFocusFineStep;
		PropertyFieldReadOnly<int32_t>     autoFocusSharpnessLowThreshold;
		PropertyFieldReadOnly<int16_t>     abiMaxImageCount;
		PropertyFieldReadOnly<int16_t>     manualSampleNudgeVolume;
		PropertyFieldReadOnly<int16_t>     manualSampleNudgeSpeed;
		PropertyFieldReadOnly<float>       flowCellDepthExperimentalConstant;
		PropertyFieldReadOnly<bool>        rfidTagSimulation_SetValidTagData; 
		PropertyFieldReadOnly<int16_t>     rfidTagSimulation_TotalTags;
		PropertyFieldReadOnly<std::string> rfidTagSimulationFile_MainBay;
		PropertyFieldReadOnly<std::string> rfidTagSimulationFile_RightDoor;
		PropertyFieldReadOnly<std::string> rfidTagSimulationFile_LeftDoor;
		PropertyFieldReadOnly<bool>        generateLegacyCellCountingData;
		PropertyFieldReadOnly<int16_t>     nightlyCleanMinutesFromMidnight;
		PropertyFieldReadOnly<uint32_t>    cameraTriggerTimeout_ms;
		PropertyFieldReadOnly<uint32_t>    cameraImageCaptureTimeout_ms;
		PropertyFieldReadOnly<int16_t>     cameraType;
		CellCounting_t                     cellCounting;
		Leds_t                             leds;

		// Dynamic configuration fields.
		PropertyField<int16_t>     instrumentType;
		PropertyField<int16_t>     brightfieldLedType;		//NOTE: not currently read from the DB.
		PropertyField<std::string> instrumentSerialNumber;
		PropertyField<int32_t>     previousFocusPosition;
		PropertyField<float>       flowCellDepth;
		PropertyField<int16_t>     securityType;
		PropertyField<int32_t>     totalSamplesProcessed;
		PropertyField<int16_t>     discardTrayCapacity;
		PropertyField<int16_t>     inactivityTimeout_mins;
		PropertyField<int16_t>     passwordExpiration;
		PropertyField<system_TP>   lastNightCleanTime;
		PropertyField<bool>        normalShutdown;
		PropertyField<int16_t>     nextAnalysisDefIndex;
		PropertyField<int32_t>     nextBCICellTypeIndex;
		PropertyField<bool>        automationInstalled;
		PropertyField<bool>        automationEnabled;
		PropertyField<bool>        acupEnabled;
		PropertyField<int32_t>     automationPort;
		PropertyField<bool>        isSimulatorCarousel;

		Database_t        database;
		ActiveDirectory_t activeDirectory;
		Smtp_t            smtp;
		MotorConfig_t     motorConfig;
		RunOptions_t      runOptions;
		MotorCalibration_t motorCal;
		HardwareConfig_t hardwareConfig;
	} HawkeyeConfig_t;

	bool Initialize();
	void SetDatabaseConfig (DatabaseConfig_t& dbConfig);
	HawkeyeConfig_t& get();
	void setHardwareForCHM();
	void setHardwareForViCell();
	void setHardwareForSimulation();
	static bool ConfigFileExists();
	bool isValidInstrumentType();
	bool isValidInstrumentType(const InstrumentType& type);
	LedType positionToLedType (std::string position);
	LedConfig_t& getLedConfigByPositionName (std::string position);

	static void ImportControllerCalInfoFile (boost::property_tree::ptree& ptConfig);
	static void ImportHawkeyeDynamicInfoFile (boost::property_tree::ptree& ptConfig, bool setSerNo);
	static void ImportHawkeyeStaticInfoFile (boost::property_tree::ptree& ptConfig);

protected:
	HawkeyeConfig_t  config_;

private:
	bool LoadDeveloperConfigFile (DatabaseConfig_t& dbConfig);

	bool isLoaded_;

	// These constants are not defined in the DB, so they're made static here.
	uint32_t cameraTriggerTimeout_ms_;
	uint32_t cameraImageCaptureTimeout_ms_;
	float omicronTolerance_;
	float bciTolerance_;
	uint32_t maxBCILedVoltage_uv_;
};
