#pragma once

#include <cstdint>

namespace instrument_error
{

	/*
	*
	*         ___ _____ _____ _____ _   _ _____ _____ _____ _   _
	*        / _ \_   _|_   _|  ___| \ | |_   _|_   _|  _  | \ | |
	*       / /_\ \| |   | | | |__ |  \| | | |   | | | | | |  \| |
	*       |  _  || |   | | |  __|| . ` | | |   | | | | | | . ` |
	*       | | | || |   | | | |___| |\  | | |  _| |_\ \_/ / |\  |
	*       \_| |_/\_/   \_/ \____/\_| \_/ \_/  \___/ \___/\_| \_/
	*
	*
	*   Do not make changes to the enumerations in this namespace without making corresponding
	*     changes to Documentation/SystemErrorCode_ResourceList.md
	*
	*   Changing (alterations, additions, subtractions) the enums in this namespace will
	*     likely result in the backend generating resource keys that the user interface does
	*     not have in its resource file table.
	*
	*   Changes here MUST be reflected in the documentation and MUST be communicated to the UI
	*     developers for the next release.
	*
	*		  ___ _____ _____ _____ _   _ _____ _____ _____ _   _
	*        / _ \_   _|_   _|  ___| \ | |_   _|_   _|  _  | \ | |
	*       / /_\ \| |   | | | |__ |  \| | | |   | | | | | |  \| |
	*       |  _  || |   | | |  __|| . ` | | |   | | | | | | . ` |
	*       | | | || |   | | | |___| |\  | | |  _| |_\ \_/ / |\  |
	*       \_| |_/\_/   \_/ \____/\_| \_/ \_/  \___/ \___/\_| \_/
	*
	*/

	constexpr uint32_t ERR_SEVERITY_MASK  = 0xC0000000;
	constexpr uint32_t ERR_SYSTEM_MASK    = 0x3F000000;
	constexpr uint32_t ERR_SUBSYSTEM_MASK = 0x00FF0000;
	constexpr uint32_t ERR_INSTANCE_MASK  = 0x0000FF00;
	constexpr uint32_t ERR_FAILURE_MASK   = 0x000000FF;

	inline uint8_t getSeverity_8 (uint32_t errorVal)
	{
		return (errorVal & ERR_SEVERITY_MASK) >> 30;
	}
	inline uint8_t getSystem_8 (uint32_t errorVal)
	{
		return (errorVal & ERR_SYSTEM_MASK) >> 24;
	}
	inline uint8_t getSubsystem_8 (uint32_t errorVal)
	{
		return (errorVal & ERR_SUBSYSTEM_MASK) >> 16;
	}
	inline uint8_t getInstance_8 (uint32_t errorVal)
	{
		return (errorVal & ERR_INSTANCE_MASK) >> 8;
	}
	inline uint8_t getFailure_8 (uint32_t errorVal)
	{
		return (errorVal & ERR_FAILURE_MASK) >> 0;
	}
	
	enum struct severity_level : uint8_t
	{
		notification = 0x01,
		warning = 0x02,
		error = 0x03,
	};

	enum struct systems : uint8_t
	{
		instrument = 0x01,
		controller_board = 0x02,
		reagents = 0x03,
		motion = 0x04,
		fluidics = 0x05,
		imaging = 0x06,
		sample = 0x07,
	};


	/********************************************************************************/
	enum struct instr_subsystems : uint8_t
	{
		general = 0x01,
		configuration = 0x02,
		storage = 0x03,
		integrity = 0x04,
		precondition = 0x05,
	};

	enum struct instrument_general_instance : uint8_t
	{
		none = 0x00,
	};

	enum struct instrument_general_failures : uint8_t
	{

	};

	enum struct instrument_configuration_instance : uint8_t
	{
		none = 0x00,
		signature_definitions = 0x01,
		userlist = 0x02,
		auditlog = 0x03,
		errorlog = 0x04,
		samplelog = 0x05,
		hawkeyelog = 0x06,
		cameralog  = 0x07,
		storagelog = 0x08
	};
	enum struct instrument_configuration_failures : uint8_t
	{
		not_present = 0x01,
		failed_validation = 0x02,
	};

