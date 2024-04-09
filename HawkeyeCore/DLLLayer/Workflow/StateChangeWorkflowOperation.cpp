#include "stdafx.h"

#include "StateChangeWorkflowOperation.hpp"

static const char MODULENAME[] = "StateChangeWorkflowOperation";

std::string StateChangeWorkflowOperation::getTypeAsString()
{
	std::string operationAsString = "StateChange";
	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}

uint16_t StateChangeWorkflowOperation::getStateNumber() const
{
	return cmd_.stateNumber;
}

void StateChangeWorkflowOperation::executeInternal(
	Wf_Operation_Callback onCompleteCallback)
{
	if (cmd_.callback == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::error, getTypeAsString().append("Invalid callback instance"));
		triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
		return;
	}

	cmd_.callback (cmd_.stateNumber, cmd_.executionComplete);
	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}
