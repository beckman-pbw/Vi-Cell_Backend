#include "stdafx.h"

#include <atomic>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "Camera_Basler.hpp"
#include "SystemErrors.hpp"

#include <GenApi/INodeMap.h>

#define _GLOBALIMAGESIZE_LOCAL_
#include "GlobalImageSize.hpp"

static const char MODULENAME[] = "Camera_Basler";

static std::atomic<bool> isCameraConnected_ = false;
std::mutex cameraMutex_;

const std::string Camera_Basler::a2A2448_75umBAS("a2A2448-75umBAS");
const std::string Camera_Basler::acA2040_90umBAS("acA2040-90umBAS");


//*****************************************************************************
bool Camera_Basler::RunProtectedSection(const std::string& sectionname, std::function<void()> code)
{
	try { code(); }
	catch (...)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Exception caught in protected code section \"" + sectionname + "\"");
		CameraErrorLog::Log("Exception caught in protected code section \"" + sectionname + "\"");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_hardwareerror, 
			instrument_error::severity_level::error));

		return false;
	}
	return true;
}

//*****************************************************************************
Camera_Basler::Camera_Basler ()
	: iCamera()
	, pCameraEventHandler_(nullptr)
	, pCameraEventWriter_(nullptr)
	, imageBitsPerChannel_(0)
	, isOnTriggerCallbackRegistered_(false)
	, version_("")
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Camera_Basler::Camera_Basler");
}

//*****************************************************************************
Camera_Basler::~Camera_Basler() {

	Logger::L().Log (MODULENAME, severity_level::debug2, "Camera_Basler::~Camera_Basler");

	if (camera_ && camera_->IsOpen()) {

		RunProtectedSection("Destructor", [this]() {
			// DeregisterCameraEventHandler deletes the handler pointer.
			camera_->DeregisterCameraEventHandler(pCameraEventHandler_, "EventExposureEndData");
			camera_->DeregisterCameraEventHandler(pCameraEventWriter_, "EventExposureEndFrameID");
			camera_->DeregisterCameraEventHandler(pCameraEventWriter_, "EventExposureEndTimestamp");
			camera_->DeregisterImageEventHandler(imageEventHandler_);
			camera_->Close();
		});
	}

	camera_.reset();

	isCameraConnected_ = false;
}

//*****************************************************************************
void CameraNotConnected()
{
	ReportSystemError::Instance().ReportError (BuildErrorInstance(
		instrument_error::imaging_camera_connection, 
		instrument_error::severity_level::error));
	CameraErrorLog::Log("Camera_Basler::Camera is not connected");
	Logger::L().Log (MODULENAME, severity_level::critical, "Camera is NOT connected");
}

void Camera_Basler::shutdown()
{
	//Basler shuts down in the destructor
}

bool Camera_Basler::isPresent()
{
	return GetCamera() != nullptr;
}

std::shared_ptr<CBaslerUniversalInstantCamera> Camera_Basler::GetCamera()
{
	// Only look for USB camera.
	CDeviceInfo info;

	CTlFactory& tlFactory = CTlFactory::GetInstance();

	// Check if there is a camera connected.
	DeviceInfoList_t lstDevices;

	int tries = 3;
	bool found = false;
	while (tries-- > 0) {
		tlFactory.EnumerateDevices(lstDevices);
		if (!lstDevices.empty()) {
			found = true;
			Logger::L().Log(MODULENAME, severity_level::normal, "Camera ID: " + std::string(lstDevices[0].GetFullName().c_str()));
			break;
		}
		Sleep(1000);
	}
	if (!found) {
		return nullptr;
	}

	return std::make_shared<CBaslerUniversalInstantCamera>(tlFactory.CreateFirstDevice(info));
}


