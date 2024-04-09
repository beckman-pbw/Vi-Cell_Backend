#pragma once

#include <cstdint>

#include "MotorStatus.hpp"
#include "SamplePosition.hpp"

struct SystemVersion
{
	char software_version[20];
	char img_analysis_version[20];
	char controller_firmware_version[20];
	char camera_firmware_version[20];
	char syringepump_firmware_version[20];
	char system_serial_number[20];
};

enum class eSystemStatus : uint16_t
{
	eIdle = 0,
	eProcessingSample,
	ePausing,
	ePaused,
	eStopping,
	eStopped,
	eFaulted,
	eSearchingTube, // Applicable only for Carousel
};

enum eSensorStatus : uint16_t
{
	ssStateUnknown = 0,
	ssStateActive,
	ssStateInactive,
};

enum eNightlyCleanStatus : uint16_t
{
	ncsIdle = 0,
	ncsInProgress,
	ncsUnableToPerform,
	ncsACupInProgress,
	ncsACupUnableToPerform,
};

struct SystemStatusData
{
	eSystemStatus status;

	// Placeholder for the "desktop" version of the code that's coming eventually for reanalysis, etc. More or less the same as simulated mode.
	bool is_standalone_mode;

	// Total since the beginning of time.
	uint32_t system_total_sample_count;

	/* SENSOR STATES */
	eSensorStatus sensor_carousel_detect;
	eSensorStatus sensor_carousel_tube_detect;
	eSensorStatus sensor_reagent_pack_door_closed;
	eSensorStatus sensor_reagent_pack_in_place;

	/* SENSOR STATES - MOTORS */
	eSensorStatus sensor_radiusmotor_home;
	eSensorStatus sensor_thetamotor_home;
	eSensorStatus sensor_probemotor_home;
	eSensorStatus sensor_focusmotor_home;
	eSensorStatus sensor_reagentmotor_upper;
	eSensorStatus sensor_reagentmotor_lower;
	eSensorStatus sensor_flopticsmotor1_home;
	eSensorStatus sensor_flopticsmotor2_home;

	/* MOTOR STATES */
	MotorStatus motor_Radius;
	MotorStatus motor_Theta;
	MotorStatus motor_Probe;
	MotorStatus motor_Focus;
	MotorStatus motor_Reagent;
	MotorStatus motor_FLRack1;
	MotorStatus motor_FLRack2;

	/* SAMPLE STAGE - may not always be a valid location */
	SamplePosition sampleStageLocation;

	/* Carousel events remaining until tray should be emptied.
	 * This is a HARD CAP.  Because of the risk of a mechanical
	 * jam, the system will refuse to process Carousel samples
	 * until the sample tray has been emptied and the API "SampleTubeDiscardTrayEmptied()"
	 * function has been called.
	 */
	uint16_t sample_tube_disposal_remaining_capacity;

	uint16_t remainingReagentPackUses;

	/* FOCUS SYSTEM */
	bool focus_IsFocused;
	int32_t focus_DefinedFocusPosition;

	/* ACTIVE ERROR STATES */
	uint16_t active_error_count;
	uint32_t* active_error_codes;

	/* Syringe */
	uint16_t syringeValvePosition;
	uint32_t syringePosition;

	/* Camera */
	float brightfieldLedPercentPower;

	/* Voltage in Volts*/
	float voltage_neg_3V;
	float voltage_3_3V;
	float voltage_5V_Sensor;
	float voltage_5V_Circuit;
	float voltage_12V;
	float voltage_24V;

	/* Temperature in Degree Celsius*/	
	float temperature_ControllerBoard;
	float temperature_CPU;
	float temperature_OpticalCase;

	/* Calibrations - days since Epoch (1 January 1970) */
	uint64_t last_calibrated_date_size;
	uint64_t last_calibrated_date_concentration;
	uint64_t last_calibrated_date_acup_concentration;

	/* Nightly clean cycle status*/
	eNightlyCleanStatus nightly_clean_cycle;

	int16_t instrumentType;
};
