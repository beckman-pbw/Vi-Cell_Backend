#include "stdafx.h"

#include <array>
#include <algorithm>

#include "ControllerBoardEzPort.hpp"
#include "FTDI_Error_To_Boost.hpp"
#include "Logger.hpp"
#include "ErrcHelpers.hpp"

static const char MODULENAME[] = "EzPort";

EzPort::EzPort(std::shared_ptr<boost::asio::io_context> iosvc, FT_HANDLE & gpio):
	_wait_timer(nullptr),
	_io_service(iosvc),
	_deviceGPIOHandle(gpio),
	_using_low_byte(true),
	_srec_index(0),
	_status_Register(0),
	_chunk_address(0)
{
	if (_using_low_byte)
	{
		_gpio_dir = 0xFB; // directions (0=input, 1=output)
	}
	else
	{
		_gpio_dir = 0x7; // directions (0=input, 1=output)
	}
	
}

bool EzPort::Init(boost::system::error_code & ec)
{
	std::vector<uint8_t> buff;
	buff.push_back(0x8A);  // disable clk divide by 5
	buff.push_back(0x97);  // disable adaptive clocking
	buff.push_back(0x8D);  // disable 3 phase clocking
	buff.push_back(0x86);  // clock divisor = TBD
	buff.push_back(1);     //20 for big-cap board
	buff.push_back(0);
	buff.push_back(0x85);  // disable loopback

	buff.push_back(0x80);  // Set Data bits LowByte
	buff.push_back(0);     // TCK = 0 = idle = inactive low

	if (!_using_low_byte)
	{
		buff.push_back(0x0B);
		buff.push_back(0x82);  // Set Data bits HighByte
		buff.push_back(0x01);
	}

	buff.push_back(_gpio_dir);
	if (!write_gpio(buff, ec)) return false;
	wait_for(50);

	/*** Setting setting pins to default
	 * p0 - Reset         - true active low
	 * p1 - Select EzPort - false
	 * p2 - EZP_CS        - true active low
	 */ 
	_gpio0_reset_line = true;
	_gpio1_ezport_port = false;
	_gpio2_ezport_chip_select = true;
	return set_gpio_pins(_gpio0_reset_line, 
                         _gpio1_ezport_port, 
                         _gpio2_ezport_chip_select, ec);
}

bool EzPort::Init(FT_HANDLE & gpio, uint16_t level, boost::system::error_code & ec)
{
	_deviceGPIOHandle = gpio;
	if (level <= 2)
	{
		_gpio_dir = 0xFB; // directions (0=input, 1=output)
	}
	else
	{
		_gpio_dir = 0x07; // directions (0=input, 1=output)
	}
	
	_wait_timer = std::make_shared <boost::asio::deadline_timer>(*_io_service);

	return Init(ec);
}

bool EzPort::GetStatus(uint8_t & status, boost::system::error_code & ec) 
{
	return (get_status(status, ec));	 
}
bool EzPort::CheckStatus(uint8_t & status, boost::system::error_code & ec) 
{
	ULONG line_status = 0; 
	if (!get_status(status, ec)) return false;

	if (!check_line_status(_deviceGPIOHandle, line_status, ec) ||
		!check_status(status, ec))
	{
		return false;
	}

	return true;
}
bool EzPort::Connect(boost::system::error_code & ec)
{
	Logger::L().Log (MODULENAME, severity_level::notification, "Establishing connection to EzPort.");

	if(!set_gpio1_ezport_port(true,ec))return false;  // select EzPort
	if (!set_gpio2_ezport_chip_select(false, ec))return false; // set EZP_CS active
	wait_for(1, true);
	if (!set_gpio0_reset_line(false, ec))return false; // set reset active
	wait_for(100, true);
	if (!set_gpio0_reset_line(true, ec))return false;  // set reset inactive
	wait_for(1, true);
	return true;
}

bool EzPort::Disconnect(boost::system::error_code & ec)
{
	Logger::L().Log (MODULENAME, severity_level::notification, "Disconnecting from EzPort and resetting the CPU.");

	if (!set_gpio2_ezport_chip_select(true, ec))return false; // set EZP_CS inactive
	wait_for(1, true);
	if (!set_gpio0_reset_line(false, ec))return false; // set reset active
	wait_for(1, true);
	if (!set_gpio0_reset_line(true, ec))return false;  // set reset inactive
	wait_for(1, true);
	if (!set_gpio1_ezport_port(false, ec))return false; // select MULTI/BDM
	return true;
}

