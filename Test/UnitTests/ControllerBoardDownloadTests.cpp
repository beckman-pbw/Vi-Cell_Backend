#include "stdafx.h"
#include <iostream>
#include <afx.h>

#include "Logger.hpp"

#include "ControllerBoardFW_TestFixtures.hpp"

namespace ControllerBoard_FWDownload_Testing
{
#define FIRMWARE_CHUNK_DOWNLOAD_TIMEOUT 200 //This is the most that it would take to program a 1k chunk of data to K70. 

	///***
	// * Test to make sure we are able to open the
	// * GPIO and serial ports
	// * NOTE!: to run this test, you must have a controller 
	// * (main) board connected to the host. 
	// */
	//TEST_F(fixture_EzPortTest, DISABLE_Init)
	//{
	//	ASSERT_TRUE(ftdi.deviceIsOpen_gpio);
	//	ASSERT_TRUE(ftdi.deviceIsOpen_serial);
	//}

	///***
	// * Test to see we are able to establish connection 
	// * to the EzPort by checking the status.
	// * NOTE!: to run this test, you must have a controller 
	// * (main) board connected to the host. 
	// */
	//TEST_F(fixture_EzPortTest, DISABLE_StatusCheck)
	//{
	//	uint8_t status;
	//	boost::system::error_code ec;
	//	EXPECT_TRUE(p_ez_port->GetStatus(status, ec));
	//}

	/***
	 * To test to see if we can execute a firmware download. 
	 * NOTE!: to run this test, you must have a controller 
	 * (main) board connected to the host. 
	 * 
	 * ALSO: The hardware under test, it's serial number for the ports must be set to default values. 
	 */
	TEST(DownloadFirmware, DISABLE_LoadSREC_Update)
	{
		boost::system::error_code ec;
		Logger::L().Initialize(ec, "Logger.info");
		Logger::L().SetLoggingSensitivity(severity_level::debug1);

		SimpleFTDI ftdi;

		//FirmwareDownload firmware_download(ftdi.deviceGPIOHandle);

		//ASSERT_TRUE(firmware_download.Init(ec));
		//EXPECT_TRUE(firmware_download.K70UpdateFirmware(".","prj.srec",ec));
		EXPECT_EQ(ec, boost::system::errc::success);
	}

	

	/***
	* Use DLL API to start the download and check to see it would not timeout. 
	* NOTE!: to run this test, you must have a controller 
	* (main) board connected to the host. 
	*/
	int fixture_ControllerBaordFWDownload::percentage_completed=0;
	bool fixture_ControllerBaordFWDownload::finished_download = false;
	TEST_F(fixture_ControllerBaordFWDownload, DISABLE_HawkeyeLogic_enableUpdate_byInfoFile)
	{
		auto previous = percentage_completed;
		auto timeout = FIRMWARE_CHUNK_DOWNLOAD_TIMEOUT;
		do
		{
			while(previous == percentage_completed && timeout > 0)
			{
				::Sleep(100);
				timeout--;
			}
			if (timeout < 0)break;//if timeout is less than 0, then timeout occurred
			timeout = FIRMWARE_CHUNK_DOWNLOAD_TIMEOUT;
			previous = percentage_completed;
		} while (!finished_download);

		EXPECT_TRUE(finished_download);
		EXPECT_EQ(100, percentage_completed);
		EXPECT_LE(0, timeout);
	}
}
