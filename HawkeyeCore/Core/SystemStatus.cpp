#include "stdafx.h"

#include "CalibrationHistoryDLL.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "MotorStatusDLL.hpp"
#include "Registers.hpp"
#include "SystemErrors.hpp"
#include "SystemStatus.hpp"

#include "../../target/properties/HawkeyeCore/dllversion.hpp"

static const char MODULENAME[] = "SystemStatus";

// Jira Bug : PC3527-1806
#define MAX_SAMPLE_TUBE_DISPOSAL_CAPACITY 120

//*****************************************************************************
void SystemStatus::loadSystemStatusForSimulation (SystemStatusData& systemStatusData) {

	systemStatusData.status = eSystemStatus::eIdle;

	systemStatusData.is_standalone_mode = false;	//NOTE: currently, this is not used for anything.
	systemStatusData.system_total_sample_count = 0;

	systemStatusData.sensor_carousel_detect = ssStateActive;
	systemStatusData.sensor_carousel_tube_detect = ssStateActive;
	systemStatusData.sensor_reagent_pack_door_closed = ssStateActive;
	systemStatusData.sensor_reagent_pack_in_place = ssStateActive;

	systemStatusData.sensor_radiusmotor_home = ssStateActive;
	systemStatusData.sensor_thetamotor_home = ssStateActive;
	systemStatusData.sensor_probemotor_home = ssStateActive;
	systemStatusData.sensor_focusmotor_home = ssStateActive;
	systemStatusData.sensor_reagentmotor_upper = ssStateActive;
	systemStatusData.sensor_reagentmotor_lower = ssStateActive;
	systemStatusData.sensor_flopticsmotor1_home = ssStateActive;
	systemStatusData.sensor_flopticsmotor2_home = ssStateActive;

	systemStatusData.motor_Radius = { 0x0008, 0, "Ready" };
	systemStatusData.motor_Theta  = { 0x0008, 0, "Ready" };
	systemStatusData.motor_Probe  = { 0x0008, 0, "Ready" };
	systemStatusData.motor_Focus  = { 0x0008, 0, "Ready" };
	systemStatusData.motor_Reagent = { 0x0008, 0, "Ready" };
	systemStatusData.motor_FLRack1 = { 0x0010, 0, "Not Found" };
	systemStatusData.motor_FLRack2 = { 0x0010, 0, "Not Found" };

	systemStatusData.sampleStageLocation = {'Z', 1};

	systemStatusData.sample_tube_disposal_remaining_capacity = 120;

	systemStatusData.focus_IsFocused = true;
	systemStatusData.focus_DefinedFocusPosition = 56123;

	systemStatusData.active_error_count = 0;
	systemStatusData.active_error_codes = nullptr;

	systemStatusData.syringeValvePosition =3;
	systemStatusData.syringePosition = 300;

	systemStatusData.brightfieldLedPercentPower = 13;

	systemStatusData.voltage_neg_3V = -2.98f;
	systemStatusData.voltage_3_3V = 3.32f;
	systemStatusData.voltage_5V_Sensor = 5.10f;
	systemStatusData.voltage_5V_Circuit = 5.11f;
	systemStatusData.voltage_12V = 12.12f;
	systemStatusData.voltage_24V = 24.10f;

	systemStatusData.temperature_ControllerBoard = 25.1f;
	systemStatusData.temperature_CPU = 26.4f;
	systemStatusData.temperature_OpticalCase = 24.7f;
}

//*****************************************************************************
bool SystemStatus::initialize() {

	Logger::L().Log (MODULENAME, severity_level::debug2, "initialize: <enter>");

	strcpy_s (systemVersion.software_version, sizeof(systemVersion.software_version), DLL_Version);

	if (!HawkeyeConfig::Instance().get().hardwareConfig.controllerBoard) {
		loadSystemStatusForSimulation (systemStatusData_);
	}

	strcpy_s (systemVersion.software_version, sizeof(systemVersion.software_version), DLL_Version);

	if (HawkeyeConfig::Instance().get().hardwareConfig.discardTray)
	{
		// update sample tube disposal remaining capacity count
		auto currentCapacity = HawkeyeConfig::Instance().get().discardTrayCapacity;
		if (currentCapacity > MAX_SAMPLE_TUBE_DISPOSAL_CAPACITY) {
			Logger::L().Log (MODULENAME, severity_level::warning, boost::str(boost::format("initialize : current sample tray capacity (%d) is more than maximum sample tube disposal capacity (%d) !!!") % currentCapacity % MAX_SAMPLE_TUBE_DISPOSAL_CAPACITY));
			currentCapacity = MAX_SAMPLE_TUBE_DISPOSAL_CAPACITY;
			HawkeyeConfig::Instance().get().discardTrayCapacity = currentCapacity; // update the configuration data with max capacity 
																		 //TODO: set an error code...
		}
		systemStatusData_.sample_tube_disposal_remaining_capacity = currentCapacity;
	}

	// Get the total sample count.
	systemStatusData_.system_total_sample_count = HawkeyeConfig::Instance().get().totalSamplesProcessed;

	Logger::L().Log (MODULENAME, severity_level::debug2, "initialize: <exit>");

	return true;
}

