#pragma once

#include <vector>
#include <sstream>
#include <boost/system/error_code.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/assign/list_of.hpp>

#include "ftd2xx.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

class EzPort
{
public:
	EzPort(std::shared_ptr<boost::asio::io_context> iosvc, FT_HANDLE & gpio);
	virtual ~EzPort() {}

	bool Init(boost::system::error_code& ec);
	bool Init(FT_HANDLE& gpio, uint16_t level, boost::system::error_code& ec);
	bool GetStatus(uint8_t & status, boost::system::error_code& ec) ;
	bool CheckStatus(uint8_t& status, boost::system::error_code& ec) ;
	bool Connect(boost::system::error_code& ec);
	bool Disconnect(boost::system::error_code & ec);
	bool EnableProgramming(boost::system::error_code & ec) ;
	bool Program(uint32_t& address, std::vector<uint8_t>& data, boost::system::error_code& ec) ;
	bool Purge(boost::system::error_code& ec) const;

private:
	enum EzPortCommands
	{
		WriteEnable = 0x06,
		ReadStatusRegister = 0x05,
		FlashRead = 0x03,
		FlashReadFast = 0x0B,
		SectionProgram = 0x02,
		SectorErase = 0xD8,
		BulkErase = 0xC7,
		ResetChip = 0xB9,
		EnableRead = 0x20,
		WriteFlashCmd = 0xBA,
		ReadFlashCmd = 0xBB
	};
	enum EzPortStatusBits
	{
		WriteInProgress = 0x01,
		WriteEnabled = 0x02,
		BulkEraseDisable = 0x04,
		FlexRamMode = 0x08,
		WriteErrorFlag = 0x40,
		FlashSecurity = 0x80,
	};
	enum EzPortLineStatusBits
	{
		FT_OE = 0x02,   // bit1 overrun error
		FT_PE = 0x04,   // bit2 parity error
		FT_FE = 0x08,   // bit3 framing error
		FT_BI = 0x10,   // bit4 break occurred
	};

	static bool check_status(uint8_t status, boost::system::error_code& ec);
	bool get_status(uint8_t& status, boost::system::error_code& ec) ;

	bool enable_write(boost::system::error_code& ec) ;
	bool reset_chip(boost::system::error_code& ec) ;
	bool erase_chip(boost::system::error_code& ec) ;
	bool select_chip(bool enabled, boost::system::error_code& ec) ;
	static bool check_line_status(const FT_HANDLE& device, ULONG status, boost::system::error_code& ec);
	
	bool set_gpio1_ezport_port(bool pin, boost::system::error_code& ec);
	bool set_gpio0_reset_line(bool pin, boost::system::error_code& ec);
	bool set_gpio2_ezport_chip_select(bool pin, boost::system::error_code & ec);
	bool set_gpio_pins(bool pin0, bool pin1, bool pin2, boost::system::error_code& ec) ;

	bool read_gpio(std::vector<uint8_t>& data, boost::system::error_code& ec) ;
	bool read_gpio_ezport_data(std::vector<uint8_t>& data, boost::system::error_code& ec) ;
	bool write_gpio(std::vector<uint8_t>& data, boost::system::error_code& ec) const;
	bool write_gpio_ezport_data(std::vector<uint8_t>& data, boost::system::error_code& ec) ;
	bool write_gpio_ezport_cmd(EzPort::EzPortCommands cmd, boost::system::error_code &ec) ;

	static bool write_to_device(const FT_HANDLE& device, std::vector<uint8_t>& data, boost::system::error_code& ec);
	void wait_for(uint32_t millisec_amount, bool use_sleep_instead = false);
	bool read_from_device(const FT_HANDLE& device, std::vector<uint8_t>& data, boost::system::error_code& ec);
	bool program_section(uint32_t chunkaddress, std::vector<uint8_t>& chunk, boost::system::error_code& ec);

	std::shared_ptr<boost::asio::deadline_timer> _wait_timer;
	std::shared_ptr<boost::asio::io_context> _io_service;
	FT_HANDLE _deviceGPIOHandle;

	bool _using_low_byte;
	size_t _srec_index;
	uint8_t _status_Register;
	uint32_t _chunk_address;

	uint8_t _gpio_dir;
	bool _gpio0_reset_line;
	bool _gpio1_ezport_port;
	bool _gpio2_ezport_chip_select;
};

template <typename t>
std::string value_to_hex_string(t value)
{
	std::stringstream hexstrs;
	hexstrs << std::hex << value;
	return hexstrs.str();
}

template <class T>
static bool compare_vectors(const std::vector<T> &a, size_t va_offset,
							const std::vector<T> &b, size_t vb_offset)
{
	//check to see the sizes match with the offsets applied. 
	if (a.size() - va_offset != b.size() - vb_offset) return false;

	//check the contents of elements match with offsets applied.
	auto va_iter = a.begin();
	std::advance(va_iter, va_offset);

	auto vb_iter = b.begin();
	std::advance(vb_iter, vb_offset);

	for (; va_iter != a.end(); ++va_iter)
	{
		if (*va_iter != *vb_iter++) return false;
	}

	//make sure vector b iterated though to the end.
	if (vb_iter == b.end()) return false; //I'm pretty sure this is redundant

	return true;
}

