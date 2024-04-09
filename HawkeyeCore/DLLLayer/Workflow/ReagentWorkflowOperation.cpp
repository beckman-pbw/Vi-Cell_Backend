#include "stdafx.h"

#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "Hardware.hpp"
#include "HawkeyeError.hpp"
#include "Logger.hpp"
#include "ReagentWorkflowOperation.hpp"

static const char MODULENAME[] = "ReagentWorkflowOperation";

//*****************************************************************************
void ReagentWorkflowOperation::executeInternal(
	Wf_Operation_Callback onCompleteCallback)
{
	std::string logStr = getTypeAsString() + " : ";
	const auto& reagent = Hardware::Instance().getReagentController();

	switch ( cmd_.operation )
	{
		case ArmUp:
		{
			logStr.append("Asynchronous");
			reagent->ArmUp([this, onCompleteCallback](bool success) -> void
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to move reagent arm up!");
				}
				triggerCallback(onCompleteCallback, success);
			});
			
			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case ArmDown:
		{
			if (!reagent->IsDoorClosed())
			{
				logStr.append("Arm down not initiated, found door open!");
				Logger::L().Log (MODULENAME, severity_level::error, logStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eNotPermittedAtThisTime);
				return;
			}
			
			logStr.append("Asynchronous");
			reagent->ArmDown([this, onCompleteCallback](bool success) -> void
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to move reagent arm down!");
				}
				triggerCallback(onCompleteCallback, success);
			});
			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case ArmPurge:
		{
			if (!reagent->IsDoorClosed())
			{
				logStr.append("arm Purge not initiated, door is open");
				Logger::L().Log (MODULENAME, severity_level::error, logStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eNotPermittedAtThisTime);
				return;
			}

			logStr.append("Asynchronous");
			reagent->ArmPurge([this, onCompleteCallback](bool success) -> void
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "failed move reagent arm to purge position");
				}
				triggerCallback(onCompleteCallback, success);
			});
			if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
			}
			return;
		}

		case UnlatchDoor:
		{
			logStr.append("Asynchronous");
			reagent->UnlatchDoor([this, onCompleteCallback](bool success) -> void
			{
				if (!success)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to unlatch reagent door!");
				}
				triggerCallback(onCompleteCallback, success);
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
			return;
		}
	}

	if (Logger::L().IsOfInterest(severity_level::debug2) && !logStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
	}

	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}

//*****************************************************************************
std::string ReagentWorkflowOperation::getTypeAsString()
{
	std::string operationAsString;
	switch (cmd_.operation)
	{
		case ReagentWorkflowOperation::ArmUp:
			operationAsString = "ArmUp";
			break;
		case ReagentWorkflowOperation::ArmDown:
			operationAsString = "ArmDown";
			break;
		case ReagentWorkflowOperation::ArmPurge:
			operationAsString = "ArmPurge";
			break;
		case ReagentWorkflowOperation::UnlatchDoor:
			operationAsString = "UnlatchDoor";
			break;
		default:
			operationAsString = "Unknown";
			break;
	}

	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}
