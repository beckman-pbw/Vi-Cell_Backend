#pragma once
#include <gtest/gtest.h>
#include "CBITestFixture.h"

namespace HawkeyeTestFixtures
{
	class RfidFixture : public ControllerBoardInterfaceFixture
	{
	public:
		RfidFixture();
		
		std::condition_variable cv_cmd_finished;

		boost::asio::io_service localtest_iosvc;
		boost::asio::io_service::work localtest_work;
		boost::thread test_thread;

		void testseq_rfid_readregister_data();

	protected:		
		virtual void SetUp();
		virtual void TearDown();
	};
}
