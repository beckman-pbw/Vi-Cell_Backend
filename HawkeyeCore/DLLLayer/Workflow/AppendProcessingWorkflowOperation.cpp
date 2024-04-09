#include "stdafx.h"

#include "AppendProcessingWorkflowOperation.hpp"

std::string AppendProcessingWorkflowOperation::getTypeAsString()
{
	std::string operationAsString = "AppendProcessing";
	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}

void AppendProcessingWorkflowOperation::executeInternal (Wf_Operation_Callback onCompleteCallback)
{
	if (cmd_.appendProcessingCb == nullptr)
	{
		Logger::L().Log (getTypeAsString(), severity_level::error, "Invalid or NULL processing callback");
		triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
		return;
	}

	auto processing = cmd_.appendProcessingCb();
	if (processing == nullptr)
	{
		Logger::L().Log (getTypeAsString(), severity_level::error, "Invalid or NULL processing lambda");
		triggerCallback(onCompleteCallback, HawkeyeError::eInvalidArgs);
		return;
	}

	processing->whenComplete([this, onCompleteCallback](HawkeyeError he)
	{
		triggerCallback(onCompleteCallback, he);
	});
}
