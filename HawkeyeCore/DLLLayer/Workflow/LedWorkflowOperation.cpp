#include "stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <string>

#include "Hardware.hpp"
#include "HawkeyeError.hpp"
#include "LedWorkflowOperation.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "LedWorkflowOperation";

//*****************************************************************************
void LedWorkflowOperation::executeInternal (Wf_Operation_Callback onCompleteCallback)
{
	std::string logStr = getTypeAsString() + " : ";
	std::string errStr = logStr;
	const auto& led = Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField);

	switch (cmd_.operation)
	{		
		case Set:
		{
			logStr.append (boost::str(boost::format("\n Setting LED : Type<%s>, Power<%f>, CVoltage<%d>, ltcd<%d>, ctld<%d>, FeedbackPhotoDiode<%d>")
									 % led->getConfig()->name
									 % cmd_.percentPower
									 % cmd_.simmerCurrentVoltage
									 % cmd_.ltcd
									 % cmd_.ctld
									 % cmd_.feedbackPhotodiode));

			bool success = led->setConfig(
				cmd_.percentPower, 
				cmd_.simmerCurrentVoltage, 
				cmd_.ltcd, 
				cmd_.ctld, 
				cmd_.feedbackPhotodiode);
			if (!success)
			{
				errStr = logStr;
				errStr.append("Failed to set led!");
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eHardwareFault);
				return;
			}
			break;
		}

		default:
		{
			errStr.append("Workflow operation is unknown!");
			Logger::L().Log (MODULENAME, severity_level::error, errStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eSoftwareFault);
			return;
		}
	}

	if (!logStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
	}

	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}

//*****************************************************************************
std::string LedWorkflowOperation::getTypeAsString()
{
	std::string operationAsString;
	switch (cmd_.operation)
	{
		case LedWorkflowOperation::Set:
			operationAsString = "Set";
			break;
		default:
			operationAsString = "Unknown";
			break;
	}

	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}
