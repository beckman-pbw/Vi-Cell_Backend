#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "HawkeyeAssert.hpp"
#include "SystemErrorFeeder.hpp"
#include "SystemStatus.hpp"

namespace instrument_error
{
	template <typename ENUM>
	static constexpr uint32_t ErrcodeBitshift(ENUM val, uint8_t shift_val);
	enum published_errors : uint32_t;
}

template <typename INSTANCE>
static constexpr uint32_t BuildErrorInstance (instrument_error::published_errors errc, INSTANCE instance, instrument_error::severity_level sev)
{
	return static_cast<uint32_t>(errc) | instrument_error::ErrcodeBitshift(sev, 30) | (static_cast<uint8_t>(instance) << 8);
}

static constexpr uint32_t BuildErrorInstance (instrument_error::published_errors errc, instrument_error::severity_level sev)
{
	return BuildErrorInstance(errc, (uint8_t)0, sev);
}

class ReportSystemError
{
public:
	~ReportSystemError();

	static ReportSystemError& Instance()
	{
		static ReportSystemError instance;
		return instance;
	}
		
	void ReportError(uint32_t errorCode, bool logDescription = true);
	void ReportUserError(uint32_t errorCode, const char* username, bool logDescription = true);
	uint16_t GetErrorCount(void);
	uint32_t GetErrorCode(uint16_t errorIndex);
	std::vector<uint32_t> GetErrorCodes(void);
	void ClearAllErrors(void);
	void ClearErrorCode(uint32_t errorCode);

	static void DecodeAndLogErrorCode(uint32_t errorCode);

private:
	std::vector<uint32_t> _errorCodes;
	std::mutex _errorCodesMutex;
	SystemStatus& _systemStatus;

	ReportSystemError();
};


namespace instrument_error
{
	template <typename ENUM, typename UNDERLYING>
	static constexpr void AssertEnumUnderlyingType()
	{
		static_assert(std::is_same<std::underlying_type_t<ENUM>, UNDERLYING>::value, "Invalid error enum type");
	}

	template <typename ENUM>
	static constexpr uint32_t ErrcodeBitshift(ENUM val, uint8_t shift_val)
	{
		AssertEnumUnderlyingType<ENUM, uint8_t>();
		return static_cast<uint8_t>(val) << shift_val;
	}

	template <typename SYS, typename SUBSYS, typename FM>
	static constexpr uint32_t BuildErrorCode(SYS sys, SUBSYS subsys, FM fm)
	{
		// By adding another template function with only explicit specializations, the value
		// of `sys` // could be inferred from the type of `subsys`.
		return ErrcodeBitshift(sys, 24) | ErrcodeBitshift(subsys, 16) | ErrcodeBitshift(fm, 0);
	}

	enum published_errors : uint32_t
	{
		instrument_configuration_notpresent = BuildErrorCode(systems::instrument, instr_subsystems::configuration, instrument_configuration_failures::not_present),
		instrument_configuration_failedvalidation = BuildErrorCode(systems::instrument, instr_subsystems::configuration, instrument_configuration_failures::failed_validation),

