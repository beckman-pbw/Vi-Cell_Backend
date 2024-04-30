#include "stdafx.h"

#include "BeckmanCamera.hpp"
#include "ErrorCode.hpp"
#include "Registers.hpp"
#include "ChronoUtilities.hpp"
#include "DeadlineTimerUtilities.hpp"
#include "SystemErrors.hpp"
#include "CameraErrorLog.hpp"
#include "Hardware.hpp"

static const char MODULENAME[] = "Camera";

#define CB_CAM_SM_TIMEOUT    1000


//*****************************************************************************
BeckmanCamera::BeckmanCamera(std::shared_ptr<CBOService> pCBOService)
	: CameraBase (pCBOService),
	  imageCount_(0),
	  darkImageCount_(0)
{
	auto config = HawkeyeConfig::Instance().get().cameraType;
	pCamera_ = Hardware::createCamera(config);
}

//*****************************************************************************
BeckmanCamera::~BeckmanCamera()
{
	pCamera_.reset();
}

void BeckmanCamera::shutdown()
{
	if (pCamera_ != nullptr)
	{
		pCamera_->shutdown();
	}
}

//*****************************************************************************
bool BeckmanCamera::setGain(double gain)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	bool retStatus = pCamera_->setGain(gain);
	if (!retStatus)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror,
			instrument_error::severity_level::error));
	}
	return retStatus;
}

//*****************************************************************************
bool BeckmanCamera::setGainLimitsState(bool state)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	bool retStatus = pCamera_->setGainLimitsState(state);
	if (!retStatus)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror, 
			instrument_error::severity_level::error));
	}
	return retStatus;
}

//*****************************************************************************
bool BeckmanCamera::getGainRange(double& minGain, double& maxGain)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	bool retStatus = pCamera_->getGainRange(minGain, maxGain);
	if (!retStatus)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror, 
			instrument_error::severity_level::error));
	}
	return retStatus;
}

//*****************************************************************************
bool BeckmanCamera::getImageSize(uint32_t& width, uint32_t& height)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	bool retStatus = pCamera_->getImageSize(width, height);
	if (!retStatus)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror, 
			instrument_error::severity_level::error));
	}
	return retStatus;
}

//*****************************************************************************
bool BeckmanCamera::getImageInfo(iCamera::ImageInfo& imageInfo)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	bool retStatus = pCamera_->getImageInfo(imageInfo);
	if (!retStatus)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror, 
			instrument_error::severity_level::error));
	}
	return retStatus;
}

//*****************************************************************************
std::string BeckmanCamera::getVersion()
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);
	return pCamera_->getVersion();
}

bool BeckmanCamera::isPresent()
{
	return pCamera_ != nullptr && pCamera_->isPresent();
}


//*****************************************************************************
void BeckmanCamera::initializeAsync(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "initialize: <enter>");
	if (!pCamera_->initialize())
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_initerror,
			instrument_error::severity_level::error));
		CameraErrorLog::Log("Camera_initializeAsync::Failed to initialize camera");
		Logger::L().Log (MODULENAME, severity_level::critical, "Camera failed to initialize");
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	if (HawkeyeConfig::Instance().get().cameraType == HawkeyeConfig::CameraType::Basler)
	{
		if (dynamic_cast<Camera_Basler*>(pCamera_.get()))
		{
			pCBOService_->enqueueInternal(callback, true);
			Logger::L().Log (MODULENAME, severity_level::debug1, "initialize: <exit, success>");
			return;
		}
		else
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_camera_connection,
				instrument_error::severity_level::error));
			CameraErrorLog::Log("Camera_initializeAsync::Camera not connected");
			Logger::L().Log (MODULENAME, severity_level::critical, "Camera not connected");
		}
	}
	else if (HawkeyeConfig::Instance().get().cameraType == HawkeyeConfig::CameraType::Allied)
	{
		if (dynamic_cast<Camera_Allied*>(pCamera_.get()))
		{
			pCBOService_->enqueueInternal(callback, true);
			Logger::L().Log(MODULENAME, severity_level::debug1, "initialize: <exit, success>");
			return;
		}
		else
		{
			ReportSystemError::Instance().ReportError(BuildErrorInstance(
				instrument_error::imaging_camera_connection,
				instrument_error::severity_level::error));
			CameraErrorLog::Log("Camera_initializeAsync::Camera not connected");
			Logger::L().Log(MODULENAME, severity_level::critical, "Camera not connected");
		}
	}
	else
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_connection, 
			instrument_error::severity_level::error));
		CameraErrorLog::Log("Camera_initializeAsync::Unknown camera type" + (uint8_t)(HawkeyeConfig::Instance().get().cameraType));
		Logger::L().Log (MODULENAME, severity_level::critical, "Unknown camera type: " + (uint8_t)(HawkeyeConfig::Instance().get().cameraType));
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "initialize: <exit, failure>");
	pCBOService_->enqueueInternal(callback, false);
}

