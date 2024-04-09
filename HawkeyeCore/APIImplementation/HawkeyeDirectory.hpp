#pragma once

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <string>

static std::string configFileExtension = ".cfg";

static std::string adjustBackgroudIntensityImageDir = "ABI";
static std::string                autofocusImageDir = "AF";
static std::string                        backupDir = "Backup";
static std::string                        configDir = "Config";
static std::string            dustReferenceImageDir = "BDS";
static std::string                        exportDir = "Export";
static std::string                      firmwareDir = "Bin";
static std::string                        imagesDir = "Images";
static std::string                     installedDir = "Installed";
static std::string                     rawImagesDir = "RawImages";
static std::string                resultBinariesDir = "ResultBinaries";
static std::string               resultsDataBaseDir = "ResultsData";
static std::string                  logFilesBaseDir = "Logs";
static std::string                     ResourcesDir = "Resources";


static std::string           analysisInfoFilename = "Analysis.info";
static std::string         bioprocessInfoFilename = "BioProcess.info";
static std::string calibrationhistoryInfoFilename = "CalibrationHistory.info";
static std::string           celltypeInfoFilename = "CellType.info";
static std::string      ControllerCalInfoFilename = "ControllerCal.info";
static std::string     dustReferenceImageFilename = "BDSImage.png";
static std::string         bestFocusImageFilename = "BestFocusImage.png";
static std::string       dataImporterInfoFilename = "DataImporter.info";
//TODO: save for future use...
//static std::string enableBCIBrightfieldLEDFilename = "EnableBCIBrightfieldLED.info";
static std::string     hawkeyeDynamicInfoFilename = "HawkeyeDynamic.info";
static std::string      hawkeyeStaticInfoFilename = "HawkeyeStatic.info";
static std::string      hawkeyeConfigInfoFilename = "HawkeyeConfig.info";
static std::string               metaDataFileName = "HawkeyeMetadata.xml";
static std::string            syringePumpFilename = "SyringePump.cfg";
static std::string       MotorControlInfoFilename = "MotorControl.info";
static std::string     qualitycontrolInfoFilename = "QualityControl.info";
static std::string        reagentpackInfoFilename = "ReagentPack.info";
static std::string           reagentsInfoFilename = "Reagents.info";
static std::string         signaturesInfoFilename = "Signatures.info";
static std::string           userlistInfoFilename = "UserList.info";

// Workflow scripts
static std::string            AFAnalaysisWfFilename = "AutoFocus_Analysis_Workflow.txt";
static std::string           AFLoadSampleWfFilename = "AutoFocus_LoadSample_Workflow.txt";
static std::string                 AFMainWfFilename = "AutoFocus_Main_Workflow.txt";
static std::string    AFPostAnalysisCleanWfFilename = "AutoFocus_PostAnalysisClean_Workflow.txt";
static std::string        AFPostLoadCleanWfFilename = "AutoFocus_PostLoadClean_Workflow.txt";
static std::string         AFPreLoadCleanWfFilename = "AutoFocus_PreLoadClean_Workflow.txt";
static std::string  DeContaminateFlowCellWfFilename = "DecontaminateFlowCell_Workflow.txt";
static std::string               DoorLeftWfFilename = "DoorLeft.txt";
static std::string              DoorRightWfFilename = "DoorRight.txt";
static std::string          DrainCleaner1WfFilename = "Drain_Cleaner1.txt";
static std::string          DrainCleaner2WfFilename = "Drain_Cleaner2.txt";
static std::string          DrainCleaner3WfFilename = "Drain_Cleaner3.txt";
static std::string          DrainReagent1WfFilename = "Drain_Reagent1.txt";
static std::string          DrainReagent2WfFilename = "Drain_Reagent2.txt";
static std::string          DustSubstractWfFilename = "DustSubtract_Workflow.txt";
static std::string          FlushFlowCellWfFilename = "FlushFlowCell_Workflow.txt";
static std::string            ExpelSampleWfFilename = "LoadNudgeExpel_Workflow_ExpelSample.txt";
static std::string             LoadSampleWfFilename = "LoadNudgeExpel_Workflow_LoadSample.txt";
static std::string   StandardNighlytCleanWfFilename = "NightClean_Workflow.txt";
static std::string AutomationNightlyCleanWfFilename = "NightClean_Workflow-ACup.txt";
static std::string           PrimeReagentWfFilename = "PrimeReagent_Workflow.txt";
static std::string                  PurgeWfFilename = "Purge_Workflow.txt";
static std::string             SampleFastWfFilename = "Sample_Workflow-Fast.txt";
static std::string           SampleNormalWfFilename = "Sample_Workflow-Normal.txt";
static std::string AutomationSampleNormalWfFilename = "Sample_Workflow-Automation.txt";
static std::string AutomationSampleNoDilutionWfFilename = "Sample_Workflow-AutomationNoDilution.txt";
static std::string                 UnclogWfFilename = "Unclog_Workflow.txt";
static std::string                      zip_utility = "C:\\Program Files\\7-Zip\\7z.exe";

