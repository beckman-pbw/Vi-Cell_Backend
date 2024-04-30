#include "stdafx.h"

#include <atomic>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "Camera_Allied.hpp"
#include "SystemErrors.hpp"

#include <GenApi/INodeMap.h>

#include "GlobalImageSize.hpp"

static const char MODULENAME[] = "Camera_Allied";

void AlliedFrameObserver::FrameReceived(const FramePtr pFrame)
{
	VmbFrameStatusType status;
	if(VmbErrorSuccess == pFrame->GetReceiveStatus(status))
	{
		if(status == VmbFrameStatusComplete)
		{
			onCompletion_();
		}
		else
		{
			std::stringstream ss;
			ss << "AlliedFrameObserver::GetReceiveStatus() error: " << status;
			Logger::L().Log(MODULENAME, severity_level::error, ss.str());
			CameraErrorLog::Log(ss.str());
		}
	}
	imageReceived.notify_all();
}


//*****************************************************************************
Camera_Allied::Camera_Allied ()
	: iCamera(), system(VimbaSystem::GetInstance()), supportsGain(false), imageBitsPerChannel(0)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Camera_Allied::Camera_Allied constructing");
	auto result = system.Startup();
	if (result != VmbErrorType::VmbErrorSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "Camera_Allied::ctor() : Failure to startup Vimba System, error=" + std::to_string(result) + ".");
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "Camera_Allied::ctor() : Success in startup of Vimba System.");
	}
}

Camera_Allied::~Camera_Allied()
{
	frameObserver.release();
}

void Camera_Allied::shutdown()
{
	if (camera != nullptr)
	{
		StopCameraCapture();
		camera->Close();
	}
	system.Shutdown();
}

//*****************************************************************************

bool Camera_Allied::SetDimensions()
{
	VmbInt64_t imageWidth, imageHeight, imageMaxWidth, imageMaxHeight;
	FeaturePtr width, height, maxWidth, maxHeight, widthOffset, heightOffset;
	if (GetImageSizeFeatures(width, height) != VmbErrorSuccess) return false;

	if (GetFeature(WidthMax, maxWidth) != VmbErrorSuccess) return false;
	if (GetFeature(HeightMax, maxHeight) != VmbErrorSuccess) return false;
	if (GetFeature(WidthOffset, widthOffset) != VmbErrorSuccess) return false;
	if (GetFeature(HeightOffset, heightOffset) != VmbErrorSuccess) return false;

	char* buf = nullptr;
	size_t sz = 0;

	if (_dupenv_s(&buf, &sz, "WIN10_VM") == 0 && buf != nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "Running in WIN10_VM, setting camera width/height to 512/512.");
		imageWidth = VmWidth;
		imageHeight = VmHeight;
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "Running in full image, setting camera width/height to 2048/2048.");
		imageWidth = FullWidth;
		imageHeight = FullHeight;
	}
	if(buf != nullptr)
	{
		free(buf);
	}

	//Figure out the offsets so we're taking a picture in the middle of the frame
	maxWidth->GetValue(imageMaxWidth);
	maxHeight->GetValue(imageMaxHeight);

	const VmbInt64_t calculatedOffsetWidth = (imageMaxWidth - imageWidth) / 2;
	const VmbInt64_t calculatedOffsetHeight = (imageMaxHeight - imageHeight) / 2;

	//Set the image width and offsets
	if (width->SetValue(imageWidth) != VmbErrorSuccess) return false;
	if (height->SetValue(imageHeight) != VmbErrorSuccess) return false;
	if (widthOffset->SetValue(calculatedOffsetWidth) != VmbErrorSuccess) return false;
	if (heightOffset->SetValue(calculatedOffsetHeight) != VmbErrorSuccess) return false;

	globalImageSize.width = imageWidth;
	globalImageSize.height = imageHeight;

	return true;
}

bool Camera_Allied::isPresent()
{
	Logger::L().Log(MODULENAME, severity_level::normal, "Camera_Allied::isPresent() : Determining camera existence..");
	CameraPtrVector cameras;
	auto result = system.GetCameras(cameras);
	Logger::L().Log(MODULENAME, severity_level::normal, "Camera_Allied::isPresent() : result=" + std::to_string(result) + ".");
	return (result == VmbErrorSuccess) && !cameras.empty();
}

