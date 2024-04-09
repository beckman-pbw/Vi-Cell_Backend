#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "HawkeyeConfig.hpp"
#include "HawkeyeError.hpp"
#include "WorkflowOperation.hpp"

#define DefaultFrameRate 15

class CameraWorkflowOperation : public WorkflowOperation
{
public:
	enum CameraOperation : uint8_t 
	{
		SetGain = 0,
		TakePicture,
		Capture,
		RemoveGainLimits,
		ABI, //AdjustBackgroundIntensity
	};

	struct Command 
	{
		CameraOperation operation;
		double   gain;
		HawkeyeConfig::LedType ledType;
		uint32_t exposureTime_usec;
//NOTE: deprecated for now.
//    uint32_t numFrames_;

//TODO: what to do if this is not met?
//    uint32_t targetFlux_;
		uint32_t syringeVolumeUL;
		uint32_t syringeRate;
		uint16_t numImages;
		uint32_t targetIntensity;
		uint32_t targetIntensityOffset;
		bool     gainLimitState;
		uint32_t numWarmupImages;
		uint8_t  frameRate;
		std::function<void(cv::Mat)> takePictureCallback;
	};

	CameraWorkflowOperation(Command cmd)
		: WorkflowOperation(WorkflowOperation::Type::Camera)
		, cmd_(cmd)
	{ }
	virtual std::string getTypeAsString() override;
	virtual uint8_t getOperation() override;

	void setImageCount (uint16_t numImages);
	uint16_t getImageCount();

	static std::unique_ptr<CameraWorkflowOperation> BuildSetGainCommand (double gain)
	{
		CameraWorkflowOperation::Command cmd{};
		cmd.operation = CameraOperation::SetGain;
		cmd.gain = gain;
		return std::make_unique<CameraWorkflowOperation>(cmd);
	};

	static std::unique_ptr<CameraWorkflowOperation> BuildRemoveGainLimitsCommand (bool state) 
	{
		CameraWorkflowOperation::Command cmd{};
		cmd.operation = CameraOperation::RemoveGainLimits;
		cmd.gainLimitState = state;
		return std::make_unique<CameraWorkflowOperation>(cmd);
	};

	static std::unique_ptr<CameraWorkflowOperation> BuildCameraAdjustBackgroundIntensityCommand(
		uint32_t syringeVolumeUL, uint32_t syringeRate,
		uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType,
		uint32_t targetIntensity, uint32_t targetIntensityOffset)
	{
		CameraWorkflowOperation::Command cmd{};

		cmd.operation = CameraOperation::ABI;
		cmd.syringeVolumeUL = syringeVolumeUL;
		cmd.syringeRate = syringeRate;
		cmd.exposureTime_usec = exposureTime_usec;
		cmd.ledType = ledType;
		cmd.targetIntensity = targetIntensity;
		cmd.targetIntensityOffset = targetIntensityOffset;
		return std::make_unique<CameraWorkflowOperation>(cmd);
	};

	static std::unique_ptr<CameraWorkflowOperation> BuildCaptureCommand(
		uint32_t syringeVolumeUL,
		uint32_t syringeRate,
		uint32_t exposureTime_usec,
		HawkeyeConfig::LedType ledType,
		uint16_t numImages,
		uint16_t numWarmupImages,
		uint8_t frameRate,
		std::function<void(cv::Mat)> takePictureCallback)
	{
		CameraWorkflowOperation::Command cmd{};

		cmd.operation = CameraOperation::Capture;
		cmd.syringeVolumeUL = syringeVolumeUL;
		cmd.syringeRate = syringeRate;
		cmd.exposureTime_usec = exposureTime_usec;
		cmd.ledType = ledType;
		cmd.numImages = numImages;
		cmd.numWarmupImages = numWarmupImages;
		cmd.frameRate = frameRate;
		cmd.takePictureCallback = takePictureCallback;
		return std::make_unique<CameraWorkflowOperation>(cmd);
	};

	static std::unique_ptr<CameraWorkflowOperation> BuildTakePictureCommand(
		HawkeyeConfig::LedType ledType, double gain, uint32_t exposureTime_usec,
		std::function<void(cv::Mat)> takePictureCallback)
	{
		CameraWorkflowOperation::Command cmd{};

		cmd.operation = CameraOperation::TakePicture;
		cmd.gain = gain;
		cmd.ledType = ledType;
		cmd.exposureTime_usec = exposureTime_usec;
		cmd.takePictureCallback = takePictureCallback;
		return std::make_unique<CameraWorkflowOperation>(cmd);
	};

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;
	std::map<uint8_t, boost::optional<bool>> mParallelOpMap_;
	
	void warmUpLed(uint32_t numImages, std::function<void(bool)> onComplete);
	void waitTillAllOpComplete(std::function<void(bool)> onComplete);
	bool getParallelOpResult();

	void executeCaptureCmd(Wf_Operation_Callback onCompleteCallback);
};
