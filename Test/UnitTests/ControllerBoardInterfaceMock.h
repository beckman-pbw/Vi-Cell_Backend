#pragma once
#include <gmock/gmock.h>
#include <boost/asio.hpp>

#include <ftd2xx.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

#include "BoardStatus.hpp"
#include "ControllerBoardInterface.hpp"
#include "ErrcHelpers.hpp"
#include "ErrorStatus.hpp"
#include "SignalStatus.hpp"

namespace Mocking
{
	using boostec=boost::system::error_code ;
	class  ControllerBoardInterfaceMock : public ControllerBoardInterface
	{
	public:
		ControllerBoardInterfaceMock( std::shared_ptr<boost::asio::io_service> iosvc, const std::string& _serialDevIdentifier, const std::string& _dataDevIdentifier);
		virtual ~ControllerBoardInterfaceMock();

		MOCK_METHOD1(fakeWriteRegister, boostec(uint32_t regAddr));
		MOCK_METHOD0(Initialize, bool());
		MOCK_METHOD2(OpenSerial, boost::system::error_code(std::string identifier, uint32_t baudRate));
		MOCK_METHOD1(OpenGPIO, boost::system::error_code(std::string identifier));
		MOCK_METHOD0(IsSerialOpen, bool());
		MOCK_METHOD0(IsGPIOOpen, bool() );
		MOCK_METHOD0(ReadVersion, void());
		MOCK_METHOD0(GetVersion, std::string&());
		MOCK_METHOD0(CloseSerial, void());
		MOCK_METHOD0(CloseGPIO, void());
		MOCK_METHOD0(Close, void());
		MOCK_METHOD0(MaxTxRxSize, std::size_t());
		MOCK_METHOD0(CancelQueue, void());

		MOCK_METHOD0(GetBoardStatus, BoardStatus&());
		MOCK_METHOD0(GetErrorStatus, ErrorStatus&());
		MOCK_METHOD0(GetHostCommError, uint32_t&());
		MOCK_METHOD0(GetSignalStatus, SignalStatus&());

		MOCK_METHOD3(WriteRegister, boostec(uint32_t regAddr, t_ptxrxbuf tx, t_xferCallback Cb));

		MOCK_CONST_METHOD0(GetBoardHardwareLevel, uint16_t());
		MOCK_METHOD0(isReleasedVersion, bool() );
		MOCK_METHOD3(SetGPIO, boost::system::error_code (bool p1, bool p2, bool p3));
		MOCK_METHOD3(GetGPIO, boost::system::error_code (bool& p1, bool& p2, bool& p3) );
		MOCK_METHOD2(SetGPIO, boost::system::error_code (std::size_t pinNum, bool pinVal) );
		MOCK_METHOD2(GetGPIO, boost::system::error_code (std::size_t pinNum, bool& pinVal) );
		MOCK_METHOD1(EnableK70FirmwareUpdate, bool(boost::system::error_code& ec) );
		MOCK_METHOD1(DisableK70FirmwareUpdate, bool(boost::system::error_code& ec) );
		MOCK_METHOD3(ProgramFirmwareK70, bool(uint32_t& address, std::vector<uint8_t>& data, boost::system::error_code& ec) );
		MOCK_METHOD1(DisconnectFirmwareK70, bool(boost::system::error_code& ec) );
		MOCK_METHOD0(StartProcessingQueue, void());

		
		MOCK_METHOD4(ReadRegister, boostec(uint32_t regAddr, uint32_t rxLen, t_ptxrxbuf rx, t_xferCallback Cb));
		
	};

}