//*****************************************************************************
bool Camera_Basler::initialize() {
	Logger::L().Log (MODULENAME, severity_level::normal, "initialize: <enter>");

	
	// Create an instant camera object with the first found camera device that matches the specified device class.
	// The next statement *may* cause the software to crash.  Anecdotally, this occurs when the camera cable is
	// plugged into a USB2 slot instead of a USB3 slot in the main computer.
	camera_ = GetCamera();
	if (!camera_) {
		CameraErrorLog::Log("Camera_Basler : initialize : Failed to create camera instance");
		Logger::L().Log (MODULENAME, severity_level::critical, "Failed to create camera shared ptr");
		return false;
	}

	isCameraConnected_ = true;

	// Create an example event handler. In the present case, we use one single camera handler for handling multiple camera events.
	// The handler prints a message for each received event.
	pCameraEventHandler_ = new Camera_BaslerCameraEventHandler;

	// Create another more generic event handler printing out information about the node for which an event callback is fired.
	pCameraEventWriter_ = new Camera_BaslerEventWriter;

	// For demonstration purposes only, add sample configuration event handlers to print out information
	// about camera use and image grabbing.
	if (isCameraConnected()) {
		if (!RunProtectedSection("Initialization->RegisterConfiguration", [this]() {
			camera_->RegisterConfiguration(new Camera_BaslerConfigurationEventPrinter, RegistrationMode_Append, Cleanup_Delete); // Camera use.
		}))
			return false;
	} else {
		CameraNotConnected();
	}

	// Camera event processing must be activated first, the default is off.
	camera_->GrabCameraEvents = true;

	// Register an event handler for the Exposure End event. For each event type, there is a "data" node
	// representing the event. The actual data that is carried by the event is held by child nodes of the
	// data node. In the case of the Exposure End event, the child nodes are EventExposureEndFrameID and EventExposureEndTimestamp.
	// The CSampleCameraEventHandler demonstrates how to access the child nodes within
	// a callback that is fired for the parent data node.
	// The user-provided ID eMyExposureEndEvent can be used to distinguish between multiple events (not shown).
	if (isCameraConnected()) {
		if (!RunProtectedSection("Initialization->RegisterCameraEventHandler:EventExposureEndData", [this]() {
			camera_->RegisterCameraEventHandler(pCameraEventHandler_, "EventExposureEndData", ExposureEndEvent, RegistrationMode_ReplaceAll, Cleanup_None);
		}))
			return false;
	} else {
		CameraNotConnected();
	}

	//NOTE: not current used...
	//if (isCameraConnected()) {
	//	camera_->RegisterCameraEventHandler (pCameraEventHandler_, "EventFrameStartWaitData", FrameStartWaitDataEvent, RegistrationMode_ReplaceAll, Cleanup_None);
	//} else {
	//	CameraNotConnected();
	//}
	if (isCameraConnected()) {
		if (!RunProtectedSection("Initialization->RegisterCameraEventHandler:EventFrameStart", [this]() {
			camera_->RegisterCameraEventHandler(pCameraEventHandler_, "EventFrameStart", FrameStartWaitDataEvent, RegistrationMode_ReplaceAll, Cleanup_None);
		}))
			return false;
	} else {
		CameraNotConnected();
	}

	// The handler is registered for both, the EventExposureEndFrameID and the EventExposureEndTimestamp
	// node. These nodes represent the data carried by the Exposure End event.
	// For each Exposure End event received, the handler will be called twice, once for the frame ID, and
	// once for the time stamp.
	if (isCameraConnected()) {
		if (!RunProtectedSection("Initialization->RegisterCameraEventHandler:EventExpousreEndFrameID", [this]() {
			camera_->RegisterCameraEventHandler(pCameraEventWriter_, "EventExposureEndFrameID", ExposureEndEvent, RegistrationMode_Append, Cleanup_None);
		}))
			return false;
	} else {
		CameraNotConnected();
	}
	if (isCameraConnected()) {
		if (!RunProtectedSection("Initialization->RegisterCameraEventHandler:EventExposureEndTimestamp", [this]() {
			camera_->RegisterCameraEventHandler(pCameraEventWriter_, "EventExposureEndTimestamp", ExposureEndEvent, RegistrationMode_Append, Cleanup_None);
		}))
			return false;
	} else {
		CameraNotConnected();
	}

	camera_->Open();

	model_ = camera_->GetDeviceInfo().GetModelName();

	if (isCameraConnected()) {
		// Get camera device information.
		if (!RunProtectedSection("Initialization->GetCameraDeviceInformation", [this]() {
			std::stringstream ss;
			ss << "Using device: " << camera_->GetDeviceInfo().GetModelName() << endl
				<< "   Max Width: " << camera_->Width.GetMax() << endl
				<< "  Max Height: " << camera_->Height.GetMax();
			Logger::L().Log (MODULENAME, severity_level::normal, ss.str());

			GenApi::INodeMap& nodemap = camera_->GetNodeMap();
			//Logger::L().Log (MODULENAME, severity_level::normal, "  Vendor: " + std::string(GenApi::CStringPtr(nodemap.GetNode("DeviceVendorName"))->GetValue()));
			//Logger::L().Log (MODULENAME, severity_level::normal, "   Model: " + std::string(GenApi::CStringPtr(nodemap.GetNode("DeviceModelName"))->GetValue()));
			std::string deviceFirmwareVersion = std::string(GenApi::CStringPtr(nodemap.GetNode("DeviceFirmwareVersion"))->GetValue());
			//Logger::L().Log (MODULENAME, severity_level::normal, "DeviceFirmware: " + deviceFirmwareVersion);

			// Example deviceFirmwareVersion: 106541 - 27; U; acA2040_90u; V1.1 - 5; 0
			std::vector<std::string> outStrings;
			boost::algorithm::split(outStrings, deviceFirmwareVersion, boost::algorithm::is_any_of(";"), boost::algorithm::token_compress_on);
			std::string deviceAndVersion = outStrings[0]; // Contains camera ID and firmware version.
			outStrings.clear();
			// Split up the camera ID and firmware version.
			boost::algorithm::split(outStrings, deviceAndVersion, boost::algorithm::is_any_of("-"), boost::algorithm::token_compress_on);

			version_ = std::string(VersionInfo::getVersionString()) + "/" + outStrings[1];
			Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("Driver/Firmware version: %s") % version_));
		}))
			return false;

	} else {
		CameraNotConnected();
	}

	// Check if we are running in a WIN10 VM.
	// If so, limit the image size and camera bandwidth to allow the application to run in the VM in a reduced mode.
	if (!RunProtectedSection("Initialization->SetWidthAndHeight", [this]() {
		char* buf = nullptr;
		size_t sz = 0;
	
		if (_dupenv_s(&buf, &sz, "WIN10_VM") == 0 && buf != nullptr)
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "Running in WIN10_VM, setting camera width/height to 512/512.");
			imageWidth_ = VmWidth;
			imageHeight_ = VmHeight;
			if (isCameraConnected())
			{
				camera_->Width.SetValue(imageWidth_);
			}
			else
			{
				CameraNotConnected();
			}
			if (isCameraConnected())
			{
				camera_->Height.SetValue(imageHeight_);
			}
			else
			{
				CameraNotConnected();
			}
			if (isCameraConnected())
			{
				camera_->DeviceLinkThroughputLimit.SetValue(10240000);	// 10Mbits <or> 10Mbytes?
			}
			else
			{
				CameraNotConnected();
			}
			free(buf);
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::normal, "Running in full image, setting camera width/height to 2048/2048.");
			imageWidth_ = FullWidth;
			imageHeight_ = FullHeight;
			camera_->Width.SetValue (imageWidth_);
			camera_->Height.SetValue (imageHeight_);
		}


		auto widthMax = camera_->WidthMax.GetValue();

		auto heightMax = camera_->HeightMax.GetValue();

		auto offsetX = (widthMax - imageWidth_) / 2;
		auto offsetY = (heightMax - imageHeight_) / 2;

		camera_->OffsetX.SetValue(offsetX);
		camera_->OffsetY.SetValue(offsetY);

		globalImageSize.width  = imageWidth_;
		globalImageSize.height = imageHeight_;
	}))
		return false;

	// Disable Frame Burst mode.
	if (!RunProtectedSection("Initialization->DisableFrameBurstMode", [this]() {
		if (isCameraConnected()) {
			camera_->TriggerSelector.SetValue(Basler_UniversalCameraParams::TriggerSelector_FrameBurstStart);
		}
		else
		{
			CameraNotConnected();
		}
		if (isCameraConnected())
		{
			camera_->TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_Off);
		}
		else
		{
			CameraNotConnected();
		}
	}))
		return false;

	// Enable Frame mode.
	if (!RunProtectedSection("Initialization->EnableFrameMode", [this]() {
		if (isCameraConnected())
		{
			camera_->TriggerSelector.SetValue(Basler_UniversalCameraParams::TriggerSelector_FrameStart);
		}
		else
		{
			CameraNotConnected();
		}
	}))
		return false;

	if (!RunProtectedSection("Initialization->SetTriggerandExposure", [this]() {
		if (isCameraConnected())
		{
			bool isa2A2448 = model_ == a2A2448_75umBAS;
			auto triggerSource = isa2A2448 ? Basler_UniversalCameraParams::TriggerSource_Line3 : Basler_UniversalCameraParams::TriggerSource_Line1;

			camera_->TriggerMode.SetValue(Basler_UniversalCameraParams::TriggerMode_On);
			camera_->TriggerSource.SetValue(triggerSource);
			camera_->TriggerActivation.SetValue(Basler_UniversalCameraParams::TriggerActivation_RisingEdge);

			if(isa2A2448)
			{
				camera_->ExposureMode.SetValue(Basler_UniversalCameraParams::ExposureMode_Timed);
				camera_->ExposureTime.SetValue(DEFAULT_EXPOSURE);
			}
			else
			{
				camera_->ExposureMode.SetValue(Basler_UniversalCameraParams::ExposureMode_TriggerWidth);
			}
		}
		else
		{
			CameraNotConnected();
		}


	}))
		return false;

	// This does not do anything other than force the *getGainRange* to log the current range.
	double minGain;
	double maxGain;
	getGainRange (minGain, maxGain);

	// Save current camera configuration for debugging.
	if (Logger::L().IsOfInterest (severity_level::debug1)) {

		try {
			const char Filename[] = "CameraNodeMap.txt"; // Pylon Feature Stream
			if (isCameraConnected()) {
				CFeaturePersistence::Save(Filename, &camera_->GetNodeMap());
			} else {
				CameraNotConnected();
			}
		}
		catch (Pylon::GenericException &e) {
			std::stringstream ss;
			ss << "Exception occurred: " << e.GetDescription();
			Logger::L().Log (MODULENAME, severity_level::error, ss.str());
			return false;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::normal, "initialize: <exit>");

	return true;
}

