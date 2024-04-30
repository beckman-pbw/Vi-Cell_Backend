#pragma once

#include "Interfaces/iCamera.hpp"
#include "CameraErrorLog.hpp"
#include <VimbaCPP/Include/VimbaCPP.h>

using namespace std;

using namespace AVT::VmbAPI;

class AlliedFrameObserver : public IFrameObserver
{
public:

	AlliedFrameObserver(CameraPtr camera, std::condition_variable& cv) : IFrameObserver(camera), imageReceived(cv)
	{
		
	}

	//****************************************************************************
	void SetCompletionHandler(iCamera::CompletionCallback_t onCompletion) {
		onCompletion_ = onCompletion;
	}

	void FrameReceived(const FramePtr pFrame) override;
private:
	iCamera::CompletionCallback_t onCompletion_;
	std::condition_variable& imageReceived;
};

class Camera_Allied : public iCamera {
public:
	Camera_Allied ();
	virtual ~Camera_Allied();
	bool SetDimensions();

	virtual bool initialize() override;
	virtual bool armTrigger (unsigned int triggerReadyTimeout, unsigned int exposureTimeInMicroseconds) override;
	virtual bool setGain (double gain) override;
	virtual bool getGainRange (double& minGain, double& maxGain) override;
	virtual bool getImage (Mat& image) override;
	virtual bool getImageSize (uint32_t& width, uint32_t& height) override;
	virtual bool getImageInfo(iCamera::ImageInfo& imageInfo) override;
	virtual bool setGainLimitsState (bool state) override;
	virtual std::string getVersion() override;
	virtual bool waitForImage (unsigned int timeout) override;
	virtual bool setOnTriggerCallback(CompletionCallback_t onCameraTriggerCompletion) override;
	virtual bool isPresent() override;
	virtual void shutdown() override;

	//NOTE: not currently used.
	//virtual bool setImage (CGrabResultPtr pGrabResult);

	static std::string getModuleName() { return moduleName; }
	static std::string moduleName;
	static bool isCameraConnected();


private:

	VimbaSystem& system;
	CameraPtr camera;
	bool supportsGain;
	VmbInt64_t imageBitsPerChannel;
	FramePtr frame;
	unique_ptr<AlliedFrameObserver> frameObserver;
	string version;

	std::mutex imageReceivedMutex;
	std::condition_variable imageReceived;

	//Gain
	//Features
	static constexpr char Gain[] = "Gain";
	static constexpr char GainSelector[] = "GainSelector";
	//Values
	static constexpr char GainSelector_All[] = "ALL";

	//Image properties
	static constexpr char Height[] = "Height";
	static constexpr char Width[] = "Width";

	static constexpr char HeightMax[] = "HeightMax";
	static constexpr char WidthMax[] = "WidthMax";

	static constexpr char HeightOffset[] = "OffsetY";
	static constexpr char WidthOffset[] = "OffsetX";

	static constexpr char PixelFormat[] = "PixelFormat";
	//Trigger
	//Features
	static constexpr char TriggerSelector[] = "TriggerSelector";
	static constexpr char TriggerSource[] = "TriggerSource";
	static constexpr char TriggerActivation[] = "TriggerActivation";
	static constexpr char TriggerMode[] = "TriggerMode";
	static constexpr char FrameStart[] = "FrameStart";
	//Constants
	static constexpr char Software[] = "Software";
	static constexpr char On[] = "On";
	static constexpr char Off[] = "Off";

	//Variables
	static constexpr char Activation[] = "RisingEdge";
	static constexpr char LineToTriggerOn[] = "Line0";

	//Exposure mode
	//Constants
	static constexpr char ExposureMode[] = "ExposureMode";
	static constexpr char ExposureTime[] = "ExposureTime";
	static constexpr char ExposureAuto[] = "ExposureAuto";

	//Variables
	static constexpr char ExpMode[] = "Timed";

	static constexpr char PayloadSize[] = "PayloadSize";
	static constexpr char PixelSize[] = "PixelSize";

	//Commands
	static constexpr char AcquisitionStart[] = "AcquisitionStart";
	static constexpr char AcquisitionStop[] = "AcquisitionStop";
	static constexpr char TriggerSoftware[] = "TriggerSoftware";

	//Acquisition Mode
	static constexpr char AcquisitionMode[] = "AcquisitionMode";

	//Variables
	static constexpr char Mode[] = "SingleFrame";

	//Device control
	static constexpr char DeviceFirmwareVersion[] = "DeviceFirmwareVersion";


	void setFeatures();
	VmbErrorType GetGainFeature(const char featureName[], FeaturePtr& feature) const;
	VmbErrorType GetImageSizeFeatures(FeaturePtr& widthFeature, FeaturePtr& heightFeature) const;
	bool SetUpTrigger(unsigned int exposureTime);
	void StopCameraCapture();
	bool ConfigureFrameCapture();

	void RunCommand(const char command[]);
	VmbErrorType GetFeature(const char name[], FeaturePtr& feature) const;


};

