#pragma once
#include "gtest\gtest.h"

#include <boost/thread.hpp>

#include "ControllerBoardInterface.hpp"
#include "FTDI_Error_To_Boost.hpp"
#include "SimpleFTDI.h"
#include "HawkeyeLogic.hpp"

namespace ControllerBoard_FWDownload_Testing
{
	using namespace UnitTests_HardwareInterfaces;

	//class fixture_EzPortTest :public ::testing::Test
	//{
	//public:
	//	std::shared_ptr<EzPort> p_ez_port;
	//	SimpleFTDI ftdi;
	//
	//	void SetUp() override
	//	{
	//		p_ez_port.reset(new EzPort(ftdi.deviceGPIOHandle));
	//	}
	//	void TearDown() override
	//	{
	//		p_ez_port.reset();
	//	}
	//};


	class fixture_ControllerBaordFWDownload :public ::testing::Test
	{
	public:
		fixture_ControllerBaordFWDownload ()
		{
		}

		static bool finished_download;
		static int  percentage_completed;
		static void firmware_progress(bool finish, int16_t total)
		{
			finished_download = finish;
			percentage_completed = total;
		};

		static void init_hawklogic_thread()
		{
			Initialize();
		}
		void SetUp() override
		{
			boost::thread t{init_hawklogic_thread};
			::Sleep(3000);//give some time for the Hawkeye Logic to initialize
		}

		void TearDown() override 
		{
			Shutdown();
		}
	};
}
