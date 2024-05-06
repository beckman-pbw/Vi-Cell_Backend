#include "stdafx.h"

#include "AdjustBackgroundIntensity.hpp"
#include "CameraErrorLog.hpp"
#include "Hardware.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "ImageWrapperUtilities.hpp"
#include "LegacyData.hpp"

static const char MODULENAME[] = "AdjustBackgroundIntensity";

static const float MaxAllowedLedPower = 100.0f;
static const float MinAllowedLedPower = 0.0f;
static const float DeadBandLimit = 0.10f;
static std::string dateTime;
std::string abiLogStr;
std::vector<std::pair<float, float>> abi_log_values;

static inline bool isEqual(float val1, float val2)
{
	return std::abs(val1 - val2) <= 0.001f;
}

AdjustBackgroundIntensity::AdjustBackgroundIntensity (ABIParams params)
	: params_(params)
{
	HAWKEYE_ASSERT (MODULENAME, params_.pIos);

	averageIntensity_ = boost::none;
	imageCnt_ = 0;
	isBusy_ = false;
	maxImageIntensity_ = 255;
	minImageIntensity_ = 0;
}

void AdjustBackgroundIntensity::execute(std::function<void(bool)> onCompleteCallback)
{
	HAWKEYE_ASSERT (MODULENAME, onCompleteCallback);

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute ABI : current instance is busy performing other task!");
		onCompleteCallback(false);
		return;
	}

	averageIntensity_ = boost::none;
	imageCnt_ = 0;
	isBusy_ = true;

	// Use this date/time as part of the directory path for all files for this ABI (time is GMT).
	// *dateTime* is also used for the ABI output filename.
	dateTime = ChronoUtilities::ConvertToString (ChronoUtilities::CurrentTime(), "%Y%m%d-%H%M%S");
	abiLogStr = "\nABI sequence started at " + dateTime + "\n";
	abi_log_values.clear();

	executeInternal([this, onCompleteCallback](bool success)
	{
		abiLogStr += boost::str(boost::format("Final ABI Status: %s, Using Background Intensity: %f (%d +/- %d) using %f percent power")
			% (success ? "success" : "failed")
			% averageIntensity_.get_value_or(0)
			% params_.targetIntensity
			% params_.offset
			% Hardware::Instance().getLed(params_.ledType)->getConfig()->percentPower);
			
		Logger::L().Log (MODULENAME, severity_level::normal, abiLogStr);

		if (Logger::L().IsOfInterest(severity_level::debug2))
		{
			// Write data for debugging...
			std::string f = HawkeyeDirectory::Instance().getABIImageDir() + "\\" + dateTime + "\\ABIEventLog.txt";
			FILE* fptr = fopen(f.c_str(), "wt");
			fputs(abiLogStr.c_str(), fptr);
			fclose(fptr);
		}

		// LH6531-5303 : Post warning if ABI finished with final power above 80% of maximum
		{
			const uint32_t LED_INTENSITY_WARNING_LEVEL = 80;
			if (Hardware::Instance().getLed(params_.ledType)->getConfig()->percentPower > LED_INTENSITY_WARNING_LEVEL)
			{
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::imaging_general_backgroundadjusthighpower,
					instrument_error::severity_level::warning));

				Logger::L().Log(MODULENAME, severity_level::warning, "Background adjustment completed near top of power range");
			}
		}
		
		imageCnt_ = 0;
		isBusy_ = false;

		if (!success)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_general_backgroundadjust, 
				instrument_error::severity_level::notification));
		}

		// this workflow does not require the user id, so do not use the transient user technique
		params_.pIos->post(std::bind(onCompleteCallback, success));
	});
}

bool AdjustBackgroundIntensity::isBusy() const
{
	return isBusy_.load();
}