//*****************************************************************************
bool Camera_Allied::initialize() {

	CameraPtrVector cameras;
	system.Startup();
	auto result = system.GetCameras(cameras);
	// Get all known cameras
	if (VmbErrorSuccess != result || cameras.empty())
	{
		Logger::L().Log(MODULENAME, severity_level::debug2, "Camera_Allied::Camera_Allied Error initializing camera, error code = " +
			to_string(result) + ", number of cameras = " + to_string(cameras.size()));
		return false;
	}

	Logger::L().Log(MODULENAME, severity_level::normal, "Initializing Allied Camera");

	camera = cameras[0];
	result = camera->Open(VmbAccessModeFull);
	if(result != VmbErrorSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "Error opening Allied camera");
		return false;
	}
	setFeatures();

	//Set up image dimensions
	if (!SetDimensions()) return false;

	FeaturePtr feature;
	if (GetFeature(ExposureMode, feature) != VmbErrorSuccess) return false;
	feature->SetValue(ExpMode);

	if (GetFeature(AcquisitionMode, feature) != VmbErrorSuccess) return false;
	feature->SetValue(Mode);

	if (GetFeature(TriggerMode, feature) != VmbErrorSuccess) return false;
	feature->SetValue(On);

	VmbVersionInfo_t apiVersion;
	if (system.QueryVersion(apiVersion) == VmbErrorSuccess)
	{
		if (GetFeature(DeviceFirmwareVersion, feature) != VmbErrorSuccess)
		{
			return false;
		}
		string firmwareVersion;
		feature->GetValue(firmwareVersion);

		auto apiVer = to_string(apiVersion.major) + "." + to_string(apiVersion.minor) + "." + to_string(apiVersion.patch);
		version = apiVer + " and " + firmwareVersion;

		Logger::L().Log(MODULENAME, severity_level::normal, boost::str(boost::format("Allied Driver/Firmware version: %s") % version));
	}
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Error getting Allied Camera API version");
	}

	if (!ConfigureFrameCapture()) return false;

	setGain(0);

	return true;
}

bool Camera_Allied::SetUpTrigger(unsigned int exposureTime)
{
	FeaturePtr feature;

	if (GetFeature(TriggerSelector, feature) != VmbErrorSuccess) return false;
	feature->SetValue(FrameStart);

	if (GetFeature(TriggerSource, feature) != VmbErrorSuccess) return false;
	feature->SetValue(LineToTriggerOn);

	if (GetFeature(TriggerActivation, feature) != VmbErrorSuccess) return false;
	feature->SetValue(Activation);

	if (GetFeature(ExposureAuto, feature) != VmbErrorSuccess) return false;
	feature->SetValue(Off);

	if (GetFeature(ExposureTime, feature) != VmbErrorSuccess) return false;
	feature->SetValue(static_cast<float>(HawkeyeConfig::ExposureOffset + exposureTime));


	return true;
}

void Camera_Allied::setFeatures()
{
	if (camera == nullptr) return;
	FeaturePtr feature;
	if(GetFeature(Gain, feature) == VmbErrorSuccess)
	{
		supportsGain = true;
	}
}

//*****************************************************************************
bool Camera_Allied::isCameraConnected() {
	return true;
}

//*****************************************************************************
bool Camera_Allied::setOnTriggerCallback (CompletionCallback_t onCameraTriggerCompletion)
{
	frameObserver->SetCompletionHandler(onCameraTriggerCompletion);
	return true;
}

//*****************************************************************************
std::string Camera_Allied::getVersion() {
	return version;
}

//*****************************************************************************
bool Camera_Allied::setGain (double gain) {
	if (!supportsGain) return false;

	FeaturePtr feature;
	if (GetGainFeature(Gain, feature) != VmbErrorSuccess) return false;
	feature->SetValue(gain);

	return true;
}

//*****************************************************************************
bool Camera_Allied::getGainRange (double& minGain, double& maxGain) {
	if (!supportsGain) return false;

	FeaturePtr feature;
	if (GetGainFeature(Gain, feature) != VmbErrorSuccess) return false;
	feature->GetRange(minGain, maxGain);

	return true;
}

//*****************************************************************************
bool Camera_Allied::setGainLimitsState (bool state) {
	//Gain limits state is not supported on allied cameras
	return true;
}

bool Camera_Allied::ConfigureFrameCapture()
{
	frameObserver.reset(new AlliedFrameObserver(camera, imageReceived));
	FeaturePtr feature;

	if (GetFeature(PayloadSize, feature) != VmbErrorSuccess) return false;

	VmbInt64_t payload;
	feature->GetValue(payload);

	frame.reset(new Frame(payload));
	frame->RegisterObserver(IFrameObserverPtr(frameObserver.get()));
	camera->AnnounceFrame(frame);

	FeaturePtr pFormatFeature;
	GetFeature(PixelFormat, feature);

	feature->SetValue(VmbPixelFormatRgb8);

	auto result = camera->StartCapture();
	if (result != VmbErrorSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Error Allied Camera capture: " + to_string(result));
		return false;
	}

	result = camera->QueueFrame(frame);
	if (result != VmbErrorSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Error Allied Camera queued frame: " + to_string(result));
		return false;
	}
	return true;
}