		instrument_storage_noconnection = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::no_connection),
		instrument_storage_filenotfound = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::file_not_found),
		instrument_storage_backuprestorefailed = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::backup_restore_failed),
		instrument_storage_readerror = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::read_error),
		instrument_storage_writeerror = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::write_error),
		instrument_storage_deleteerror = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::delete_error),
		instrument_storage_storagenearcapacity = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::storage_near_capacity),
		instrument_storage_notallowed = BuildErrorCode(systems::instrument, instr_subsystems::storage, instrument_storage_failures::not_allowed),

		instrument_integrity_verifyonread = BuildErrorCode(systems::instrument, instr_subsystems::integrity, instrument_integrity_failures::verification_on_read),
		instrument_integrity_softwarefault = BuildErrorCode(systems::instrument, instr_subsystems::integrity, instrument_integrity_failures::software_fault),
		instrument_integrity_hardwarefault = BuildErrorCode(systems::instrument, instr_subsystems::integrity, instrument_integrity_failures::hardware_fault),

		instrument_precondition_notmet = BuildErrorCode(systems::instrument, instr_subsystems::precondition, instrument_precondition_failures::not_met),

		controller_general_connectionerror = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::connection_error),
		controller_general_hostcommerror = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::host_comm_error),
		controller_general_firmwareupdate = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::firmware_update_error),
		controller_general_firmwarebootup = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::firmware_bootup_error),
		controller_general_firmwareinvalid = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::firmware_invalid_version),
		controller_general_interface = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::firmware_interface_error),
		controller_general_fw_operation = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::firmware_operation),
		controller_general_hardware_health = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::hardware_health),
		controller_general_eepromreaderror  = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::eeprom_read_error),
		controller_general_eepromwriteerror = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::eeprom_write_error),
		controller_general_eepromeraseerror = BuildErrorCode(systems::controller_board, cntrlr_subsystems::general, cntrlr_general_failures::eeprom_erase_error),

		reagent_rfid_hardwareerror = BuildErrorCode(systems::reagents, reagents_subsystems::rfid_hw, reagent_rfid_hw_failures::hardware_error),
		reagent_rfid_rfiderror = BuildErrorCode(systems::reagents, reagents_subsystems::rfid_hw, reagent_rfid_hw_failures::rfid_error),

		reagent_bayhw_sensorerror = BuildErrorCode(systems::reagents, reagents_subsystems::bay_hw, reagent_bay_hw_failures::sensor_error),
		reagent_bayhw_latcherror = BuildErrorCode(systems::reagents, reagents_subsystems::bay_hw, reagent_bay_hw_failures::latch_error),
		
		reagent_pack_invalid = BuildErrorCode(systems::reagents, reagents_subsystems::reagent_pack, reagent_pack_failures::pack_invalid),
		reagent_pack_expired = BuildErrorCode(systems::reagents, reagents_subsystems::reagent_pack, reagent_pack_failures::pack_expired),
		reagent_pack_empty = BuildErrorCode(systems::reagents, reagents_subsystems::reagent_pack, reagent_pack_failures::pack_empty),
		reagent_pack_nopack = BuildErrorCode(systems::reagents, reagents_subsystems::reagent_pack, reagent_pack_failures::no_pack),
		reagent_pack_updatefail = BuildErrorCode(systems::reagents, reagents_subsystems::reagent_pack, reagent_pack_failures::update_fail),
		reagent_pack_loaderror = BuildErrorCode(systems::reagents, reagents_subsystems::reagent_pack, reagent_pack_failures::load_error),
		
		motion_sampledeck_notcalibrated = BuildErrorCode(systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::not_calibrated),
		motion_sampledeck_calfailure = BuildErrorCode(systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::calibration_failure),
		motion_sampledeck_initerror = BuildErrorCode(systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::init_error),
		motion_sampledeck_positionfail = BuildErrorCode (systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::position_fail),
		motion_sampledeck_ejectfail = BuildErrorCode(systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::eject_fail),
		motion_sampledeck_homefail = BuildErrorCode(systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::home_fail),
		motion_sampledeck_tubedetected = BuildErrorCode(systems::motion, motion_subsystems::sample_deck, motion_sample_deck_failures::tube_detected),
		
		motion_flrack_notcalibrated = BuildErrorCode(systems::motion, motion_subsystems::fl_rack, motion_fl_rack_failures::not_calibrated),
		motion_flrack_calfailure = BuildErrorCode(systems::motion, motion_subsystems::fl_rack, motion_fl_rack_failures::calibration_failure),
		motion_flrack_initerror = BuildErrorCode(systems::motion, motion_subsystems::fl_rack, motion_fl_rack_failures::init_error),
		motion_flrack_positionfail = BuildErrorCode(systems::motion, motion_subsystems::fl_rack, motion_fl_rack_failures::position_fail),
		
		motion_motor_initerror = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::init_error),
		motion_motor_timeout = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::timeout),
		motion_motor_homefail = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::homing_fail),
		motion_motor_positionfail = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::position_fail),
		motion_motor_holdingcurrentfail = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::holdingcurrent_fail),
		motion_motor_drivererror = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::driver_error),
		motion_motor_operationlogic = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::operation_logic),
		motion_motor_thermal = BuildErrorCode(systems::motion, motion_subsystems::motor, motion_motor_failures::thermal),

		fluidics_general_primingfail = BuildErrorCode(systems::fluidics, fluidics_subsystems::general, fluidics_general_failures::priming_failure),
		fluidics_general_nightlyclean = BuildErrorCode(systems::fluidics, fluidics_subsystems::general, fluidics_general_failures::nightly_clean_skipped),

		fluidics_syringepump_initerror = BuildErrorCode(systems::fluidics, fluidics_subsystems::syringepump, fluidics_syringepump_failures::init_error),
		fluidics_syringepump_hardwareerror = BuildErrorCode(systems::fluidics, fluidics_subsystems::syringepump, fluidics_syringepump_failures::hardware_error),
		fluidics_syringepump_commserror = BuildErrorCode(systems::fluidics, fluidics_subsystems::syringepump, fluidics_syringepump_failures::communication_error),
		fluidics_syringepump_overpressure = BuildErrorCode(systems::fluidics, fluidics_subsystems::syringepump, fluidics_syringepump_failures::over_pressure_error),

		imaging_general_timeout = BuildErrorCode(systems::imaging, imaging_subsystems::general, imaging_general_failures::operation_timeout),
		imaging_general_logicerror = BuildErrorCode(systems::imaging, imaging_subsystems::general, imaging_general_failures::logic_error),
		imaging_general_imagequality = BuildErrorCode(systems::imaging, imaging_subsystems::general, imaging_general_failures::image_quality),
		imaging_general_backgroundadjust = BuildErrorCode(systems::imaging, imaging_subsystems::general, imaging_general_failures::background_adjust_fail),
		imaging_general_backgroundadjusthighpower = BuildErrorCode(systems::imaging, imaging_subsystems::general, imaging_general_failures::background_adjust_nearmax),
		
		imaging_camera_hardwareerror = BuildErrorCode(systems::imaging, imaging_subsystems::camera, imaging_camera_failures::hardware_error),
		imaging_camera_timeout = BuildErrorCode(systems::imaging, imaging_subsystems::camera, imaging_camera_failures::operation_timeout),
		imaging_camera_connection = BuildErrorCode(systems::imaging, imaging_subsystems::camera, imaging_camera_failures::cannot_connect),
		imaging_camera_initerror = BuildErrorCode(systems::imaging, imaging_subsystems::camera, imaging_camera_failures::init_error),
		imaging_camera_noimage = BuildErrorCode(systems::imaging, imaging_subsystems::camera, imaging_camera_failures::no_image_captured),
		
		imaging_trigger_hardwareerror = BuildErrorCode(systems::imaging, imaging_subsystems::trigger, imaging_trigger_failures::hardware_error),
		imaging_trigger_timeout = BuildErrorCode(systems::imaging, imaging_subsystems::trigger, imaging_trigger_failures::operation_timeout),
		
		imaging_led_hardwareerror = BuildErrorCode(systems::imaging, imaging_subsystems::led, imaging_led_failures::hardware_error),
		imaging_led_timeout = BuildErrorCode(systems::imaging, imaging_subsystems::led, imaging_led_failures::operation_timeout),
		imaging_led_initerror = BuildErrorCode(systems::imaging, imaging_subsystems::led, imaging_led_failures::init_error),
		imaging_led_powerthreshold = BuildErrorCode(systems::imaging, imaging_subsystems::led, imaging_led_failures::power_threshold),
		imaging_led_commserror = BuildErrorCode(systems::imaging, imaging_subsystems::led, imaging_led_failures::comms_error),
		imaging_led_reset = BuildErrorCode(systems::imaging, imaging_subsystems::led, imaging_led_failures::reset),

		imaging_omicron_response_too_short = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::response_too_short),
		imaging_omicron_interlock = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::interlock),
		imaging_omicron_cdrh_error = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::cdrh_error),
		imaging_omicron_internal_comm_error = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::internal_comm_error),
		imaging_omicron_underover_voltage = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::underover_voltage),
		imaging_omicron_external_interlock = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::external_interlock),
		imaging_omicron_diode_current = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::diode_current),
		imaging_omicron_ambient_temp = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::ambient_temp),
		imaging_omicron_diode_temp = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::diode_temp),
		imaging_omicron_internal_error = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::internal_error),
		imaging_omicron_diode_power = BuildErrorCode(systems::imaging, imaging_subsystems::omicronled, imaging_led_failures::diode_power),