	enum struct instrument_storage_instance : uint8_t
	{
		none = 0x00,
		analysis = 0x01,
		audit = 0x02,
		bioprocess = 0x03,
		calibration = 0x04,
		celltype = 0x05,
		instrument_configuration = 0x06,
		error = 0x07,
		qualitycontrol = 0x08,
		reagentpack = 0x09,
		sample = 0x0A,
		signatures = 0x0B,
		systemstatus = 0x0C,
		userlist = 0x0D,
		reagent = 0x0E,
		result = 0x0F,
		configStatic = 0x10,
		configDynamic = 0x11,
		motor_info = 0x12,
		motor_config = 0x13,
		syringe_config = 0x14,
		focus_config_simulator = 0x15,
		dust_image = 0x16,
		debug_image = 0x17,
		workflow_script = 0x018,
		directory = 0x19,
		workflow_design = 0x1A,
		sample_set = 0x1B,
		illuminator = 0x1C,
		analysis_definition = 0x1D,
		celltype_definition = 0x1E,
		sample_item = 0x1F,
	};
	enum struct instrument_storage_failures : uint8_t
	{
		no_connection = 0x01,
		file_not_found = 0x02,
		backup_restore_failed = 0x03,
		read_error = 0x04,
		write_error = 0x05,
		storage_near_capacity = 0x06,
		not_allowed = 0x07,
		delete_error = 0x08,
	};

	enum struct instrument_integrity_instance : uint8_t
	{
		none = 0x00,
	};
	enum struct instrument_integrity_failures : uint8_t
	{
		verification_on_read = 0x01,  // WHY IS THIS HERE? WHAT IS EXPECTED USE? IS IT SPECIFIC ENOUGH?
		software_fault = 0x02,
		hardware_fault = 0x03,
	};

	enum struct instrument_precondition_instance : uint8_t
	{
		focus = 0x01,
		plate_registration = 0x02,
		carousel_registration = 0x03,
		concentration_config = 0x04,
		size_config = 0x05,
		plate_present = 0x06,
		carousel_present = 0x07,
		reagent_door = 0x08,
		wastetubetray_capacity = 0x09,
		instrument_serialnumber = 0x0A,
		zip_utility_installed = 0x0B,
		database = 0x0C,
	};

	enum struct instrument_precondition_failures : uint8_t
	{
		not_met = 0x01,
	};

	/********************************************************************************/
	enum struct cntrlr_subsystems : uint8_t
	{
		general = 0x01,
	};
	enum struct cntrlr_general_instance : uint8_t
	{
		none = 0x00,
	};
	enum struct cntrlr_general_failures : uint8_t
	{
		connection_error = 0x01,
		host_comm_error = 0x02,             // based on the error status reported in board status
		firmware_update_error = 0x03,
		firmware_bootup_error = 0x04,
		firmware_invalid_version = 0x05,
		firmware_interface_error = 0x06,    // error in the software transaction somewhere in multiple layers of the host software
		firmware_operation = 0x07,          // firmware state machine timeout
		hardware_health = 0x08,
		eeprom_read_error  = 0x09,
		eeprom_write_error = 0x0A,
		eeprom_erase_error = 0x0B
	};
	/*****************************************************************************/
	enum struct reagents_subsystems : uint8_t
	{
		rfid_hw = 0x01,
		bay_hw = 0x02,
		reagent_pack = 0x03,
	};

	enum struct reagent_rfid_hw_failures : uint8_t
	{
		hardware_error = 0x01,
		rfid_error = 0x02,
	};
	enum struct reagent_bay_hw_failures : uint8_t
	{
		sensor_error = 0x01,
		latch_error = 0x02,
	};

	enum struct reagent_pack_instance : uint8_t
	{
		general = 0xff,  // WHAT IS EXPECTED USE? IS IT HELPFUL TO HAVE THIS?
		main_bay = 0x00,
		door_right = 0x01,
		door_left = 0x02,
	};
	enum struct reagent_pack_failures : uint8_t
	{
		pack_invalid = 0x01,
		pack_expired = 0x02,
		pack_empty = 0x03,
		no_pack = 0x04,	// GENERAL instance ?
		update_fail = 0x05,
		load_error = 0x06, // GENERAL instance ?
	};

	/*****************************************************************************/
	enum struct motion_subsystems : uint8_t
	{
		sample_deck = 0x01,
		fl_rack = 0x03,
		motor = 0x04,
	};

	enum struct motion_sample_deck_instances : uint8_t
	{
		general = 0x00,
		plate = 0x01,
		carousel = 0x02,
	};
	enum struct motion_sample_deck_failures : uint8_t
	{
		not_calibrated = 0x01,
		calibration_failure = 0x02,
		init_error = 0x03,
		position_fail = 0x04,
		eject_fail = 0x05,
		home_fail = 0x06,
		tube_detected = 0x07, // Specific to Carousel instance homing / registration
	};

