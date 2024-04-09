#pragma once
#include <gtest/gtest.h>
#include <memory>

#include "ControllerBoardInterface.hpp"
#include "ControllerBoardInterfaceMock.h"
#include "RfidTestFixture.h"

namespace RfidTests
{
	class RfidTestCases : public HawkeyeTestFixtures::RfidFixture
	{
	public:
		RfidTestCases();
		Rfid utest_Rfid;

	protected:		
		virtual void SetUp();
		virtual void TearDown();
	};

}
