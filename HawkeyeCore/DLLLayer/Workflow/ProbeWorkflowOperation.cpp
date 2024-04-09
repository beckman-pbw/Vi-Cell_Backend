#include "stdafx.h"

#include "Hardware.hpp"
#include "ProbeWorkflowOperation.hpp"

static const char MODULENAME[] = "ProbeWorkflowOperation";

std::string ProbeWorkflowOperation::getTypeAsString()
{
	std::string operationAsString;
	switch (cmd_.operation)
	{		
		case ProbeWorkflowOperation::ProbeUp:
			operationAsString = "ProbeUp";
			break;
		case ProbeWorkflowOperation::ProbeDown:
			operationAsString = "ProbeDown";
			break;
		default:
			operationAsString = "Unknown";
			break;
	}

	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}

void ProbeWorkflowOperation::executeInternal (Wf_Operation_Callback onCompleteCallback)
{
	std::string logStr = getTypeAsString() + " : ";
	const auto& stageController = Hardware::Instance().getStageController();
	
	switch (cmd_.operation)
	{
		case ProbeUp:
		{
			logStr.append("Asynchronous");
			stageController->ProbeUp([this, onCompleteCallback](bool success) -> void
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to move sample probe up!");
				}
				triggerCallback(onCompleteCallback, success);
				return;
			});
			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case ProbeDown:
		{
			logStr.append("Asynchronous");
			stageController->ProbeDown([this, onCompleteCallback](bool success) -> void
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to move sample probe down!");
				}
				triggerCallback(onCompleteCallback, success);
				return;
			});
			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		default:
		{
			logStr.append("Workflow operation is unknown!");
			Logger::L().Log (MODULENAME, severity_level::error, logStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eSoftwareFault);
			return ;
		}
	}

	if (Logger::L().IsOfInterest(severity_level::debug1) && !logStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, logStr);
	}

	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}