//*****************************************************************************
bool Camera_Allied::armTrigger (unsigned int triggerReadyTimeout, unsigned int exposureTimeInMicroseconds) {
	SetUpTrigger(exposureTimeInMicroseconds);
	RunCommand(AcquisitionStart);

	return true;
}

//*****************************************************************************
bool Camera_Allied::waitForImage (unsigned int imageTimeout) {

	std::unique_lock<std::mutex> lck(imageReceivedMutex);
	auto timeOut = imageReceived.wait_for(lck, std::chrono::milliseconds(imageTimeout));

	RunCommand(AcquisitionStop);
	VmbFrameStatusType status;
	frame->GetReceiveStatus(status);
	if(status != VmbFrameStatusComplete)
	{
		return false;
	}
	return true;
}

//NOTE: not currently used.
////*****************************************************************************
//bool Camera_Allied::setImage (CGrabResultPtr pGrabResult) {
//
//	if (pGrabResult) {
//		image_ = Mat (pGrabResult->GetHeight(), pGrabResult->GetWidth(), CV_8U, (uint8_t*)pGrabResult->GetBuffer(), Mat::AUTO_STEP);
//		return true;
//	} else {
//		return false;
//	}
//}

//*****************************************************************************
//TODO: add captureImage() API...



bool Camera_Allied::getImage (Mat& image) {

	if (imageBitsPerChannel == 0)
	{
		FeaturePtr feature;
		auto ret = camera->GetFeatureByName(PixelSize, feature);
		
		if (ret != VmbErrorSuccess)
		{
			Logger::L().Log(MODULENAME, severity_level::critical, "getImage: undefined pixel type");
			CameraErrorLog::Log("Camera_Allied getImage : undefined pixel type");
			return false;
		}

		feature->GetValue(imageBitsPerChannel);
	}

	auto imageBytesPerChannel = static_cast<uint16_t>((imageBitsPerChannel + 7) / 8);
	VmbUint32_t height, width;
	VmbUchar_t* buffer;
	frame->GetHeight(height);
	frame->GetWidth(width);
	frame->GetBuffer(buffer);

	if (imageBytesPerChannel == 1)
	{
		image = Mat(height, width, CV_8U,
			buffer,
			Mat::AUTO_STEP).clone();
	}
	else if (imageBytesPerChannel == 2)
	{
		image = Mat(height, width,
			CV_16U, buffer,
			Mat::AUTO_STEP).clone();
	}
	else
	{
		CameraErrorLog::Log("Camera_Allied_getImage : Invalid bits per channel");
		return false;
	}

	if (image.empty())
	{
		CameraErrorLog::Log("Camera_Allied_getImage : Image is empty");
		return false;
	}
	camera->QueueFrame(frame);
	return true;
}

void Camera_Allied::StopCameraCapture()
{
	camera->EndCapture();
	camera->FlushQueue();
	camera->RevokeAllFrames();
	frame->UnregisterObserver();
}

//*****************************************************************************
bool Camera_Allied::getImageSize (uint32_t& width, uint32_t& height) {
	FeaturePtr widthFeature, heightFeature;
	VmbInt64_t recvdWidth, recvHeight;

	if (GetImageSizeFeatures(widthFeature, heightFeature) != VmbErrorSuccess) return false;

	widthFeature->GetValue(recvdWidth);
	heightFeature->GetValue(recvHeight);

	width = recvdWidth;
	height = recvHeight;
	return true;

}

//*****************************************************************************
bool Camera_Allied::getImageInfo(iCamera::ImageInfo& imageInfo)
{
	if (imageBitsPerChannel == 0)
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "getCameraInfo: undefined bits per channel");
		return false;
	}

	imageInfo.bitsPerChannel = imageBitsPerChannel;
	return true;

}

VmbErrorType Camera_Allied::GetGainFeature(const char featureName[], FeaturePtr& feature) const
{
	auto result = GetFeature(GainSelector, feature);
	if (result != VmbErrorSuccess) return result;
	feature->SetValue(GainSelector_All);

	result = camera->GetFeatureByName(featureName, feature);
	if (result != VmbErrorSuccess) return result;

	return VmbErrorSuccess;
}

VmbErrorType Camera_Allied::GetImageSizeFeatures(FeaturePtr& widthFeature, FeaturePtr& heightFeature) const
{

	auto result = GetFeature(Width, widthFeature);
	if (result != VmbErrorSuccess) return result;

	result = GetFeature(Height, heightFeature);
	return result;
}

VmbErrorType Camera_Allied::GetFeature(const char name[], FeaturePtr& feature) const
{
	auto result = camera->GetFeatureByName(name, feature);
	if (result != VmbErrorSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Error getting Allied Camera Feature: " + std::string(name) + to_string(result));
	}
	return result;
}


void Camera_Allied::RunCommand(const char command[])
{
	FeaturePtr feature;
	camera->GetFeatureByName(command, feature);
	feature->RunCommand();
}

