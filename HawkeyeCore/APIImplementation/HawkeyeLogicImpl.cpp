#include "stdafx.h"

#include <regex>
#include <windows.h>

#include <boost/filesystem.hpp>

#include "opencv2/core/version.hpp"

#include <DBif_Api.h>

#include "AuditLog.hpp"
#include "CalibrationHistoryDLL.hpp"
#include "CellHealthReagents.hpp"
#include "DLLVersion.hpp"
#include "EnumConversion.hpp"
#include "GetAsStrFunctions.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"

#include "FileSystemUtilities.hpp"
#include "InstrumentConfig.hpp"

#include "../../target/properties/HawkeyeCore/dllversion.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl";
static InitializationState initializationState_ = eInitializationInProgress;
static std::map<uint32_t,string> cellHealthErrorCodes;

extern bool g_currentSystemAutomationLockstatus;

std::string ConvertSystemErrorCodeSeverity_to_Resource(const std::string& sev)
{
	std::regex r("\\s+");
	return "LID_API_SystemErrorCode_Severity_" + std::regex_replace(sev, r, "");
}

std::string  ConvertSystemErrorCodeSystem_to_Resource(const std::string& sys)
{
	std::regex r("\\s+");
	return "LID_API_SystemErrorCode_System_" + std::regex_replace(sys, r, "");
}

std::string ConvertSystemErrorCodeSubsystem_to_Resource(const std::string& subsys)
{
	std::regex r("\\s+");
	return "LID_API_SystemErrorCode_Subsystem_" + std::regex_replace(subsys, r, "");
}

std::string ConvertSystemErrorCodeInstance_to_Resource(const std::string& inst)
{
	std::regex r("\\s+");
	return "LID_API_SystemErrorCode_Instance_" + std::regex_replace(inst, r, "");
}

std::string ConvertSystemErrorCodeFailureMode_to_Resource(const std::string& fm)
{
	std::regex r("\\s+");
	return "LID_API_SystemErrorCode_Failure_" + std::regex_replace(fm, r, "");
}

class HawkeyeServicesImpl : public HawkeyeServices
{
public:
	HawkeyeServicesImpl (std::shared_ptr<boost::asio::io_context> pMainIos)
		: HawkeyeServices(pMainIos)
	{ }
};

//*****************************************************************************
// This code may be used to trap serious exceptions such as Access Violation.
// Currently this is deprecated since it needs more work.
// It is being kept in case its use is needed in the future.
//*****************************************************************************
// This URL was a reference for this code.
// https://docs.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model
// 
// All code that initializes or uses this construct must be compiled with 
// "Yes with SEH Exceptions (/EHa)" in C++ / Code Generation enabled.
// Documentation indicates that doing this will probably increase the code size.
//
// To use call: _set_se_translator (se_translator); during s/w initialization.
// Then use the normal try/catch block construct.
//
//		try {
//			// Divide by zero...
//		}
//		catch (std::exception e) {
//			Logger::L().Log (MODULENAME, severity_level::critical, boost::str (boost::format ("Critical SE error: %s") % e.what()));
//		}
//
//*****************************************************************************
//*****************************************************************************
//void se_translator (unsigned int u, EXCEPTION_POINTERS* pExp) {
//    
//    std::string error = "SE Exception: ";
//    
//    switch (u) {
//        case 0xC0000005:
//            error += "Access Violation";
//            break;
//        default:
//            char result[11];
//            sprintf_s(result, 11, "0x%08X", u);
//            error += result;
//    }
//
//    throw std::exception(error.c_str());
//}


const std::map<HawkeyeLogicImpl::InitializationSequence, std::string>
EnumConversion<HawkeyeLogicImpl::InitializationSequence>::enumStrings<HawkeyeLogicImpl::InitializationSequence>::data =
{
	{ HawkeyeLogicImpl::InitializationSequence::Hardware, std::string("Hardware") },
	{ HawkeyeLogicImpl::InitializationSequence::UserList, std::string("UserList") },
	{ HawkeyeLogicImpl::InitializationSequence::SerialNumber, std::string("SerialNumber") },
	{ HawkeyeLogicImpl::InitializationSequence::SystemStatus, std::string("SystemStatus") },
	{ HawkeyeLogicImpl::InitializationSequence::Analysis, std::string("Analysis") },
	{ HawkeyeLogicImpl::InitializationSequence::CalibrationHistory, std::string("CalibrationHistory") },
	{ HawkeyeLogicImpl::InitializationSequence::CellType, std::string("CellType") },
	{ HawkeyeLogicImpl::InitializationSequence::QualityControl, std::string("QualityControl") },
	{ HawkeyeLogicImpl::InitializationSequence::ReagentPack, std::string("ReagentPack") },
	{ HawkeyeLogicImpl::InitializationSequence::Results, std::string("Results") },
	{ HawkeyeLogicImpl::InitializationSequence::Signatures, std::string("Signatures") },
	{ HawkeyeLogicImpl::InitializationSequence::SystemStatus, std::string("SystemStatus") },
	{ HawkeyeLogicImpl::InitializationSequence::NightlyClean, std::string("NightlyClean") },
	{ HawkeyeLogicImpl::InitializationSequence::Validation, std::string("Validation") },
	{ HawkeyeLogicImpl::InitializationSequence::Complete, std::string("Complete") },
	{ HawkeyeLogicImpl::InitializationSequence::Error, std::string("Error") },
};

//*****************************************************************************
HawkeyeLogicImpl::HawkeyeLogicImpl()
{
	/*
	* Class constructor should do very little.
	* Save the "real work" for the "Initialize()" call.
	*/

	initializationRun = false;
	loggersInitialized = false;
	asioInitialized = false;
	isShutdownComplete_ = false;

	serialNumber_Canonical.clear();
	serialNumber_ConfigCopy.clear();
	serialNumber_FirmwareCacheCopy.clear();
}

//*****************************************************************************
HawkeyeLogicImpl::~HawkeyeLogicImpl()
{

}

template<typename T>
static void FreeUntaggedPrimitiveArray(void* pointer)
{
	delete[] static_cast<T*>(pointer);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::FreeTaggedBuffer(NativeDataType type, void* pointer)
{
	if (!pointer) return HawkeyeError::eSuccess;

	switch (type)
	{
	case NativeDataType::eChar:
		FreeUntaggedPrimitiveArray<char>(pointer);
		break;
	case NativeDataType::eUint16:
		FreeUntaggedPrimitiveArray<uint16_t>(pointer);
		break;
	case NativeDataType::eInt16:
		FreeUntaggedPrimitiveArray<int16_t>(pointer);
		break;
	case NativeDataType::eUint32:
		FreeUntaggedPrimitiveArray<uint32_t>(pointer);
		break;
	case NativeDataType::eInt32:
		FreeUntaggedPrimitiveArray<int32_t>(pointer);
		break;
	case NativeDataType::eUint64:
		FreeUntaggedPrimitiveArray<uint64_t>(pointer);
		break;
	case NativeDataType::eInt64:
		FreeUntaggedPrimitiveArray<int64_t>(pointer);
		break;
	default:
		return HawkeyeError::eInvalidArgs;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
template <typename T>
static void DeleteListOfBuffers(void* pointer, uint32_t num_items)
{
	auto arr = static_cast<T**>(pointer);
	for (uint32_t i = 0; i < num_items; i++)
	{
		FreeUntaggedPrimitiveArray<T>(arr[i]);
	}
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::FreeListOfTaggedBuffers(NativeDataType type, void* pointer, uint32_t num_items)
{
	auto status = HawkeyeError::eSuccess;

	if (!pointer && num_items == 0)
		return HawkeyeError::eSuccess;

	if (!pointer || num_items == 0)
		return HawkeyeError::eInvalidArgs;

	switch (type)
	{
		case NativeDataType::eChar:
			DeleteListOfBuffers<char>(pointer, num_items);
			FreeUntaggedPrimitiveArray<char*>(pointer);
			break;
		default:
			// Currently, the API never returns a pointer-to-array-of-pointers-to-integral-buffers.
			status = HawkeyeError::eInvalidArgs;
	}

	return status;
}

#if 0	// Code for testing QcStatus without having to run samples...
constexpr double TenToThe6th = 1000000;
static void Test_QC (double dataValue, std::string qcName)
{
	QualityControlDLL qc = {};
	getQualityControlByName (qcName, qc);

	std::string assayTypeStr;
	double limit = 0;
	double minValue = 0;
	double maxValue = 0;
	
	switch (qc.assay_type)
	{
		case ap_Concentration:
		{
			// "qc.assay_value" is 10 to the sixth.
			//float concentration_general;    // x10^6	  USE THIS ONE

			assayTypeStr = "Concentration";
			limit = qc.assay_value * TenToThe6th * (qc.plusminus_percentage / 100.0);
			minValue = qc.assay_value * TenToThe6th - limit;
			maxValue = qc.assay_value * TenToThe6th + limit;

			break;
		}
		case ap_PopulationPercentage: // (viability)
		{
			// "qc.assay_value" is a percentage here.
			//uint32_t percent_pop_ofinterest;	USE THIS ONE

			assayTypeStr = "Population Percentage";
			limit = qc.assay_value * (qc.plusminus_percentage / 100.0);
			minValue = qc.assay_value - limit;
			maxValue = qc.assay_value + limit;

			break;
		}
		case ap_Size:
		{
			// "qc.assay_value" is in microns (um).
			//float avg_diameter_pop; (general)   USE THIS ONE

			assayTypeStr = "Size";
			limit = qc.assay_value * (qc.plusminus_percentage / 100.0);
			minValue = qc.assay_value - limit;
			maxValue = qc.assay_value + limit;

			break;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("%s:: value: %.6lf, limit: %.6lf, range: %.6lf to %.6lf")
			% assayTypeStr
			% dataValue
			% limit
			% minValue
			% maxValue));

	if (dataValue >= minValue && dataValue <= maxValue)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("\tsample %s is in range") % assayTypeStr));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("\tsample %s out of range") % assayTypeStr));
	}

}