//*****************************************************************************
void BeckmanCamera::captureSingleFrame(
	uint32_t exposureTime_usec,
	HawkeyeConfig::LedType ledType, 
	std::function<void(cv::Mat)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	const uint8_t retryCount = 2;	// Retries Count = 2, that means Total tries = 3
	auto pTakePictureParams = std::make_shared<takePictureParams>();
	pTakePictureParams->exposureTime_usec = exposureTime_usec;
	pTakePictureParams->ledType = ledType;
	pTakePictureParams->perImageCallback = callback;
	pTakePictureParams->retryCount = retryCount;
	pTakePictureParams->isImageCaptured = false;

	takePictureInternal(eTakePictureStates::eEntryPoint, pTakePictureParams);
}

//*****************************************************************************
void BeckmanCamera::captureMultipleFrame(
	uint32_t exposureTime_usec,
	HawkeyeConfig::LedType ledType,
	uint32_t numFrames, 
	uint8_t frameRatePerSecond,
	std::function<void(cv::Mat)> perImageCallack, 
	std::function<void(bool)> completionCallback)
{
	HAWKEYE_ASSERT (MODULENAME, perImageCallack);
	HAWKEYE_ASSERT (MODULENAME, completionCallback);

	const uint8_t retryCount = 2;	// Retries Count = 2, that means Total tries = 3
	takePictureParams params = {};
	params.exposureTime_usec = exposureTime_usec;
	params.ledType = ledType;
	params.retryCount = retryCount;
	params.perImageCallback = perImageCallack;
	params.isImageCaptured = false;

	double minimumTimePerImage_ms = (1000.0 / (double)frameRatePerSecond) /*1 sec = 1000 millisecond*/;
	Logger::L().Log (MODULENAME, severity_level::debug1, "captureMultipleFrame : minimum time(ms) for each picture : " + std::to_string(minimumTimePerImage_ms));

	const double offset = 200.0;		// 200 milliseconds offset
	const double timeRequiredMillisec = (numFrames * minimumTimePerImage_ms) + offset;

	auto startTime = ChronoUtilities::CurrentTime();
	takeMultiplePicturesInternal (params, [this, completionCallback, startTime, timeRequiredMillisec](bool status)
	{
		auto endTime = ChronoUtilities::CurrentTime();
		double timeTakenMillisec = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

		Logger::L().Log (MODULENAME, severity_level::notification, boost::str(boost::format("captureMultipleFrame :\nTimeRequired: %d ms\nTimeTaken: %d ms") 
		                                                                                   % timeRequiredMillisec % timeTakenMillisec));

		if (timeTakenMillisec > timeRequiredMillisec)
		{
// Do not post this error in debug builds - development platform timing is
// too different than "real world" timing for this to be valid.
#ifndef _DEBUG
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_general_timeout, 
				instrument_error::severity_level::notification));
#endif
		}

		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "captureMultipleFrame : Failed to capture images!!!");
		}
		pCBOService_->enqueueInternal(completionCallback, status);

	}, numFrames, static_cast<uint64_t>(minimumTimePerImage_ms));
}

//*****************************************************************************
bool BeckmanCamera::getImage(Mat& image) const
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	bool retStatus = pCamera_->getImage(image);
	if (!retStatus)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror, 
			instrument_error::severity_level::error));
	}
	return retStatus;
}

//*****************************************************************************
bool BeckmanCamera::fireTrigger (uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType)
{
	HAWKEYE_ASSERT (MODULENAME, pCamera_);

	Logger::L().Log (MODULENAME, severity_level::debug2, "fireTrigger: <before>");

	auto onCameraTriggerComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
	{
		if (cbData.status != ControllerBoardOperation::eStatus::Success)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("fireTrigger: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_trigger_hardwareerror, 
				instrument_error::severity_level::error));
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::debug2, "fireTrigger: <exit>");
	};

	const uint32_t framesPerSec = 1u;
	const uint32_t totalNumOfFrames = 1u;
	const uint32_t led_type = (1u << (static_cast<uint32_t>(ledType) - 1u));

	auto tid = pCBOService_->CBO()->Execute(
		AssertCameraTriggerOperation(exposureTime_usec, framesPerSec, totalNumOfFrames, led_type),
		CB_CAM_SM_TIMEOUT, onCameraTriggerComplete);
	if (!tid)
	{
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("AssertCameraTrigger, task %d") % (*tid)));
	return true;
}

// ? target flux ?