//*****************************************************************************
bool Camera_Basler::isCameraConnected() {

	return isCameraConnected_.load();
}

//*****************************************************************************
void setCameraConnectedState (bool state) {

	if (isCameraConnected_ != state && state == false)
	{
		// Report error once when camera is disconnected
		CameraNotConnected();
	}
	isCameraConnected_ = state;
}

//*****************************************************************************
bool Camera_Basler::setOnTriggerCallback (CompletionCallback_t onCameraTriggerCompletion)
{
	if (isOnTriggerCallbackRegistered_)
	{
		if (!RunProtectedSection("SetTriggerCallback", [this]() {
			camera_->DeregisterImageEventHandler(imageEventHandler_);
			}))
		return false;
	}

	// Register camera trigger event handler.
	if (isCameraConnected())
	{
		if (!RunProtectedSection("SetTriggerCallback", [this, onCameraTriggerCompletion]() {
			imageEventHandler_ = new Camera_BaslerImageEventHandler();
			imageEventHandler_->setCompletionHandler(onCameraTriggerCompletion);
			camera_->RegisterImageEventHandler (imageEventHandler_, RegistrationMode_ReplaceAll, Cleanup_Delete);

			isOnTriggerCallbackRegistered_ = true;
			}))
			return false;
	}
	else
	{
		CameraNotConnected();
		return false;
	}
	return true;
}

