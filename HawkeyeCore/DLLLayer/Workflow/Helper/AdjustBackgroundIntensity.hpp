#pragma once

#include <opencv2/opencv.hpp>

#include <iostream>
#include <atomic>
#include "LedBase.hpp"
#include "HawkeyeConfig.hpp"

class AdjustBackgroundIntensity
{
public:
	typedef struct adjustBackgroundIntensityParams
	{
		HawkeyeConfig::LedType ledType;
		uint32_t targetIntensity;
		uint32_t offset;
		uint32_t exposureTime_usec;
		uint16_t maxImageCount;
		std::shared_ptr<boost::asio::io_context> pIos;
	}ABIParams;

	AdjustBackgroundIntensity (ABIParams params);
	~AdjustBackgroundIntensity() = default;

	void execute(std::function<void(bool)> onCompleteCallback);
	bool isBusy() const;

private:

	enum class ABIExecutionStates : uint8_t
	{
		eEntryPoint = 0,
		eUpdateLedPower,
		eGetLedPower,
		eTakePicture,
		eComplete,
		eError
	};
	
	void onAdjustBackgroundIntensityCameraTrigger(cv::Mat image);
	cv::Mat getMaskImage(cv::Mat& input, uint16_t bitsPerChannel, bool background);
	
	void executeInternal(std::function<void(bool)> callback);
	void adjustBgIntensityStateExecution(
		ABIExecutionStates currentState, uint16_t numFrames,
		float targetIntensityUpper, float targetIntensityLower,
		std::function<void(bool)> callback);

	void adjustLedPowerAsync(
		std::function<void(bool)> callback, float targetIntensityUpper, float targetIntensityLower);
	void setLedPowerAsync(std::function<void(bool)> callback, float power);
	void saveABIData (std::string abiImageDir, uint32_t imageCnt, cv::Mat image);

	ABIParams params_;
	boost::optional<float> averageIntensity_;
	uint32_t imageCnt_;
	std::atomic_bool isBusy_;
	uint32_t maxImageIntensity_;
	uint32_t minImageIntensity_;
	std::vector<std::pair<float, float>> intensities_;
};