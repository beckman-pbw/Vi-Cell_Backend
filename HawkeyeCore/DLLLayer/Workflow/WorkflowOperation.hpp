#pragma once

#include <stdint.h>

#include "HawkeyeError.hpp"
#include "HawkeyeServices.hpp"
#include "Logger.hpp"

typedef std::function<void(uint16_t, bool)> StateChangeWorkflowOperation_callback;
typedef std::function<void(HawkeyeError)> Workflow_Callback;

class WorkflowOperation
{
public:
	typedef Workflow_Callback Wf_Operation_Callback;

	typedef enum Type : uint8_t
	{
		Camera = 0,
		Led,
		SyringePump,
		Reagent,
		Focus,
		Tube,
		StateChange,
		WaitState,
		Probe,
		AppendState,
		TimeStamp,
	} Type;

public:

	virtual ~WorkflowOperation();
	void executeAsync (std::shared_ptr<HawkeyeServices> pServices, Wf_Operation_Callback onCompleteCallback);
	
	virtual std::string getTypeAsString();
	virtual uint8_t getOperation();
	bool cancelOperation();
	Type getType() { return type_; }

protected:
	WorkflowOperation (Type type);

	WorkflowOperation(const WorkflowOperation&) = delete;
	WorkflowOperation& operator=(const WorkflowOperation&) = delete;

	virtual void executeInternal(Wf_Operation_Callback onCompleteCallback) = 0;
	virtual bool supportsCancellation();
	virtual bool cancelOperationInternal();

	void triggerCallback(Wf_Operation_Callback onCompleteCallback, HawkeyeError he);
	void triggerCallback(Wf_Operation_Callback onCompleteCallback, bool status);

	Type type_;
	std::shared_ptr<HawkeyeServices> pServices_;

private:
	bool isOperationComplete_;
};