static void Test_QCs()
{
	// Concentration...
	Test_QC (253184.00, "QC 1");
	Test_QC (253184.00, "QC 2");
	Test_QC (253184.00, "QC 3");

	// Population Percentage
	Test_QC (20, "QC 4");
	Test_QC (20, "QC 5");
	Test_QC (20, "QC 6");

	// Size
	Test_QC (1.7815356255, "QC 7");
	Test_QC (1.7815356255, "QC 8");
	Test_QC (1.7815356255, "QC 9");

}
#endif

//*****************************************************************************
void HawkeyeLogicImpl::Initialize (bool withHardware)
{
	// Set the availability of hardware in the HawkeyeConfig so it can be accessed by
	// all of the code simply by *get*ting the HawkeyeConfig_t.
	if (withHardware)
	{		
		HawkeyeConfig::Instance().setHardwareForShepherd();
	}
	else
	{
		HawkeyeConfig::Instance().setHardwareForSimulation();
	}
	
	auto onInitializeComplete = [this](bool status) -> void
	{
		void CreateCellHealthErrorCodes();
		CreateCellHealthErrorCodes();
		
		if (status)
		{
			SystemStatus::Instance().getData().status = eSystemStatus::eIdle;
			initializationState_ = eInitializationComplete;
			Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitialize: <exit, successful>");
		}
		else
		{
			SystemStatus::Instance().getData().status = eSystemStatus::eFaulted;

			// Do not override the "eInitializationStopped_CarosuelTubeDetected" initialization state.
			if (initializationState_ != eInitializationStopped_CarosuelTubeDetected)
			{
				initializationState_ = eInitializationFailed;
			}
			Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitialize: <exit, failed>");
		}
	};

	initializationState_ = eInitializationInProgress;

	if (!InstrumentConfig::Instance().Initialize())
	{
		// Error reported in the called function
		AuditLogger::L().Log (generateAuditWriteData(
			"", 
			audit_event_type::evt_instrumentconfignotfound, 
			"Instrument Config"));
		onInitializeComplete (false);
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "Initialize: <enter>");

	isShutdownInProgress_ = false;

	if ( !initializationRun )
	{
		if ( !loggersInitialized )
		{
			if ( !intializeLoggers() )
			{
				// Initialize has failed, no need to continue further
				onInitializeComplete( false );
				return;
			}
			loggersInitialized = true;
		}

		if ( !asioInitialized )
		{
			if ( !intializeAsio() )
			{
				// Initialize has failed, no need to continue further
				onInitializeComplete( false );
				return;
			}
			asioInitialized = true;
		}

		initializationRun = true;
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitialize : Reinitializing the software");
		Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitialize : Stopping previous hardware operations");
		Hardware::Instance().StopHardwareOperations();

		auto errorCodes = ReportSystemError::Instance().GetErrorCodes();
		if (!errorCodes.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitialize : Clearing all previous errors");
			for (auto& item : errorCodes)
			{
				ReportSystemError::Instance().DecodeAndLogErrorCode(item);
			}
			ReportSystemError::Instance().ClearAllErrors();
			errorCodes.clear();
		}
	}

	// the workflow in internalInitialize (and sub-methods) does not require the user id, so do not use the transient user technique
	pLocalIosvc_->post (std::bind (&HawkeyeLogicImpl::internalInitialize, this, onInitializeComplete));
}

//*****************************************************************************
// Initialize modules which are one time initialize until shutdown.
//*****************************************************************************
bool HawkeyeLogicImpl::intializeLoggers()
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "intializeLoggers: <enter>");

	bool success = true;
	boost::system::error_code ec;

	const DBApi::DB_InstrumentConfigRecord& dbInstConfig = InstrumentConfig::Instance().Get();
	{
		LoggerSettings_t loggerSettings = { dbInstConfig.CameraErrorLogName, severity_level::normal, static_cast<size_t>(dbInstConfig.CameraErrorLogMaxSize), 100, true, false };
		CameraErrorLogger::L().Initialize (ec, loggerSettings);
	}

	if (!HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			"", 
			audit_event_type::evt_offlinemode, 
			"Offline Mode"));
	}

	Logger::L().Log( MODULENAME, severity_level::debug1, "intializeLoggers: <exit>" );

	return success;
}

//*****************************************************************************
bool HawkeyeLogicImpl::intializeAsio()
{
	Logger::L().Log( MODULENAME, severity_level::debug1, "intializeAsio: <enter>" );

	// Initialize ASIO timers and threads

	bool success = true;
	boost::system::error_code ec;

	// This local io_service is used as a *master* io_service for the HawkeyeCore DLL.
	// This object is passed into classes to be used for connecting timers.
	pLocalIosThreadPool_ = std::make_unique<HawkeyeThreadPool>(true/*auto start*/, 3/*num thread*/, "Main_Hawkeye_Thread");
	pLocalIosvc_ = pLocalIosThreadPool_->GetIoService();

	// 2. Initialize HawkeyeServices
	pHawkeyeServices_ = std::make_shared<HawkeyeServicesImpl>(pLocalIosvc_);

#if 1
	if (diskUsageLoggerTimer_)
	{
		diskUsageLoggerTimer_->cancel();
		diskUsageLoggerTimer_.reset();
	}

	diskUsageLoggerTimer_ = std::make_shared<boost::asio::deadline_timer>(*pLocalIosvc_);
	
	LogMemoryAndDiskUsage(boost::system::errc::make_error_code(boost::system::errc::success));

	// 4. Create CPU usage logger timer
	if (cpuUsageTimer_)
	{
		cpuUsageTimer_->cancel();
		cpuUsageTimer_.reset();
	}
	
	cpuUsageTimer_ = std::make_shared<boost::asio::deadline_timer>(*pLocalIosvc_);

	LogCPUUsage (boost::system::errc::make_error_code(boost::system::errc::success));
#else
	Logger::L().Log (MODULENAME, severity_level::warning, "intializeOnce: <disk and CPU logging is DISABLED>");
#endif

	Logger::L().Log( MODULENAME, severity_level::debug1, "intializeAsio: <exit>" );

	return success;
}

