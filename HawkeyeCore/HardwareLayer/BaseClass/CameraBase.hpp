#pragma once

#include <stdint.h>
#include <queue>
#include <opencv2/opencv.hpp>

#include "CBOService.hpp"
#include "Logger.hpp"
#include "LedBase.hpp"
#include "Interfaces/iCamera.hpp"

#define DEFAULTIMAGEWIDTH  2048
#define DEFAULTIMAGEHEIGHT 2048

class CameraBase
{
public:
	CameraBase (std::shared_ptr<CBOService> pCBOService);
	virtual ~CameraBase();

	// Initialize the camera module
	void initialize(std::function<void(bool)> callback);

	// Takes single picture with provided exposure time for given led type
	void takePicture (
		uint32_t exposureTime_usec, 
		HawkeyeConfig::LedType ledType, 
		std::function<void(cv::Mat)> callback);

	// Takes multiple pictures with provided exposure time for given led type
	// Trigger per image callback as soon as frame is captured for all the frames
	// and finally completion callback when done
	void takePictures(
		uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType, uint32_t numFrames,
		uint8_t frameRatePerSecond, std::function<void(cv::Mat)> perImageCallack,
		std::function<void(bool)> completionCallback);

	virtual bool setGain (double gain) = 0;
	virtual bool setGainLimitsState(bool state) = 0;
	virtual bool getGainRange (double& minGain, double& maxGain) = 0;
	virtual bool getImageSize (uint32_t& width, uint32_t& height) = 0;
	virtual bool getImageInfo(iCamera::ImageInfo& imageInfo) = 0;
	virtual std::string getVersion() = 0;

	bool isBusy() { return isBusy_; }
	virtual bool isPresent() = 0;
	virtual void shutdown() = 0;

protected:

	virtual void initializeAsync(std::function<void(bool)> callback) = 0;

	// Capture single frame and trigger callback with captured frame
	virtual void captureSingleFrame (
		uint32_t exposureTime_usec, 
		HawkeyeConfig::LedType ledType, 
		std::function<void(cv::Mat)> callback) = 0;
	
	// Capture provided number of frames with user defined fps
	// Trigger per image callback as soon as frame is captured for all the frames
	// and finally completion callback when done
	virtual void captureMultipleFrame(
		uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType, uint32_t numFrames, uint8_t frameRatePerSecond,
		std::function<void(cv::Mat)> perImageCallack, std::function<void(bool)> completionCallback) = 0;

	std::shared_ptr<CBOService> pCBOService_;
	bool isBusy_ = false;

private:

	class QueueHandler
	{
	public:

		QueueHandler(std::shared_ptr<CBOService> pCBOService)
			: pCBOService_(pCBOService)
		{ }

		template<typename ... Args>
		std::function<void(Args...)> wrapCompletionCallback(std::function<void(Args...)> callback)
		{
			return [this, callback](Args... callbackArgs)
			{
				triggerCallbackAndDeleteEntry(callback, callbackArgs...);
			};
		}

		void enqueueRequest(std::function<void()> work)
		{
			pCBOService_->enqueueInternal([this, work]()
			{
				// If queue is currently empty then we can process input work immediately
				bool canProcessNow = queue_.empty();
				queue_.emplace(work);
				if (canProcessNow)
				{
					processNextRequest();
				}
			});
		}

	private:

		void processNextRequest()
		{
			if (queue_.empty())
			{
				return;
			}

			auto item = queue_.front();
			if (!item)
			{
				queue_.pop();
				processNextRequest();
				return;
			}

			pCBOService_->enqueueInternal(item);
		}

		template<typename ... Args>
		void triggerCallbackAndDeleteEntry(
			std::function<void(Args...)> callback, Args... callbackArgs)
		{
			if (callback)
			{
				pCBOService_->enqueueExternal (std::bind(callback, callbackArgs...));
			}

			// At this point "queue_" must contain at-least one entry
			HAWKEYE_ASSERT ("CameraQueueHandler", queue_.size () > 0);
			queue_.pop();
			processNextRequest();
		}

		std::queue<std::function<void()>> queue_;
		std::shared_ptr<CBOService> pCBOService_;
	};

	QueueHandler cameraQueueHandler_;
};

class CameraTriggerOperation : public ControllerBoardOperation::Operation
{
public:
	enum CameraTriggerCommand
	{
		TakePictures = 1,
	};
		
	CameraTriggerOperation ()
	{
		Operation::Initialize (&regs_);
		regAddr_ = RegisterIds::Camera1Regs;
	}
		
protected:
	CameraRegisters regs_;
};

class AssertCameraTriggerOperation : public CameraTriggerOperation
{
public:
	AssertCameraTriggerOperation (uint32_t exposureTime, uint32_t framesPerSec, uint32_t totalNumOfFrames, uint32_t ledType)
	{
		mode_ = WriteMode;
		regs_.Command = static_cast<uint32_t>(CameraTriggerOperation::TakePictures);
		regs_.ErrorCode = 0;
		regs_.LEDs = ledType;
		regs_.CET = exposureTime;
		regs_.FPS = framesPerSec;
		regs_.TNF = totalNumOfFrames;
		lengthInBytes_ = CAMERA_REGS_SIZE;
	}
};