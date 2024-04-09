#include "stdafx.h"

#include "HawkeyeAssert.hpp"
#include "TimestampWorkflowOperation.hpp"

static const char MODULENAME[] = "TimestampWorkflowOperation";

void TimestampWorkflowOperation::executeInternal(Wf_Operation_Callback onCompleteCallback)
{
	HAWKEYE_ASSERT (MODULENAME, onCompleteCallback);
	
	if (cmd_.canLog && !cmd_.comment.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Comment : " + cmd_.comment);
	}

	pServices_->enqueueInternal (onCompleteCallback, HawkeyeError::eSuccess);
}