//*****************************************************************************
void BeckmanCamera::takePictureInternal (eTakePictureStates currentState, std::shared_ptr<takePictureParams> pTakePictureParams)
{
	HAWKEYE_ASSERT (MODULENAME, pTakePictureParams);
	HAWKEYE_ASSERT (MODULENAME, pTakePictureParams->perImageCallback);

	auto onCurrentStateComplete = [this, pTakePictureParams](eTakePictureStates nextState)
	{
		pCBOService_->enqueueInternal([=]()
		{
			takePictureInternal(nextState, pTakePictureParams);
		});
	};

	Logger::L().Log (MODULENAME, severity_level::debug2, "takePictureInternal : Current state is : " + std::to_string((int)currentState));

	switch (currentState)
	{
		case eTakePictureStates::eEntryPoint:
		{
			if (!pCamera_)
			{
				CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eEntryPoint :Camera Object is empty");
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eEntryPoint : Invalid camera object!");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::imaging_camera_initerror, 
					instrument_error::severity_level::error));
				onCurrentStateComplete(eTakePictureStates::eError);
				return;
			}

			// set capture frame capture callback
			bool status = pCamera_->setOnTriggerCallback([this, onCurrentStateComplete, pTakePictureParams, tempRetryCount = pTakePictureParams->retryCount]() -> void
			{
				if (tempRetryCount > pTakePictureParams->retryCount)
				{
					// If "tempRetryCount" is greater than <shared_ptr> "pTakePictureParams->retryCount" that means camera is disconnected
					// or some error has occurred after the image fire trigger was success.
					// Since camera is disconnected and "takePictureInternal" method has already started retry process with state "eTakePictureStates::eRetryOnFailure"
					// So no need to trigger the "eTakePictureStates::eComplete" state now
					return;
				}
				pTakePictureParams->isImageCaptured = true;
				onCurrentStateComplete(eTakePictureStates::eComplete);
			});

			if (!status)
			{
				CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eEntryPoint : Failed to set camera trigger callback");
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eEntryPoint : Failed to set camera trigger callback!");
				onCurrentStateComplete(eTakePictureStates::eError);
				return;
			}

			// Intentional fall thru
		}
		case eTakePictureStates::eArmTrigger:
		{
			if (!pCamera_->armTrigger(HawkeyeConfig::Instance().get().cameraTriggerTimeout_ms, pTakePictureParams->exposureTime_usec))
			{
				CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eArmTrigger : Failed to arm trigger");
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eArmTrigger : Failed to arm trigger : (milliseconds)" + std::to_string(HawkeyeConfig::Instance().get().cameraTriggerTimeout_ms));
				onCurrentStateComplete(eTakePictureStates::eError);
				return;
			}
			// Intentional fall thru
		}
		case eTakePictureStates::eFireTrigger:
		{
			if (!fireTrigger(pTakePictureParams->exposureTime_usec, pTakePictureParams->ledType))
			{
				CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eFireTrigger : Failed to fire trigger");
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eFireTrigger : Failed to fire trigger");
				onCurrentStateComplete(eTakePictureStates::eError);
				return;
			}
			onCurrentStateComplete(eTakePictureStates::eWaitForImage);
			return;
		}
		case eTakePictureStates::eWaitForImage:
		{
			if (!pCamera_->waitForImage(HawkeyeConfig::Instance().get().cameraImageCaptureTimeout_ms))
			{
				CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eWaitForImage : Failed to grab current image frame");
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eWaitForImage : Failed to grab current frame");
				onCurrentStateComplete(eTakePictureStates::eRetryOnFailure);
				return;
			}
			
			// If "waitForImage" is success that means "pCamera_->setOnTriggerCallback()"
			// will be triggered which will call the "eTakePictureStates::eComplete" state
			// Simply return from here.
			return;
		}
		case eTakePictureStates::eRetryOnFailure:
		{
			if (pTakePictureParams->isImageCaptured)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eRetryOnFailure : Image is already captured, No need for retry now");
				// "setOnTriggerCallback" callback handler will trigger the "eTakePictureStates::eComplete" event
				return;
			}

			if (pTakePictureParams->retryCount == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eRetryOnFailure : Ran out of retries to image capture!");

				// Instead of failure, just continue with a dark image and let the algorithm deal with it.
				pCBOService_->enqueueInternal(pTakePictureParams->perImageCallback, lastImageCaptured_);
				return;
			}

			if (!Camera_Basler::isCameraConnected())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eRetryOnFailure : Camera not connected");
				onCurrentStateComplete(eTakePictureStates::eError);
				return;
			}

			pTakePictureParams->retryCount--;
			CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eRetryOnFailure :Failed to capture image. Retrying");
			Logger::L().Log (MODULENAME, severity_level::normal, "takePictureInternal : eRetryOnFailure : Retrying image capture <retries left>: " + std::to_string(pTakePictureParams->retryCount));
			onCurrentStateComplete(eTakePictureStates::eEntryPoint);
			return;
		}
		case eTakePictureStates::eComplete:
		{
			cv::Mat img = cv::Mat();
			if (getImage(img))
			{

				this->imageCount_++;

				// Cause of this issue appears to be that the LED did not flash when triggered.
				// This leads to a VERY DARK but not ENTIRELY BLACK image.  We can pick these up by 
				// calculating the mean pixel value - it'll be LOW. "50" here is right around 20% of full-scale - 
				// Much brighter than a "dark" image (nominally a "10" or so) but far darker than a normal
				// image from the system (a 15M cells/ml sample is still >120 mean).
				auto m = cv::mean(img);
				if (m[0] < 50)
				{
					this->darkImageCount_++;
					CameraErrorLog::Log(boost::str(boost::format("Camera_takePictureInternal_eTakePictureStates::eComplete : Dark image is captured\n(Total: %d   Dark: %d")
						                                            % this->imageCount_.load() % this->darkImageCount_.load()));
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_general_imagequality, 
						instrument_error::severity_level::notification));

					// Attempt a retry on the dark images.
					pTakePictureParams->isImageCaptured = false;
					lastImageCaptured_ = img;
					onCurrentStateComplete(eTakePictureStates::eRetryOnFailure);
				}
				else
				{
					pCBOService_->enqueueInternal(pTakePictureParams->perImageCallback, img);
				}
				
				return;
			}

			CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eComplete : Failed to get Image");
			// Intentional fall thru
		}
		case eTakePictureStates::eError:
		default:
		{
			CameraErrorLog::Log("Camera_takePictureInternal_eTakePictureStates::eError :Failed to capture image");
			Logger::L().Log (MODULENAME, severity_level::error, "takePictureInternal : eError : Failed to get image!");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_camera_hardwareerror, 
				instrument_error::severity_level::warning));
			pCBOService_->enqueueInternal(pTakePictureParams->perImageCallback, cv::Mat{});
			return;
		}
	}

	// unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void BeckmanCamera::takeMultiplePicturesInternal(
	takePictureParams params,
	std::function<void(bool)>completionCallback,
	uint32_t framesLeftToCapture,
	uint64_t minimumTimePerImageMillisec)
{
	HAWKEYE_ASSERT (MODULENAME, params.perImageCallback);
	HAWKEYE_ASSERT (MODULENAME, completionCallback);
	HAWKEYE_ASSERT (MODULENAME, minimumTimePerImageMillisec > 0);

	// Trigger completion callback when no frames left to capture
	if (framesLeftToCapture == 0)
	{
		pCBOService_->enqueueInternal(completionCallback, true);
		return;
	}

	auto onTakePictureComplete = [=](bool status)
	{
		if (!status)
		{
			pCBOService_->enqueueInternal(completionCallback, false);
			return;
		}

		// Trigger next frame if frames remaining, otherwise trigger completion callback
		auto nextCount = framesLeftToCapture - 1;
		if (nextCount > 0)
		{
			pCBOService_->enqueueInternal([=]()
				{
					takeMultiplePicturesInternal(params, completionCallback, (framesLeftToCapture - 1), minimumTimePerImageMillisec);
				});
		}
		else
		{
			// Directly invoke.
			completionCallback(status);
		}
	};

	auto isPictureTaken = std::make_shared<bool>(false);
	auto timer = std::make_shared<DeadlineTimerUtilities>();
	timer->wait (pCBOService_->getInternalIosRef(), minimumTimePerImageMillisec,
				[this, timer, onTakePictureComplete, isPictureTaken](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "takeMultiplePicturesInternal : Timer was cancelled explicitly!");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_camera_timeout,
				instrument_error::severity_level::error));
		}
		if (*isPictureTaken)
		{
			onTakePictureComplete(status);
		}
	});

	auto pTakePictureParams = std::make_shared<takePictureParams>();
	*pTakePictureParams = params;
	pTakePictureParams->perImageCallback = [this, params, timer, onTakePictureComplete, isPictureTaken](cv::Mat image)
	{
		if (image.empty())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "takeMultiplePicturesInternal : Failed to capture image!");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_camera_noimage, 
				instrument_error::severity_level::warning));
			onTakePictureComplete(false);
			return;
		}

		pCBOService_->enqueueInternal(params.perImageCallback, image);

		if (!timer->isRunning())
		{
			Logger::L().Log (MODULENAME, severity_level::notification, "takeMultiplePicturesInternal : Frame capture took more time than expected");
			onTakePictureComplete(true);
			return;
		}

		*isPictureTaken = true;
	};

	takePictureInternal(eTakePictureStates::eEntryPoint, pTakePictureParams);
}
