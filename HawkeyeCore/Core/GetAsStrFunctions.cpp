#include "stdafx.h"

#include <string>

#include "AuditLog.hpp"
#include "EnumConversion.hpp"
#include "GetAsStrFunctions.hpp"
#include "MotorStatusDLL.hpp"
#include "SampleDefinition.hpp"

static const char MODULENAME[] = "GetAsStrFunctions";

//*****************************************************************************
std::string getAuditEventTypeAsStr (audit_event_type eType)
{
	return EnumConversion<audit_event_type>::enumToString (eType);
}

//*****************************************************************************
const char* getCalibrationStateAsStr (CalibrationState cs)
{
	switch (cs)
	{
		case CalibrationState::eIdle:
			return "Idle State";
		case CalibrationState::eApplyDefaults:
			return "Applying Default parameters";
		case CalibrationState::eHomingRadiusTheta:
			return "Homing Radius And Theta Motor";
		case CalibrationState::eWaitingForRadiusThetaPos:
			return  "Waiting For Radius/Theta Position";
		case CalibrationState::eCalibrateRadiusTheta:
			return "Adjusting Radius and Theta Motor Positions";
		case CalibrationState::eWaitingForFinish:
			return  "Waiting For Finish";
		case CalibrationState::eInitialize:
			return "Initializing the Motor with new radius and Theta Positions";
		case CalibrationState::eCompleted:
			return "Adjustment Completed";
		case CalibrationState::eFault:
			return "HardwareFault";
		default:
			return "Unknown Error";
	}
}

//*****************************************************************************
const char* getErrorAsStr (HawkeyeError he) {

	return HawkeyeErrorAsString (he);
}


//*****************************************************************************
const char* getMotorStatusAsStr (MotorStatus& motorStatus)
{
	return (MotorStatusDLL::getMotorStatusAsString (motorStatus)).c_str();
}

//*****************************************************************************
const char* getPermissionLevelAsStr (UserPermissionLevel permission) {

	switch (permission) {
		case eNormal:
			return "Normal";
		case eElevated:
			return "Advanced"; //"Elevated"; User interface names "Elevated" as Advanced. 
		case eAdministrator:
			return "Administrator";
		case eService:
			return "Service";
		default:
			return "Unknown";
	}
}

//*****************************************************************************
std::string getReagentContainerStatusAsStr (ReagentContainerStatus eType)
{
	return EnumConversion<ReagentContainerStatus>::enumToString (eType);
}

//*****************************************************************************
const std::map<ReagentContainerStatus, std::string> EnumConversion<ReagentContainerStatus>::enumStrings<ReagentContainerStatus>::data =
{
	{ ReagentContainerStatus::eOK, std::string ("OK") },
	{ ReagentContainerStatus::eEmpty, std::string ("Empty") },
	{ ReagentContainerStatus::eNotDetected, std::string ("Not_Detected") },
	{ ReagentContainerStatus::eInvalid, std::string ("Invalid") },
	{ ReagentContainerStatus::eExpired, std::string ("Expired") },
	{ ReagentContainerStatus::eFaulted, std::string ("Faulted") },
	{ ReagentContainerStatus::eUnloading, std::string ("Unloading") },
	{ ReagentContainerStatus::eUnloaded, std::string ("Unloaded") },
	{ ReagentContainerStatus::eLoading, std::string ("Loading") }
};

//*****************************************************************************
const char* getReagentFlushFlowCellStatusAsStr (eFlushFlowCellState status)
{
	switch (status)
	{
		case ffc_Idle:
			return "Idle";
		case ffc_FlushingCleaner:
			return "Flushing Cleaner";
		case ffc_FlushingConditioningSolution:
			return "Flushing Conditioning Solution";
		case ffc_FlushingBuffer:
			return "Flushing Buffer";
		case ffc_FlushingAir:
			return "Flushing Air";
		case ffc_Completed:
			return "Completed";
		case ffc_Failed:
			return "Failed";
		default:
			return "Unknown";
	}
}

const char* getCleanFluidicsStatusAsStr(eFlushFlowCellState status)
{
	return getReagentFlushFlowCellStatusAsStr(status);
}

//*****************************************************************************
const char* getReagentPackDecontaminateFlowCellStatusAsStr (eDecontaminateFlowCellState status)
{
	switch (status)
	{
		case dfc_Idle:
			return "Idle";
		case dfc_AspiratingBleach:
			return "Aspirating Bleach";
		case dfc_Dispensing1:
			return "Dispensing 1";
		case dfc_DecontaminateDelay:
			return "Decontaminate Delay";
		case dfc_Dispensing2:
			return "Dispensing 2";
		case dfc_FlushingBuffer:
			return "Flushing Buffer";
		case dfc_FlushingAir:
			return "Flushing Air";
		case dfc_Completed:
			return "Completed";
		case dfc_Failed:
			return "Failed";
		case dfc_FindingTube:
			return "Finding tube";
		case dfc_TfCancelled:
			return "Cancelled";
		default:
			return "Unknown";
	}
}

//*****************************************************************************
const char* getReagentPackDrainStatusAsStr (eDrainReagentPackState status)
{
	switch (status)
	{
		case drp_Idle:
			return "Idle";
		case drp_DrainCleaner1:
			return "Drain Cleaner 1";
		case drp_DrainCleaner2:
			return "Drain Cleaner 2";
		case drp_DrainCleaner3:
			return "Drain Cleaner 3";
		case drp_DrainReagent1:
			return "Drain Reagent 1";
		case drp_DrainDiluent:
			return "Drain Diluent";
		case drp_Completed:
			return "Completed";
		case drp_Failed:
			return "Failed";
		default:
			return "Unknown";
	}
}

