#pragma once

#include <functional>
#include "WorkflowOperation.hpp"
#include "CompletionHandlerUtilities.hpp"

class AppendProcessingWorkflowOperation : public WorkflowOperation
{
public:
	struct Command
	{
		std::function<ICompletionHandler*()> appendProcessingCb;

		Command(std::function<ICompletionHandler*()> apc)
			: appendProcessingCb(apc)
		{ }
	};

	AppendProcessingWorkflowOperation (Command cmd) 
		: WorkflowOperation(WorkflowOperation::Type::AppendState)
		, cmd_(cmd) 
	{ }
	virtual std::string getTypeAsString() override;

	static std::unique_ptr<AppendProcessingWorkflowOperation> Build (std::function<ICompletionHandler*()> appendProcessingCb)
	{
		AppendProcessingWorkflowOperation::Command cmd{ appendProcessingCb };
		return std::make_unique<AppendProcessingWorkflowOperation>(cmd);
	}

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;
};
