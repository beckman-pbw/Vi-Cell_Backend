#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "HawkeyeConfig.hpp"
#include "HawkeyeError.hpp"
#include "WorkflowOperation.hpp"

//*****************************************************************************
class LedWorkflowOperation : public WorkflowOperation
{
public:
	enum LedOperation : uint8_t
	{
		Set = 0,
	};

	struct Command 
	{
		LedOperation operation;
		HawkeyeConfig::LedType ledType;
		float    percentPower;
		uint32_t simmerCurrentVoltage;
		uint32_t ltcd;
		uint32_t ctld;
		uint32_t feedbackPhotodiode;
	};

	LedWorkflowOperation (Command cmd)
		: WorkflowOperation(WorkflowOperation::Type::Led)
		, cmd_(cmd)
	{ }
	virtual std::string getTypeAsString() override;

	static std::unique_ptr<LedWorkflowOperation> BuildSetCommand(
		HawkeyeConfig::LedType ledType,
		float    percentPower,
		uint32_t simmerCurrentVoltage,
		uint32_t ltcd,
		uint32_t ctld,
		uint32_t  feedbackPhotodiode)
	{
		LedWorkflowOperation::Command cmd{};

		cmd.operation = LedOperation::Set;
		cmd.ledType = ledType;
		cmd.percentPower = percentPower;
		cmd.simmerCurrentVoltage = simmerCurrentVoltage;
		cmd.ltcd = ltcd;
		cmd.ctld = ctld;
		cmd.feedbackPhotodiode = feedbackPhotodiode;
		return std::make_unique<LedWorkflowOperation>(cmd);
	};

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;
};
