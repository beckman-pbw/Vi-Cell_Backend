#pragma once

#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#include <pylon/PylonGUI.h>
#endif
#include <pylon/BaslerUniversalInstantCamera.h>

#include "Interfaces/iCamera.hpp"
#include "CameraErrorLog.hpp"

using namespace Pylon;
using namespace std;

//*************************************************************************
// Enumeration used for distinguishing different events.
//*************************************************************************
enum Camera_BaslerEvents {
	ExposureEndEvent = 100,
	FrameStartWaitDataEvent,
	// More events can be added here.
};

//*************************************************************************
// Example handler for camera events.
//*************************************************************************
class Camera_BaslerCameraEventHandler : public CBaslerUniversalCameraEventHandler
{
public:
	// Only very short processing tasks should be performed by this method. Otherwise, the event notification will block the
	// processing of images.
	virtual void OnCameraEvent (CBaslerUniversalInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* /* pNode */) override
	{
		std::stringstream ss;

		switch (userProvidedId) {
			case ExposureEndEvent: // Exposure End event
				ss << "Exposure End event: FrameID: " << camera.EventExposureEndFrameID.GetValue() << ", Timestamp: " << camera.EventExposureEndTimestamp.GetValue();
				Logger::L().Log ("Camera_Basler", severity_level::normal, ss.str());
				break;
			case FrameStartWaitDataEvent: // Exposure End event
				ss << "Frame Start Wait Data event: FrameID: " << camera.EventExposureEndFrameID.GetValue() << ", Timestamp: " << camera.EventExposureEndTimestamp.GetValue();//
				Logger::L().Log ("Camera_Basler", severity_level::normal, "recvd: FrameStartWaitDataEvent");
				break;
				// More events can be added here.

		} // End "switch (userProvidedId)"
	}
};

//*****************************************************************************
// Image event handler.
//*****************************************************************************
class Camera_BaslerImageEventHandler : public CImageEventHandler
{
public:
	//****************************************************************************
	virtual void OnImageGrabbed (Pylon::CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult) override
	{
		onCompletion_();
	}

	//****************************************************************************
	void setCompletionHandler (iCamera::CompletionCallback_t onCompletion) {
		onCompletion_ = onCompletion;
	}

private:
	iCamera::CompletionCallback_t onCompletion_;
};

//*************************************************************************
class Camera_BaslerEventWriter : public CCameraEventHandler {
public:
	virtual void OnCameraEvent(Pylon::CInstantCamera& camera, intptr_t userProvidedId, GenApi::INode* pNode) override
	{
		std::stringstream ss;

		ss << "OnCameraEvent event for device " << camera.GetDeviceInfo().GetModelName()
			<< "User provided ID: " << userProvidedId << std::endl
			<< "Event data node name: " << pNode->GetName();

		GenApi::CValuePtr ptrValue(pNode);
		if (ptrValue.IsValid()) {
			ss << "Event node data: " << ptrValue->ToString();
		}
		Logger::L().Log ("Camera_Basler", severity_level::normal, ss.str());
	}
};

//*************************************************************************
class Camera_BaslerConfigurationEventPrinter : public CConfigurationEventHandler {
public:
	Camera_BaslerConfigurationEventPrinter() {
		//severityLevel_ = severity_level::debug1;
		severityLevel_ = severity_level::debug3;
	}

	virtual void OnAttach (CInstantCamera& /*camera*/) override {
		Logger::L().Log ("Camera_Basler", severityLevel_, "OnAttach event");
	}

	virtual void OnAttached (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnAttached event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnOpen (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnOpen event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnOpened (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnOpened event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnGrabStart (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnGrabStart event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnGrabStarted (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnGrabStarted event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnGrabStop (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnGrabStop event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnGrabStopped (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnGrabStopped event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnClose(CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnClose event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnClosed (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnClosed event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnDestroy (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnDestroy event for device " << camera.GetDeviceInfo().GetModelName();
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnDestroyed (CInstantCamera& /*camera*/) override {
		Logger::L().Log ("Camera_Basler", severityLevel_, "OnDestroyed event");
	}

	virtual void OnDetach (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnDetach event for device " << camera.GetDeviceInfo().GetModelName();
		CameraErrorLog::Log("Camera_Basler : OnDetach : " + ss.str());
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnDetached (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnDetached event for device " << camera.GetDeviceInfo().GetModelName();
		CameraErrorLog::Log("Camera_Basler : OnDetached : " + ss.str());
		Logger::L().Log ("Camera_Basler", severityLevel_, ss.str());
	}

	virtual void OnGrabError (CInstantCamera& camera, const char* errorMessage) override {
		std::stringstream ss;
		ss << "OnGrabError event for device " << camera.GetDeviceInfo().GetModelName()
			<< "Error Message: " << errorMessage;
		CameraErrorLog::Log("Camera_Basler : OnGrabError : " + ss.str());
		Logger::L().Log ("Camera_Basler", severity_level::critical, ss.str());
	}

	virtual void OnCameraDeviceRemoved (CInstantCamera& camera) override {
		std::stringstream ss;
		ss << "OnCameraDeviceRemoved event for device " << camera.GetDeviceInfo().GetModelName();
		CameraErrorLog::Log("Camera_Basler : OnCameraDeviceRemoved : " + ss.str());
		Logger::L().Log ("Camera_Basler", severity_level::critical, ss.str());
		
		void setCameraConnectedState (bool state);
		setCameraConnectedState (false);
	}

private:
	severity_level severityLevel_;
};

class Camera_Basler : public iCamera {
	
public:
	Camera_Basler ();
	virtual ~Camera_Basler();

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
	// Automagically call PylonInitialize and PylonTerminate to ensure that the pylon runtime
	// system is initialized during the lifetime of this object.
	// This MUST be the first private variable!!!
	PylonAutoInitTerm autoInitTerm;

	std::shared_ptr<CBaslerUniversalInstantCamera > camera_;
	CGrabResultPtr pGrabResult_;
	Camera_BaslerCameraEventHandler* pCameraEventHandler_;
	Camera_BaslerEventWriter* pCameraEventWriter_;
	double minGain_;
	double maxGain_;
	Mat image_;
	uint32_t imageWidth_;
	uint32_t imageHeight_;
	uint16_t imageBitsPerChannel_;
	Camera_BaslerImageEventHandler* imageEventHandler_;
	bool isOnTriggerCallbackRegistered_;
	std::string version_;
	std::string model_;

	static const std::string a2A2448_75umBAS;
	static const std::string acA2040_90umBAS;
	static const int DEFAULT_EXPOSURE = 50;


	bool RunProtectedSection(const std::string& sectionname, std::function<void()> code);
	std::shared_ptr<CBaslerUniversalInstantCamera> GetCamera();

};

