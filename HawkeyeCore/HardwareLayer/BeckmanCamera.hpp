#pragma once

#include <atomic>
#include "CameraBase.hpp"
#include "Camera_Basler.hpp"
#include "LedBase.hpp"

class BeckmanCamera : public CameraBase
{
public:
	BeckmanCamera(std::shared_ptr<CBOService> pCBOService);
	virtual ~BeckmanCamera();

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
	virtual void captureSingleFrame(
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

	enum class eTakePictureStates : uint8_t
	{
		eEntryPoint = 0,
		eArmTrigger,
		eFireTrigger,
		eWaitForImage,
		eRetryOnFailure,
		eGetImage,
		eComplete,
		eError
	};

	typedef struct takePictureParams
	{
		uint32_t exposureTime_usec;
		HawkeyeConfig::LedType ledType;
		uint8_t retryCount;
		std::function<void(cv::Mat)> perImageCallback;
		bool isImageCaptured;
	}takePictureParams;

	bool getImage(cv::Mat& image) const;
	bool fireTrigger(uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType);

	void takePictureInternal(
		eTakePictureStates currentState, std::shared_ptr<takePictureParams> pTakePictureParams);

	void takeMultiplePicturesInternal(
		takePictureParams params,
		std::function<void(bool)>completionCallback,
		uint32_t framesLeftToCapture,
		uint64_t minimumTimePerImageMillisec);

	std::shared_ptr<iCamera> pCamera_;
	CameraRegisters cameraCmd_;

	std::atomic<std::size_t> imageCount_;
	std::atomic<std::size_t> darkImageCount_;
	cv::Mat lastImageCaptured_;
};