//*****************************************************************************
void HawkeyeLogicImpl::internalInitialize (BooleanCallback callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitialize: <enter>");
	Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("internalInitialize: DLL software version : %s") % DLL_Version));

	std::string strType;
	
	Logger::L().Log (MODULENAME, severity_level::normal, "brightfield_led_type: " + strType);

	if ( !HawkeyeConfig::Instance().get().normalShutdown )
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Restarting from an abnormal shutdown");
	}

	// Assume that there will be an abnormal shutdown.
	//NOTE: currently this is not used for anything.
	HawkeyeConfig::Instance().get().normalShutdown = false;

	Logger::L().Log (MODULENAME, severity_level::normal, "OpenCV v" + std::string(CV_VERSION));

	pHawkeyeServices_->enqueueInternal([this, callback]() -> void
		{
			setNextInitializationSequence ([this, callback](bool status) -> void
				{
					pHawkeyeServices_->enqueueInternal (callback, status);
				}, InitializationSequence::Hardware);
		});
}

//*****************************************************************************
void HawkeyeLogicImpl::internalInitializationProcess (std::function<void( bool )> callback, InitializationSequence sequence)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitializationProcess : " + EnumConversion<InitializationSequence>::enumToString(sequence));

	switch (sequence)
	{
		case InitializationSequence::Hardware:
		{
			Hardware::Instance().initialize (pHawkeyeServices_->getInternalIos(), initializationState_,
				[this, callback](bool status)
				{
					InitializationSequence nextSeq = InitializationSequence::Analysis;

					if ( status )
					{
						Logger::L().Log( MODULENAME, severity_level::normal, "Successfully initialized hardware." );

						WorkflowController::Instance().Initialize( pHawkeyeServices_->getMainIos() );

						pDigitalSignature_ = std::make_shared<SecurityHelpers::RSADigitalSignature>();
						pReanalysisImgProcUtilities_ = std::make_shared<ImageProcessingUtilities>();
						pReanalysisImgProcUtilities_->initialize( pLocalIosvc_ );
						LiveScanningUtilities::Instance().Initialize( pLocalIosvc_ );

						// Now that hardware is running, begin monitoring for comms faults that will require us to shutdown the system
						if (commsHealthTimer_)
						{
							commsHealthTimer_->cancel();
							commsHealthTimer_.reset();
						}

						commsHealthTimer_ = std::make_shared<boost::asio::deadline_timer>(*pLocalIosvc_);
						CheckCommsStatusAndShutdown(boost::system::errc::make_error_code(boost::system::errc::success));
					}
					else
					{
						Logger::L().Log( MODULENAME, severity_level::critical, "Failed to initialize hardware" );
						nextSeq = InitializationSequence::Error;
					}

					setNextInitializationSequence( callback, nextSeq );
					return;
				});
			return;
		}

		case InitializationSequence::Analysis:
		{
			if (initializeAnalysis())
			{
				setNextInitializationSequence (callback, InitializationSequence::CellType);
			}
			else
			{
				setNextInitializationSequence (callback, InitializationSequence::Error);
				AuditLogger::L().Log (generateAuditWriteData(
					"",
					audit_event_type::evt_instrumentconfignotfound,
					"Analyses"));
			}
			return;
		}

		case InitializationSequence::CellType:
		{
			if (initializeCellType())
			{
				setNextInitializationSequence (callback, InitializationSequence::UserList);
			}
			else
			{
				AuditLogger::L().Log (generateAuditWriteData(
					"",
					audit_event_type::evt_instrumentconfignotfound,
					"Cell Type"));
				setNextInitializationSequence (callback, InitializationSequence::Error);
			}
			return;
		}

		case InitializationSequence::UserList:
		{
			HawkeyeError he = UserList::Instance().Initialize (false);
			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log(MODULENAME, severity_level::critical, "internalInitialize: <exit: failed to load UserList>");
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_readerror,
					instrument_error::instrument_storage_instance::userlist,
					instrument_error::severity_level::error));

				if (he == HawkeyeError::eValidationFailed)
					AuditLogger::L().Log (generateAuditWriteData(
						"", 
						audit_event_type::evt_datavalidationfailure, 
						"User List"));
				else if (he == HawkeyeError::eStorageFault)
					AuditLogger::L().Log (generateAuditWriteData(
						"", 
						audit_event_type::evt_instrumentconfignotfound, 
						"User List"));

				pHawkeyeServices_->enqueueInternal(callback, false);
			}

			setNextInitializationSequence(callback, InitializationSequence::SerialNumber);
			return;
		}

		case InitializationSequence::SerialNumber:
		{
			if ( initializeSerialNumber() )
			{
				Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("Initialized serial number. \"%s\"") % serialNumber_Canonical));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, boost::str (boost::format ("Failed to initialize serial number.\nStored in configuration: \"%s\"\nStored on controller: \"%s\"") 
					% serialNumber_ConfigCopy % serialNumber_FirmwareCacheCopy));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_precondition_notmet,
					instrument_error::instrument_precondition_instance::instrument_serialnumber,
					instrument_error::severity_level::error));
				AuditLogger::L().Log (generateAuditWriteData(
					"", 
					audit_event_type::evt_instrumentconfignotfound, 
					"Serial Number"));
			}

			setNextInitializationSequence (callback, InitializationSequence::SystemStatus);
			return;
		}

		case InitializationSequence::SystemStatus:
		{
			if ( SystemStatus::Instance().initialize())
			{
				setNextInitializationSequence (callback, InitializationSequence::CalibrationHistory);
			}
			else
			{
				// Error reported in the called function
				AuditLogger::L().Log (generateAuditWriteData(
					"", 
					audit_event_type::evt_instrumentconfignotfound, 
					"System status"));
				setNextInitializationSequence (callback, InitializationSequence::Error);
			}
			return;
		}

		case InitializationSequence::CalibrationHistory:
		{
			if ( initializeCalibrationHistory() )
			{
				setNextInitializationSequence (callback, InitializationSequence::QualityControl);
			}
			else
			{
				AuditLogger::L().Log (generateAuditWriteData(
					"", 
					audit_event_type::evt_instrumentconfignotfound, 
					"Concentration Factor"));
				setNextInitializationSequence (callback, InitializationSequence::Error);
			}
			return;
		}

		case InitializationSequence::QualityControl:
		{
			if ( initializeQualityControl() )
			{
				setNextInitializationSequence (callback, InitializationSequence::ReagentPack);
			}
			else
			{
				AuditLogger::L().Log (generateAuditWriteData(
					"", 
					audit_event_type::evt_instrumentconfignotfound, 
					"Quality Control"));
				setNextInitializationSequence (callback, InitializationSequence::Error);
			}
			return;
		}

		case InitializationSequence::ReagentPack:
		{
			// Read the current reagent volumes from the database.
			CellHealthReagents::Initialize();

			// the workflow in initializationReagentPack does not require the user id, so do not use the transient user technique
			pHawkeyeServices_->getMainIos()->post ([this, callback]() -> void {
				initializeReagentPack ([this, callback](bool status) -> void {
					if (status) {
						setNextInitializationSequence (callback, InitializationSequence::Results);
					} else {
						AuditLogger::L().Log (generateAuditWriteData(
							"", 
							audit_event_type::evt_instrumentconfignotfound, 
							"Reagent Pack"));
						setNextInitializationSequence (callback, InitializationSequence::Error);
					}
				});
			});			
			return;
		}

		case InitializationSequence::Results:
		{
			pHawkeyeServices_->enqueueInternal([this, callback]() {

				initializeResults();
				setNextInitializationSequence (callback, InitializationSequence::Signatures);
			});
			return;
		}

		case InitializationSequence::Signatures:
		{
			if ( initializeSignatures() )
			{
				setNextInitializationSequence (callback, InitializationSequence::NightlyClean);
			}
			else
			{
				AuditLogger::L().Log (generateAuditWriteData(
					"", 
					audit_event_type::evt_instrumentconfignotfound, 
					"Signature Definitions"));
				setNextInitializationSequence (callback, InitializationSequence::Error);
			}
			return;
		}

		case InitializationSequence::NightlyClean:
		{
			// Nightly clean is unnecessary if we are not in "real hardware" mode.
			if (!HawkeyeConfig::Instance().get().hardwareConfig.syringePump || initializeNightlyClean())
			{
				setNextInitializationSequence (callback, InitializationSequence::Validation);
			}
			else
			{
				AuditLogger::L().Log (generateAuditWriteData(
					"", 
					audit_event_type::evt_instrumentconfignotfound, 
					"Nightly Clean"));
				setNextInitializationSequence (callback, InitializationSequence::Error);
			}
			return;
		}

		case InitializationSequence::Validation:
		{
			validateInstrumentStatusOnLogin();
			setNextInitializationSequence (callback, InitializationSequence::Complete);
			return;
		}

		case InitializationSequence::Complete:
		{
			pHawkeyeServices_->enqueueInternal (callback, true);
			return;
		}

		case InitializationSequence::Error:
		{
			pHawkeyeServices_->enqueueInternal (callback, false);
			return;
		}

	} // End "switch (sequence)"

	HAWKEYE_ASSERT (MODULENAME, false);

	Logger::L().Log (MODULENAME, severity_level::debug1, "internalInitializationProcess: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::setNextInitializationSequence( std::function<void( bool )> callback, InitializationSequence nextSequence )
{
	// the workflow in internalInitializationProcess does not require the user id, so do not use the transient user technique
	pHawkeyeServices_->getMainIos()->post (std::bind (&HawkeyeLogicImpl::internalInitializationProcess, this, callback, nextSequence));
}

//*****************************************************************************
InitializationState HawkeyeLogicImpl::IsInitializationComplete()
{
	return Hardware::Instance().getInitializationState().get_value_or(initializationState_);
}

//*****************************************************************************
bool HawkeyeLogicImpl::InitializationComplete()
{
	return (initializationState_ == InitializationState::eInitializationComplete);
}

//*****************************************************************************
void HawkeyeLogicImpl::Shutdown()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Shutdown: <enter>");

	if ( isShutdownComplete_ )
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Shutdown: <exit, already called>");
		return;
	}

	isShutdownInProgress_ = true;

	if (diskUsageLoggerTimer_)
	{
		diskUsageLoggerTimer_->cancel();
		diskUsageLoggerTimer_.reset();
	}

	if (cpuUsageTimer_)
	{
		cpuUsageTimer_->cancel();
		cpuUsageTimer_.reset();
	}

	if (commsHealthTimer_)
	{
		commsHealthTimer_->cancel();
		commsHealthTimer_.reset();
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "Shutdown: <shutting down Hawkeye DLL>");

	// Eject the sample stage and remove holding current; use override angle to force disable of holding current and use of non-automation angle
	ejectSampleStageInternal( ShutdownEjectAngle, EJECT_HOLDING_CURRENT_OFF,
							  [ this ](HawkeyeError he)
							  {
								  pLocalIosvc_->post( std::bind( &HawkeyeLogicImpl::internalShutdown, this ) );
							  });

	Logger::L().Log (MODULENAME, severity_level::debug1, "Shutdown: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::internalShutdown()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "internalShutdown: <enter>");

	// Stop continuously reading the RFID tags
	ReagentPack::Instance().Stop();

	// Stop further communication to the Controller board, and workflow execution.
	Hardware::Instance().StopHardwareOperations();

	// Provide some time for the Controller board to finish if any RFID tag update is started
	//  before we exit the software.
	// 25 secs considered here for worst case Syringe move operation + RFID tag update.
	auto wt = std::make_shared<boost::asio::deadline_timer>(pHawkeyeServices_->getInternalIosRef());

	auto timeout = boost::posix_time::seconds(25);
	if (!HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard)
	{
		timeout = boost::posix_time::seconds(1);
	}

	wt->expires_from_now (timeout);

	wt->async_wait([this, wt](const boost::system::error_code& ec)->void
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "internalShutdown: <completion handler>");
			// Regardless of status of 'ec', continue with shutdown sequence

			destroyNightlyClean();

			pReanalysisImgProcUtilities_.reset();

			pHawkeyeServices_->enqueueExternal ([this]()->void
				{
					pHawkeyeServices_->StopServices();
				});

			isShutdownComplete_ = true;
			HawkeyeConfig::Instance().get().normalShutdown = isShutdownComplete_;
			Logger::L().Log (MODULENAME, severity_level::debug1, "internalShutdown: <completion exit>");
		});
}

