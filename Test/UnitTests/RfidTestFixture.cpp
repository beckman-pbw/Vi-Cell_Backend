#include "stdafx.h"
#include "gtest\gtest.h"

#include <chrono>

#include "Logger.hpp"
#include "boost\filesystem.hpp"

#include "Rfid.hpp"
#include "ControllerBoardInterfaceMock.h"
#include <condition_variable>
#include "RfidTestFixture.h"

namespace HawkeyeTestFixtures
{
	using ::testing::_;
	using ::testing::A;
	using ::testing::Return;
	using ::testing::ReturnRef;
	using ::testing::DoAll;
	using ::testing::Pointee;
	using ::testing::NotNull;
	using ::testing::SaveArg;

	/**
	 * RFID Test Cases Test Fixture
	 */
	RfidFixture::RfidFixture() : localtest_work(localtest_iosvc),
        test_thread(std::bind(static_cast<std::size_t(boost::asio::io_service::*)(void)>(&boost::asio::io_service::run), &localtest_iosvc))
	{
	}

	void RfidFixture::testseq_rfid_readregister_data()
	{
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		//test data should have been setup before calling this test sequence.
		EXPECT_EQ(rx_length, rx_buff.size());
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));
	}

	void RfidFixture::SetUp()
	{
		ControllerBoardInterfaceFixture::SetUp();
	}

	void RfidFixture::TearDown()
	{
		ControllerBoardInterfaceFixture::TearDown();
	}

}