//TODO: TBD... BCILed errors

		imaging_photodiode_hardwareerror = BuildErrorCode(systems::imaging, imaging_subsystems::photodiode, imaging_photodiode_failures::hardware_error),
		imaging_photodiode_timeout = BuildErrorCode(systems::imaging, imaging_subsystems::photodiode, imaging_photodiode_failures::operation_timeout),

		sample_analysis_invalidtype = BuildErrorCode (systems::sample, sample_subsystems::analysis, sample_analysis_failures::unknown_type),		
		sample_celltype_invalidtype = BuildErrorCode (systems::sample, sample_subsystems::celltype, sample_celltype_failures::unknown_type),

		sample_cellcounting_invalidtype = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::unknown_type),
		sample_cellcounting_configurationinvalid = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::configuration_invalid),
		sample_cellcounting_gpinvalid = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::general_population_invalid),
		sample_cellcounting_poiinvalid = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::population_of_interest_invalid),
		sample_cellcounting_initerror = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::initerror),
		sample_cellcounting_bubblewarning = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::bubble_detected),
		sample_cellcounting_clusterwarning = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::clusters_detected),
		sample_cellcounting_imagedropwarning = BuildErrorCode(systems::sample, sample_subsystems::cellcounting, sample_cellcounting_failures::images_dropped),

		sample_general_invalidtype = BuildErrorCode (systems::sample, sample_subsystems::general, sample_general_failures::unknown_type),
		sample_general_timeout = BuildErrorCode(systems::sample, sample_subsystems::general, sample_general_failures::timeout),
		sample_general_processingerror = BuildErrorCode (systems::sample, sample_subsystems::general, sample_general_failures::processingerror),
	};

	void DecodeErrorInstance(uint32_t errorval, std::string& level, std::string& system, std::string& subsystem, std::string& instance, std::string& failure);
	inline severity_level GetErrorSeverity(uint32_t errorval) { return (severity_level)((errorval & 0xC0000000) >> 30); }
}