bool EzPort::EnableProgramming(boost::system::error_code & ec) 
{
	if (!enable_write(ec))return false;
	if (!erase_chip(ec))return false;
	return true;
}

bool EzPort::Program(uint32_t & address, std::vector<uint8_t> & data, boost::system::error_code & ec) 
{
	return program_section(address, data, ec );
}

bool EzPort::Purge(boost::system::error_code& ec) const
{
	auto fstat = FT_Purge(_deviceGPIOHandle, FT_PURGE_RX | FT_PURGE_TX);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec) return false;
		
	return true;
}


bool EzPort::check_status(uint8_t status, boost::system::error_code & ec)
{
	if ((status & uint8_t(EzPortStatusBits::WriteInProgress)) != 0)
		Logger::L().Log (MODULENAME, severity_level::error, "EzPort status: WriteInProgress is high");

	if ((status & uint8_t(EzPortStatusBits::FlashSecurity)) != 0)
		Logger::L().Log (MODULENAME, severity_level::error, "EzPort status: FlashSecurity is high");

	if ((status & uint8_t(EzPortStatusBits::FlexRamMode)) != 0)
		Logger::L().Log (MODULENAME, severity_level::error, "EzPort status: FlexRamMode is high");

	if ((status & uint8_t(EzPortStatusBits::BulkEraseDisable)) != 0)
		Logger::L().Log (MODULENAME, severity_level::error, "EzPort status: BulkEraseDisable is high");

	if ((status & uint8_t(EzPortStatusBits::WriteErrorFlag)) != 0)
		Logger::L().Log (MODULENAME, severity_level::error, "EzPort status: WriteErrorFlag is high");

	if ((status & uint8_t(EzPortStatusBits::WriteInProgress |
		EzPortStatusBits::WriteErrorFlag |
		EzPortStatusBits::FlashSecurity |
		EzPortStatusBits::FlexRamMode |
		EzPortStatusBits::BulkEraseDisable)) != 0)
		return false;

	return true;
}

bool EzPort::get_status(uint8_t & status, boost::system::error_code & ec) 
{
	std::vector<uint8_t>buff;

	if (!select_chip(true, ec))	return false;
	if (!write_gpio_ezport_cmd(ReadStatusRegister, ec))	return false;

	buff.resize(1);//expecting to get one byte from reading the status register;
	if (!read_gpio_ezport_data(buff, ec))	return false;
	if (!select_chip(false, ec)) return false;
	status = buff[0];

	return true;
}

bool EzPort::enable_write(boost::system::error_code & ec) 
{
	if (!select_chip(true, ec)) return false;
	if (!write_gpio_ezport_cmd(EzPortCommands::WriteEnable, ec))	return false;
	return select_chip(false, ec);
}

bool EzPort::reset_chip(boost::system::error_code& ec) 
{
	if (!select_chip(true, ec))  return false;
	//if (FT_ResetDevice(_deviceGPIOHandle)) return false;
	if (!write_gpio_ezport_cmd(EzPortCommands::ResetChip, ec))	return false;
	return select_chip(false, ec);
}

bool EzPort::erase_chip(boost::system::error_code& ec) 
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "EzPort erasing flash on the chip.");
	if (!select_chip(true,ec))  return false;
	if (!write_gpio_ezport_cmd(EzPortCommands::BulkErase, ec))	return false;
	if (!select_chip(false,ec)) return false;

	uint8_t status;
	do
	{ // wait for erase to complete
		GetStatus(status, ec);
		if ((status & uint8_t(EzPortStatusBits::WriteErrorFlag)) != 0)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "EzPort write error during erase.");
			return false;
		}
	} while ((status & uint8_t(EzPortStatusBits::WriteInProgress)) != 0);
	return true;
}

bool EzPort::select_chip(bool enabled, boost::system::error_code & ec) 
{
	if (enabled) {
		return set_gpio2_ezport_chip_select(false, ec);
	}
		
	return set_gpio2_ezport_chip_select(true, ec);
}

