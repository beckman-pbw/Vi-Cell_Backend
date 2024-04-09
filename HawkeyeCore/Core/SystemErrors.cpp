#include "stdafx.h"

#include <algorithm>
#include <vector>

#include "ChronoUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"
#include "ErrorLog.hpp"
#include "Reagent.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "ReportSystemError";

static SystemStatus systemStatusInit = {};

// Make the current username available to the error logging code.
extern std::string g_currentSysErrorUsername;

//*****************************************************************************
ReportSystemError::ReportSystemError():
	_systemStatus(systemStatusInit)
{
}

//*****************************************************************************
ReportSystemError::~ReportSystemError()
{
	if (!_errorCodes.empty()) {
		_errorCodes.clear();
	}
}

//*****************************************************************************
void ReportSystemError::ReportError(uint32_t errorCode, bool logDescription)
{
	ReportUserError(errorCode, g_currentSysErrorUsername.c_str(), logDescription);
}

//*****************************************************************************
void ReportSystemError::ReportUserError(uint32_t errorCode, const char* username, bool logDescription)
{
	std::lock_guard<std::mutex> guard(_errorCodesMutex);

	if (logDescription)
	{
		ReportSystemError::DecodeAndLogErrorCode(errorCode);
	}

	ErrorLogger::L().Log(boost::str(boost::format("%d|%lu|%s\n")
		% ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime())
		% errorCode
		% username));

	// Add new error code and remove if it is a duplicate.
	_errorCodes.push_back(errorCode);
	std::sort(_errorCodes.begin(), _errorCodes.end());
	_errorCodes.resize(std::distance(_errorCodes.begin(), std::unique(_errorCodes.begin(), _errorCodes.end())));

}

uint16_t ReportSystemError::GetErrorCount(void)
{
	return (uint16_t)(_errorCodes.size());
}

uint32_t ReportSystemError::GetErrorCode(uint16_t errorIndex)
{
	std::lock_guard<std::mutex> guard(_errorCodesMutex);
	if (errorIndex < _errorCodes.size())
		return _errorCodes.at(errorIndex);
		
	return 0;
}

std::vector<uint32_t> ReportSystemError::GetErrorCodes(void)
{
	_errorCodesMutex.lock();
	std::vector<uint32_t> errors = _errorCodes;
	_errorCodesMutex.unlock();
	return errors;
}

void ReportSystemError::ClearAllErrors(void)
{
	_errorCodesMutex.lock();
	if (!_errorCodes.empty())
		_errorCodes.clear();
	_errorCodesMutex.unlock();
}

void ReportSystemError::ClearErrorCode(uint32_t errorCode)
{
	_errorCodesMutex.lock();
	_errorCodes.erase(std::remove(_errorCodes.begin(), _errorCodes.end(), errorCode), _errorCodes.end());
	_errorCodesMutex.unlock();
}

void ReportSystemError::DecodeAndLogErrorCode(uint32_t errorCode)
{
	std::string level;
	std::string system;
	std::string subsystem;
	std::string instance;
	std::string failure;

	instrument_error::DecodeErrorInstance(errorCode, level, system, subsystem, instance, failure);

	//ErrorLogger::L().Log (boost::str(boost::format("%d|0x%08X|%s|%s|%s|%s|%s\n")
	//	% ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime())
	//	% errorCode
	//	% level
	//	% system
	//	% subsystem
	//	% instance
	//	% failure));

	std::string str = boost::str (boost::format ("Code: 0x%08X (%u), Level: %s, System: %s, Subsystem: %s, Instance: %s, Failure: %s")
		% errorCode % errorCode % level%  system % subsystem % instance % failure);

	Logger::L().Log (MODULENAME, severity_level::normal, str);
}