//*****************************************************************************
bool HawkeyeLogicImpl::IsShutdownComplete()
{
	if (isShutdownComplete_ && pLocalIosThreadPool_)
	{
		pLocalIosvc_.reset();
		pLocalIosThreadPool_->Stop();
		pLocalIosThreadPool_.reset();

		pUiDllLayerMediator_.reset();
	}

	return isShutdownComplete_;
}

//*****************************************************************************
void HawkeyeLogicImpl::GetVersionInformation (SystemVersion& version)
{
	version = SystemStatus::Instance().systemVersion;
}

//*****************************************************************************
void HawkeyeLogicImpl::RotateCarousel(std::function<void(HawkeyeError, SamplePositionDLL)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RotateCarousel <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	HawkeyeError status = UserList::Instance().CheckPermissionAtLeast(UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{
			pHawkeyeServices_->enqueueExternal (callback, status, SamplePositionDLL{});
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			"", 
			audit_event_type::evt_notAuthorized,
			"Rotate Carousel"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser, SamplePositionDLL{});
		return;
	}

	// TODO : Check for the System eBusy status 
	auto pStageController = Hardware::Instance().getStageController();

	if (!pStageController->IsStageCalibrated(eCarrierType::eCarousel))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "RotateCarousel: <exit, Carousel is not registered");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eStageNotRegistered, SamplePositionDLL{});
		return;
	}

	if (!pStageController->IsCarouselPresent())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "RotateCarousel : Carousel not detected!");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime, SamplePositionDLL{});
		return;
	}

	if (HawkeyeConfig::Instance().get().hardwareConfig.stageController) // Check tube present only for hardware, not for simulator.
	{
		if (pStageController->IsTubePresent())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "RotateCarousel : Tube is detected at current position!");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_sampledeck_tubedetected, 
				instrument_error:: motion_sample_deck_instances::carousel, 
				instrument_error::severity_level::warning));
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime, SamplePositionDLL{});
			return;
		}
	}

	// Get the current tube number.
	SamplePositionDLL position = pStageController->GetStagePosition();
	int32_t currentTubeNum = position.getColumn();

	pStageController->GotoNextTube([this, callback, currentTubeNum](int32_t nextTubeNum) -> void
	{
		SamplePositionDLL pos = {};

		// Here the assumption is, if the carousel couldn't move to next tube then the current tube position will be returned.
		if (nextTubeNum == currentTubeNum
			|| !pos.setRowColumn(eStageRows::CarouselRow, static_cast<uint8_t>(nextTubeNum)))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "RotateCarousel: <exit, h/w fault>");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_sampledeck_positionfail, 
				instrument_error::motion_sample_deck_instances::carousel,
				instrument_error::severity_level::error));
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault, SamplePositionDLL{});
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, "RotateCarousel <exit 2>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess, pos);
	});
}

//*****************************************************************************
// Eject the sample stage to a default outward position and angle (0) with holding current off, or to the outward position with the angle specified and holding current enabled or disabled
// NOTE: calls through this method have NO carrier context, and will act based on the hardware-detected absence of a carousel (the inferred 'presence' of a plate).
HawkeyeError HawkeyeLogicImpl::EjectSampleStage (const char* username, const char* password,
	HawkeyeErrorCallback cb, int32_t angle, bool loading)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "EjectSampleStage: <enter>");

#if(0)		// this is the implementation of an externally accessible API; it should be doing user validation at his level
	HawkeyeError status = UserList::Instance().CheckPermissionAtLeast(UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{
			pHawkeyeServices_->enqueueExternal (cb, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			"",
			audit_event_type::evt_notAuthorized,
			"Eject Sample Stage"));
		pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eNotPermittedByUser);
		return;
	}
#endif

	HawkeyeError he = HawkeyeError::eSuccess;
	switch (SystemStatus::Instance().getData().status)
	{
		case eSystemStatus::eIdle:
		case eSystemStatus::ePaused:
		case eSystemStatus::eStopped:
		case eSystemStatus::eFaulted:
			ejectSampleStageInternal(angle, loading, cb);
			break;
		default:
			he = HawkeyeError::eBusy;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "EjectSampleStage <exit>");

	return he;
}