void AdjustBackgroundIntensity::saveABIData (std::string abiImageDir, uint32_t imageCnt, cv::Mat image) {
	
	bool success = createLegacyImageDataDirectories (imageCnt, abiImageDir);
	if (!success)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onAdjustBackgroundIntensityCameraTrigger: Failed to create directories to save image!");
	}

	std::string dataPath;
	success = success && getLegacyDataPath (params_.ledType, dataPath);

	std::string dataType;
	success = success && getLegacyDataType (params_.ledType, dataType);

	if (success)
	{
		std::string imagePath = boost::str(boost::format("%s/%s_%d.png") % dataPath % dataType % imageCnt);
		Logger::L().Log (MODULENAME, severity_level::debug2, "onAdjustBackgroundIntensityCameraTrigger: writing " + imagePath);
		if (!HDA_WriteImageToFile(imagePath, image))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "onAdjustBackgroundIntensityCameraTrigger: Failed to save image to path : " + imagePath);
			success = false;
		}
	}

	if (!success)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::published_errors::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::debug_image,
			instrument_error::severity_level::notification));
		return;
	}
}

void AdjustBackgroundIntensity::onAdjustBackgroundIntensityCameraTrigger(cv::Mat image)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "onAdjustBackgroundIntensityCameraTrigger <enter>");

	if (image.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onAdjustBackgroundIntensityCameraTrigger: error getting image");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_noimage, 
			instrument_error::severity_level::warning));
		return;
	}

	iCamera::ImageInfo info;
	if (!Hardware::Instance().getCamera()->getImageInfo(info))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onAdjustBackgroundIntensityCameraTrigger: unable to get image info from camera");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_general_logicerror, 
			instrument_error::severity_level::warning));
		return;
	}
	maxImageIntensity_ = static_cast<int>(std::pow(2.0, info.bitsPerChannel)) - 1;

	bool generateBGMask = true;
	cv::Mat maskImage = getMaskImage(image, info.bitsPerChannel, generateBGMask);

	averageIntensity_ = 0.0f;
	if (generateBGMask)
	{
		averageIntensity_ = static_cast<float>(cv::mean(image, maskImage)[0]);
	}
	else
	{
		averageIntensity_ = static_cast<float>(cv::mean(image, ~maskImage)[0]);
	}

	if (averageIntensity_.get() > 1.0f)
	{
		Logger::L().Log (
			MODULENAME, severity_level::debug2,
			boost::str(boost::format("onAdjustBackgroundIntensityCameraTrigger:: average background intensity: %f: ") % averageIntensity_));
	}
	else
	{
		CameraErrorLog::Log("AdjustBackgroundIntensity : average image intensity is almost zero(0)");
		Logger::L().Log (
			MODULENAME, severity_level::warning,
			boost::str(boost::format("onAdjustBackgroundIntensityCameraTrigger:: average background intensity: %f: ") % averageIntensity_));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_general_imagequality, 
			instrument_error::severity_level::notification));
	}

	imageCnt_++;

	// Save the ABI data, this gets overwritten every time the ABI is run.
	saveABIData (HawkeyeDirectory::Instance().getABIImageDir(), imageCnt_ - 1, image);

	std::string dir = HawkeyeDirectory::Instance().getABIImageDir() + "\\" + dateTime;

	if (Logger::L().IsOfInterest(severity_level::debug2)) {

		// Save the ABI data for debugging, data is written to a new directory that is persistent.
		saveABIData (HawkeyeDirectory::Instance().getABIImageDir() + "\\" + dateTime, imageCnt_ - 1, image);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "onAdjustBackgroundIntensityCameraTrigger: <exit>");
}

cv::Mat AdjustBackgroundIntensity::getMaskImage(
	cv::Mat& input, uint16_t bitsPerChannel, bool background)
{
	if (input.empty())
	{
		return input;
	}

	cv::Mat greyImage;
	// convert image to greyscale
	bool isColorImage = input.channels() == 3;
	if (isColorImage)
	{
		cv::cvtColor(input, greyImage, cv::COLOR_BGR2GRAY);
	}
	else
	{
		greyImage = input;
	}

	int histSize = static_cast<int>(std::pow(2.0, bitsPerChannel));
	const int HistPeakOffSet = 20;

	// Set the ranges for grayscale.
	float range[] = { 0, static_cast<float>(histSize) };
	const float* histRange = { range };
	bool uniform = true;
	bool accumulate = false;
	cv::Mat histogram;

	// Calculate histogram and save result in "histogram" image
	cv::calcHist(&greyImage, 1, nullptr, Mat(), histogram, 1, &histSize, &histRange, uniform, accumulate);

	// Find the max location in histogram
	double minVal; double maxVal; Point minLoc; Point maxLoc;
	cv::minMaxLoc(histogram, &minVal, &maxVal, &minLoc, &maxLoc);

	auto minThreshold = maxLoc.y - HistPeakOffSet;
	auto maxThreshold = maxLoc.y + HistPeakOffSet;

	cv::Mat outputImage;
	// Create a binary image considering the threshold range as background
	cv::inRange(greyImage, minThreshold, maxThreshold, outputImage);

	// if background mask image is requested then return.
	if (background)
	{
		return outputImage;
	}

	// Create foreground mask image by inverting each pixel
	bitwise_not(outputImage, outputImage);

	if (isColorImage)
	{
		greyImage.release();
	}

	return outputImage;
}

