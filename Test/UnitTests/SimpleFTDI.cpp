#include "stdafx.h"

#include "SimpleFTDI.h"
#include "FTDI_Error_To_Boost.hpp"


namespace UnitTests_HardwareInterfaces
{
	
	SimpleFTDI::SimpleFTDI() :
		deviceIDSerial(CNTLR_SN_A_STR),
		deviceIDGPIO(CNTLR_SN_B_STR),
		deviceIsOpen_gpio(false),
		deviceIsOpen_serial(false)

	{
		auto ft_result = FT_OpenEx(PVOID(deviceIDGPIO.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &deviceGPIOHandle);
		boost::system::error_code ec_gpio = FTDI_Error_To_Boost(ft_result);
		ft_result = FT_OpenEx(PVOID(deviceIDSerial.c_str()), FT_OPEN_BY_SERIAL_NUMBER, &deviceSerialHandle);
		boost::system::error_code ec_serial = FTDI_Error_To_Boost(ft_result);
		
		if (!ec_gpio && !init_gpio_handle())
			deviceIsOpen_gpio = true;
		if (!ec_serial && !init_serial_handle())
			deviceIsOpen_serial = true;
	}
	SimpleFTDI::~SimpleFTDI()
	{
		if (deviceIsOpen_serial)
			FT_Close(deviceSerialHandle);
		deviceIsOpen_serial = false;

		if (deviceIsOpen_gpio)
			FT_Close(deviceGPIOHandle);
		deviceIsOpen_gpio = false;
	}

	boost::system::error_code SimpleFTDI::init_serial_handle()
	{
		boost::system::error_code ec;

		auto fstat = FT_ResetDevice(deviceSerialHandle);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBitMode(deviceSerialHandle, 0, FT_BITMODE_RESET);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_Purge(deviceSerialHandle, FT_PURGE_RX | FT_PURGE_TX);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBaudRate(deviceSerialHandle, 115200);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetDataCharacteristics(deviceSerialHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetFlowControl(deviceSerialHandle, FT_FLOW_NONE, 0, 0);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetTimeouts(deviceSerialHandle, 1100, 1100);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		deviceIsOpen_serial = true;
		//StartLocalIOService();
		return ec;
	}

	boost::system::error_code SimpleFTDI::init_gpio_handle()
	{
		boost::system::error_code ec;
		auto fstat = FT_ResetDevice(deviceGPIOHandle);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_Purge(deviceGPIOHandle, FT_PURGE_RX | FT_PURGE_TX);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetUSBParameters(deviceGPIOHandle, 4096, 0);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBitMode(deviceGPIOHandle, 0, FT_BITMODE_RESET);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetChars(deviceGPIOHandle, 0, false, 0, false);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetTimeouts(deviceGPIOHandle, 2000, 2000);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetLatencyTimer(deviceGPIOHandle, 255);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetFlowControl(deviceGPIOHandle, FT_FLOW_NONE, 0, 0);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBitMode(deviceGPIOHandle, 0, FT_BITMODE_RESET);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBitMode(deviceGPIOHandle, 0, FT_BITMODE_MPSSE);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBaudRate(deviceGPIOHandle, 115200);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetDataCharacteristics(deviceGPIOHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetTimeouts(deviceGPIOHandle, 1100, 1100);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		Sleep(50);

		deviceIsOpen_gpio = true;


		return ec;
	}

}
