#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "HawkeyeError.hpp"
#include "WorkflowOperation.hpp"

class ReagentWorkflowOperation : public WorkflowOperation
{
public:
	enum ReagentOperation : uint8_t
	{
		ArmUp = 0,
		ArmDown,
		ArmPurge,
		UnlatchDoor,
	};

	struct Command
	{
		ReagentOperation operation;
	};

	ReagentWorkflowOperation (Command cmd)
		: WorkflowOperation(WorkflowOperation::Type::Reagent)
		, cmd_(cmd)
	{ }
	virtual std::string getTypeAsString() override;

	static std::unique_ptr<ReagentWorkflowOperation> BuildUpCommand()
	{
		ReagentWorkflowOperation::Command cmd{};
		cmd.operation = ReagentWorkflowOperation::ReagentOperation::ArmUp;
		return std::make_unique<ReagentWorkflowOperation>(cmd);
	};

	static std::unique_ptr<ReagentWorkflowOperation> BuildDownCommand()
	{
		ReagentWorkflowOperation::Command cmd{};
		cmd.operation = ReagentWorkflowOperation::ReagentOperation::ArmDown;
		return std::make_unique<ReagentWorkflowOperation>(cmd);
	};

	static std::unique_ptr<ReagentWorkflowOperation> BuildPurgeCommand()
	{
		ReagentWorkflowOperation::Command cmd{};
		cmd.operation = ReagentWorkflowOperation::ReagentOperation::ArmPurge;
		return std::make_unique<ReagentWorkflowOperation>(cmd);
	};

	static std::unique_ptr<ReagentWorkflowOperation> BuildDoorLatchCommand()
	{
		ReagentWorkflowOperation::Command cmd{};

		cmd.operation = ReagentWorkflowOperation::ReagentOperation::UnlatchDoor;
		return std::make_unique<ReagentWorkflowOperation>(cmd);
	};

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;

};