//*****************************************************************************
std::string Camera_Basler::getVersion() {

	return version_;
}

//*****************************************************************************
bool Camera_Basler::setGain (double gain) {

	if (!camera_|| !camera_->IsOpen()) {
		return false;
	}

	if (gain < minGain_ || gain > maxGain_) {
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format("Invalid camera gain %8.5f, valid range is %8.5f to %8.5f") % gain % minGain_ % maxGain_));
		return false;
	}
	
	return RunProtectedSection("setGain", [this, gain]() {
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format("Setting camera gain to %8.5f") % gain));
		if (isCameraConnected()) {
			camera_->GainSelector.SetValue(Basler_UniversalCameraParams::GainSelector_All); 
		} else {
			CameraNotConnected();
		}

		if (isCameraConnected()) {
			camera_->Gain.SetValue(gain);
		} else {
			CameraNotConnected();
		}
	});
}

//*****************************************************************************
bool Camera_Basler::getGainRange (double& minGain, double& maxGain) {

	if (!camera_ || !camera_->IsOpen()) {
		return false;
	}

	//Logger::L().Log (MODULENAME, severity_level::debug1, "Before Sleep...");
	//Sleep (30000);
	//Logger::L().Log (MODULENAME, severity_level::debug1, "After Sleep...");
	return RunProtectedSection("getGainRange", [this, &minGain, &maxGain]() {
		if (isCameraConnected()) {
			minGain = minGain_ = camera_->Gain.GetMin();
		} else {
			CameraNotConnected();
		}
		if (isCameraConnected()) {
			maxGain = maxGain_ = camera_->Gain.GetMax();
		} else {
			CameraNotConnected();
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Camera gain: min=%3.2f, max=%3.2f") % minGain_ % maxGain_));
	});
}

//*****************************************************************************
bool Camera_Basler::setGainLimitsState (bool state) {

	if (!camera_ || !camera_->IsOpen()) {
		return false;
	}

	if (!RunProtectedSection("setGainLimitsState", [this, state]() {
		if (isCameraConnected()) {
			camera_->RemoveParameterLimitSelector.SetValue (Basler_UniversalCameraParams::RemoveParameterLimitSelector_Gain);
		} else {
			CameraNotConnected();
		}
		if (isCameraConnected()) {
			camera_->RemoveParameterLimit.SetValue (state);
		} else {
			CameraNotConnected();
		}
	}))
		return false;

	double minGain;
	double maxGain;
	getGainRange (minGain, maxGain);

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Gain Limit State: %s") % (state ? "on" : "off")));

	return true;
}

//*****************************************************************************
bool Camera_Basler::armTrigger (unsigned int triggerReadyTimeout, unsigned int exposureTimeInMicroseconds = 0) {

	if (!camera_ || !camera_->IsOpen()) {
		return false;
	}

	if (model_ == a2A2448_75umBAS)
	{
		auto desiredExposure = HawkeyeConfig::ExposureOffset + exposureTimeInMicroseconds;
		camera_->ExposureTime.SetValue(desiredExposure);

		// Ace2 cameras might have a slightly different exposure based on clock rate, etc, versus what we set.
		// Figure out this difference and account for it.
		auto actualExposure = camera_->BslEffectiveExposureTime.GetValue();
		auto difference = actualExposure - desiredExposure; //The extra bit it tacked on the end.
		auto newExposure = desiredExposure - difference;
		if (newExposure > 0)
		{
			camera_->ExposureTime.SetValue(newExposure);
		}
	}

	// Start the grabbing of one image.
	if (!isCameraConnected())
	{
		CameraNotConnected();
		return false;
	}	
	
	return RunProtectedSection("armTrigger", [this, triggerReadyTimeout]() {
		// Make sure camera is not grabbing anything
		if (camera_->IsGrabbing())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "armTrigger : Camera still grabbing image, something went wrong. Stopping image grab manually");
			CameraErrorLog::Log("armTrigger : Could not stop image grab, stopping manually");
			camera_->StopGrabbing();
		}
		camera_->StartGrabbing (1);
	});
}

