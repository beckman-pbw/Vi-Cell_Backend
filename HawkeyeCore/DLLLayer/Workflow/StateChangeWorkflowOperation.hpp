#pragma once

#include "WorkflowOperation.hpp"

class StateChangeWorkflowOperation : public WorkflowOperation
{
public:
	struct Command
	{
		StateChangeWorkflowOperation_callback callback;
		uint16_t stateNumber;
		bool executionComplete;

		Command(StateChangeWorkflowOperation_callback cb, uint16_t state, bool ec)
			: callback(cb), stateNumber(state), executionComplete(ec)
		{ }
	};

	StateChangeWorkflowOperation (Command cmd) 
		: WorkflowOperation(WorkflowOperation::Type::StateChange)
		, cmd_(cmd)
	{ }
	virtual std::string getTypeAsString() override;

	uint16_t getStateNumber() const;

	static std::unique_ptr<StateChangeWorkflowOperation> Build(
		StateChangeWorkflowOperation_callback callback,
		uint16_t state, bool executionComplete = false)
	{
		StateChangeWorkflowOperation::Command cmd(callback, state, executionComplete);
		return std::make_unique<StateChangeWorkflowOperation>(cmd);
	}

protected:
	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) override;

private:
	struct Command cmd_;
};
