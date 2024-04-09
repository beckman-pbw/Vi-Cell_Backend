#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "HawkeyeError.hpp"
#include "WorkflowOperation.hpp"
#include "CameraAutofocus.hpp"

class FocusWorkflowOperation : public WorkflowOperation
{
public:
	enum FocusOperation : uint8_t
	{
		Home = 0,
		Auto,
		RunAnalysis,
	};

	struct Command
	{
		FocusOperation operation;
		HawkeyeConfig::LedType ledType;
		uint32_t exposureTime_usec;
	};

	FocusWorkflowOperation (Command cmd)
		: WorkflowOperation(WorkflowOperation::Type::Focus)
		, cmd_(cmd)
	{ }
	virtual std::string getTypeAsString() override;
	virtual uint8_t getOperation() override;
	virtual bool supportsCancellation() override;
	virtual bool cancelOperationInternal() override;
	std::shared_ptr<AutofocusResultsDLL> getAutofocusResult();

	static std::unique_ptr<FocusWorkflowOperation> BuildHomeCommand()
	{
		FocusWorkflowOperation::Command cmd{};
		cmd.operation = Home;
		return std::make_unique<FocusWorkflowOperation>(cmd);
	};

	static std::unique_ptr<FocusWorkflowOperation> BuildAutoCommand(
		HawkeyeConfig::LedType ledType, uint32_t exposureTime_usec) {
		FocusWorkflowOperation::Command cmd{};
		cmd.operation = Auto;
		cmd.ledType = ledType;
		cmd.exposureTime_usec = exposureTime_usec;
		
		return std::make_unique<FocusWorkflowOperation>(cmd);
	};

	static std::unique_ptr<FocusWorkflowOperation> BuildRunAnalysisCommand()
	{
		FocusWorkflowOperation::Command cmd{};
		cmd.operation = RunAnalysis;
		return std::make_unique<FocusWorkflowOperation>(cmd);
	};

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;
	std::unique_ptr<CameraAutofocus> pAutoFocus_;

};