//*****************************************************************************
namespace instrument_error
{
	void DecodeErrorInstance(uint32_t errorval, std::string& level, std::string& system, std::string& subsystem, std::string& instance, std::string& failure)
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
		*   Do not make changes to this function without also making changes to 
		*     Documentation/SystemErrorCode_ResourceList.md
		*
		*   The strings here are used to programmatically generate the resource key strings
		*    within that file.  Alterations or additions to the strings we return here will
		*    result in UNMATCHED resource keys within the user interface code!
		*
		*
		*         ___ _____ _____ _____ _   _ _____ _____ _____ _   _
		*        / _ \_   _|_   _|  ___| \ | |_   _|_   _|  _  | \ | |
		*       / /_\ \| |   | | | |__ |  \| | | |   | | | | | |  \| |
		*       |  _  || |   | | |  __|| . ` | | |   | | | | | | . ` |
		*       | | | || |   | | | |___| |\  | | |  _| |_\ \_/ / |\  |
		*       \_| |_/\_/   \_/ \____/\_| \_/ \_/  \___/ \___/\_| \_/
		*
		*/

		uint8_t severity_8  = getSeverity_8(errorval);
		uint8_t system_8    = getSystem_8(errorval);
		uint8_t subsystem_8 = getSubsystem_8(errorval);
		uint8_t instance_8  = getInstance_8(errorval);
		uint8_t failure_8   = getFailure_8(errorval);
		auto severity_e  = static_cast<severity_level>(severity_8);
		auto system_e    = static_cast<systems>(system_8);

		level = "";
		system = "";
		subsystem = "";
		instance = "";
		failure = "";

		//*********************************************************************
		switch (severity_e)
		{
			case instrument_error::severity_level::error: 
				level = "Error";
				break;
			case instrument_error::severity_level::warning:
				level = "Warning";
				break;
			case instrument_error::severity_level::notification:
				level = "Notification";
				break;
			default:
				level = "???";
				break;
		}

