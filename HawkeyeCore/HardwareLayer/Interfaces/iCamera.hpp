#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "LedBase.hpp"

using namespace cv;

class iCamera
{
public:

	typedef std::function<void(void)> CompletionCallback_t;

	enum class Error {
		UnknownError = 0,
	};

	typedef struct iCamera_Image_Information
	{
		uint16_t bitsPerChannel;
	}ImageInfo;

	iCamera ();
	virtual ~iCamera();

    virtual bool initialize() = 0;
    virtual bool setGain (double gain) = 0;
    virtual bool getGainRange (double& minGain, double& maxGain) = 0;
    virtual bool getImage (Mat& image) = 0;
    virtual bool getImageSize (uint32_t& width, uint32_t& height) = 0;
    virtual bool getImageInfo (iCamera::ImageInfo& imageInfo) = 0;
    virtual bool setGainLimitsState (bool state) = 0;
    virtual std::string getVersion() = 0;
	virtual bool setOnTriggerCallback (CompletionCallback_t onCameraTriggerCompletion) = 0;
	virtual bool armTrigger (unsigned int triggerReadyTimeout, unsigned int exposureTimeInMicroseconds) = 0;
	virtual bool waitForImage (unsigned int timeout) = 0;
	virtual bool isPresent() = 0;
	virtual void shutdown() = 0;

	static constexpr int VmWidth = 512;
	static constexpr int VmHeight = 512;

	static constexpr int FullWidth = 2048;
	static constexpr int FullHeight = 2048;
};