void AdjustBackgroundIntensity::executeInternal(std::function<void(bool)> callback)
{
	// Set offset of 1 if params_.offset is zero.
	const uint32_t minOffSet = 1;
	auto offset = std::max(params_.offset, minOffSet);

	// lower and upper target intensities should be between 0 - 2^(image_bps)	
	float targetIntensityUpper = static_cast<float>(
		std::min (params_.targetIntensity + offset, maxImageIntensity_));

	float targetIntensityLower = 0.0;
	if (params_.targetIntensity > offset)
	{
		targetIntensityLower = static_cast<float>(params_.targetIntensity - offset);
		// lower target intensity should be at least 2 step smaller
		// than upper target intensity
		if (targetIntensityLower > targetIntensityUpper - (2.0f * minOffSet))
		{
			targetIntensityLower = targetIntensityUpper - (2.0f * minOffSet);
		}
	}

	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str(boost::format("ABI: target intensities: lower: %f, upper: %f")
			% targetIntensityLower % targetIntensityUpper));

	adjustBgIntensityStateExecution(
		ABIExecutionStates::eEntryPoint, params_.maxImageCount, targetIntensityUpper, targetIntensityLower, [this, callback](bool success)
	{
		if (success == false || imageCnt_ >= params_.maxImageCount)
		{
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(callback, success));
			return;
		}

		// Reduce the maximum number of image by already processed image count.
		params_.maxImageCount = params_.maxImageCount - imageCnt_;

		Logger::L().Log (
			MODULENAME, severity_level::normal,
			boost::str(boost::format("ABI Status with %d images processed is %s : Background Intensity is now %f using %f percent power")
				% imageCnt_
				% (success ? "Success" : "Failed")
				% averageIntensity_.get_value_or(-1.0f)
				% Hardware::Instance().getLed(params_.ledType)->getConfig()->percentPower));

		abiLogStr += boost::str(boost::format("Image %d, Intensity %f, Next power %f\n")
			% imageCnt_ 
			% averageIntensity_.get_value_or(-1.0f) 
			% Hardware::Instance().getLed(params_.ledType)->getConfig()->percentPower);

		// this workflow does not require the user id, so do not use the transient user technique
		params_.pIos->post(std::bind(&AdjustBackgroundIntensity::executeInternal, this, callback));
	});
}

