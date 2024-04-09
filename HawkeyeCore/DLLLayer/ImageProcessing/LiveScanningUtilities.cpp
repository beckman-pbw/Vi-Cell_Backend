#include "stdafx.h"

#include "Hardware.hpp"
#include "ImageWrapperUtilities.hpp"
#include "LiveScanningUtilities.hpp"

static const char MODULENAME[] = "LiveScanningUtilities";

void LiveScanningUtilities::Initialize (std::shared_ptr<boost::asio::io_context> pWorkerIoSvc)
{
	if (pWorkerIoSvc_)
		return;

	pWorkerIoSvc_ = pWorkerIoSvc;
	isBusy_ = false;
	ledType_ = HawkeyeConfig::LedType::LED_BrightField;
	exposureTime_usec_ = 50;
}

LiveScanningUtilities::~LiveScanningUtilities()
{
	StopLiveScanning();
	//pWorkerIoSvc_.reset();
}

HawkeyeError LiveScanningUtilities::StartLiveScanning (service_live_image_callback_DLL callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartLiveScanning: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	if (IsBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartLiveScanning: <exit, busy>");
		return HawkeyeError::eBusy;
	}

	isBusy_ = true;
	timer_ = DeadlineTimerUtilities();
	callback_ = callback;

	// this workflow does not require the user id, so do not use the transient user technique
	pWorkerIoSvc_->post(std::bind(&LiveScanningUtilities::captureFrame, this));

	Logger::L().Log (MODULENAME, severity_level::debug1, "StartLiveScanning: <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError LiveScanningUtilities::StopLiveScanning()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StopLiveScanning: <enter>");

	if (!IsBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StopLiveScanning: <exit, not busy>");
		return HawkeyeError::eSuccess;
	}

	isBusy_ = false;
	if (timer_.isRunning())
	{
		timer_.cancel();
	}
	callback_ = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "StopLiveScanning: <exit>");

	return HawkeyeError::eSuccess;
}

bool LiveScanningUtilities::IsBusy() const
{
	return isBusy_.load();
}

void LiveScanningUtilities::captureFrame()
{
	if (IsBusy())
	{
		auto cameraCaptureCallback = [this](cv::Mat image) -> void
		{
			// this workflow does not require the user id, so do not use the transient user technique
			pWorkerIoSvc_->post(std::bind(&LiveScanningUtilities::onCameraTrigger, this, image));
		};
		Hardware::Instance().getCamera()->takePicture (exposureTime_usec_, ledType_, cameraCaptureCallback);
	}
}

void LiveScanningUtilities::onCameraTrigger(cv::Mat image)
{
	if (!IsBusy() || !callback_)
	{
		return;
	}

	auto status = image.empty() ? HawkeyeError::eHardwareFault : HawkeyeError::eSuccess;
	// this workflow does not require the user id, so do not use the transient user technique
	pWorkerIoSvc_->post([this, status, image]() -> void
	{
		if (callback_)
		{
			callback_(status, image);
		}
	});

	if (IsBusy()) {
		// this workflow does not require the user id, so do not use the transient user technique
		pWorkerIoSvc_->post(std::bind(&LiveScanningUtilities::waitForNextframe, this));
	}
}

void LiveScanningUtilities::waitForNextframe()
{
	const uint64_t waitMillisec = 500;
	auto onComplete = [this](bool status) -> void
	{
		if (status && IsBusy())
		{
			// this workflow does not require the user id, so do not use the transient user technique
			pWorkerIoSvc_->post(std::bind(&LiveScanningUtilities::captureFrame, this));
		}
	};
	timer_.wait(*pWorkerIoSvc_, waitMillisec, onComplete);
}