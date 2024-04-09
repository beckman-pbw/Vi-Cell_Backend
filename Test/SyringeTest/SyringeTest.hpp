#pragma once

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "stdafx.h"

#include "Camera_Basler.hpp"
#include "ControllerBoardInterface.hpp"
#include "LedControllerBoardCommand.hpp"
#include "LedControllerBoardInterface.hpp"


class SyringeTest
{
public:
	SyringeTest();
	virtual ~SyringeTest();

//TODO?	bool init (std::string controlBoardSerialNumber);
	bool init();
	void quit();
	void armTrigger();
	void waitForImage();
	void onCameraTrigger();
	void onCameraTriggerTimeout();
	void getLedControllerBoardStatus (LedControllerBoardCommand& lcbc);
	std::shared_ptr<ControllerBoardInterface> getCBI() { return pCBI_; }
	std::shared_ptr<LedControllerBoardInterface> getLCBI() { return pLCBI_; }
	void testErrorDecoding();

private:
	void signalHandler (const boost::system::error_code& ec, int signal_number);

	std::shared_ptr<boost::asio::signal_set> pSignals_;
	std::shared_ptr<boost::asio::io_service> pLocalIosvc_;
	std::shared_ptr<boost::asio::io_service::work> pLocalWork_;
	std::shared_ptr<std::thread> pThread_;
	std::shared_ptr<ControllerBoardInterface> pCBI_;
	std::shared_ptr<LedControllerBoardInterface> pLCBI_;
	std::shared_ptr <Camera_Basler> camera_;
};