		//*********************************************************************
		switch (system_e)
		{
			//=================================================================
			case systems::instrument:
			{
				system = "Instrument";

				switch(static_cast<instr_subsystems>(subsystem_8))
				{
					case instr_subsystems::general:
					{
						subsystem = "General";

						switch (static_cast<instrument_general_instance>(instance_8))
						{
							case instrument_general_instance::none:
							{
								instance = "";
								break;
							}
							default:
								break;
						}
						switch (static_cast<instrument_general_failures>(failure_8))
						{
							default:
								break;
						}
						break;
					}
					case instr_subsystems::configuration:
					{
						subsystem = "Configuration";

						switch (static_cast<instrument_configuration_instance>(instance_8))
						{
							case instrument_configuration_instance::none:
								instance = "";
								break;
							case instrument_configuration_instance::signature_definitions:
								instance = "Signature Definitions";
								break;
							case instrument_configuration_instance::userlist:
								instance = "User List";
								break;
							case instrument_configuration_instance::auditlog:
								instance = "Audit Log";
								break;
							case instrument_configuration_instance::errorlog:
								instance = "Error Log";
								break;
							case instrument_configuration_instance::samplelog:
								instance = "Sample Log";
								break;
							case instrument_configuration_instance::hawkeyelog:
								instance = "Hawkeye Log";
								break;
							case instrument_configuration_instance::cameralog:
								instance = "Camera Log";
								break;
							case instrument_configuration_instance::storagelog:
								instance = "Storage Log";
								break;
							default:
								break;
						}
						switch (static_cast<instrument_configuration_failures>(failure_8))
						{
							case instrument_configuration_failures::not_present:
								failure = "Not present";
								break;
							case instrument_configuration_failures::failed_validation:
								failure = "Failed validation";
								break;
							default:
								break;
						}
						break;
					}
					case instr_subsystems::storage:
					{
						subsystem = "Storage";

						switch (static_cast<instrument_storage_instance>(instance_8))
						{
							case instrument_storage_instance::none:
								instance = "";
								break;
							case instrument_storage_instance::analysis:
								instance = "Analysis";
								break;
							case instrument_storage_instance::audit:
								instance = "Audit";
								break;
							case instrument_storage_instance::bioprocess:
								instance = "Bioprocess";
								break;
							case instrument_storage_instance::calibration:
								instance = "Concentration and Sizing";
								break;
							case instrument_storage_instance::celltype:
								instance = "CellType";
								break;
							case instrument_storage_instance::instrument_configuration:
								instance = "Instrument Configuration";
								break;
							case instrument_storage_instance::error:
								instance = "Error";
								break;
							case instrument_storage_instance::qualitycontrol:
								instance = "QualityControl";
								break;
							case instrument_storage_instance::reagentpack:
								instance = "ReagentPack";
								break;
							case instrument_storage_instance::sample:
								instance = "Sample";
								break;
							case instrument_storage_instance::signatures:
								instance = "Signatures";
								break;
							case instrument_storage_instance::userlist:
								instance = "UserList";
								break;
							case instrument_storage_instance::reagent:
								instance = "Reagent";
								break;
							case instrument_storage_instance::result:
								instance = "Result";
								break;
							case instrument_storage_instance::configStatic:
								instance = "Configuration (Static)";
								break;
							case instrument_storage_instance::configDynamic:
								instance = "Configuration (Dynamic)";
								break;
							case instrument_storage_instance::motor_info:
								instance = "Motor Info";
								break;
							case instrument_storage_instance::motor_config:
								instance = "Motor Controller Config";
								break;
							case instrument_storage_instance::syringe_config:
								instance = "Syringe Config";
								break;
							case instrument_storage_instance::focus_config_simulator:
								instance = "Focus config";
								break;
							case instrument_storage_instance::dust_image:
								instance = "Dust Image";
								break;
							case instrument_storage_instance::debug_image:
								instance = "Debug Image";
								break;
							case instrument_storage_instance::workflow_script:
								instance = "Workflow script";
								break;
							case instrument_storage_instance::directory:
								instance = "Directory";
								break;
							case instrument_storage_instance::workflow_design:
								instance = "Workflow design";
								break;
							case instrument_storage_instance::sample_set:
								instance = "Sample Set";
								break;
							case instrument_storage_instance::illuminator:
								instance = "Illuminator";
								break;
							case instrument_storage_instance::analysis_definition:
								instance = "Analysis Definition";
								break;
							case instrument_storage_instance::celltype_definition:
								instance = "Celltype Definition";
								break;
							case instrument_storage_instance::sample_item:
								instance = "Sample Item";
								break;
							default:
								break;
						}
						switch (static_cast<instrument_storage_failures>(failure_8))
						{
							case instrument_storage_failures::no_connection:
								failure = "No Connection";
								break;
							case instrument_storage_failures::file_not_found:
								failure = "File not found";
								break;
							case instrument_storage_failures::backup_restore_failed:
								failure = "Backup restore failed";
								break;
							case instrument_storage_failures::read_error:
								failure = "Read error";
								break;
							case instrument_storage_failures::write_error:
								failure = "Write error";
								break;
							case instrument_storage_failures::storage_near_capacity:
								failure = "Storage near capacity";
								break;
							case instrument_storage_failures::not_allowed:
								failure = "Operation not allowed";
								break;
							case instrument_storage_failures::delete_error:
								failure = "Deletion error";
								break;
							default:
								break;
						}
						break;
					}
					case instr_subsystems::integrity:
					{
						subsystem = "Integrity";

						switch (static_cast<instrument_integrity_instance>(instance_8))
						{
							case instrument_integrity_instance::none:
								instance = "";
								break;
							default:
								break;
						}
						switch (static_cast<instrument_integrity_failures>(failure_8))
						{
							case instrument_integrity_failures::verification_on_read:
								failure = "Verification on read";
								break;
							case instrument_integrity_failures::software_fault:
								failure = "Software Fault";
								break;
							case instrument_integrity_failures::hardware_fault:
								failure = "Hardware Fault";
								break;
							default:
								break;
						}
						break;
					}
					case instr_subsystems::precondition:
					{
						subsystem = "Precondition";

						switch (static_cast<instrument_precondition_instance>(instance_8))
						{
							case instrument_precondition_instance::focus:
								instance = "Focus";
								break;
							case instrument_precondition_instance::plate_registration:
								instance = "Plate registration";
								break;
							case instrument_precondition_instance::carousel_registration:
								instance = "Carousel registration";
								break;
							case instrument_precondition_instance::concentration_config:
								instance = "Concentration configuration";
								break;
							case instrument_precondition_instance::size_config:
								instance = "Sizing configuration";
								break;
							case instrument_precondition_instance::plate_present:
								instance = "Plate present";
								break;
							case instrument_precondition_instance::carousel_present:
								instance = "Carousel present";
								break;
							case instrument_precondition_instance::reagent_door:
								instance = "Reagent door";
								break;
							case instrument_precondition_instance::wastetubetray_capacity:
								instance = "Waste tube tray capacity";
								break;
							case instrument_precondition_instance::instrument_serialnumber:
								instance = "instrument serial number";
								break;
							case instrument_precondition_instance::zip_utility_installed:
								instance = "7-Zip utility installed ";
								break;
							default:
								break;
						}
						switch (static_cast<instrument_precondition_failures>(failure_8))
						{
							case instrument_precondition_failures::not_met:
								failure = "Not met";
								break;
							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;

			} // End "case systems::instrument"

			//==========================================================================
			case systems::controller_board:
			{
				system = "Controller Board";

				switch(static_cast<cntrlr_subsystems>(subsystem_8))
				{
					case cntrlr_subsystems::general:
					{
						subsystem = "General";

						switch (static_cast<cntrlr_general_instance>(instance_8))
						{
							case cntrlr_general_instance::none:
								instance = "";
								break;
							default:
								break;
						}
						switch (static_cast<cntrlr_general_failures>(failure_8))
						{
							case cntrlr_general_failures::connection_error:
								failure = "Connection error";
								break;
							case cntrlr_general_failures::host_comm_error:
								failure = "Host Communication error";
								break;
							case cntrlr_general_failures::firmware_update_error:
								failure = "Firmware update error";
								break;
							case cntrlr_general_failures::firmware_bootup_error:
								failure = "Firmware bootup error";
								break;
							case cntrlr_general_failures::firmware_invalid_version:
								failure = "Invalid firmware version";
								break;
							case cntrlr_general_failures::firmware_interface_error:
								failure = "Firmware interface error";
								break;
							case cntrlr_general_failures::firmware_operation:
								failure = "Firmware state machine error";
								break;
							case cntrlr_general_failures::hardware_health:
								failure = "Hardware health";
								break;
							case cntrlr_general_failures::eeprom_read_error:
								failure = "EEPROM read error";
								break;
							case cntrlr_general_failures::eeprom_write_error:
								failure = "EEPROM write error";
								break;
							case cntrlr_general_failures::eeprom_erase_error:
								failure = "EEPROM erase error";
								break;
							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;

			} // End "case systems::controller_board"

			//==========================================================================
			case systems::reagents:
			{
				system = "Reagents";

				switch(static_cast<reagents_subsystems>(subsystem_8))
				{
					case reagents_subsystems::rfid_hw:
					{
						subsystem = "RFID hardware";
						
						switch (static_cast<reagent_rfid_hw_failures>(failure_8))
						{
							case reagent_rfid_hw_failures::hardware_error:
								failure = "Hardware error";
								break;
							case reagent_rfid_hw_failures::rfid_error:
								failure = "RFID error";
								break;
							default:
								break;
						}
						break;
					}
					case reagents_subsystems::bay_hw:
					{
						subsystem = "Reagent bay hardware";

						switch (static_cast<reagent_bay_hw_failures>(failure_8))
						{
							case reagent_bay_hw_failures::sensor_error:
								failure = "Sensor error";
								break;
							case reagent_bay_hw_failures::latch_error:
								failure = "Latch error";
								break;
							default:
								break;
						}
						break;
					}
					case reagents_subsystems::reagent_pack:
					{
						subsystem = "Reagent pack";

						switch (static_cast<reagent_pack_instance>(instance_8))
						{
							case reagent_pack_instance::general:
								instance = "General";
								break;
							case reagent_pack_instance::main_bay:
								instance = "Main bay";
								break;
							case reagent_pack_instance::door_left:
								instance = "Door Left";
								break;
							case reagent_pack_instance::door_right:
								instance = "Door Right";
								break;
							default:
								break;
						}
						switch (static_cast<reagent_pack_failures>(failure_8))
						{
							case reagent_pack_failures::pack_invalid:
								failure = "Invalid";
								break;
							case reagent_pack_failures::pack_expired:
								failure = "Expired";
								break;
							case reagent_pack_failures::pack_empty:
								failure = "Empty";
								break;
							case reagent_pack_failures::no_pack:
								failure = "No pack found";
								break;
							case reagent_pack_failures::update_fail:
								failure = "Write failure";
								break;
							case reagent_pack_failures::load_error:
								failure = "Load failed";
								break;
							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;
			
			} // End "case systems::reagents"

			//==========================================================================
			case systems::motion:
			{
				system = "Motion";

				switch(static_cast<motion_subsystems>(subsystem_8))
				{
					case motion_subsystems::sample_deck:
					{
						subsystem = "Sample deck";

						switch (static_cast<motion_sample_deck_instances>(instance_8))
						{
							case motion_sample_deck_instances::general:
								instance = "General";
								break;
							case motion_sample_deck_instances::plate:
								instance = "Plate";
								break;
							case motion_sample_deck_instances::carousel:
								instance = "Carousel";
								break;
							default:
								break;
						}
						switch (static_cast<motion_sample_deck_failures>(failure_8))
						{
							case motion_sample_deck_failures::not_calibrated:
								failure = "Not registered";
								break;
							case motion_sample_deck_failures::calibration_failure:
								failure = "Registration failure";
								break;
							case motion_sample_deck_failures::init_error:
								failure = "Initialization failure";
								break;
							case motion_sample_deck_failures::position_fail:
								failure = "Positioning failure";
								break;
							case motion_sample_deck_failures::eject_fail:
								failure = "Eject failure";
								break;
							case motion_sample_deck_failures::home_fail:
								failure = "Homing failure";
								break;
							case motion_sample_deck_failures::tube_detected:
								failure = "Tube detected";
								break;
							default:
								break;
						}
						break;
					}
					case motion_subsystems::fl_rack:
					{
						subsystem = "Fluorescent rack";

						switch (static_cast<motion_fl_rack_failures>(failure_8))
						{
							case motion_fl_rack_failures::not_calibrated:
								failure = "Not registered";
								break;
							case motion_fl_rack_failures::calibration_failure:
								failure = "Registration failure";
								break;
							case motion_fl_rack_failures::init_error:
								failure = "Initialization failure";
								break;
							case motion_fl_rack_failures::position_fail:
								failure = "Positioning failure";
								break;
							default:
								break;
						}
						break;
					}
					case motion_subsystems::motor:
					{
						subsystem = "Motor";

						switch (static_cast<motion_motor_instances>(instance_8))
						{
							case motion_motor_instances::sample_probe:
								instance = "Sample probe";
								break;
							case motion_motor_instances::radius:
								instance = "Radius";
								break;
							case motion_motor_instances::theta:
								instance = "Theta";
								break;
							case motion_motor_instances::focus:
								instance = "Focus";
								break;
							case motion_motor_instances::reagent_probe:
								instance = "Reagent probe";
								break;
							case motion_motor_instances::fl_rack1:
								instance = "FL Rack 1";
								break;
							case motion_motor_instances::fl_rack2:
								instance = "FL Rack 2";
								break;
							default:
								break;
						}
						switch (static_cast<motion_motor_failures>(failure_8))
						{
							case motion_motor_failures::init_error:
								failure = "Initialization failure";
								break;
							case motion_motor_failures::timeout:
								failure = "Timeout";
								break;
							case motion_motor_failures::homing_fail:
								failure = "Homing failure";
								break;
							case motion_motor_failures::position_fail:
								failure = "Positioning failure";
								break;
							case motion_motor_failures::holdingcurrent_fail:
								failure = "Holding current failure";
								break;
							case motion_motor_failures::driver_error:
								failure = "Motor driver error";
								break;
							case motion_motor_failures::operation_logic:
								failure = "Logic error";
								break;
							case motion_motor_failures::thermal:
								failure = "Thermal";
								break;
							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;
			
			} // End "case systems::motion"

			//==========================================================================
			case systems::fluidics:
			{
				system = "Fluidics";

				switch(static_cast<fluidics_subsystems>(subsystem_8))
				{
					case fluidics_subsystems::general:
					{
						subsystem = "General";

						switch (static_cast<fluidics_general_instances>(instance_8))
						{
							case fluidics_general_instances::none:
								instance = "";
								break;
							default:
								break;
						}
						switch (static_cast<fluidics_general_failures>(failure_8))
						{
							case fluidics_general_failures::nightly_clean_skipped:
								failure = "Nightly clean cycle skipped";
								break;
							case fluidics_general_failures::priming_failure:
								failure = "Priming failure";
							default:
								break;
						}
						break;
					}
					case fluidics_subsystems::syringepump:
					{
						subsystem = "Syringe pump";
						
						switch (static_cast<fluidics_syringepump_instances>(instance_8))
						{
							case fluidics_syringepump_instances::pump_control:
								instance = "Pump controller";
								break;
							case fluidics_syringepump_instances::valve_control:
								instance = "Valve controller";
								break;
						}
						switch (static_cast<fluidics_syringepump_failures>(failure_8))
						{
							case fluidics_syringepump_failures::init_error:
								failure = "Initialization error";
								break;
							case fluidics_syringepump_failures::hardware_error:
								failure = "Hardware error";
								break;
							case fluidics_syringepump_failures::communication_error:
								failure = "Communication error";
								break;
							case fluidics_syringepump_failures::over_pressure_error:
								failure = "Over pressure error";
								break;
							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;
			} // End "case systems::fluidics"

			//==========================================================================
			case systems::imaging:
			{
				system = "Imaging";

				switch(static_cast<imaging_subsystems>(subsystem_8))
				{
					case imaging_subsystems::general:
					{
						subsystem = "General";
					
						switch (static_cast<imaging_general_failures>(failure_8))
						{
							case imaging_general_failures::operation_timeout:
								failure = "Timeout";
								break;
							case imaging_general_failures::logic_error:
								failure = "Logic Error";
								break;
							case imaging_general_failures::image_quality:
								failure = "Image quality";
								break;
							case imaging_general_failures::background_adjust_fail:
								failure = "Background intensity adjustment failure";
								break;
							case imaging_general_failures::background_adjust_nearmax:
								failure = "Background intensity adjustment near top of range";
								break;
							default:
								break;
						}
						break;
					}
					case imaging_subsystems::camera:
					{
						subsystem = "Camera";
						
						switch (static_cast<imaging_camera_failures>(failure_8))
						{
							case imaging_camera_failures::hardware_error:
								failure = "Hardware error";
								break;
							case imaging_camera_failures::operation_timeout:
								failure = "Timeout";
								break;
							case imaging_camera_failures::cannot_connect:
								failure = "Connection error";
								break;
							case imaging_camera_failures::init_error:
								failure = "Initialization failure";
								break;
							case imaging_camera_failures::no_image_captured:
								failure = "No image captured";
								break;
							default:
								break;
						}
						break;
					}
					
					case imaging_subsystems::trigger:
					{
						subsystem = "Trigger";
						
						switch (static_cast<imaging_trigger_failures>(failure_8))
						{
							case imaging_trigger_failures::hardware_error:
								failure = "Hardware error";
								break;
							case imaging_trigger_failures::operation_timeout:
								failure = "Timeout";
								break;
							default:
								break;
						}
						break;
					}
					
					case imaging_subsystems::led:
					{
						subsystem = "LED";
						switch (static_cast<imaging_led_instances>(instance_8))
						{
							case imaging_led_instances::brightfield:
								instance = "BRIGHTFIELD";
								break;
							case imaging_led_instances::top_1:
								instance = "TOP_1";
								break;
							case imaging_led_instances::bottom_1:
								instance = "BOTTOM_1";
								break;
							case imaging_led_instances::top_2:
								instance = "TOP_2";
								break;
							case imaging_led_instances::bottom_2:
								instance = "BOTTOM_2";
								break;
							default:
								break;
						}
							
						switch (static_cast<imaging_led_failures>(failure_8))
						{
							case imaging_led_failures::hardware_error:
								failure = "Hardware error";
								break;
							case imaging_led_failures::operation_timeout:
								failure = "Timeout";
								break;
							case imaging_led_failures::init_error:
								failure = "Initialization error";
								break;
							case imaging_led_failures::power_threshold:
								failure = "Power threshold";
								break;
							case imaging_led_failures::comms_error:
								failure = "Communication error";
								break;
							case imaging_led_failures::reset:
								failure = "Reset";
								break;
							case imaging_led_failures::response_too_short:
								failure = "Response too short";
								break;
							case imaging_led_failures::interlock:
								failure = "Interlock";
								break;
							case imaging_led_failures::cdrh_error:
								failure = "CDRH error";
								break;
							case imaging_led_failures::internal_comm_error:
								failure = "Internal comm error";
								break;
							case imaging_led_failures::underover_voltage:
								failure = "Under/over voltage";
								break;
							case imaging_led_failures::external_interlock:
								failure = "External interlock";
								break;
							case imaging_led_failures::diode_current:
								failure = "Diode current";
								break;
							case imaging_led_failures::ambient_temp:
								failure = "Ambient temp";
								break;
							case imaging_led_failures::diode_temp:
								failure = "Diode temp";
								break;
							case imaging_led_failures::internal_error:
								failure = "Internal error";
								break;
							case imaging_led_failures::diode_power:
								failure = "Diode power";
								break;
							default:
								break;
						}
						break;
					}

					case imaging_subsystems::omicronled:
					{
						subsystem = "Omicron LED";
						switch (static_cast<imaging_led_instances>(instance_8))
						{
						case imaging_led_instances::brightfield:
							instance = "BRIGHTFIELD";
							break;
						case imaging_led_instances::top_1:
							instance = "TOP_1";
							break;
						case imaging_led_instances::bottom_1:
							instance = "BOTTOM_1";
							break;
						case imaging_led_instances::top_2:
							instance = "TOP_2";
							break;
						case imaging_led_instances::bottom_2:
							instance = "BOTTOM_2";
							break;
						default:
							break;
						}

						switch (static_cast<imaging_led_failures>(failure_8))
						{
						case imaging_led_failures::hardware_error:
							failure = "Hardware error";
							break;
						case imaging_led_failures::operation_timeout:
							failure = "Timeout";
							break;
						case imaging_led_failures::init_error:
							failure = "Initialization error";
							break;
						case imaging_led_failures::power_threshold:
							failure = "Power threshold";
							break;
						case imaging_led_failures::comms_error:
							failure = "Communication error";
							break;
						case imaging_led_failures::reset:
							failure = "Reset";
							break;
						case imaging_led_failures::response_too_short:
							failure = "Response too short";
							break;
						case imaging_led_failures::interlock:
							failure = "Interlock";
							break;
						case imaging_led_failures::cdrh_error:
							failure = "CDRH error";
							break;
						case imaging_led_failures::internal_comm_error:
							failure = "Internal comm error";
							break;
						case imaging_led_failures::underover_voltage:
							failure = "Under/over voltage";
							break;
						case imaging_led_failures::external_interlock:
							failure = "External interlock";
							break;
						case imaging_led_failures::diode_current:
							failure = "Diode current";
							break;
						case imaging_led_failures::ambient_temp:
							failure = "Ambient temp";
							break;
						case imaging_led_failures::diode_temp:
							failure = "Diode temp";
							break;
						case imaging_led_failures::internal_error:
							failure = "Internal error";
							break;
						case imaging_led_failures::diode_power:
							failure = "Diode power";
							break;
						default:
							break;
						}
						break;
					}
					
					case imaging_subsystems::photodiode:
					{
						subsystem = "Photodiode";
						
						switch (static_cast<imaging_photodiode_failures>(failure_8))
						{
							case imaging_photodiode_failures::hardware_error:
								failure = "Hardware error";
								break;
							case imaging_photodiode_failures::operation_timeout:
								failure = "Timeout";
								break;

							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;

			} // End "case systems::imaging"

			//==========================================================================
			case systems::sample:
			{
				system = "Sample";

				switch(static_cast<sample_subsystems>(subsystem_8))
				{
					case sample_subsystems::analysis:
					{
						subsystem = "Analysis";

						switch (static_cast<sample_analysis_failures>(failure_8))
						{
							case sample_analysis_failures::unknown_type:
								failure = "Unknown type";
								break;
							default:
								break;
						}
						break;
					}
					case sample_subsystems::celltype:
					{
						subsystem = "CellType";

						switch (static_cast<sample_celltype_failures>(failure_8))
						{
							case sample_celltype_failures::unknown_type:
								failure = "Unknown type";
								break;
							default:
								break;
						}
						break;
					}
					case sample_subsystems::cellcounting:
					{
						subsystem = "CellCounting";

						switch (static_cast<sample_cellcounting_failures>(failure_8))
						{
							case sample_cellcounting_failures::unknown_type:
								failure = "Unknown type";
								break;
							case sample_cellcounting_failures::configuration_invalid:
								failure = "Configuration invalid";
								break;
							case sample_cellcounting_failures::general_population_invalid:
								failure = "General Population invalid";
								break;
							case sample_cellcounting_failures::population_of_interest_invalid:
								failure = "Population Of Interest invalid";
								break;
							case sample_cellcounting_failures::initerror:
								failure = "Initialization error";
								break;
							case sample_cellcounting_failures::bubble_detected:
								failure = "Bubbles detected";
								break;
							case sample_cellcounting_failures::clusters_detected:
								failure = "Large clusters detected";
								break;
							case sample_cellcounting_failures::images_dropped:
								failure = "Images discarded from analysis";
								break;
							default:
								break;
						}
						break;
					}
					case sample_subsystems::general:
					{
						subsystem = "General";

						switch (static_cast<sample_general_failures>(failure_8))
						{
							case sample_general_failures::unknown_type:
								failure = "Unknown type";
								break;
							case sample_general_failures::timeout:
								failure = "Timeout";
								break;
							case sample_general_failures::processingerror:
								failure = "Processing error";
								break;
							default:
								break;
						}
						break;
					}
					default:
						break;
				}
				break;
			} // End "case systems::sample"

			default:
				break;

		} // End "switch (system_e)"
	}
}
