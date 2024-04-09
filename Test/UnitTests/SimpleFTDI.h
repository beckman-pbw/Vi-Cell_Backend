#pragma once
#include <string>
#include <boost/system/error_code.hpp>
#include "ControllerBoardInterface.hpp"


namespace UnitTests_HardwareInterfaces
{
	/**
	 * \brief 
	 */
	class SimpleFTDI
	{
	public:
		SimpleFTDI();
		~SimpleFTDI();
	
		boost::system::error_code init_serial_handle();
		boost::system::error_code init_gpio_handle();

		std::string deviceIDSerial;
		FT_HANDLE deviceSerialHandle;

		std::string deviceIDGPIO;
		FT_HANDLE deviceGPIOHandle;

		bool deviceIsOpen_gpio;
		bool deviceIsOpen_serial;

	};
}