//*****************************************************************************
// Eject the sample stage to a default outward position and angle (0) with holding current off, or to the outward position with the angle specified and holding current enabled or disabled
// NOTE: calls through this method have NO carrier context, and will act based on the hardware-detected absence of a carousel (the inferred 'presence' of a plate).
void HawkeyeLogicImpl::ejectSampleStageInternal(int32_t angle, bool holding, HawkeyeErrorCallback cb)
{
	Logger::L().Log( MODULENAME, severity_level::debug1, "ejectSampleStageInternal: <enter>" );

	// TO DO : Check for the System eBusy status 

	//TO DO: There is an issue in Theta motor position reporting, and API returns false most of the Times. This needs to be resolved.
	//       However as EjectPate will take care of the probe motion we can use this API without any harm to Probe. Hence return type Eject API's are not verified.		


	// TO DO : Future use may require a change or removal of this test
	if ( Hardware::Instance().getStageController()->IsCarouselPresent() || angle == ShutdownEjectAngle )	// ShutdownEjectAngle is used during shutdown to force removal of holding current
	{
		angle = NoEjectAngle;
		holding = EJECT_HOLDING_CURRENT_OFF;
	}
	else
	{
		if ( HawkeyeConfig::Instance().get().automationEnabled )
		{
			if ( angle == AutoSelectEjectAngle )	// use specified automation eject angle
			{
				// TO DO : Future use may require a change to use stored values for eject angle
				angle = DfltAutomationEjectAngle;
			}
			holding = EJECT_HOLDING_CURRENT_ON;
		}
		else
		{
			if ( angle == AutoSelectEjectAngle )	// use normal non-automation eject angle
			{
				// TO DO : Future use may require a change to use stored values for eject angle
				angle = NoEjectAngle;
			}
		}
	}

	if ( angle < -359 || angle > 359 )
	{
		angle = (angle % 360);
	}

	Hardware::Instance().getStageController()->EjectStage( [ this, cb, holding ]( bool eject_ok )
		{
			auto ejectCompletion = [ this, cb, eject_ok ]( bool status )
			{
				auto he = eject_ok ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault;
				if ( !status )
				{
					he = HawkeyeError::eHardwareFault;
				}
				pHawkeyeServices_->enqueueExternal( cb, he );
			};

			auto stageProfileCompletion = [ this, ejectCompletion ]( bool status )
			{
				if ( !status )
				{
					Logger::L().Log( MODULENAME, severity_level::error, "ejectSampleStageInternal: <exit: error setting stage holding current>" );
				}
				ejectCompletion( status );
			};

			if ( !eject_ok )
			{
				Logger::L().Log( MODULENAME, severity_level::error, "ejectSampleStageInternal : <exit: Failed to Eject the Stage>" );
				ReportSystemError::Instance().ReportError( BuildErrorInstance(
					instrument_error::motion_sampledeck_ejectfail,
					instrument_error::motion_sample_deck_instances::general,
					instrument_error::severity_level::warning ) );
			}

			Hardware::Instance().getStageController()->SetStageProfile( stageProfileCompletion, holding );		// false to disable holding current, true to enable it
		}, angle );

	Logger::L().Log( MODULENAME, severity_level::debug1, "ejectSampleStageInternal <exit>" );
}

//*****************************************************************************
void SystemErrorCodeToExpandedStrings_internal (
	uint32_t system_error_code, std::string& severity, std::string& system, std::string& subsystem, std::string& instance, std::string& failure_mode )
{

	instrument_error::DecodeErrorInstance (system_error_code, severity, system, subsystem, instance, failure_mode);
}

//*****************************************************************************
std::string GetSystemErrorCodeAsStr( uint32_t system_error_code )
{

	std::string output, sev, sys, subsys, inst, fm;
	SystemErrorCodeToExpandedStrings_internal (system_error_code, sev, sys, subsys, inst, fm);

	std::stringstream ss;
	ss << "[" << sev << "] " << sys << " - " << subsys;
	if ( !inst.empty() )
	{
		ss << " - " << inst;
	}
	ss << ": " << fm;

	return ss.str();
}

//*****************************************************************************
char* HawkeyeLogicImpl::SystemErrorCodeToString( uint32_t system_error_code )
{
	char* op;

	DataConversion::convertToCharPointer (op, GetSystemErrorCodeAsStr (system_error_code));

	return op;
}

//*****************************************************************************
void HawkeyeLogicImpl::SystemErrorCodeToExpandedStrings (
	uint32_t system_error_code,
	char*& severity,
	char*& system, 
	char*& subsystem,
	char*& instance, 
	char*& failure_mode,
	char*& cellHealthErrorCode)
{
	std::string output, sev, sys, subsys, inst, fm;
	SystemErrorCodeToExpandedStrings_internal(system_error_code, sev, sys, subsys, inst, fm);

	DataConversion::convertToCharPointer(severity, sev);
	DataConversion::convertToCharPointer(system, sys);
	DataConversion::convertToCharPointer(subsystem, subsys);
	DataConversion::convertToCharPointer(instance, inst);
	DataConversion::convertToCharPointer(failure_mode, fm);

	// Remove "severity" and "instance" fields to allow matching the error code in
	// "cellHealthErrorCodes" since the values in "cellHealthErrorCodes" do not contain
	// the instance value.
	system_error_code &= ~instrument_error::SEVERITY_MASK;
	system_error_code &= ~instrument_error::INSTANCE_MASK;

	std::string chmErrorCode = cellHealthErrorCodes[system_error_code];
	if (chmErrorCode.empty())
	{
		chmErrorCode = "CH0";	// Unknown CHM error code, this should never occur.
	}
	DataConversion::convertToCharPointer(cellHealthErrorCode, chmErrorCode);
}

//*****************************************************************************
void HawkeyeLogicImpl::SystemErrorCodeToExpandedResourceStrings(
	uint32_t system_error_code,
	char*& severity, 
	char*& system, 
	char*& subsystem,
	char*& instance, 
	char*& failure_mode,
	char*& cellHealthErrorCode)
{

	std::string output, sev, sys, subsys, inst, fm;
	SystemErrorCodeToExpandedStrings_internal(system_error_code, sev, sys, subsys, inst, fm);

	sev = ConvertSystemErrorCodeSeverity_to_Resource(sev);
	sys = ConvertSystemErrorCodeSystem_to_Resource(sys);
	subsys = ConvertSystemErrorCodeSubsystem_to_Resource(subsys);
	inst = ConvertSystemErrorCodeInstance_to_Resource(inst);
	fm = ConvertSystemErrorCodeFailureMode_to_Resource(fm);

	DataConversion::convertToCharPointer(severity, sev);
	DataConversion::convertToCharPointer(system, sys);
	DataConversion::convertToCharPointer(subsystem, subsys);
	DataConversion::convertToCharPointer(instance, inst);
	DataConversion::convertToCharPointer(failure_mode, fm);

	// Remove "severity" and "instance" fields to allow matching the error code in
	// "cellHealthErrorCodes" since the values in "cellHealthErrorCodes" do not contain
	// the instance value.
	system_error_code &= ~instrument_error::SEVERITY_MASK;
	system_error_code &= ~instrument_error::INSTANCE_MASK;

	std::string chmErrorCode = cellHealthErrorCodes[system_error_code];
	if (chmErrorCode.empty())
	{
		chmErrorCode = "CH0";	// Unknown CHM error code, this should never occur.
	}
	DataConversion::convertToCharPointer(cellHealthErrorCode, chmErrorCode);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SampleTubeDiscardTrayEmptied()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "SampleTubeDiscardTrayEmptied: <enter>");

	// This must be done by any logged-in  User.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast( UserPermissionLevel::eNormal );
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			"",
			audit_event_type::evt_notAuthorized,
			"Sample Tube Discard Tray Emptied"));
		return HawkeyeError::eNotPermittedByUser;
	}

	SystemStatus::Instance().sampleTubeDiscardTrayEmptied();

	Logger::L().Log (MODULENAME, severity_level::debug1, "SampleTubeDiscardTrayEmptied: <exit>");

	return HawkeyeError::eSuccess;
}

