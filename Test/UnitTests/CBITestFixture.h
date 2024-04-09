#pragma once
#include <gtest/gtest.h>
#include <memory>

#include "ControllerBoardInterface.hpp"
#include "ControllerBoardInterfaceMock.h"
namespace HawkeyeTestFixtures
{
	/**
	 * \brief 
	 */
	class ControllerBoardInterfaceFixture : public ::testing::Test
	{
	public:
		ControllerBoardInterfaceFixture();

		static std::shared_ptr<std::condition_variable> pcv_cb_assgined;
		std::shared_ptr<boost::asio::io_service> pLocalmock_iosvc;
        std::shared_ptr<boost::asio::io_service::work> pLocalmock_work;
		boost::thread mock_thread;

		std::string serial_dev_identifier;
		std::string data_dev_identifier;

		std::shared_ptr<Mocking::ControllerBoardInterfaceMock> pmockCbi;
		std::shared_ptr<ControllerBoardInterface> pCbi;
		ControllerBoardInterface::t_xferCallback pwrite_reg_cb;
		ControllerBoardInterface::t_xferCallback pread_reg_cb;
		ControllerBoardInterface::t_ptxrxbuf ptx_buff;
		ControllerBoardInterface::t_ptxrxbuf prx_buff;
		ControllerBoardInterface::t_txrxbuf tx_buff;
		ControllerBoardInterface::t_txrxbuf rx_buff;
		uint32_t rx_length;

		BoardStatus  board_status;
		ErrorStatus  error_status;
		uint32_t     comm_error;
		SignalStatus signal_status;


	protected:		
		virtual void SetUp();
		virtual void TearDown();
		std::cv_status cv_cb_wait_unill_sec(uint32_t sec) const;
	};

	

	/**
	*\brief -df
	*/
	ACTION_TEMPLATE(SaveArgPtr,
					// Note the comma between int and k:
					HAS_2_TEMPLATE_PARAMS(int, k, typename, T),
					AND_1_VALUE_PARAMS(output))
	{
		(*output) = std::dynamic_pointer_cast<T>(::testing::get<k>(args));
	}

	/**
	*\brief -df
	*/
	ACTION_TEMPLATE(SaveArgFncPtr,
					// Note the comma between int and k:
					HAS_2_TEMPLATE_PARAMS(int, k, typename, T),
					AND_1_VALUE_PARAMS(output))
	{
		(*output) = static_cast<T>(::testing::get<k>(args));
		ControllerBoardInterfaceFixture::pcv_cb_assgined->notify_all();
	}
}