//*****************************************************************************
void SystemStatus::free (SystemStatusData* status) {

	if (status == nullptr) {
		return;
	}

	if (status->active_error_codes != nullptr)
	{
		delete[] status->active_error_codes;
		status->active_error_codes = nullptr;
	}
	delete status;
}

//*****************************************************************************
SystemStatusData& SystemStatus::getData() {

	return systemStatusData_;
}

//*****************************************************************************
void SystemStatus::ToCStyle (SystemStatusData*& systemStatusData) {

	std::lock_guard<std::mutex> guard(systemStatusMutex_);

	systemStatusData = nullptr;

	// Clear the previous error codes
	if (systemStatusData_.active_error_codes != nullptr) {
		delete[]systemStatusData_.active_error_codes;
	}

	systemStatusData_.active_error_codes = nullptr;

	systemStatusData_.active_error_count = ReportSystemError::Instance().GetErrorCount();

	if (systemStatusData_.active_error_count != 0) {
	
		// Set the system health based on the severity of the errors.
		systemStatusData_.active_error_codes = new uint32_t[systemStatusData_.active_error_count];
		std::vector<uint32_t> errorCodes = ReportSystemError::Instance().GetErrorCodes();

		for (uint16_t index = 0; index < systemStatusData_.active_error_count; index++) {
			systemStatusData_.active_error_codes[index] = errorCodes.at(index);
			if (instrument_error::GetErrorSeverity(systemStatusData_.active_error_codes[index]) == instrument_error::severity_level::error)
			{
				systemStatusData_.status = eSystemStatus::eFaulted;
			}
		}
	}

	CalibrationHistoryDLL::Instance().GetLastCalibratedDates (
		systemStatusData_.last_calibrated_date_concentration,
		systemStatusData_.last_calibrated_date_size,
		systemStatusData_.last_calibrated_date_acup_concentration);

	systemStatusData = new SystemStatusData;
	*systemStatusData = systemStatusData_;

	systemStatusData->active_error_codes = nullptr;
	systemStatusData->active_error_count = systemStatusData_.active_error_count;
	if (systemStatusData->active_error_count != 0) {
		systemStatusData->active_error_codes = new uint32_t[systemStatusData->active_error_count];
		for (uint16_t index = 0; index < systemStatusData->active_error_count; index++) {
			systemStatusData->active_error_codes[index] = systemStatusData_.active_error_codes[index];
		}
	}


	//NOTE: save for future debugging.
	//if (Logger::L().IsOfInterest(severity_level::debug1)) {

	//	std::string format = "\nHealth: %d\nStandAloneMode: %d\nTotalSampleCount: %d\nStageLocation: %c-%d\nDisposalTrayCapacity: %d\nIsFocused: %d\n";
	//	format += "FocusPosition: %d\nSyringeValvePosition: %d\nSyringePosition: %d\nBrightfieldPower: %f\nNeg_3V: %f\n";
	//	format += "3.3V: %f\n5VSensor: %f\n5VCircuit: %f\n12V: %f\n24V: %f\nCBTemp: %f\nCPUTemp: %f\nOCTemp: %f\nLastSizeCalDate: %d\nLastConcCalDate: %f\nNightCleanCycle: %d";

	//	std::string str =
	//		boost::str (boost::format (format)
	//			% (int)systemStatusData_.health
	//			% (int) systemStatusData_.is_standalone_mode
	//			% systemStatusData_.system_total_sample_count
	//			% systemStatusData_.sampleStageLocation.row
	//			% (int)systemStatusData_.sampleStageLocation.col
	//			% systemStatusData_.sample_tube_disposal_remaining_capacity
	//			% (int)systemStatusData_.focus_IsFocused
	//			% systemStatusData_.focus_DefinedFocusPosition
	//			% systemStatusData_.syringeValvePosition
	//			% systemStatusData_.syringePosition
	//			% systemStatusData_.brightfieldLedPercentPower
	//			% systemStatusData_.voltage_neg_3V
	//			% systemStatusData_.voltage_3_3V
	//			% systemStatusData_.voltage_5V_Sensor
	//			% systemStatusData_.voltage_5V_Circuit
	//			% systemStatusData_.voltage_12V
	//			% systemStatusData_.voltage_24V
	//			% systemStatusData_.temperature_ControllerBoard
	//			% systemStatusData_.temperature_CPU
	//			% systemStatusData_.temperature_OpticalCase
	//			% systemStatusData_.last_calibrated_date_size
	//			% systemStatusData_.last_calibrated_date_concentration
	//			% (int)systemStatusData_.nightly_clean_cycle
	//	);
	//
	//	Logger::L().Log (MODULENAME, severity_level::debug1, str);

	//	Logger::L().Log (MODULENAME, severity_level::debug1, "\n# Active Errors: " + std::to_string(systemStatusData_.active_error_count));

	//}

	///* SENSOR STATES */
	//eSensorStatus sensor_carousel_detect;
	//eSensorStatus sensor_carousel_tube_detect;
	//eSensorStatus sensor_reagent_pack_door_closed;
	//eSensorStatus sensor_reagent_pack_in_place;

	///* SENSOR STATES - MOTORS */
	//eSensorStatus sensor_radiusmotor_home;
	//eSensorStatus sensor_thetamotor_home;
	//eSensorStatus sensor_probemotor_home;
	//eSensorStatus sensor_focusmotor_home;
	//eSensorStatus sensor_reagentmotor_upper;
	//eSensorStatus sensor_reagentmotor_lower;
	//eSensorStatus sensor_flopticsmotor1_home;
	//eSensorStatus sensor_flopticsmotor2_home;

	///* MOTOR STATES */
	//MotorStatus motor_Radius;
	//MotorStatus motor_Theta;
	//MotorStatus motor_Probe;
	//MotorStatus motor_Focus;
	//MotorStatus motor_Reagent;
	//MotorStatus motor_FLRack1;
	//MotorStatus motor_FLRack2;
}