//*****************************************************************************
bool Camera_Basler::waitForImage (unsigned int imageTimeout) {

	if (!camera_ || !camera_->IsOpen()) {
		return false;
	}

	bool status = true;
	if (!RunProtectedSection("waitForImage", [this, &status, imageTimeout]() {
		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when the number of images specified in the *StarGrabbing* method have been retrieved.
		while (camera_->IsGrabbing()) 
		{
//TODO: save this code in case there is a need for it later.
//			// Check that grab results are waiting.
//			if (isCameraConnected()) {
//				if (camera_->GetGrabResultWaitObject().Wait(20)) {
//					Logger::L().Log (MODULENAME, severity_level::debug3, "waitForImage: Grab results wait in the output queue.");
//				} else {
//					Logger::L().Log (MODULENAME, severity_level::error, "waitForImage: failed to grab image from camera within 5ms.");
////TODO: need to log an error here??? and return???
//				}
//			} else {
//				CameraNotConnected();
//				return false;
//			}

			// Retrieve grab results and notify the camera event and image event handlers.
			// The grab results are handled by the registered event handler.

			if (isCameraConnected()) {
				camera_->RetrieveResult (imageTimeout, pGrabResult_, TimeoutHandling_ThrowException);
			} else {
				CameraNotConnected();
				status = false;
				break;
			}

			if (!pGrabResult_) {
				Logger::L().Log (MODULENAME, severity_level::critical, "waitForImage: camera grab result pointer is not valid");
				status = false;
				break;
			}

			// Image grabbed successfully?
			if (!pGrabResult_->GrabSucceeded())
			{
				std::stringstream ss;
				ss << "waitForImage::GrabSucceeded() error: " << pGrabResult_->GetErrorCode() << " (" << pGrabResult_->GetErrorDescription() << ")" << endl;
				Logger::L().Log (MODULENAME, severity_level::error, ss.str());
				CameraErrorLog::Log(ss.str());
				status = false;
				break;
			}
		}

	}))
	{
		status = false;
	}

	if (!RunProtectedSection("waitForImage->Cleanup", [this, status]() {
		if (!status && camera_->IsGrabbing())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "waitForImage : Camera still grabbing image, something went wrong. Stopping image grab manually");
			CameraErrorLog::Log("waitForImage : Could not stop image grab, stopping manually");
			camera_->StopGrabbing();
		}
	}))
		status = false;

	return status;
}

