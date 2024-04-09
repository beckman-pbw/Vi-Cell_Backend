#pragma once

#include <functional>

#include "WorkflowOperation.hpp"
#include "DeadlineTimerUtilities.hpp"

class WaitStateWorkflowOperation : public WorkflowOperation
{
public:
	struct Command
	{
		uint32_t waitMillisec;
		timer_countdown_callback cb;

		Command(uint32_t waitMs, timer_countdown_callback tcc)
			: waitMillisec(waitMs), cb(tcc)
		{ }
	};

	WaitStateWorkflowOperation (Command cmd)
		: WorkflowOperation(WorkflowOperation::Type::WaitState)
		, cmd_(cmd)
	{ }
	~WaitStateWorkflowOperation();

	static std::unique_ptr<WaitStateWorkflowOperation> Build (uint32_t waitMillisec, timer_countdown_callback cb)
	{
		WaitStateWorkflowOperation::Command cmd(waitMillisec, cb);
		return std::make_unique<WaitStateWorkflowOperation>(cmd);
	}

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;
	virtual bool supportsCancellation() override;
	virtual bool cancelOperationInternal() override;

private:
	struct Command cmd_;
	std::shared_ptr<DeadlineTimerUtilities> timerHelper_;
	bool cancelExplicit_;
};