//*****************************************************************************
bool SystemStatus::decrementSampleTubeCapacityCount()
{
	SamplePositionDLL tPos (systemStatusData_.sampleStageLocation);
	if (HawkeyeConfig::Instance().get().automationEnabled && tPos.isValidForACup())
	{
		return true;
	}

	auto capacity = systemStatusData_.sample_tube_disposal_remaining_capacity;

//TODO: add a warning case (@ 10 left)...

	if (capacity == 0) {
		Logger::L().Log (MODULENAME, severity_level::error, "decrementSampleTubeCapacityCount: <exit, sample tube tray is full>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::wastetubetray_capacity, 
			instrument_error::severity_level::warning));
		return false;
	}

	capacity--;
	Logger::L().Log (MODULENAME, severity_level::debug1, "decrementSampleTubeCapacityCount: <remaining Sample Tube Discard Tray Capacity:" + std::to_string(capacity));

	HawkeyeConfig::Instance().get().discardTrayCapacity = capacity;
	systemStatusData_.sample_tube_disposal_remaining_capacity = capacity;

	return true;
}

//*****************************************************************************
bool SystemStatus::incrementSampleProcessingCount(
	eCarrierType carrierType,
	uint32_t& totalSamplesProcessed) {

	uint32_t total_sample_processed = HawkeyeConfig::Instance().get().totalSamplesProcessed;
	total_sample_processed++;

	bool status = true;
	if (carrierType == eCarrierType::eCarousel) {
		status = decrementSampleTubeCapacityCount();
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "incrementSampleProcessingCount: <total number of samples processed: " + std::to_string(total_sample_processed));

	totalSamplesProcessed = total_sample_processed;

	HawkeyeConfig::Instance().get().totalSamplesProcessed = total_sample_processed;
	systemStatusData_.system_total_sample_count = total_sample_processed;

	return status;
}

//*****************************************************************************
void SystemStatus::sampleTubeDiscardTrayEmptied() {

	HawkeyeConfig::Instance().get().discardTrayCapacity = MAX_SAMPLE_TUBE_DISPOSAL_CAPACITY;
	systemStatusData_.sample_tube_disposal_remaining_capacity = MAX_SAMPLE_TUBE_DISPOSAL_CAPACITY;
}