bool EzPort::check_line_status(const FT_HANDLE& device, ULONG status, boost::system::error_code& ec)
{
	ULONG modem_status;
	auto fstat = FT_GetModemStatus(device, &modem_status);
	ec = FTDI_Error_To_Boost(fstat);
	if (ec)
		return false;
	status = ((modem_status >> 8) & 0x000000FF);
	if (0 != (status & (
		FT_OE |   // bit1 overrun error
		FT_PE |   // bit2 parity error
		FT_FE |   // bit3 framing error
		FT_BI))) // bit4 break occurred
	{
		std::stringstream ss;
		ss << std::hex << status;
		Logger::L().Log (MODULENAME, severity_level::error, "ERROR: FTDI Line Status = 0x" + ss.str());
		return false;
	}
	return true;
}

bool EzPort::set_gpio0_reset_line(bool pin, boost::system::error_code & ec)
{
	_gpio0_reset_line = pin;
	return set_gpio_pins(pin, _gpio1_ezport_port, _gpio2_ezport_chip_select, ec);
}
bool EzPort::set_gpio1_ezport_port(bool pin, boost::system::error_code & ec)
{
	_gpio1_ezport_port = pin;
	return set_gpio_pins(_gpio1_ezport_port, pin, _gpio2_ezport_chip_select, ec);
}
bool EzPort::set_gpio2_ezport_chip_select(bool pin, boost::system::error_code & ec)
{
	_gpio2_ezport_chip_select = pin;
	return set_gpio_pins(_gpio0_reset_line, _gpio1_ezport_port, pin, ec);
}
bool EzPort::set_gpio_pins(bool pin0, bool pin1, bool pin2, boost::system::error_code & ec) 
{
	std::vector<uint8_t> buff;
	uint8_t mask = 0;

	buff.push_back(0x81); // Read Data bits LowByte
	if (!write_gpio(buff, ec)) return false;
	if (!read_gpio(buff, ec))return false;
	if (_using_low_byte)
	{
		mask = (buff[0] & 0x0F); // preserve DI,DO,CS,CLK
		buff[0] = 0x80; // cmd=Set Data bits LowByte
		if (pin0) mask |= 0x10;
		if (pin1) mask |= 0x20;
		if (pin2) mask |= 0x40;
	}
	else
	{
		mask = (buff[0] & 0xF0); // preserve DI,DO,CS,CLK
		buff[0] = 0x82; // cmd=Set Data bits HighByte
		if (pin0) mask |= 0x04;
		if (pin1) mask |= 0x02;
		if (pin2) mask |= 0x01;
	}
	buff.push_back(mask); // values
	buff.push_back(_gpio_dir); // directions (0=input, 1=output)
	return write_gpio(buff, ec);
}

bool EzPort::read_gpio(std::vector<uint8_t>& data, boost::system::error_code & ec) 
{
	return read_from_device(_deviceGPIOHandle, data, ec);
}

bool EzPort::read_gpio_ezport_data(std::vector<uint8_t>& data, boost::system::error_code & ec) 
{
	std::vector<uint8_t> tx_buff;
	tx_buff.push_back(EzPortCommands::EnableRead);//We need to tell EzPort how many bytes to read.
	tx_buff.push_back(static_cast<uint8_t>((data.size() - 1) & 0xFF));
	tx_buff.push_back(static_cast<uint8_t>(((data.size() - 1) >> 8) & 0xFF));
	if (!write_gpio(tx_buff, ec)) return false;
	return read_from_device(_deviceGPIOHandle, data, ec);
}

bool EzPort::write_gpio(std::vector<uint8_t> & data, boost::system::error_code & ec) const
{
	return write_to_device(_deviceGPIOHandle, data, ec);
}

bool EzPort::write_gpio_ezport_data(std::vector<uint8_t> &data, boost::system::error_code & ec) 
{
	std::vector<uint8_t> tx_buff;

	tx_buff.push_back(0x11);
	tx_buff.push_back(static_cast<uint8_t>((data.size() - 1) & 0xFF));
	tx_buff.push_back(static_cast<uint8_t>(((data.size() - 1) >> 8) & 0xFF));
	tx_buff.insert(tx_buff.end(), data.begin(), data.end());

	return write_gpio(tx_buff, ec);
}

bool EzPort::write_gpio_ezport_cmd(EzPort::EzPortCommands cmd, boost::system::error_code & ec) 
{
	
	std::vector<uint8_t> tx_buff;
	
	tx_buff.push_back(0x11);
	tx_buff.push_back(0x00);
	tx_buff.push_back(0x00);
	tx_buff.push_back(cmd);

	return write_gpio(tx_buff, ec);
}