class HawkeyeDirectory
{
public:
	enum class FileType
	{
		Analysis,
		BioProcess,
		CalibrationHistory,
		CellType,
		ControllerCal,
		DataImporter,
		DustReference,
		//TODO: save for future use...
		//EnableBCIBrightfieldLED,
		HawkeyeDynamic,
		HawkeyeStatic,
		HawkeyeConfig,
		SyringePump,
		MotorControl,
		QualityControl,
		ReagentPack,
		Reagents,
		Signatures,
		UserList,
	};

	enum class WorkFlowScriptType
	{
		eAFAnalaysis = 0,
		eAFLoadSample,
		eAFMain,
		eAFPostAnalysisClean,
		eAFPostLoadClean,
		eAFPreLoadClean,
		eDecontaminateFlowCell,
		eDoorLeft,
		eDoorRight,
		eDrainCleaner1,
		eDrainCleaner2,
		eDrainCleaner3,
		eDrainReagent1,
		eDrainReagent2,
		eDustSubstract,
		eFlushFlowCell,
		eExpelSample,
		eLoadSample,
		eStandardNightlyClean,
		eACupNightlyClean,
		ePrimeReagent,
		ePurge,
		eSampleFast,
		eSampleNormal,
		eSampleACup,
		eUnclog,
		eSampleACupNoDilution,
	};

	static HawkeyeDirectory& Instance()
	{
		static HawkeyeDirectory instance;
		return instance;
	}

	// Returns the complete file path.
	std::string getFilePath(FileType infoFile)
	{
		return boost::str (boost::format ("%s\\%s") % getConfigDir() % getFilename(infoFile));
	}

	// Returns the complete file path.
	std::string getWorkFlowScriptFile(WorkFlowScriptType scriptType)
	{
		return boost::str(boost::format("%s\\%s\\%s") % boost::filesystem::current_path().string() % ResourcesDir %  getWorkFlowScriptFilename(scriptType));
	}

	std::string getBackupFile(FileType infoFile)
	{
		return boost::str (boost::format ("%s\\%s") % getBackupDir() % getFilename(infoFile));
	}

	std::string getInstalledFile(FileType infoFile)
	{
		return boost::str (boost::format ("%s\\%s") % getInstalledDir() % getFilename(infoFile));
	}

	//Returns just file name, not the complete file path
	static std::string getFilename (FileType fileType)
	{
		switch ( fileType )
		{
			case FileType::Analysis:
				return analysisInfoFilename;
			case FileType::BioProcess:
				return bioprocessInfoFilename;
			case FileType::CalibrationHistory:
				return calibrationhistoryInfoFilename;
			case FileType::CellType:
				return celltypeInfoFilename;
			case FileType::ControllerCal:
				return ControllerCalInfoFilename;
			case FileType::DustReference:
				return dustReferenceImageFilename;
			case FileType::DataImporter:
				return dataImporterInfoFilename;
		//TODO: save for future use...
			//case FileType::EnableBCIBrightfieldLED:
			//	return enableBCIBrightfieldLEDFilename;
			case FileType::HawkeyeDynamic:
				return hawkeyeDynamicInfoFilename;
			case FileType::HawkeyeStatic:
				return hawkeyeStaticInfoFilename;
			case FileType::HawkeyeConfig:
				return hawkeyeConfigInfoFilename;
			case FileType::SyringePump:
				return syringePumpFilename;
			case FileType::MotorControl:
				return MotorControlInfoFilename;
			case FileType::QualityControl:
				return qualitycontrolInfoFilename;
			case FileType::ReagentPack:
				return reagentpackInfoFilename;
			case FileType::Reagents:
				return reagentsInfoFilename;
			case FileType::Signatures:
				return signaturesInfoFilename;
			case FileType::UserList:
				return userlistInfoFilename;
			default:
				assert (false);
				return "";
		}
	}