//NOTE: not currently used.
////*****************************************************************************
//bool Camera_Basler::setImage (CGrabResultPtr pGrabResult) {
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



bool Camera_Basler::getImage (Mat& image) {

	if (!camera_ || !camera_->IsOpen())
	{
		CameraErrorLog::Log("Camera_Basler_getImage : Unable to open camera!");
		return false;
	}

	if (!pGrabResult_)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "getImage: camera grab result pointer is not valid");
		CameraErrorLog::Log("Camera_Basler_getImage : camera grab result pointer is not valid!");
		return false;
	}

	if (imageBitsPerChannel_ == 0)
	{
		auto pixelType = pGrabResult_->GetPixelType();
		if (pixelType == EPixelType::PixelType_Undefined)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "getImage: undefined pixel type");
			CameraErrorLog::Log("Camera_Basler_getImage : undefined pixel type");
			return false;
		}

		imageBitsPerChannel_ = static_cast<uint16_t>(Pylon::BitDepth(pixelType));
	}

	auto imageBytesPerChannel = static_cast<uint16_t>( (imageBitsPerChannel_ + 7) / 8 );
	if (imageBytesPerChannel == 1)
	{
		image = Mat(
			pGrabResult_->GetHeight(),
			pGrabResult_->GetWidth(), CV_8U,
			(uint8_t*)pGrabResult_->GetBuffer(),
			Mat::AUTO_STEP).clone();
	}
	else if (imageBytesPerChannel == 2)
	{
		image = Mat(
			pGrabResult_->GetHeight(),
			pGrabResult_->GetWidth(),
			CV_16U, (uint16_t*)pGrabResult_->GetBuffer(),
			Mat::AUTO_STEP).clone();
	}
	else
	{
		CameraErrorLog::Log("Camera_Basler_getImage : Invalid bits per channel");
		return false;
	}

	if (image.empty())
	{
		CameraErrorLog::Log("Camera_Basler_getImage : Image is empty");
		return false;
	}
	return true;
}

//*****************************************************************************
bool Camera_Basler::getImageSize (uint32_t& width, uint32_t& height) {

	width  = imageWidth_;
	height = imageHeight_;

	return true;
}

//*****************************************************************************
bool Camera_Basler::getImageInfo(iCamera::ImageInfo& imageInfo)
{
	if (imageBitsPerChannel_ == 0)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "getCameraInfo: undefined bits per channel");
		return false;
	}

	imageInfo.bitsPerChannel = imageBitsPerChannel_;

	return true;
}
