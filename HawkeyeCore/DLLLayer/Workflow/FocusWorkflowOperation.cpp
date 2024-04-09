#include "stdafx.h"

#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "FocusWorkflowOperation.hpp"
#include "Hardware.hpp"
#include "HawkeyeError.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "FocusWorkflowOperation";

//*****************************************************************************
std::string FocusWorkflowOperation::getTypeAsString()
{
	std::string operationAsString;
	switch (cmd_.operation)
	{
		case Home:
			operationAsString = "Home";
			break;
		case Auto:
			operationAsString = "Auto";
			break;
		case RunAnalysis:
			operationAsString = "RunAnalysis";
			break;
		default:
			operationAsString = "Unknown";
			break;
	}

	return WorkflowOperation::getTypeAsString() + ":" + operationAsString;
}

uint8_t FocusWorkflowOperation::getOperation()
{
	return cmd_.operation;
}

bool FocusWorkflowOperation::supportsCancellation()
{
	// AutoFocus workflow operation supports cancellation
	return getOperation() == DataConversion::enumToRawVal(FocusOperation::Auto);
}

bool FocusWorkflowOperation::cancelOperationInternal()
{
	if (!supportsCancellation())
	{
		return WorkflowOperation::cancelOperationInternal();
	}

	Logger::L().Log (MODULENAME, severity_level::normal, "Cancelling workflow operation  : " + getTypeAsString());
	pAutoFocus_->cancelExecute();
	return true;
}

void FocusWorkflowOperation::executeInternal(Wf_Operation_Callback onCompleteCallback)
{
	std::string logStr = getTypeAsString() + " : ";
	std::string errStr = logStr;

	auto focusController = Hardware::Instance().getFocusController();

	switch (cmd_.operation)
	{
		case FocusWorkflowOperation::Home:
		{
			focusController->FocusHome(
				[this, focusController, onCompleteCallback, logStr](bool status)
			{
				if (!status || !focusController->IsHome())
				{
					Logger::L().Log (MODULENAME, severity_level::error, (logStr + "Failed to move focus motor to home!"));
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, 
						(logStr + "Homing completed : Current focus position : " + 
						 std::to_string (focusController->Position())));
				}
				triggerCallback(onCompleteCallback, status);
			});
			return;
		}
		case FocusWorkflowOperation::Auto:
		{
			CameraAutofocus::CAFParams params = {};
			params.ledType = cmd_.ledType;
			params.exposureTime_usec = cmd_.exposureTime_usec;
			params.pInternalIos = pServices_->getInternalIos();
			params.pUpstreamIos = pServices_->getInternalIos(); // set upstream as internal service

			if (!pAutoFocus_)
			{
				pAutoFocus_ = std::make_unique<CameraAutofocus>(params);
			}
			if (pAutoFocus_->isBusy())
			{
				errStr.append("System is busy in performing other operations");
				Logger::L().Log (MODULENAME, severity_level::error, errStr);
				triggerCallback(onCompleteCallback, HawkeyeError::eBusy);
				return;
			}

			pAutoFocus_->execute([this, onCompleteCallback, logStr](bool success) -> void
			{
				if (!success)
				{
					std::string errStr = logStr;
					errStr.append("Unable to execute camera auto focus operation : Operation Failed!");
					Logger::L().Log (MODULENAME, severity_level::error, errStr);
				}

				if (Hardware::Instance().getStageController()->IsCarouselPresent()) {
					SystemStatus::Instance().decrementSampleTubeCapacityCount();
				}

				triggerCallback(onCompleteCallback, success);
			});
			return;
		}
		case FocusWorkflowOperation::RunAnalysis:
		{
			errStr.append("Workflow operation not implemented yet!");
			Logger::L().Log (MODULENAME, severity_level::error, errStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eNotPermittedAtThisTime);
			return;
		}
		default:
		{
			errStr.append("Workflow operation is unknown!");
			Logger::L().Log (MODULENAME, severity_level::error, errStr);
			triggerCallback(onCompleteCallback, HawkeyeError::eNotPermittedAtThisTime);
			return;
		}
	}

	if (!logStr.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, logStr);
	}

	triggerCallback(onCompleteCallback, HawkeyeError::eSuccess);
}

std::shared_ptr<AutofocusResultsDLL> FocusWorkflowOperation::getAutofocusResult()
{
	if (!pAutoFocus_)
	{
		return std::shared_ptr<AutofocusResultsDLL>();
	}
	return pAutoFocus_->getResult();
}

