#include "stdafx.h"

#include <functional>
#include "WaitStateWorkflowOperation.hpp"

static const char MODULENAME[] = "WaitStateWorkflowOperation";

WaitStateWorkflowOperation::~WaitStateWorkflowOperation()
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "*** Destructor - WaitStateWorkflowOperation ***");
	cancelExplicit_ = false;
	timerHelper_.reset();
}

void WaitStateWorkflowOperation::executeInternal (Wf_Operation_Callback onCompleteCallback)
{
	cancelExplicit_ = false;

	auto feedbackTime = boost::posix_time::seconds(1);
#ifdef _DEBUG
	auto waitTime = boost::posix_time::milliseconds(1000);
#else
	auto waitTime = boost::posix_time::milliseconds(cmd_.waitMillisec);
#endif
	
	Logger::L().Log(MODULENAME, severity_level::debug1,
		boost::str(boost::format("Waiting for %u milliseconds") % cmd_.waitMillisec));

	if (!timerHelper_.get())
	{
		timerHelper_ = std::make_shared<DeadlineTimerUtilities>();
	}

	auto success = timerHelper_->wait(
		pServices_->getInternalIosRef(), waitTime, [&, onCompleteCallback](bool status)
	{
		status = status || cancelExplicit_; // Check if timer is cancelled explicitly then consider it as success
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "WaitStateWorkflowOperation : Timer operation failed!");
		}

		triggerCallback(onCompleteCallback, status);
	}, feedbackTime, cmd_.cb);

	if (!success)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "WaitStateWorkflowOperation : Unable to start timer!");
		triggerCallback(onCompleteCallback, HawkeyeError::eBusy);
	}
}

bool WaitStateWorkflowOperation::supportsCancellation()
{
	return true;
}

bool WaitStateWorkflowOperation::cancelOperationInternal()
{
	bool success = false;
	if (supportsCancellation() && timerHelper_.get() != nullptr)
	{
		cancelExplicit_ = true;
		success = timerHelper_->cancel();
	}

	if (!success)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "WaitStateWorkflowOperation : cancelOperationInternal : Unable to cancel timer!");
	}
	return success;
}
