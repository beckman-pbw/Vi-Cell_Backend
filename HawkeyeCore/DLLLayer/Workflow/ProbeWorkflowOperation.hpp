#pragma once

#include "WorkflowOperation.hpp"

class ProbeWorkflowOperation : public WorkflowOperation
{
public:
	enum ProbeOperation : uint8_t
	{
		ProbeUp = 0,
		ProbeDown = 1,
	};

	struct Command
	{
		ProbeOperation operation;
	};

	ProbeWorkflowOperation (Command cmd)
		: WorkflowOperation(WorkflowOperation::Type::Probe)
		, cmd_(cmd)
	{ }
	virtual std::string getTypeAsString() override;

	static std::unique_ptr<ProbeWorkflowOperation> BuildProbeUpCommand()
	{
		ProbeWorkflowOperation::Command cmd{};
		cmd.operation = ProbeWorkflowOperation::ProbeOperation::ProbeUp;
		return std::make_unique<ProbeWorkflowOperation>(cmd);
	};

	static std::unique_ptr<ProbeWorkflowOperation> BuildProbeDownCommand()
	{
		ProbeWorkflowOperation::Command cmd{};
		cmd.operation = ProbeWorkflowOperation::ProbeOperation::ProbeDown;
		return std::make_unique<ProbeWorkflowOperation>(cmd);
	};

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	Command cmd_;
};
