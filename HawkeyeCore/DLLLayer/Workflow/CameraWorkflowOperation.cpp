#include "stdafx.h"

#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "CameraWorkflowOperation.hpp"
#include "Helper/AdjustBackgroundIntensity.hpp"
#include "Hardware.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "CameraWorkflowOperation";

//*****************************************************************************
void CameraWorkflowOperation::executeInternal(
	Wf_Operation_Callback onCompleteCallback)
{
	std::string logStr = getTypeAsString() + ": ";	
	std::string errStr = logStr;

	switch (cmd_.operation)
	{
		case SetGain:
		{
			logStr.append(std::to_string(cmd_.gain));

			const auto& camera = Hardware::Instance().getCamera();
			bool success = camera->setGain(cmd_.gain);
			if (!success)
			{
				errStr.append("Unable to set gain : " + std::to_string(cmd_.gain));
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, false);
			}
			break;
		}

		case TakePicture:
		{
			logStr.append("exposure time: " + std::to_string(cmd_.exposureTime_usec));

			auto& led = Hardware::Instance().getLed (HawkeyeConfig::LedType::LED_BrightField);
			float power = led->getConfig()->percentPower;

			led->setPower(power, [this, onCompleteCallback, errStr, power, led](bool success)
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, 
						errStr + boost::str (boost::format ("Unable to set %s power to %f")
							% led->getConfig()->name % power));
					triggerCallback(onCompleteCallback, HawkeyeError::eHardwareFault);
					return;
				}

				Hardware::Instance().getCamera()->takePicture(
					cmd_.exposureTime_usec, cmd_.ledType,
					[this, errStr, onCompleteCallback](cv::Mat image)
				{
					pServices_->enqueueExternal (cmd_.takePictureCallback, image);

					if (image.empty())
					{
						Logger::L().Log (MODULENAME, severity_level::error, errStr + "Unable to take picture");
						triggerCallback(onCompleteCallback, HawkeyeError::eHardwareFault);
						return;
					}
					triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
				});
			});
			return;
		}

		case Capture:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "Capture: <begin>");
			mParallelOpMap_.clear();

			auto& led = Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField);
			float power = led->getConfig()->percentPower;

			led->setPower(power, [this, onCompleteCallback, errStr, power, led](bool success)
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, 
						errStr + boost::str(boost::format("Unable to set %s power to %f") 
							% led->getConfig()->name
							% power));
					triggerCallback(onCompleteCallback, HawkeyeError::eHardwareFault);
					return;
				}
				executeCaptureCmd(onCompleteCallback);
			});
			return;
		}

		case ABI:
		{
			logStr.append("Target Intensity : " + std::to_string(cmd_.targetIntensity));

			bool success = false;
			mParallelOpMap_.clear();

			// Only move the syringe if the rate value is greater than zero.
			// This supports testing and debugging.
			if (cmd_.syringeRate != 0)
			{
				uint8_t syringeOpId = 0;
				mParallelOpMap_[syringeOpId] = boost::none;
				auto syringeSetPosCB = [this, syringeOpId](bool success) -> void
				{
					if (!success)
					{
						const auto errorString = boost::str(boost::format(" Unable to set syringe position : volume < %d > speed < %d >") % cmd_.syringeVolumeUL % cmd_.syringeRate);
						Logger::L().Log (MODULENAME, severity_level::error, errorString);
					}
					mParallelOpMap_[syringeOpId] = success;
				};
				Hardware::Instance().getSyringePump()->setPosition (syringeSetPosCB, cmd_.syringeVolumeUL, cmd_.syringeRate);
			}

			AdjustBackgroundIntensity::ABIParams params = { };
			params.ledType = cmd_.ledType;
			params.exposureTime_usec = cmd_.exposureTime_usec;
			params.targetIntensity = cmd_.targetIntensity;
			params.offset = cmd_.targetIntensityOffset;
			params.maxImageCount = HawkeyeConfig::Instance().get().abiMaxImageCount;
			params.pIos = pServices_->getInternalIos();

			if (params.maxImageCount == 0)
			{
				// set to default number if value is zero
				params.maxImageCount = 6; //default max number of images
			}

			auto pABI = std::make_shared<AdjustBackgroundIntensity>(params);
			uint8_t abiOpId = 1;
			mParallelOpMap_[abiOpId] = boost::none;
			
			pABI->execute(
				[this, abiOpId, pABI /*Capture to keep it alive*/](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Unable to execute background intensity operation : Operation Failed!");
				}
				mParallelOpMap_[abiOpId] = status;
			});

			this->waitTillAllOpComplete([this, onCompleteCallback](bool status) -> void
			{
				triggerCallback(onCompleteCallback, status && getParallelOpResult());
			});
			return;
		}
		
		case RemoveGainLimits:
		{
			//TODO:			pCamera->setGainLimitsState (op_.);
			errStr.append("Operation not implemented yet!");
			Logger::L().Log (MODULENAME, severity_level::error, errStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eNotPermittedAtThisTime);
			return;
		}

		default:
		{
			errStr.append("Invalid/Unknown operation!");
			Logger::L().Log (MODULENAME, severity_level::error, errStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eSoftwareFault);
			return;
		}
	}

	if (!logStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, logStr);
	}
	
	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}

