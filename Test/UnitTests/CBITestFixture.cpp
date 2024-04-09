#include "stdafx.h"
#include "gtest\gtest.h"

#include <chrono>

#include "Logger.hpp"
#include "boost\filesystem.hpp"

//#include "Rfid.hpp"
//#include "RfidTests.h"
#include "ControllerBoardInterfaceMock.h"
#include <condition_variable>
#include "SystemErrors.hpp"
#include "Simulation/RfidSim.hpp"
#include "CBITestFixture.h"

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

	std::shared_ptr<std::condition_variable> ControllerBoardInterfaceFixture::pcv_cb_assgined;

	/**
	 * RFID Test Cases Test Fixture
	 */
    ControllerBoardInterfaceFixture::ControllerBoardInterfaceFixture()
    {
        pLocalmock_iosvc.reset( new boost::asio::io_service() );
        pcv_cb_assgined.reset( new std::condition_variable() );

        pLocalmock_work.reset( new boost::asio::io_service::work( *pLocalmock_iosvc ) );

        auto THREAD = std::bind( static_cast <std::size_t( boost::asio::io_service::* )( void )> ( &boost::asio::io_service::run ), pLocalmock_iosvc.get() );
        mock_thread = boost::thread( new std::thread( THREAD ) );

        pmockCbi.reset( new Mocking::ControllerBoardInterfaceMock( pLocalmock_iosvc, serial_dev_identifier, data_dev_identifier ) );
        pCbi = std::dynamic_pointer_cast<ControllerBoardInterface>( pmockCbi );
    }

//    ControllerBoardInterfaceFixture::ControllerBoardInterfaceFixture()
//                                   : localmock_work(localmock_iosvc),
//	                                 , mock_thread(std::bind(static_cast<std::size_t(boost::asio::io_service::*)(void)>(&boost::asio::io_service::run), &localmock_iosvc))
//	                                 , pmockCbi(new Mocking::ControllerBoardInterfaceMock(localmock_iosvc, serial_dev_identifier, data_dev_identifier))
//	                                 , pCbi(std::dynamic_pointer_cast<ControllerBoardInterface>(pmockCbi))
//                                    //, prx_buff(&rx_buff), ptx_buff(&tx_buff), 
//	{
//		pcv_cb_assgined.reset(new std::condition_variable());
//	}

	void ControllerBoardInterfaceFixture::SetUp()
	{
		EXPECT_CALL(*pmockCbi, WriteRegister(_, _, _))
			.WillRepeatedly(DoAll(SaveArgPtr<1, ControllerBoardInterface::t_txrxbuf>(&ptx_buff),
                                 SaveArgFncPtr<2, ControllerBoardInterface::t_xferCallback>(&pwrite_reg_cb),
                                 Return(MAKE_ERRC(boost::system::errc::success))));

		EXPECT_CALL(*pmockCbi, ReadRegister(_, _, _, _))
			.WillRepeatedly(DoAll(SaveArg<1>(&rx_length),
                                  SaveArgPtr<2, ControllerBoardInterface::t_txrxbuf>(&prx_buff),
                                  SaveArgFncPtr<3, ControllerBoardInterface::t_xferCallback>(&pread_reg_cb),
                                  Return(MAKE_ERRC(boost::system::errc::success))));

		EXPECT_CALL(*pmockCbi, GetBoardStatus())
			.WillRepeatedly(ReturnRef(board_status));
		EXPECT_CALL(*pmockCbi, GetErrorStatus())
			.WillRepeatedly(ReturnRef(error_status));
		EXPECT_CALL(*pmockCbi, GetHostCommError())
			.WillRepeatedly(ReturnRef(comm_error));
		EXPECT_CALL(*pmockCbi, GetSignalStatus())
			.WillRepeatedly(ReturnRef(signal_status));
	}

	void ControllerBoardInterfaceFixture::TearDown()
	{
		pLocalmock_iosvc->stop();
		pmockCbi.reset();
		pcv_cb_assgined.reset();
	}

	std::cv_status ControllerBoardInterfaceFixture::cv_cb_wait_unill_sec(uint32_t sec) const
	{
		std::mutex mtx_cv;
		std::unique_lock<std::mutex> lck(mtx_cv);
		return pcv_cb_assgined->wait_until(lck, std::chrono::system_clock::now() + std::chrono::seconds(sec));
	}


}
