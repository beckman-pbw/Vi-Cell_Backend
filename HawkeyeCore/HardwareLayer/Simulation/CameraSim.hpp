#pragma once

#include "CameraBase.hpp"

class CameraSim : public CameraBase
{
public:
	CameraSim (std::shared_ptr<CBOService> pCBOService);
	virtual ~CameraSim();

	virtual bool setGain(double gain) override;
	virtual bool setGainLimitsState(bool state) override;
	virtual bool getGainRange(double& minGain, double& maxGain) override;
	virtual bool getImageSize(uint32_t& width, uint32_t& height) override;
	virtual bool getImageInfo(iCamera::ImageInfo& imageInfo) override;
	virtual std::string getVersion() override;
	virtual bool isPresent() override;
	virtual void shutdown() override;

protected:

	virtual void initializeAsync(std::function<void(bool)> callback) override;

	// Capture single frame and trigger callback with captured frame
	virtual void captureSingleFrame (
		uint32_t exposureTime_usec,
		HawkeyeConfig::LedType ledType,
		std::function<void(cv::Mat)> callback) override;

	// Capture provided number of frames with user defined fps
	// Trigger per image callback as soon as frame is captured for all the frames
	// and finally completion callback when done
	virtual void captureMultipleFrame(
		uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType, uint32_t numFrames, uint8_t frameRatePerSecond,
		std::function<void(cv::Mat)> perImageCallack, std::function<void(bool)> completionCallback) override;

private:
	cv::Mat getImage();

	uint32_t imageCnt_;
	const uint32_t MaxSimulationImageCount;

	bool use_external_images;
	boost::filesystem::directory_iterator external_image_iterator;
};

