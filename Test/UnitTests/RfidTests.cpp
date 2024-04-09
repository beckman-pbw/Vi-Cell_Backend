#include "stdafx.h"
#include "gtest\gtest.h"

#include <chrono>

#include "Logger.hpp"
#include "boost\filesystem.hpp"

#include "Rfid.hpp"
#include "RfidTests.h"
#include "ControllerBoardInterfaceMock.h"
#include <condition_variable>
#include "SystemErrors.hpp"
#include "Simulation/RfidSim.hpp"

namespace RfidTests
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
	* \brief – Confirm we are able to instantiate Rfid class with passing mock object of ControllerBoardInterface.
	* \test - Use smart pointer to create a new instance of the Rfid class.
	*/
	TEST(RfidTestCase, DefaultConstructor)
	{
		std::shared_ptr<boost::asio::io_service> iosvc;
		boost::asio::io_service::work work(*iosvc);
		boost::thread th(std::bind(static_cast <std::size_t(boost::asio::io_service::*)(void)> (&boost::asio::io_service::run), iosvc));

		const std::string serial_dev_identifier;
		const std::string data_dev_identifier;

		std::shared_ptr<Mocking::ControllerBoardInterfaceMock> pmockCbi(new Mocking::ControllerBoardInterfaceMock(iosvc, serial_dev_identifier, data_dev_identifier));
		std::shared_ptr<ControllerBoardInterface> pCbi = std::dynamic_pointer_cast<ControllerBoardInterface>(pmockCbi);

		std::shared_ptr<Rfid> pRfid;
		pRfid.reset(new Rfid(pCbi));
		EXPECT_NE(nullptr, pRfid);
	}

	TEST_F(RfidTestCases, ErrorReporting_RfReaderError)
	{
		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		localtest_iosvc.post([this]() -> void
		{
			utest_Rfid.scan();
			cv_cmd_finished.notify_all();
		});

		board_status = 0; //no error board status for clearing error codes
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));// The first write call is to clear error codes
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		board_status = 0x01;//set error in board status
		error_status.set(uint64_t(1) << uint32_t(ErrorStatus::RfReader));// Set the RFID hardware error status.
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::io_error), 0x01, nullptr, nullptr);
		
		EXPECT_EQ(std::cv_status::no_timeout, cv_cmd_finished.wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(3)));
		//EXPECT_EQ(1,ReportSystemError::Instance().GetErrorCount());// we should only have one error reported. 
		//EXPECT_EQ(0xc3020001, ReportSystemError::Instance().GetErrorCode(0));
	}

	TEST_F(RfidTestCases, ErrorReporting_ReagentError)
	{
		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		localtest_iosvc.post([this]() -> void
		{
			utest_Rfid.scan();
			cv_cmd_finished.notify_all();
		});

		board_status = 0; //no error board status for clearing error codes
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));// The first write call is to clear error codes
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		board_status = 0x01;//set error in board status
		error_status.set(uint64_t(1) << uint32_t(ErrorStatus::Reagent));// Set the RFID hardware error status.
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::io_error), 0x01, nullptr, nullptr);

		
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length);
		rx_buff.push_back(0x12);//for Reagent error, it will read the RFID error code.
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		EXPECT_EQ(std::cv_status::no_timeout, cv_cmd_finished.wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(3)));
		//EXPECT_EQ(1, ReportSystemError::Instance().GetErrorCount());// we should only have one error reported. 
		//EXPECT_EQ(0xc3040005, ReportSystemError::Instance().GetErrorCode(0));
	}


	TEST_F(RfidTestCases, ErrorReporting_ProgramStatus)
	{
		
		std::vector<RfidTag_t> rfidTags;
		std::map<uint16_t, uint16_t> reagent_remainingvar_offset;

		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		localtest_iosvc.post([this, &rfidTags, &reagent_remainingvar_offset]() -> void
		{
			utest_Rfid.read(rfidTags);
//            utest_Rfid.read( rfidTags, reagent_remainingvar_offset );
            cv_cmd_finished.notify_all();
		});

		/**
		 * Respond to get total tags request with write/read callbacks and inject test data.
		 */
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0);

		//test data
		rx_buff[0]=0x01;//TODO: there must be a better way to respond with data
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/**
		* Respond to get tag data request with write/read callbacks callbacks and inject test data.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0); 
		
		//test data
		rx_buff[5]  = 0x01;// Authorized
		rx_buff[6]  = 0x01;// Validated
		rx_buff[7]  = 0x00;// Not Programmed
		rx_buff[29] = 0x01;// one cleaner
		rx_buff[31] = 0x08;// main bay location
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/*
		 * Check to see the Async command has been completed and system has correct error code. 
		 */
		EXPECT_EQ(std::cv_status::no_timeout, cv_cmd_finished.wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(3)));
		//EXPECT_EQ(0xC3040201, ReportSystemError::Instance().GetErrorCode(0));
	}

	TEST_F(RfidTestCases, ErrorReporting_ValidationStatus)
	{		
		std::vector<RfidTag_t> rfidTags;
		std::map<uint16_t, uint16_t> reagent_remainingvar_offset;

		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		localtest_iosvc.post([this, &rfidTags, &reagent_remainingvar_offset]() -> void
		{
			utest_Rfid.read(rfidTags);
//            utest_Rfid.read( rfidTags, reagent_remainingvar_offset );
            cv_cmd_finished.notify_all();
		});

		/**
		* Respond to get total tags request with write/read callbacks and inject test data.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0);

		//test data
		rx_buff[0] = 0x01;//TODO: there must be a better way to respond with data
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/**
		* Respond to get tag data request with write/read callbacks callbacks and inject test data.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0);

		//test data
		rx_buff[5] = 0x01;// Authorized
		rx_buff[6] = 0x00;// No Validated
		rx_buff[7] = 0x01;// Programmed
		rx_buff[29] = 0x01;// one cleaner
		rx_buff[31] = 0x08;// main bay location
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/*
		* Check to see the Async command has been completed and system has correct error code.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cmd_finished.wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(3)));
		//EXPECT_EQ(0xC3040201, ReportSystemError::Instance().GetErrorCode(0));
	}

	TEST_F(RfidTestCases, ErrorReporting_AuthenticateStatus_blanktag)
	{		
		std::vector<RfidTag_t> rfidTags;
		std::map<uint16_t, uint16_t> reagent_remainingvar_offset;

		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		localtest_iosvc.post([this, &rfidTags, &reagent_remainingvar_offset]() -> void
		{
			utest_Rfid.read(rfidTags);
//            utest_Rfid.read( rfidTags, reagent_remainingvar_offset );
            cv_cmd_finished.notify_all();
		});

		/**
		* Respond to get total tags request with write/read callbacks and inject test data.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0);

		//test data
		rx_buff[0] = 0x01;//TODO: there must be a better way to respond with data
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/**
		* Respond to get tag data request with write/read callbacks callbacks and inject test data.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0);

		//test data
		rx_buff[5] = 0x00;// Not Authorized-Blank Tag
		rx_buff[6] = 0x01;// Validated
		rx_buff[7] = 0x01;// Programmed
		rx_buff[29] = 0x01;// one cleaner
		rx_buff[31] = 0x08;// main bay location
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/*
		* Check to see the Async command has been completed and system has correct error code.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cmd_finished.wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(3)));
		//EXPECT_EQ(0xC3040201, ReportSystemError::Instance().GetErrorCode(0));
	}

	TEST_F(RfidTestCases, ErrorReporting_AuthenticateStatus_invalidkey)
	{
		
		std::vector<RfidTag_t> rfidTags;
		std::map<uint16_t, uint16_t> reagent_remainingvar_offset;

		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		localtest_iosvc.post([this, &rfidTags, &reagent_remainingvar_offset]() -> void
		{
			utest_Rfid.read(rfidTags);
//            utest_Rfid.read( rfidTags, reagent_remainingvar_offset );
            cv_cmd_finished.notify_all();
		});

		/**
		* Respond to get total tags request with write/read callbacks and inject test data.
		*	The data size request should be length of 4. 
		*/
		rx_buff.resize(4);
		rx_buff[0] = 0x01; //test data
		testseq_rfid_readregister_data();

		/**
		* Respond to get tag data request with write/read callbacks and inject test data.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		pwrite_reg_cb(MAKE_ERRC(boost::system::errc::success), 0, nullptr, nullptr);

		EXPECT_EQ(std::cv_status::no_timeout, cv_cb_wait_unill_sec(3));
		rx_buff.resize(rx_length); std::fill(rx_buff.begin(), rx_buff.end(), 0);

		//test data
		rx_buff[5] = 0x02;// Authorized-Invalid Key
		rx_buff[6] = 0x01;// Validated
		rx_buff[7] = 0x01;// Programmed
		rx_buff[29] = 0x01;// one cleaner
		rx_buff[31] = 0x08;// main bay location
		pread_reg_cb(MAKE_ERRC(boost::system::errc::success), 0x00, nullptr, std::make_shared<ControllerBoardInterface::t_txrxbuf>(rx_buff));

		/*
		* Check to see the Async command has been completed and system has correct error code.
		*/
		EXPECT_EQ(std::cv_status::no_timeout, cv_cmd_finished.wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(3)));
		//EXPECT_EQ(0xC3040201, ReportSystemError::Instance().GetErrorCode(0));
	}

	/**
	 * RFID Test Cases Test Fixture
	 */
	RfidTestCases::RfidTestCases(): utest_Rfid(pCbi)
	{
	}

	void RfidTestCases::SetUp()
	{
		RfidFixture::SetUp();
	}

	void RfidTestCases::TearDown()
	{
		RfidFixture::TearDown();
		//ReportSystemError::Instance().ClearAllErrors();
	}

}