//*****************************************************************************
void CameraWorkflowOperation::warmUpLed(uint32_t numImages, std::function<void(bool)> onComplete)
{
	// Warm up images should be taken once the syringe flow has started.
	if (numImages > 0)
	{
		Hardware::Instance().getCamera()->takePicture(
			cmd_.exposureTime_usec, cmd_.ledType, [this, onComplete, numImages](cv::Mat)
		{
			pServices_->enqueueInternal(std::bind(&CameraWorkflowOperation::warmUpLed, this, (numImages - 1), onComplete));
		});
	}
	else
	{
		pServices_->enqueueInternal(onComplete, true);
	}
}

//*****************************************************************************
void CameraWorkflowOperation::executeCaptureCmd(Wf_Operation_Callback onCompleteCallback)
{
	// Only move the syringe if the rate value is greater than zero.
	// This supports testing and debugging.
	if (cmd_.syringeRate != 0)
	{
		uint8_t syringeOpId = 0;
		mParallelOpMap_[syringeOpId] = boost::none;
		auto syringeSetPosCB = [this, syringeOpId](bool success) -> void
		{
			if (!success)
			{
				const auto errorString = boost::str(boost::format(" Unable to set syringe position : volume < %d > speed < %d >") % cmd_.syringeVolumeUL % cmd_.syringeRate);
				Logger::L().Log (MODULENAME, severity_level::error, errorString);
			}
			mParallelOpMap_[syringeOpId] = success;
		};

		Hardware::Instance().getSyringePump()->setPosition (syringeSetPosCB, cmd_.syringeVolumeUL, cmd_.syringeRate);
	}
	
	uint8_t cameraOpId = 1;
	mParallelOpMap_[cameraOpId] = boost::none;

	Logger::L().Log (MODULENAME, severity_level::debug1, "Warming Led with: " + std::to_string(cmd_.numWarmupImages) + " images");
	warmUpLed(cmd_.numWarmupImages, [=](bool warmUpSuccess)
	{
		if (!warmUpSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Capture: Failed to warm up led");
			mParallelOpMap_[cameraOpId] = false;
			return;
		}
		
		Logger::L().Log (MODULENAME, severity_level::debug1, "Capture: <start image capture, capturing: " + std::to_string(cmd_.numImages) + " images>");
		
		Hardware::Instance().getCamera()->takePictures(
			cmd_.exposureTime_usec, cmd_.ledType, cmd_.numImages, cmd_.frameRate,
			cmd_.takePictureCallback, [=](bool status) -> void
		{
			mParallelOpMap_[cameraOpId] = status;
			Logger::L().Log (MODULENAME, severity_level::debug1, "Capture: <end image capture>");
		});
	});

	this->waitTillAllOpComplete([this, onCompleteCallback](bool status)-> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "executeCaptureCmd : Failed to take pictures!!!");
		}
		Logger::L().Log (MODULENAME, severity_level::debug1, "executeCaptureCmd: <exit>");
		triggerCallback(onCompleteCallback, status && getParallelOpResult());
	});
}

//*****************************************************************************
void CameraWorkflowOperation::waitTillAllOpComplete(std::function<void(bool)> onComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	if (mParallelOpMap_.empty())
	{
		onComplete(true);
		return;
	}

	auto timer = std::make_shared<DeadlineTimerUtilities>();
	auto repeatConditionLambda = [this, timer/*Capture timer to keep it alive*/]() -> bool
	{
		// Check if all parallel operations have been completed
		for (const auto& item : mParallelOpMap_)
		{
			// If any item is not completed then return true (so that rechecking occur)
			if (!item.second) // Boost::optional value is still not set
			{
				return true;
			}
		}
		// All the items have been completed, return false to stop timer
		return false;
	};

	bool timerOk = timer->waitRepeat(
		pServices_->getInternalIosRef(),
		boost::posix_time::milliseconds(25),
		onComplete,
		repeatConditionLambda,
		boost::none);

	if (!timerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "waitTillAllOpComplete : unable to start timer!");
		onComplete(false);
	}
}

//*****************************************************************************
bool CameraWorkflowOperation::getParallelOpResult()
{
	if (mParallelOpMap_.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getParallelOpResult : No Parallel Operation found");
		return false;
	}

	for (const auto& item : mParallelOpMap_)
	{
		if (!item.second || !item.second.get())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "getParallelOpResult : Operation was unsuccessful : Id " + std::to_string(item.first));
			return false;
		}
	}
	return true;
}

//*****************************************************************************
std::string CameraWorkflowOperation::getTypeAsString()
{
	std::string operationAsString;
	switch (cmd_.operation)
	{		
		case CameraWorkflowOperation::SetGain:
			operationAsString = "SetGain";
			break;
		case CameraWorkflowOperation::TakePicture:
			operationAsString = "TakePicture";
			break;
		case CameraWorkflowOperation::Capture:
			operationAsString = "Capture";
			break;
		case CameraWorkflowOperation::RemoveGainLimits:
			operationAsString = "RemoveGainLimits";
			break;
		case CameraWorkflowOperation::ABI:
			operationAsString = "AdjustBackgroundIntensity";
			break;
		default:
			operationAsString = "Unknown";
			break;
	}

	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}

//*****************************************************************************
uint8_t CameraWorkflowOperation::getOperation()
{
	return cmd_.operation;
}

//*****************************************************************************
void CameraWorkflowOperation::setImageCount(uint16_t numImages)
{
	cmd_.numImages = numImages;
}

//*****************************************************************************
uint16_t CameraWorkflowOperation::getImageCount()
{
	return cmd_.numImages;
}