void HawkeyeLogicImpl::LogMemoryAndDiskUsage (boost::system::error_code ec)
{
	if (ec || !diskUsageLoggerTimer_)
		return;

	// Schedule next round
	diskUsageLoggerTimer_->expires_from_now(boost::posix_time::seconds(60));
	diskUsageLoggerTimer_->async_wait([this](boost::system::error_code er)->void { this->LogMemoryAndDiskUsage(er); });
	
	// in bytes
	boost::filesystem::space_info si = boost::filesystem::space("C:\\");

	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);

	/*
	_tprintf (TEXT("There is  %*ld percent of memory in use.\n"),
	WIDTH, statex.dwMemoryLoad);
	_tprintf (TEXT("There are %*I64d total KB of physical memory.\n"),
	WIDTH, statex.ullTotalPhys/DIV);
	_tprintf (TEXT("There are %*I64d free  KB of physical memory.\n"),
	WIDTH, statex.ullAvailPhys/DIV);
	_tprintf (TEXT("There are %*I64d total KB of paging file.\n"),
	WIDTH, statex.ullTotalPageFile/DIV);
	_tprintf (TEXT("There are %*I64d free  KB of paging file.\n"),
	WIDTH, statex.ullAvailPageFile/DIV);
	_tprintf (TEXT("There are %*I64d total KB of virtual memory.\n"),
	WIDTH, statex.ullTotalVirtual/DIV);
	_tprintf (TEXT("There are %*I64d free  KB of virtual memory.\n"),
	WIDTH, statex.ullAvailVirtual/DIV);
	*/

	std::string logMsg = boost::str(boost::format(
		"DiskFreeSpace: %d MB, MemoryInUse: %d percent")
			% (si.free / (1024 * 1024))
			% (statex.dwMemoryLoad));

	Logger::L().Log (MODULENAME, severity_level::notification, logMsg);
}

//*****************************************************************************
// From https://stackoverflow.com/questions/23143693/retrieving-cpu-load-percent-total-in-windows-with-c
//*****************************************************************************
static float CalculateCPULoad( unsigned long long idleTicks, unsigned long long totalTicks )
{

	static unsigned long long _previousTotalTicks = 0;
	static unsigned long long _previousIdleTicks = 0;

	unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
	unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

	float load = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

	_previousTotalTicks = totalTicks;
	_previousIdleTicks = idleTicks;

	return load;
}

//*****************************************************************************
static unsigned long long FileTimeToInt64( const FILETIME& ft )
{
	return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

constexpr int collectionInterval_sec = 10;
constexpr int cpuUsageTimerInterval_ms = 250;
constexpr int maxCountsPerInterval = (1000 / cpuUsageTimerInterval_ms) * collectionInterval_sec;
//*****************************************************************************
void HawkeyeLogicImpl::LogCPUUsage( boost::system::error_code ec )
{
	static int counts;

	if ( ec || !cpuUsageTimer_ )
	{
		return;
	}

	// Schedule next round
	cpuUsageTimer_->expires_from_now(boost::posix_time::milliseconds(cpuUsageTimerInterval_ms));
	cpuUsageTimer_->async_wait([this](boost::system::error_code er)->void {
		this->LogCPUUsage (er);
	});

	FILETIME idleTime, kernelTime, userTime;
	float cpuUsage = GetSystemTimes (&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
	cpuUsage *= 100.0f;


	if ( counts++ >= maxCountsPerInterval && cpuUsage < 10.0)
	{
		Logger::L().Log (MODULENAME, severity_level::notification, boost::str (boost::format ("CPU Usage: %5.2f%%") % cpuUsage));
		counts = 0;
	}
}

constexpr int commsCheckInterval_sec = 5;

void HawkeyeLogicImpl::CheckCommsStatusAndShutdown(boost::system::error_code ec)
{
	if (ec || !commsHealthTimer_)
		return;

	if (Hardware::Instance().IsControllerBoardFaulted())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Faulted communciations detected; shutting down internal logic.");

		pHawkeyeServices_->enqueueInternal([this]()->void {
			this->Shutdown();
			});

		return;
	}

	// Schedule next round
	commsHealthTimer_->expires_from_now(boost::posix_time::seconds(commsCheckInterval_sec));
	commsHealthTimer_->async_wait([this](boost::system::error_code er)->void {
		this->CheckCommsStatusAndShutdown(er);
		});


}

//*****************************************************************************
UiDllLayerMediator& HawkeyeLogicImpl::getUiDllInstance()
{
	std::shared_ptr<boost::asio::io_context> localAsio;
	if (pLocalIosvc_ && !isShutdownInProgress_)
	{
		localAsio = pLocalIosvc_;
	}

	if (!pUiDllLayerMediator_)
	{
		pUiDllLayerMediator_ = std::make_unique<UiDllLayerMediator>(localAsio);
	}
	return *pUiDllLayerMediator_;
}

//*****************************************************************************
void HawkeyeLogicImpl::validateInstrumentStatusOnLogin()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "validateInstrumentStatusOnLogin : <enter>");

	auto stageController = Hardware::Instance().getStageController();

	if (!stageController)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "validateInstrumentStatusOnLogin : Invalid stage controller instance!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::motion_sampledeck_initerror, 
			instrument_error::motion_sample_deck_instances::general,
			instrument_error::severity_level::error));
		return;
	}

	// Check if plate is calibrated
	if (!stageController->IsStageCalibrated(eCarrierType::ePlate_96))
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "validateInstrumentStatusOnLogin : Plate is not calibrated!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::plate_registration,
			instrument_error::severity_level::error));
	}

	// Check if carousel is calibrated
	if (!stageController->IsStageCalibrated(eCarrierType::eCarousel))
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "validateInstrumentStatusOnLogin : Carousel is not calibrated!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::carousel_registration,
			instrument_error::severity_level::error));
	}

	// Check if Concentration calibration not set
	if (!CalibrationHistoryDLL::Instance().IsCurrentConcentrationCalibrationValid())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "validateInstrumentStatusOnLogin : Invalid concentration calibration!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::concentration_config,
			instrument_error::severity_level::error));
	}

	// Check if Size calibration not set
	if (!CalibrationHistoryDLL::Instance().IsCurrentSizeCalibrationValid())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "validateInstrumentStatusOnLogin : Invalid size calibration!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::size_config,
			instrument_error::severity_level::error));
	}

	// Check if focus is not set
	if (!SystemStatus::Instance().getData().focus_IsFocused)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "validateInstrumentStatusOnLogin: Focus operation is not done!");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::focus, 
			instrument_error::severity_level::warning));
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "validateInstrumentStatusOnLogin: <exit>");
}

