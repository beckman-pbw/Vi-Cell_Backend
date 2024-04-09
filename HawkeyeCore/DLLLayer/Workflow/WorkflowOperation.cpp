#include "stdafx.h"

#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "HawkeyeAssert.hpp"
#include "WorkflowOperation.hpp"

static const char MODULENAME[] = "WorkflowOperation";

//*****************************************************************************
WorkflowOperation::WorkflowOperation (WorkflowOperation::Type type)
	: type_(type)
	, isOperationComplete_(false)
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "WorkflowOperation: <constructor>");
}

//*****************************************************************************
WorkflowOperation::~WorkflowOperation()
{
	Logger::L().Log ( MODULENAME, severity_level::debug3, "WorkflowOperation: <destructor>");
}

//*****************************************************************************
void WorkflowOperation::executeAsync (std::shared_ptr<HawkeyeServices> pServices, Wf_Operation_Callback onCompleteCallback)
{
	HAWKEYE_ASSERT ("WorkflowOperation::executeAsync", onCompleteCallback);

	Logger::L().Log (MODULENAME, severity_level::debug3, "WorkflowOperation::executeAsync");

	pServices_ = pServices;
	pServices_->enqueueInternal (std::bind(&WorkflowOperation::executeInternal, this, onCompleteCallback));
}

//*****************************************************************************
std::string WorkflowOperation::getTypeAsString()
{
	switch (type_)
	{
		case Camera:
			return "Camera";
		case Led:
			return "LED";
		case SyringePump:
			return "SyringePump";
		case Reagent:
			return "Reagent";
		case Focus:
			return "Focus";
		case Tube:
			return "Tube";
		case StateChange:
			return "StateChange";
		case WaitState:
			return "WaitState";
		case Probe:
			return "Probe";
		case AppendState:
			return "AppendState";
		case WorkflowOperation::TimeStamp:
			return "TimeStamp";
		default:
			return "Unknown";
	}
	return std::string();
}

//*****************************************************************************
bool WorkflowOperation::cancelOperation()
{
	if (supportsCancellation())
	{
		return cancelOperationInternal();
	}
	return true;
}

//*****************************************************************************
bool WorkflowOperation::supportsCancellation()
{
	return false;
}

//*****************************************************************************
bool WorkflowOperation::cancelOperationInternal()
{
	return false;
}

//*****************************************************************************
void WorkflowOperation::triggerCallback (Wf_Operation_Callback onCompleteCallback, HawkeyeError he)
{
	HAWKEYE_ASSERT (MODULENAME, onCompleteCallback);
	HAWKEYE_ASSERT (MODULENAME, !isOperationComplete_);

	pServices_->enqueueInternal (onCompleteCallback, he);
	isOperationComplete_ = true;
}

//*****************************************************************************
void WorkflowOperation::triggerCallback (Wf_Operation_Callback onCompleteCallback, bool status)
{
	auto he = status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault;
	triggerCallback (onCompleteCallback, he);
}

//*****************************************************************************
uint8_t WorkflowOperation::getOperation()
{
	return 0;
}