void AdjustBackgroundIntensity::adjustBgIntensityStateExecution(
	ABIExecutionStates currentState, uint16_t numFrames,
	float targetIntensityUpper, float targetIntensityLower,
	std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [this, targetIntensityUpper, targetIntensityLower, callback](ABIExecutionStates nextState, uint16_t framesLeft)
	{
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pIos->post([=]()
		{
			adjustBgIntensityStateExecution(nextState, framesLeft, targetIntensityUpper, targetIntensityLower, callback);
		});
	};

	if (Logger::L().IsOfInterest(severity_level::debug1))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "adjustBgIntensityStateExecution:: currentState: " + DataConversion::enumToRawValueString(currentState));
	}

	switch (currentState)
	{
		case ABIExecutionStates::eEntryPoint:
		{
			if (std::abs(targetIntensityUpper - targetIntensityLower) <= 1.0f)
			{
				Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("adjustBgIntensityStateExecution:: eEntryPoint: invalid target intensities. Lower<%f> Upper<%f>") % targetIntensityLower % targetIntensityUpper));
				onCurrentStateComplete(ABIExecutionStates::eError, numFrames);
				return;
			}
			// Intentional fall thru
		}
		case ABIExecutionStates::eUpdateLedPower:
		{
			adjustLedPowerAsync([onCurrentStateComplete, numFrames](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "adjustBgIntensityStateExecution:: eUpdateLedPower: Failed to set led power");
					onCurrentStateComplete(ABIExecutionStates::eError, numFrames);
					return;
				}
				onCurrentStateComplete(ABIExecutionStates::eGetLedPower, numFrames);
			}, targetIntensityUpper, targetIntensityLower);
			return;
		}
		case ABIExecutionStates::eGetLedPower:
		{
			//Logger::L().Log (MODULENAME, severity_level::debug1, "ABIExecutionStates::eGetLedPower: ***************************************************************");

			Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->getPower([this, onCurrentStateComplete, numFrames](boost::optional<float> power)
			{
				Logger::L().Log (MODULENAME, severity_level::normal, "getPower:: power: " + std::to_string(power.get()));
				onCurrentStateComplete(ABIExecutionStates::eTakePicture, numFrames);
			});
			return;
		}
		case ABIExecutionStates::eTakePicture:
		{
			if (numFrames == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::normal, "adjustBgIntensityStateExecution:: eTakePicture: all pictures taken");
				onCurrentStateComplete(ABIExecutionStates::eComplete, numFrames);
				return;
			}

			uint16_t newFrameCount = numFrames - 1;
			auto onTakePictureComplete = [this, onCurrentStateComplete, newFrameCount](cv::Mat image)
			{
				// this workflow does not require the user id, so do not use the transient user technique
				params_.pIos->post([=]()
				{
					onAdjustBackgroundIntensityCameraTrigger(image);
					if (image.empty())
					{
						Logger::L().Log (MODULENAME, severity_level::error, "adjustBgIntensityStateExecution:: eTakePicture: failed to take picture");
						onCurrentStateComplete(ABIExecutionStates::eError, newFrameCount);
						return;
					}
					onCurrentStateComplete(ABIExecutionStates::eUpdateLedPower, newFrameCount);
				});
			};

			Hardware::Instance().getCamera()->takePicture(params_.exposureTime_usec, params_.ledType, onTakePictureComplete);
			return;
		}
		case ABIExecutionStates::eComplete:
		{
			if (intensities_.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "adjustBgIntensityStateExecution:: eComplete: intensities_ vector is empty!");
				onCurrentStateComplete(ABIExecutionStates::eError, numFrames);
				return;
			}

			abiLogStr += "Intensity     Power\n";
			for (auto abi : abi_log_values)
			{
				abiLogStr += boost::str(boost::format("%f, %f\n") % abi.first % abi.second);
			}

			averageIntensity_ = intensities_.rbegin()->first;
			float currentPower = intensities_.rbegin()->second;
			intensities_.clear();

			setLedPowerAsync(callback, currentPower);
			return;
		}
		case ABIExecutionStates::eError:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "adjustBgIntensityStateExecution:: eError: failed to adjust background intensity");
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(callback, false));
			return;
		}
	}

	//unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void AdjustBackgroundIntensity::adjustLedPowerAsync(
	std::function<void(bool)> callback, float targetIntensityUpper, float targetIntensityLower)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto getPowerCallback = [this, callback, targetIntensityUpper, targetIntensityLower](boost::optional<float> power) {

		float currentPercentPower = 0.0;

		if (power) {
			currentPercentPower = power.get();
		}

		// We are starting the process.
		if (!averageIntensity_)
		{
			if (Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->isPowerSame(currentPercentPower, 0.0f))
			{
				Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("adjustLedPowerAsync:: setting power to %f") % currentPercentPower));
				currentPercentPower = 10.0; // Set initial led power to 10% if not set already
			}
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(&AdjustBackgroundIntensity::setLedPowerAsync, this, callback, currentPercentPower));
			return;
		}

		float targetIntensity = (targetIntensityLower + targetIntensityUpper) / 2.0f;
		// Consider average image intensity value as 0.0f if it is less than 1.0f
		const float currentAvgIntensity = averageIntensity_.get() < 1.0f ? 0.0f : averageIntensity_.get();

		// Periodically we receive an "all-black" image from that camera.  Previous versions of the code didn't actually check the 
		// success of "takePicture", so that could be involved...  ...but we'll do an explicit check to discard that image so it
		// doesn't throw out our calibrations:
		if (currentAvgIntensity < 20.0f)
		{
			Logger::L().Log (MODULENAME, severity_level::notification,
							boost::str(boost::format("adjustLedPowerAsync:: camera provided a zero-intensity image on image %d of %d")
									   % imageCnt_ % params_.maxImageCount));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_general_imagequality,
				instrument_error::severity_level::notification));
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(callback, true));
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::normal,
						boost::str(boost::format("adjustLedPowerAsync:: average background intensity: %f, currentPercentPower: %f for image %d/%d")
								   % currentAvgIntensity % currentPercentPower % imageCnt_ % params_.maxImageCount));

		// Record intensity values;
		abi_log_values.emplace_back(currentAvgIntensity, currentPercentPower);

		// Current intensity is already set to expected.
		if (currentAvgIntensity >= targetIntensityLower && currentAvgIntensity <= targetIntensityUpper)
		{
			auto difference = std::abs(currentAvgIntensity - targetIntensity);
			intensities_.emplace_back(currentAvgIntensity, currentPercentPower);

			if (difference <= DeadBandLimit)
			{
				Logger::L().Log (MODULENAME, severity_level::normal, "adjustLedPowerAsync:: difference <= DeadBandLimit: done...");
				// No need to adjust led power further
				// this workflow does not require the user id, so do not use the transient user technique
				params_.pIos->post(std::bind(callback, true));
				return;
			}
		}

		if (currentAvgIntensity < targetIntensityLower && currentPercentPower >= MaxAllowedLedPower)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, boost::str(boost::format("adjustLedPowerAsync:: image %d of %d: image too dim at high range of LED output. Current intensity: %f") 
				% imageCnt_ % params_.maxImageCount % currentAvgIntensity));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_general_imagequality, 
				instrument_error::severity_level::notification));
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(callback, false));
			return;
		}

		if (currentAvgIntensity > targetIntensityUpper && currentPercentPower <= MinAllowedLedPower)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, boost::str(boost::format("adjustLedPowerAsync:: image %d of %d: image too bright at low range of LED output. Current intensity: %f") 
				% imageCnt_ % params_.maxImageCount % currentAvgIntensity));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_general_imagequality, 
				instrument_error::severity_level::notification));
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(callback, false));
			return;
		}

		float scale = targetIntensity; // params_.targetIntensity;
		if (currentAvgIntensity > 0.0f)
		{
			scale /= currentAvgIntensity;
		}

		auto scaledPower = std::min ((currentPercentPower * scale), MaxAllowedLedPower);
		if (Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->isPowerSame(scaledPower, currentPercentPower))
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "adjustLedPowerAsync:: scaledPower is the same as currentPercentPower done...");

			// No further correction possible
			// this workflow does not require the user id, so do not use the transient user technique
			params_.pIos->post(std::bind(callback, true));
			return;
		}

		currentPercentPower = scaledPower;
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pIos->post(std::bind(&AdjustBackgroundIntensity::setLedPowerAsync, this, callback, currentPercentPower));
	};

	Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->getPower (getPowerCallback);
}

void AdjustBackgroundIntensity::setLedPowerAsync(std::function<void(bool)> callback, float power)
{
	HAWKEYE_ASSERT (MODULENAME, callback);
	
	if (power >= MaxAllowedLedPower
		|| power <= MinAllowedLedPower)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setLedPowerAsync : Invalid led power : " + std::to_string(power));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_led_powerthreshold, 
			instrument_error::imaging_led_instances::brightfield, 
			instrument_error::severity_level::error));
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pIos->post(std::bind(callback, false));
		return;
	}

	// If power is more than 50.0 percent, notify thru error reporting
	const float thresholdPower = 50.0f;
	if (power > thresholdPower)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_led_powerthreshold,
			instrument_error::imaging_led_instances::brightfield, 
			instrument_error::severity_level::notification));
	}
	
	Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->setPower(power, [this, power, callback](bool setPowerOk)
	{
		if (!setPowerOk)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setLedPowerAsync : Failed to set led power : " + std::to_string(power));
		} else
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "setLedPowerAsync:: new power setting: " + std::to_string(power));
		}
		// this workflow does not require the user id, so do not use the transient user technique
		params_.pIos->post(std::bind(callback, setPowerOk));
	});
}