	enum struct motion_fl_rack_failures : uint8_t
	{
		not_calibrated = 0x01,
		calibration_failure = 0x02,
		init_error = 0x03,
		position_fail = 0x04,
	};

	// The MotorIds in Registers.hpp are actually used to report the error instance in Motor base class, so this enum should be always matching to MotorIds 
	enum struct motion_motor_instances : uint8_t
	{
		sample_probe = 0x01,
		radius = 0x02,
		theta = 0x03,
		focus = 0x04,
		reagent_probe = 0x05,
		fl_rack1 = 0x06,
		fl_rack2 = 0x07,
	};
	enum struct motion_motor_failures : uint8_t
	{
		init_error = 0x01,
		timeout = 0x02,
		homing_fail = 0x03,
		position_fail = 0x04,
		holdingcurrent_fail = 0x05,
		driver_error = 0x06,
		operation_logic = 0x07,
		thermal = 0x08,
	};

	/*****************************************************************************/
	enum struct fluidics_subsystems : uint8_t
	{
		general = 0x01,
		syringepump = 0x02,
	};
	enum struct fluidics_general_instances : uint8_t
	{
		none = 0x00,
	};
	enum struct fluidics_general_failures : uint8_t
	{
		nightly_clean_skipped = 0x01,
		priming_failure = 0x02,
	};

	enum struct fluidics_syringepump_instances : uint8_t
	{
		pump_control = 0x01,
		valve_control = 0x02,
	};
	enum struct fluidics_syringepump_failures : uint8_t
	{
		init_error = 0x01,
		hardware_error = 0x02,
		communication_error = 0x03,
		over_pressure_error = 0x04,
	};

	/*****************************************************************************/
	enum struct imaging_subsystems : uint8_t
	{
		general = 0x01,
		camera = 0x02,
		trigger = 0x03,
		led = 0x04,
		photodiode = 0x05,
		omicronled = 0x06,
	};

	enum struct imaging_general_failures : uint8_t
	{
		operation_timeout = 0x01,
		logic_error = 0x02,
		image_quality = 0x03,
		background_adjust_fail = 0x04,
		background_adjust_nearmax = 0x05,

	};
	enum struct imaging_camera_failures : uint8_t
	{
		hardware_error = 0x01,
		operation_timeout = 0x02,
		cannot_connect = 0x03,
		init_error = 0x04,
		no_image_captured = 0x05,
	};
	enum struct imaging_trigger_failures : uint8_t
	{
		hardware_error = 0x01,
		operation_timeout = 0x02,
	};

	enum struct imaging_led_instances : uint8_t
	{
		brightfield = 0x01,
		top_1 = 0x02,
		bottom_1 = 0x03,
		top_2 = 0x04,
		bottom_2 = 0x05,
	};

	enum struct imaging_led_failures : uint8_t
	{
		hardware_error = 0x01,
		operation_timeout = 0x02,
		init_error = 0x03,
		power_threshold = 0x04,
		comms_error = 0x05,
		reset = 0x06,

		//Omicron errors
		response_too_short = 0x07,
		interlock = 0x08,
		cdrh_error = 0x09,
		internal_comm_error = 0x0A,
		underover_voltage = 0x0B,
		external_interlock = 0x0C,
		diode_current = 0x0D,
		ambient_temp = 0x0E,
		diode_temp = 0x0F,
		internal_error = 0x10,
		diode_power = 0x11,
	};

	enum struct imaging_photodiode_failures : uint8_t
	{
		hardware_error = 0x01,
		operation_timeout = 0x02,
	};

	/*****************************************************************************/
	enum struct sample_subsystems : uint8_t
	{
		analysis = 0x01,
		celltype = 0x02,
		cellcounting = 0x03,
		general = 0x04,
	};

	enum struct sample_analysis_failures : uint8_t
	{
		unknown_type = 0x01,
	};

	enum struct sample_celltype_failures : uint8_t
	{
		unknown_type = 0x01,
	};

	enum struct sample_cellcounting_failures : uint8_t
	{
		unknown_type = 0x01,
		configuration_invalid = 0x02,
		general_population_invalid = 0x03,
		population_of_interest_invalid = 0x04,
		initerror = 0x05,
		bubble_detected = 0x06,
		clusters_detected = 0x07,
		images_dropped = 0x08,
	};

	enum struct sample_general_failures : uint8_t
	{
		unknown_type = 0x01,
		timeout = 0x02,
		processingerror = 0x03,
	};
};