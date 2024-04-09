#pragma once

#include "WorkflowOperation.hpp"

class TimestampWorkflowOperation : public WorkflowOperation
{
public:
	struct Command
	{
		std::string comment;
		bool canLog;

		Command(std::string text, bool canLogText)
			: comment(text), canLog(canLogText)
		{ }
	};

	TimestampWorkflowOperation (Command cmd)
		: cmd_(cmd)
		, WorkflowOperation(WorkflowOperation::Type::TimeStamp)
	{
	}

	static std::unique_ptr<TimestampWorkflowOperation> Build (std::string comment)
	{
		const bool canLog = Logger::L().IsOfInterest(severity_level::debug1);
		TimestampWorkflowOperation::Command cmd{ comment, canLog };
		return std::make_unique<TimestampWorkflowOperation>(std::move(cmd));
	}

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	Command cmd_;
};