	//Returns just file name, not the complete file path
	static std::string getWorkFlowScriptFilename(WorkFlowScriptType scriptType)
	{
		switch (scriptType)
		{
			case WorkFlowScriptType::eAFAnalaysis:
				return AFAnalaysisWfFilename;
			case WorkFlowScriptType::eAFLoadSample:
				return AFLoadSampleWfFilename;
			case WorkFlowScriptType::eAFMain:
				return AFMainWfFilename;
			case WorkFlowScriptType::eAFPostAnalysisClean:
				return AFPostAnalysisCleanWfFilename;
			case WorkFlowScriptType::eAFPostLoadClean:
				return AFPostLoadCleanWfFilename;
			case WorkFlowScriptType::eAFPreLoadClean:
				return AFPreLoadCleanWfFilename;
			case WorkFlowScriptType::eDecontaminateFlowCell:
				return DeContaminateFlowCellWfFilename;
			case WorkFlowScriptType::eDoorLeft:
				return DoorLeftWfFilename;
			case WorkFlowScriptType::eDoorRight:
				return DoorRightWfFilename;
			case WorkFlowScriptType::eDrainCleaner1:
				return DrainCleaner1WfFilename;
			case WorkFlowScriptType::eDrainCleaner2:
				return DrainCleaner2WfFilename;
			case WorkFlowScriptType::eDrainCleaner3:
				return DrainCleaner3WfFilename;
			case WorkFlowScriptType::eDrainReagent1:
				return DrainReagent1WfFilename;
			case WorkFlowScriptType::eDrainReagent2:
				return DrainReagent2WfFilename;
			case WorkFlowScriptType::eDustSubstract:
				return DustSubstractWfFilename;
			case WorkFlowScriptType::eFlushFlowCell:
				return FlushFlowCellWfFilename;
			case WorkFlowScriptType::eExpelSample:
				return ExpelSampleWfFilename;
			case WorkFlowScriptType::eLoadSample:
				return LoadSampleWfFilename;
			case WorkFlowScriptType::eStandardNightlyClean:
				return StandardNighlytCleanWfFilename;
			case WorkFlowScriptType::eACupNightlyClean:
				return AutomationNightlyCleanWfFilename;
			case WorkFlowScriptType::ePrimeReagent:
				return PrimeReagentWfFilename;
			case WorkFlowScriptType::ePurge:
				return PurgeWfFilename;
			case WorkFlowScriptType::eSampleFast:
				return SampleFastWfFilename;
			case WorkFlowScriptType::eSampleNormal:
				return SampleNormalWfFilename;
			case WorkFlowScriptType::eSampleACup:
				return AutomationSampleNormalWfFilename;
			case WorkFlowScriptType::eUnclog:
				return UnclogWfFilename;
			case WorkFlowScriptType::eSampleACupNoDilution:
				return AutomationSampleNoDilutionWfFilename;
			default:
				assert(false);
				return "";
		}
	}

	std::string getDriveId() 
	{
		return driveId;
	}

	void setDriveId (std::string tDriveId)
	{
		driveId = tDriveId + ":";
	}

	void setInstrumentDir (std::string dir)
	{
		instrumentDir = dir;
	}

	void setImportInstrumentDir (std::string dir)
	{
		importInstrumentDir = dir;
	}

	void useInstrumentDir()
	{
		curInstrumentDir = instrumentDir;
	}

	void useImportInstrumentDir()
	{
		curInstrumentDir = importInstrumentDir;
	}

	std::string getInstrumentDir (bool useDriveLetter = true)
	{
		if (useDriveLetter)
		{
			return boost::str (boost::format ("%s\\%s") % driveId % curInstrumentDir);
		}

		return boost::str (boost::format ("%s") % curInstrumentDir);
	}

