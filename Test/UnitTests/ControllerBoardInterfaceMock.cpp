#include "stdafx.h"
#include "ControllerBoardInterfaceMock.h"

namespace Mocking
{

	ControllerBoardInterfaceMock::ControllerBoardInterfaceMock( std::shared_ptr<boost::asio::io_service> iosvc, const std::string& _serialDevIdentifier,
                                                                const std::string& _dataDevIdentifier)
		: ControllerBoardInterface(iosvc, _serialDevIdentifier, _dataDevIdentifier)
	{
	}

	ControllerBoardInterfaceMock::~ControllerBoardInterfaceMock()
	{
	}

	//boostec ControllerBoardInterfaceMock::WriteRegister(uint32_t regAddr, t_ptxrxbuf tx, t_xferCallback Cb)
	//{
	//	ptx_buff = tx;
	//	pwrite_reg_cb.reset(&Cb);
	//	fakeWriteRegister(regAddr);

	//	return MAKE_ERRC(boost::system::errc::success);
	//}

}
