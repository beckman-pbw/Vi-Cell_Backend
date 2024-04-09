#include "stdafx.h"

#include "CameraBase.hpp"

static const char MODULENAME[] = "CameraBase";

//*****************************************************************************
CameraBase::CameraBase (std::shared_ptr<CBOService> pCBOService)
	:pCBOService_(std::move(pCBOService))
	, cameraQueueHandler_(pCBOService_)
{ }

//*****************************************************************************
CameraBase::~CameraBase()
{ }

//*****************************************************************************
void CameraBase::initialize(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCompletionCallback = cameraQueueHandler_.wrapCompletionCallback(callback);
	cameraQueueHandler_.enqueueRequest([this, wrappedCompletionCallback]()
	{
		initializeAsync(wrappedCompletionCallback);
	});
}

//*****************************************************************************
void CameraBase::takePicture (
	uint32_t exposureTime_usec,
	HawkeyeConfig::LedType ledType,
	std::function<void(cv::Mat)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrappedCompletionCallback = cameraQueueHandler_.wrapCompletionCallback(callback);
	cameraQueueHandler_.enqueueRequest([this, wrappedCompletionCallback, exposureTime_usec, ledType]()
	{
		captureSingleFrame (exposureTime_usec, ledType, wrappedCompletionCallback);
	});
}

//*****************************************************************************
void CameraBase::takePictures(
	uint32_t exposureTime_usec,
	HawkeyeConfig::LedType ledType,
	uint32_t numFrames,
	uint8_t frameRatePerSecond,
	std::function<void(cv::Mat)> perImageCallack,
	std::function<void(bool)> completionCallback)
{
	HAWKEYE_ASSERT (MODULENAME, perImageCallack);
	HAWKEYE_ASSERT (MODULENAME, completionCallback);

	auto wrappedPerImageCallback = [this, perImageCallack](cv::Mat image)
	{
		pCBOService_->enqueueExternal (perImageCallack, image);
	};

	auto wrappedCompletionCallback = cameraQueueHandler_.wrapCompletionCallback(completionCallback);

	cameraQueueHandler_.enqueueRequest(
		[this, wrappedPerImageCallback, wrappedCompletionCallback, exposureTime_usec, ledType, numFrames, frameRatePerSecond]()
	{
		captureMultipleFrame (exposureTime_usec, ledType, numFrames, frameRatePerSecond, wrappedPerImageCallback, wrappedCompletionCallback);
	});
}

void CameraBase::shutdown()
{
	
}

