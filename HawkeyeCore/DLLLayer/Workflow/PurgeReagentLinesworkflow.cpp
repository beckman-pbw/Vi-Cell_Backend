#include "stdafx.h"

#include "EnumConversion.hpp"
#include "Fluidics.hpp"
#include "PurgeReagentLinesWorkflow.hpp"
#include "ReagentCommon.hpp"

static const char MODULENAME[] = "PurgeReagentLinesWorkflow";

const std::map<ePurgeReagentLinesState, std::string>
EnumConversion<ePurgeReagentLinesState>::enumStrings<ePurgeReagentLinesState>::data =
{
	{ ePurgeReagentLinesState::dprl_Idle, std::string("Idle") },
	{ ePurgeReagentLinesState::dprl_PurgeCleaner1, std::string("PurgingCleaner1") },
	{ ePurgeReagentLinesState::dprl_PurgeCleaner2, std::string("PurgingCleaner2") },
	{ ePurgeReagentLinesState::dprl_PurgeCleaner3, std::string("PurgingCleaner3") },
	{ ePurgeReagentLinesState::dprl_PurgeReagent1, std::string("prl_PurgingReagent1") },
	{ ePurgeReagentLinesState::dprl_PurgeDiluent, std::string("prl_PurgingDiluent") },
	{ ePurgeReagentLinesState::dprl_Completed, std::string("Completed") },
	{ ePurgeReagentLinesState::dprl_Failed , std::string("Failed") },
};

PurgeReagentLinesWorkflow::PurgeReagentLinesWorkflow (purge_reagentlines_callback_DLL callback)
	: Workflow(Workflow::Type::PurgeReagentLines)
	, callback_(callback)
{
	currentState_ = ePurgeReagentLinesState::dprl_Idle;
}

PurgeReagentLinesWorkflow::~PurgeReagentLinesWorkflow()
{}

HawkeyeError PurgeReagentLinesWorkflow::execute()
{
	if (currentState_ != ePurgeReagentLinesState::dprl_Idle)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	return  Workflow::execute();
}

HawkeyeError PurgeReagentLinesWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "aborting running operation");

	if (currentState_ != ePurgeReagentLinesState::dprl_Idle)
	{
		return Workflow::abortExecute();
	}

	return HawkeyeError::eSuccess;
}

std::string PurgeReagentLinesWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{	
	auto pos = static_cast<ReagentContainerPosition>(workflowSubType);
	switch (pos)
	{		
		case ReagentContainerPosition::eMainBay_1:
			return HawkeyeDirectory::Instance().GetWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::ePurge);
		case ReagentContainerPosition::eDoorLeft_2:
			return HawkeyeDirectory::Instance().GetWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eDoorLeft);
		case ReagentContainerPosition::eDoorRight_3:
			return HawkeyeDirectory::Instance().GetWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eDoorRight);
		case ReagentContainerPosition::eUnknown:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile() : Not supported ReagentContainerPosition type : " + std::to_string(workflowSubType));
			break;
		}
	}

	return std::string();
}

void PurgeReagentLinesWorkflow::triggerCompletionHandler (HawkeyeError he)
{
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Workflow execution failed!");		
		onWorkflowStateChanged (ePurgeReagentLinesState::dprl_Failed, true);
	}
	else
	{
		onWorkflowStateChanged (ePurgeReagentLinesState::dprl_Completed, true);
	}
	currentState_ = ePurgeReagentLinesState::dprl_Idle;

	Workflow::triggerCompletionHandler(he);
}

void PurgeReagentLinesWorkflow::onWorkflowStateChanged (uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged: " +
		EnumConversion<ePurgeReagentLinesState>::enumToString(static_cast<ePurgeReagentLinesState>(currentState_)));

	if (callback_ != nullptr)
	{
		// Update host with current state.
		auto state = static_cast<ePurgeReagentLinesState>(currentState);
		callback_(state);
	}
}