bool EzPort::write_to_device(const FT_HANDLE & device, std::vector<uint8_t>& data, boost::system::error_code & ec)
{
	DWORD bytes_written;
	FT_STATUS fstat = FT_Write(device,
							   static_cast<VOID*>(data.data()),
							   static_cast<DWORD>(data.size()), &bytes_written);

	ec = FTDI_Error_To_Boost(fstat);
	if (fstat != FT_OK || (bytes_written != data.size()))
	{
		return true;
	}

	return true;
}

void EzPort::wait_for(uint32_t millisec_amount, bool use_sleep_instead) 
{
	if (_wait_timer != nullptr && !use_sleep_instead)
	{
		_wait_timer->expires_from_now(boost::posix_time::milliseconds(millisec_amount));
		_wait_timer->wait();
	}
	else
		::Sleep(millisec_amount);
}

bool EzPort::read_from_device(const FT_HANDLE & device, std::vector<uint8_t>& data, boost::system::error_code & ec)
{
	FT_STATUS fstat = FT_OK;
	DWORD bytesRead=0;
	DWORD bytesAvail=0;
	auto timeout = 20; // 1 sec
	do
	{
		fstat= FT_GetQueueStatusEx(device, &bytesAvail);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return false;
		
		if (timeout-- < 0)
		{
			Logger::L().Log (MODULENAME, severity_level::notification, "Read timeout: only " +std::to_string(bytesAvail) + " of " + std::to_string(data.size()) + " bytes available");
			break;
		}
		wait_for(50);
	} while (bytesAvail < data.size());


	fstat = FT_Read(device, static_cast<void*>(data.data()), DWORD(data.size()), &bytesRead);
	ec = FTDI_Error_To_Boost(fstat);

	if (fstat != FT_OK)
	{
		return false;
	}
	if (bytesRead != data.size())
	{
		ec = MAKE_ERRC(boost::system::errc::stream_timeout);
		return false;
	}
	return true;
}

bool EzPort::program_section(uint32_t chunkaddress, std::vector<uint8_t> & chunk, boost::system::error_code & ec)
{
	if (!enable_write(ec))
		return false;

	std::vector<uint8_t> buff;
	buff.push_back(uint8_t(EzPortCommands::SectionProgram));
	buff.push_back(uint8_t((chunkaddress >> 16) & 0xFF));
	buff.push_back(uint8_t((chunkaddress >> 8) & 0xFF));
	buff.push_back(uint8_t(chunkaddress & 0xFF));
	buff.insert(buff.end(), chunk.begin(), chunk.end());

	if (!select_chip(true, ec)) return false;
	if (!write_gpio_ezport_data(buff, ec)) return false;
	if (!select_chip(false, ec)) return false;

	uint8_t status;
	int loopCount = 100;
	do // wait for the program/write to finish
	{
		wait_for(1, true);
		if (!CheckStatus(status, ec)) return false;
		if (loopCount-- == 0)
		{
			Logger::L().Log (MODULENAME, severity_level::notification, "EzPort.ProgramSection timeout waiting for SectionProgram to complete");
			return false;
		}
	} while ((status & EzPortStatusBits::WriteInProgress) != 0);

	// Read back the data just written.
	
	std::vector<uint8_t> dummy_buff;
	dummy_buff.insert(dummy_buff.begin(), buff.begin(), buff.begin() + 5);
	dummy_buff[0]=EzPortCommands::FlashReadFast;
	if (!select_chip(true, ec)) return false;
	if (!write_gpio_ezport_data(dummy_buff, ec)) return false;  // extra dummy byte for fastRead

	std::vector<uint8_t> read_buff;
	read_buff.resize(chunk.size());
	if (!read_gpio_ezport_data(read_buff, ec))  return false;
	if (!select_chip(false, ec))    return false;

	if(compare_vectors<uint8_t>(buff, 4, read_buff, 0))
	{
		std::vector<int> diff;
		Logger::L().Log (MODULENAME, severity_level::error, "EzPort validation of data failed at 0x" + value_to_hex_string(chunkaddress));
		ec = MAKE_ERRC(boost::system::errc::operation_canceled);
		return false;
	}

	return true;
}
