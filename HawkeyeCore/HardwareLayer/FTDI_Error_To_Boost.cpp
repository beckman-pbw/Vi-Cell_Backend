#include "stdafx.h"

#include "FTDI_Error_To_Boost.hpp"

boost::system::error_code FTDI_Error_To_Boost(DWORD FTDI)
{
	switch (FTDI)
	{
	case FT_OK: return MAKE_ERRC(boost::system::errc::success);
	case FT_INVALID_HANDLE: return MAKE_ERRC(boost::system::errc::not_connected);
	case FT_DEVICE_NOT_FOUND: return MAKE_ERRC(boost::system::errc::no_such_device);
	case FT_DEVICE_NOT_OPENED: return MAKE_ERRC(boost::system::errc::connection_refused);
	case FT_IO_ERROR: return MAKE_ERRC(boost::system::errc::io_error);
	case FT_INSUFFICIENT_RESOURCES: return MAKE_ERRC(boost::system::errc::not_enough_memory);
	case FT_INVALID_PARAMETER: return MAKE_ERRC(boost::system::errc::invalid_argument);
	case FT_INVALID_BAUD_RATE: return MAKE_ERRC(boost::system::errc::argument_out_of_domain);
	case FT_DEVICE_NOT_OPENED_FOR_ERASE: return MAKE_ERRC(boost::system::errc::read_only_file_system);
	case FT_DEVICE_NOT_OPENED_FOR_WRITE: return MAKE_ERRC(boost::system::errc::read_only_file_system);
	case FT_FAILED_TO_WRITE_DEVICE:
	case FT_EEPROM_READ_FAILED:
	case FT_EEPROM_WRITE_FAILED:
	case FT_EEPROM_ERASE_FAILED: return MAKE_ERRC(boost::system::errc::interrupted);
	case FT_EEPROM_NOT_PRESENT: return MAKE_ERRC(boost::system::errc::no_such_device_or_address);
	case FT_EEPROM_NOT_PROGRAMMED: return MAKE_ERRC(boost::system::errc::no_such_file_or_directory);
	case FT_INVALID_ARGS: return MAKE_ERRC(boost::system::errc::invalid_argument);
	case FT_NOT_SUPPORTED: return MAKE_ERRC(boost::system::errc::operation_not_supported);
	case FT_OTHER_ERROR:
	default:
		return MAKE_ERRC(boost::system::errc::state_not_recoverable);
	}
}