//*****************************************************************************
bool HawkeyeLogicImpl::SystemHasData()
{
	std::vector <DBApi::DB_SampleRecord> dbSampleRecordlist = {};

	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleList(
		dbSampleRecordlist,
		DBApi::eListFilterCriteria::NoFilter,		// Filter type.
		"=",										// Filter compare operator: '=' '<' '>' etc.
		"",											// Filter target value for comparison: true false value etc
		1,											// Limit count.
		DBApi::eListSortCriteria::SortNotDefined,	// primary Sort type.
		DBApi::eListSortCriteria::SortNotDefined,	// secondary Sort type.
		-1,											// Sort direction, descending.
		"",											// Order string.
		-1,											// Start index.
		-1);										// Start ID num
	if (dbStatus == DBApi::eQueryResult::QueryOk)
	{
		return true;
	}
	else if (DBApi::eQueryResult::NoResults == dbStatus)
	{
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::error,
		boost::str (boost::format ("DbGetWorklistList: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));
	ReportSystemError::Instance().ReportError (BuildErrorInstance(
		instrument_error::instrument_storage_writeerror,
		instrument_error::instrument_storage_instance::workflow_design,
		instrument_error::severity_level::error));

	return false;
}

//*****************************************************************************
void HawkeyeLogicImpl::WriteToAuditLog (const char* username, audit_event_type type, const char* resource)
{
	// LH6531-6576 - correctly attribute remote activities to remote users.
	// Record whether or not we are in an automation locked state.  This is the 
	// only place where the backend is going to get this information.
	switch (type)
	{
		case audit_event_type::evt_automationlocked:
		{
			g_currentSystemAutomationLockstatus = true;
			break;
		}
		case audit_event_type::evt_automationunlocked:
		{
			g_currentSystemAutomationLockstatus = false;
			break;
		}
		default:
		{
			/*no-op*/
			break;
		}
	};


	AuditLogger::L().Log (generateAuditWriteData(
		std::string(username),
		type,
		std::string(resource)));
}

//*****************************************************************************
// This code is from: https://iq.direct/blog/332-how-to-shutdown-a-computer-programmatically-in-windows.html
//*****************************************************************************
static BOOL EnableShutdownPrivilege(BOOL bEnable)
{
	HANDLE hToken = NULL;
	TOKEN_PRIVILEGES tkp;
	BOOL bRet = FALSE;
	
	// Get a token for this process. 
	if (OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		// Get the LUID
		if (LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid))
		{
			tkp.PrivilegeCount = 1;  // one privilege to set    
			if (bEnable)
				tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			else
				tkp.Privileges[0].Attributes = 0;

			bRet = AdjustTokenPrivileges (hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
		}

		CloseHandle(hToken);
	}

	return bRet;
}

//*****************************************************************************
void HawkeyeLogicImpl::ShutdownOrReboot (ShutdownOrRebootEnum operation)
{
	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str (boost::format ("ShutdownOrReboot: <%s requested>") % (operation == ShutdownOrRebootEnum ::Shutdown? "Shutdown" : "Reboot")));

	const BOOL stat = EnableShutdownPrivilege (TRUE);
	if (stat == TRUE)
	{
		BOOL stat = FALSE;
		
		// EWX_FORCE flag forces all applicaitons to close before the shutdown.		
		if (operation == ShutdownOrRebootEnum::Shutdown)
		{	// Shutdown Windows
			stat = ExitWindowsEx (EWX_POWEROFF | EWX_SHUTDOWN | EWX_FORCE, 0);
		}
		else
		{	// Reboot Windows 			
			stat = ExitWindowsEx (EWX_REBOOT | EWX_FORCE, 0);
		}

		if (stat == FALSE)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("ShutdownOrReboot: <%s failed>") % (operation == ShutdownOrRebootEnum::Shutdown ? "Shutdown" : "Reboot")));
		}
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("ShutdownOrReboot: insufficient privilege for operation")));
	}
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetReagentVolume (CellHealthReagents::FluidType type, int32_t& volume_ul)
{
	return CellHealthReagents::GetVolume (type, volume_ul);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetReagentVolume (CellHealthReagents::FluidType type, int32_t volume_ul)
{
	// LH66531-6724 : Allow reagent volume update when we are not actively running samples or transitioning states (allow refills)
	if (SystemStatus::Instance().getData().status != eSystemStatus::eIdle &&
		SystemStatus::Instance().getData().status != eSystemStatus::ePaused &&
		SystemStatus::Instance().getData().status != eSystemStatus::eStopped)

	{
		return HawkeyeError::eBusy;
	}
	
	return CellHealthReagents::SetVolume (type, volume_ul);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::AddReagentVolume (CellHealthReagents::FluidType type, int32_t volume_ul)
{
	if (SystemStatus::Instance().getData().status != eSystemStatus::eIdle)
	{
		return HawkeyeError::eBusy;
	}

	return CellHealthReagents::AddVolume (type, volume_ul);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::DeleteCampaignData()
{
	std::list<std::pair<std::string, std::string>> tableNames = {
		{"Worklists",        "ViCellInstrument"},
		{"SampleSets",       "ViCellInstrument"},
		{"SampleItems",      "ViCellInstrument"},
		{"Analyses",         "ViCellData"},
		{"DetailedResults",  "ViCellData"},
		{"ImageReferences",  "ViCellData"},
		{"ImageResults",     "ViCellData"},
		{"ImageSequences",   "ViCellData"},
		{"ImageSets",        "ViCellData"},
		{"SResults",         "ViCellData"},
		{"SampleProperties", "ViCellData"},
		{"SummaryResults",   "ViCellData"},
	};

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::BadQuery; 

	if (DBApi::DbTruncateTableContents(queryResult, tableNames) && queryResult == DBApi::eQueryResult::QueryOk)
	{
		for (auto tbl : tableNames)
		{
			Logger::L().Log(MODULENAME, severity_level::normal, boost::str(
				boost::format("DeleteCampaignData: deleted data from %s.%s table")
				% tbl.second
				% tbl.first));
		}
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error, "DeleteCampaignData: error deleting data from tables");

		AuditLogger::L().Log(generateAuditWriteData(
			UserList::Instance().GetAttributableUserName(),
			audit_event_type::evt_deletecampaigndata,
			"Clearing campaign data: error deleting data from tables"));

		return HawkeyeError::eDatabaseError;
	}

	const std::string pathToDelete = HawkeyeDirectory::Instance().getInstrumentDir() + "\\ResultsData\\Images";
	Logger::L().Log(MODULENAME, severity_level::normal, "Deleting image files in " + pathToDelete);

	if (!FileSystemUtilities::RemoveAll (pathToDelete))
	{
		std::string str = "Unable to delete image files in " + pathToDelete;
		Logger::L().Log(MODULENAME, severity_level::error, str);

		AuditLogger::L().Log(generateAuditWriteData(
			UserList::Instance().GetAttributableUserName(),
			audit_event_type::evt_deletecampaigndata,
			str));

		return HawkeyeError::eSoftwareFault;
	}

	AuditLogger::L().Log(generateAuditWriteData(
		UserList::Instance().GetAttributableUserName(),
		audit_event_type::evt_deletecampaigndata,
		"All campaign data deleted"));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void CreateCellHealthErrorCode (uint32_t errorNum, uint32_t errorCode)
{
	if (!cellHealthErrorCodes[errorCode].empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug3,
			boost::str (boost::format ("Error code: %d is occupied: %s")
				% errorCode
				% cellHealthErrorCodes[errorCode]));
	}

	cellHealthErrorCodes[errorCode] = "CH" + std::to_string(errorNum);

	std::string severity;
	std::string system;
	std::string subsystem;
	std::string instance;
	std::string failure_mode;

	instrument_error::DecodeErrorInstance (errorCode, severity, system, subsystem, instance, failure_mode);
	Logger::L().Log (MODULENAME, severity_level::debug3,
		boost::str (boost::format ("CHM Code: %s, error_code: %d, Severity: %s, System: %s, Subsystem: %s, Instance: %s, FailureMode: %s")
			% cellHealthErrorCodes[errorCode]
			% errorCode
			% severity
			% system
			% subsystem
			% instance
			% failure_mode));
}

//*****************************************************************************
void CreateCellHealthErrorCodes()
{
	cellHealthErrorCodes.clear();

	// !!! WARNING !!! any added error codes MUST be added to the end of this list.
	CreateCellHealthErrorCode (1, instrument_error::published_errors::instrument_configuration_failedvalidation);	// error
	CreateCellHealthErrorCode (2, instrument_error::published_errors::instrument_storage_filenotfound);			// error
	CreateCellHealthErrorCode (3, instrument_error::published_errors::instrument_storage_readerror);				// error
	CreateCellHealthErrorCode (4, instrument_error::published_errors::instrument_storage_writeerror);				// error
	CreateCellHealthErrorCode (5, instrument_error::published_errors::instrument_storage_deleteerror);			// error
	CreateCellHealthErrorCode (6, instrument_error::published_errors::instrument_storage_storagenearcapacity);	// warning, error
	CreateCellHealthErrorCode (7, instrument_error::published_errors::instrument_storage_notallowed);				// warning
	CreateCellHealthErrorCode (8, instrument_error::published_errors::instrument_integrity_softwarefault);		// error
	CreateCellHealthErrorCode (9, instrument_error::published_errors::instrument_integrity_hardwarefault);		// error
	CreateCellHealthErrorCode (10, instrument_error::published_errors::instrument_precondition_notmet);			// warning, error
	CreateCellHealthErrorCode (11, instrument_error::published_errors::controller_general_connectionerror);		// error
	CreateCellHealthErrorCode (12, instrument_error::published_errors::controller_general_hostcommerror);			// warning, error
	CreateCellHealthErrorCode (13, instrument_error::published_errors::controller_general_firmwareupdate);		// notification, error
	CreateCellHealthErrorCode (14, instrument_error::published_errors::controller_general_firmwarebootup);		// error
	CreateCellHealthErrorCode (15, instrument_error::published_errors::controller_general_firmwareinvalid);		// error
	CreateCellHealthErrorCode (16, instrument_error::published_errors::controller_general_interface);				// notification
	CreateCellHealthErrorCode (17, instrument_error::published_errors::controller_general_fw_operation);			// error
	CreateCellHealthErrorCode (18, instrument_error::published_errors::controller_general_hardware_health);		// warning, error
	CreateCellHealthErrorCode (19, instrument_error::published_errors::controller_general_eepromreaderror);		// error
	CreateCellHealthErrorCode (20, instrument_error::published_errors::controller_general_eepromwriteerror);		// error
	CreateCellHealthErrorCode (21, instrument_error::published_errors::controller_general_eepromeraseerror);		// error
	CreateCellHealthErrorCode (23, instrument_error::published_errors::reagent_pack_invalid);						// warning
	CreateCellHealthErrorCode (24, instrument_error::published_errors::reagent_pack_expired);						// warning
	CreateCellHealthErrorCode (25, instrument_error::published_errors::reagent_pack_empty);						// warning
	CreateCellHealthErrorCode (26, instrument_error::published_errors::reagent_pack_nopack);						// notification, warning
	CreateCellHealthErrorCode (27, instrument_error::published_errors::reagent_pack_updatefail);					// warning
	CreateCellHealthErrorCode (28, instrument_error::published_errors::reagent_pack_loaderror);					// warning
	CreateCellHealthErrorCode (29, instrument_error::published_errors::motion_motor_initerror);					// warning, error
	CreateCellHealthErrorCode (30, instrument_error::published_errors::motion_motor_timeout);						// warning, error
	CreateCellHealthErrorCode (31, instrument_error::published_errors::motion_motor_homefail);					// error
	CreateCellHealthErrorCode (32, instrument_error::published_errors::motion_motor_positionfail);				// warning, error
	CreateCellHealthErrorCode (33, instrument_error::published_errors::motion_motor_holdingcurrentfail);			// error
	CreateCellHealthErrorCode (34, instrument_error::published_errors::motion_motor_drivererror);					// warning
	CreateCellHealthErrorCode (35, instrument_error::published_errors::motion_motor_operationlogic);				// warning, error
	CreateCellHealthErrorCode (36, instrument_error::published_errors::fluidics_general_nightlyclean);			// warning	
	CreateCellHealthErrorCode (37, instrument_error::published_errors::fluidics_syringepump_initerror);			// error
	CreateCellHealthErrorCode (38, instrument_error::published_errors::fluidics_syringepump_hardwareerror);		// error
	CreateCellHealthErrorCode (39, instrument_error::published_errors::fluidics_syringepump_commserror);			// error
	CreateCellHealthErrorCode (40, instrument_error::published_errors::fluidics_syringepump_overpressure);		// error
	CreateCellHealthErrorCode (41, instrument_error::published_errors::imaging_general_timeout);					// notification
	CreateCellHealthErrorCode (42, instrument_error::published_errors::imaging_general_logicerror);				// warning
	CreateCellHealthErrorCode (43, instrument_error::published_errors::imaging_general_imagequality);				// notification
	CreateCellHealthErrorCode (44, instrument_error::published_errors::imaging_general_backgroundadjust);			// notification	
	CreateCellHealthErrorCode (45, instrument_error::published_errors::imaging_camera_hardwareerror);				// warning, error
	CreateCellHealthErrorCode (46, instrument_error::published_errors::imaging_camera_timeout);					// error
	CreateCellHealthErrorCode (47, instrument_error::published_errors::imaging_camera_connection);				// error
	CreateCellHealthErrorCode (48, instrument_error::published_errors::imaging_camera_initerror);					// error
	CreateCellHealthErrorCode (49, instrument_error::published_errors::imaging_camera_noimage);					// warning, error	
	CreateCellHealthErrorCode (50, instrument_error::published_errors::imaging_trigger_hardwareerror);			// error
	CreateCellHealthErrorCode (51, instrument_error::published_errors::imaging_led_hardwareerror);				// error
	CreateCellHealthErrorCode (52, instrument_error::published_errors::imaging_led_initerror);					// error
	CreateCellHealthErrorCode (53, instrument_error::published_errors::imaging_led_powerthreshold);				// notification, error
	CreateCellHealthErrorCode (54, instrument_error::published_errors::imaging_led_commserror);					// error
	CreateCellHealthErrorCode (55, instrument_error::published_errors::imaging_led_reset);						// warning	
	CreateCellHealthErrorCode (56, instrument_error::published_errors::imaging_omicron_response_too_short);		// error
	CreateCellHealthErrorCode (57, instrument_error::published_errors::imaging_omicron_interlock);				// error
	CreateCellHealthErrorCode (58, instrument_error::published_errors::imaging_omicron_cdrh_error);				// error
	CreateCellHealthErrorCode (59, instrument_error::published_errors::imaging_omicron_underover_voltage);		// error
	CreateCellHealthErrorCode (60, instrument_error::published_errors::imaging_omicron_external_interlock);		// error
	CreateCellHealthErrorCode (61, instrument_error::published_errors::imaging_omicron_diode_current);			// error
	CreateCellHealthErrorCode (62, instrument_error::published_errors::imaging_omicron_ambient_temp);				// error
	CreateCellHealthErrorCode (63, instrument_error::published_errors::imaging_omicron_diode_temp);				// error
	CreateCellHealthErrorCode (64, instrument_error::published_errors::imaging_omicron_internal_error);			// error
	CreateCellHealthErrorCode (65, instrument_error::published_errors::imaging_omicron_diode_power);				// error	
	CreateCellHealthErrorCode (66, instrument_error::published_errors::sample_analysis_invalidtype);				// warning
	CreateCellHealthErrorCode (67, instrument_error::published_errors::sample_celltype_invalidtype);				// warning
	CreateCellHealthErrorCode (68, instrument_error::published_errors::sample_cellcounting_configurationinvalid);	// error
	CreateCellHealthErrorCode (69, instrument_error::published_errors::sample_cellcounting_gpinvalid);			// error
	CreateCellHealthErrorCode (70, instrument_error::published_errors::sample_cellcounting_poiinvalid);			// error
	CreateCellHealthErrorCode (71, instrument_error::published_errors::sample_general_processingerror);			// warning
	CreateCellHealthErrorCode (72, instrument_error::published_errors::sample_cellcounting_bubblewarning);		// warning (LH6531-5300)
	CreateCellHealthErrorCode (73, instrument_error::published_errors::sample_cellcounting_clusterwarning);		// warning (LH6531-5301)
	CreateCellHealthErrorCode (74, instrument_error::published_errors::sample_cellcounting_imagedropwarning);	// warning (LH6531-5302)
	CreateCellHealthErrorCode (75, instrument_error::published_errors::imaging_general_backgroundadjusthighpower);			// warning (LH6531-5303)	


	// Keep for reference.
	//CreateCellHealthErrorCode (instrument_error::published_errors::instrument_configuration_notpresent);		// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::instrument_storage_noconnection);			// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::instrument_storage_backuprestorefailed);	// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::instrument_integrity_verifyonread);		// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::reagent_rfid_hardwareerror);				// error	
	//CreateCellHealthErrorCode (instrument_error::published_errors::reagent_rfid_rfiderror);					// error	
	//CreateCellHealthErrorCode (instrument_error::published_errors::reagent_bayhw_sensorerror);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::reagent_bayhw_latcherror);					// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_notcalibrated);			// error
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_calfailure);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_initerror);				// error
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_positionfail);			// error
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_ejectfail);				// warning, error
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_homefail);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_sampledeck_tubedetected);			// warning
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_flrack_notcalibrated);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_flrack_calfailure);					// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_flrack_initerror);					// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_flrack_positionfail);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::motion_motor_thermal);						// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::fluidics_general_primingfail);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::imaging_trigger_timeout);					// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::imaging_led_timeout);						// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::imaging_omicron_internal_comm_error);		// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::imaging_photodiode_hardwareerror);			// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::imaging_photodiode_timeout);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::sample_cellcounting_invalidtype);			// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::sample_cellcounting_initerror);			// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::sample_general_invalidtype);				// not used
	//CreateCellHealthErrorCode (instrument_error::published_errors::sample_general_timeout);					// not used

	// Keep for debugging...
	//std::string output, sev, sys, subsys, inst, fm;
	//SystemErrorCodeToExpandedStrings_internal(instrument_error::published_errors::instrument_configuration_failedvalidation,
	//	sev, sys, subsys, inst, fm);
}
