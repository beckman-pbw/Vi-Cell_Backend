#pragma once

#include <iostream>
#include <atomic>

#include "HawkeyeError.hpp"
#include "ImageWrapperDLL.hpp"
#include "LedBase.hpp"
#include "DeadlineTimerUtilities.hpp"

class LiveScanningUtilities
{
public:
	typedef std::function<void (HawkeyeError, ImageWrapperDLL)> service_live_image_callback_DLL;

	static LiveScanningUtilities& Instance()
	{
		static LiveScanningUtilities instance;
		return instance;
	}

	void Initialize (std::shared_ptr<boost::asio::io_context> pWorkerIoSvc);
	virtual ~LiveScanningUtilities();

	HawkeyeError StartLiveScanning (service_live_image_callback_DLL callBack);
	HawkeyeError StopLiveScanning();
	bool IsBusy() const;

private:

	std::shared_ptr<boost::asio::io_context> pWorkerIoSvc_;
	std::atomic<bool> isBusy_;
	service_live_image_callback_DLL callback_;
	HawkeyeConfig::LedType ledType_;
	uint32_t exposureTime_usec_;
	DeadlineTimerUtilities timer_;

	void captureFrame();
	void onCameraTrigger(cv::Mat image);
	void waitForNextframe();
};