//*****************************************************************************
const char* getReagentPackLoadStatusAsStr (ReagentLoadSequence status) {

	switch (status) {
	case eLIdle:
		return "Load Idle";
	case eLWaitingForDoorLatch:
		return "Waiting For Door to Latch";
	case eLWaitingForReagentSensor:
		return "Waiting For Reagent Present Sensor";
	case eLIdentifyingReagentContainers:
		return "Identifying Reagent Containers";
	case eLInsertingProbes:
		return "Inserting Probes";
	case eLSynchronizingReagentData:
		return "Synchronizing Reagent RFID Data";
	case eLPrimingFluidLines:
		return "Priming Fluid Lines";
	case eLComplete:
		return "Load Complete";
	case eLFailure_DoorLatchTimeout:
		return "Door Latch Timeout";
	case eLFailure_ReagentSensorDetect:
		return "No Reagent Sensor";
	case eLFailure_NoReagentsDetected:
		return "No Reagents Detected";
	case eLFailure_NoWasteDetected:
		return "No Waste Detected";
	case eLFailure_ReagentInvalid:
		return "Reagent Invalid";
	case eLFailure_ReagentEmpty:
		return "Reagent Empty";
	case eLFailure_ReagentExpired:
		return "Reagent Expired";
	case eLFailure_InvalidContainerLocations:
		return "Invalid Container Locations";
	case eLFailure_ProbeInsert:
		return "Probe Insert";
	case eLFailure_Fluidics:
		return "Fluidics Failure";
	case eLFailure_StateMachineTimeout:
		return "Unload State Machine Timeout";
	default:
		return "Unload State Unknown";
	}
}

//*****************************************************************************
const char* getReagentPackUnloadStatusAsStr (ReagentUnloadSequence status) {

	switch (status)
	{
	case eULIdle:
		return "Unload Idle";
	case eULDraining1:
		return "Draining Bottle 1";
	case eULDraining2:
		return "Draining Bottle 2";
	case eULDraining3:
		return "Draining Bottle 3";
	case eULDraining4:
		return "Draining Bottle 4";
	case eULDraining5:
		return "Draining Bottle 5";
	case eULPurging1:
		return "Purging Fluid Line 1";
	case eULPurging2:
		return "Purging Fluid Line 2";
	case eULPurging3:
		return "Purging Fluid Line 3";
	case eULPurgingReagent1:
		return "Purging Reagent Line 1";
	case eULPurgingReagent2:
		return "Purging Reagent Line 2";
	case eULRetractingProbes:
		return "Retracting Probes";
	case eULUnlatchingDoor:
		return "Opening Door";
	case eULFailed_DrainPurge:
		return "Drain/Purge Failed";
	case eULComplete:
		return "Unload Complete";
	case eULFailed_ProbeRetract:
		return "Probe Retract Failed";
	case eULFailed_DoorUnlatch:
		return "Door Unlatch Failed";
	case eULFailure_StateMachineTimeout:
		return "State Machine Timeout";
	default:
		return "Unload Unknown State";
	}
}

//*****************************************************************************
const char* getSensorStatusAsStr (eSensorStatus status) {

	switch (status) {
		default:
		case ssStateUnknown:
			return "Unknown";
		case ssStateActive:
			return "Active";
		case ssStateInactive:
			return "Inactive";
	}
}

//*****************************************************************************
const char* getSystemStatusAsStr (eSystemStatus sysStatus)
{
	switch (sysStatus) {
		case eSystemStatus::eIdle:
			return "Idle";
		case eSystemStatus::eSearchingTube:
			return "SearchingTube";
		case eSystemStatus::eProcessingSample:
			return "ProcessingSample";
		case eSystemStatus::ePausing:
			return "Pausing";
		case eSystemStatus::ePaused:
			return "Paused";
		case eSystemStatus::eStopping:
			return "Stopping";
		case eSystemStatus::eStopped:
			return "Stopped";
		case eSystemStatus::eFaulted:
			return "Faulted";
		default:
			return "Unknown";
	}
}

//*****************************************************************************
const char* getCarrierTypeAsStr (eCarrierType carrierType)
{
	switch (carrierType) {
		default:
		case eCarrierType::eUnknown:
			return "Unknown";
		case eCarrierType::eCarousel:
			return "Carousel";
		case eCarrierType::ePlate_96:
			return "Plate_96";
		case eCarrierType::eACup:
			return "ACup";
	}
}

//*****************************************************************************
const char* getSampleStatusAsStr (eSampleStatus status)
{
	switch (status) {
		case eNotProcessed:
			return "NotProcessed";
		case eInProcess_Aspirating:
			return "Aspirating";
		case eInProcess_Mixing:
			return "Mixing";
		case eInProcess_ImageAcquisition:
			return "ImageAcquisition";
		case eInProcess_Cleaning:
			return "Cleaning";
		case eAcquisition_Complete:
			return "Acquisition_Complete";
		case eCompleted:
			return "Completed";
		case eSkip_Manual:
			return "Skip_Manual";
		case eSkip_Error:
			return "Skip_Error";
		case eSkip_NotProcessed:
			return "Skip_NotProcessed";
		default:
			return "UmknownSampleStatus";
	}
}