	std::string getConfigDir()
	{
		return boost::str (boost::format ("%s\\%s\\%s") % driveId % curInstrumentDir % configDir);
	}

	std::string getBackupDir() 
	{
		return boost::str (boost::format ("%s\\%s\\%s\\%s") % driveId % curInstrumentDir % configDir % backupDir);
	}

	std::string getInstalledDir() 
	{
		return boost::str (boost::format ("%s\\%s\\%s\\%s") % driveId % curInstrumentDir % configDir % installedDir);
	}

	std::string getConfigFileExtension() 
	{
		return  configFileExtension;
	}

	std::string getExportDir (bool useDriveLetter = false)
	{
		if (useDriveLetter)
		{
			return boost::str (boost::format ("%s\\%s\\%s") % driveId % curInstrumentDir % exportDir);
		}

		return boost::str (boost::format ("\\%s\\%s") % curInstrumentDir % exportDir);
	}

	std::string getFirmwareDir()
	{
		return boost::str (boost::format ("%s\\%s\\%s") % driveId % curInstrumentDir % firmwareDir);
	}

	std::string getFirmwareArchiveDir()
	{
		return boost::str(boost::format("%s\\%s\\%s\\archive") % driveId % curInstrumentDir % firmwareDir);
	}

	std::string getDustReferenceImagePath()
	{
		return boost::str(boost::format("%s\\%s") % getDustReferenceImageDir() % dustReferenceImageFilename);
	}

	static std::string getABIImageDir()
	{
		return boost::str(boost::format("%s\\%s") % boost::filesystem::current_path().string() % adjustBackgroudIntensityImageDir);
	}

	static std::string getAutoFocusImageDir()
	{
		return boost::str(boost::format("%s\\%s") % boost::filesystem::current_path().string() % autofocusImageDir);
	}

	static std::string getBestFocusImagePath(int32_t location)
	{
		return boost::str(boost::format("%s\\%d_%s") % getAutoFocusImageDir() %location % bestFocusImageFilename);
	}

	std::string getMetaDataFile()
	{
		return boost::str(boost::format("%s\\%s\\%s\\%s") % driveId % curInstrumentDir % resultsDataBaseDir % metaDataFileName);
	}

	std::string getLegacyCellCountingDataDir()
	{
		return boost::str(boost::format("%s\\%s\\%s\\%s") % driveId % curInstrumentDir % resultsDataBaseDir % "LegacyCellCountingData");
	}

	static std::string getDustReferenceImageDir()
	{
		return boost::str(boost::format("%s\\%s") % boost::filesystem::current_path().string() % dustReferenceImageDir);
	}

	std::string getResultsDataBaseDir()
	{
		return boost::str(boost::format("%s\\%s\\%s") % driveId % curInstrumentDir % resultsDataBaseDir);
	}

	// Build the appropriate path with or without the device (drive) identifier (letter).
	std::string getResultBinariesDir (bool useDriveLetter=true)
	{
		if ( useDriveLetter )
		{
			return boost::str(boost::format("%s\\%s\\%s\\%s") % driveId % curInstrumentDir % resultsDataBaseDir % resultBinariesDir);
		}
		
		return boost::str(boost::format("\\%s\\%s\\%s") % curInstrumentDir % resultsDataBaseDir % resultBinariesDir);
	}

	// Build the appropriate path with or without the device (drive) identifier (letter).
	std::string getImagesBaseDir (bool useDriveLetter = true)
	{
		if ( useDriveLetter )
		{
			return boost::str(boost::format("%s\\%s\\%s\\%s") % driveId % curInstrumentDir % resultsDataBaseDir % imagesDir);
		}

		return boost::str(boost::format("\\%s\\%s\\%s") % curInstrumentDir % resultsDataBaseDir % imagesDir);
	}

	std::string getLogsDir()
	{
		return boost::str(boost::format("%s\\%s\\%s") % driveId % curInstrumentDir % "Logs");
	}

	static std::string getZipUtility()
	{
		return zip_utility;
	}

	private:
		HawkeyeDirectory();
		std::string driveId;
		std::string instrumentDir;
		std::string curInstrumentDir;
		std::string importInstrumentDir;
};
