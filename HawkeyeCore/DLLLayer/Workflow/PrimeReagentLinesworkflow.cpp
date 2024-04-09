#include "stdafx.h"

#include "EnumConversion.hpp"
#include "Fluidics.hpp"
#include "PrimeReagentLinesWorkflow.hpp"

static const char MODULENAME[] = "PrimeReagentLinesWorkflow";

const std::map<ePrimeReagentLinesState, std::string>
EnumConversion<ePrimeReagentLinesState>::enumStrings<ePrimeReagentLinesState>::data =
{
	{ ePrimeReagentLinesState::prl_Idle, std::string("Idle") },
	{ ePrimeReagentLinesState::prl_PrimingCleaner1, std::string("PrimingCleaner1") },
	{ ePrimeReagentLinesState::prl_PrimingCleaner2, std::string("PrimingCleaner2") },
	{ ePrimeReagentLinesState::prl_PrimingCleaner3, std::string("PrimingCleaner3") },
	{ ePrimeReagentLinesState::prl_PrimingReagent1, std::string("prl_PrimingReagent1") },
	{ ePrimeReagentLinesState::prl_PrimingDiluent, std::string("prl_PrimingDiluent") },
	{ ePrimeReagentLinesState::prl_Completed, std::string("Completed") },
	{ ePrimeReagentLinesState::prl_Failed, std::string("Failed") },
};

PrimeReagentLinesWorkflow::PrimeReagentLinesWorkflow (prime_reagentlines_callback_DLL callback)
	: Workflow(Workflow::Type::PrimeReagentLines)
	, callback_(callback)
{
	currentState_ = ePrimeReagentLinesState::prl_Idle;
}

PrimeReagentLinesWorkflow::~PrimeReagentLinesWorkflow()
{}

HawkeyeError PrimeReagentLinesWorkflow::execute()
{
	if (currentState_ != ePrimeReagentLinesState::prl_Idle)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	return Workflow::execute();
}

HawkeyeError PrimeReagentLinesWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "aborting running operation");

	if (currentState_ != ePrimeReagentLinesState::prl_Idle)
	{
		return Workflow::abortExecute();
	}

	return HawkeyeError::eSuccess;
}

std::string PrimeReagentLinesWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	if (workflowSubType > 0)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile: unsupported input index : " + std::to_string(workflowSubType));
		return std::string();
	}
	return HawkeyeDirectory::Instance().getWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::ePrimeReagent);
}

HawkeyeError PrimeReagentLinesWorkflow::load (std::string filename)
{
	HawkeyeError he = Workflow::load (filename);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	// Now add the workflow operation (not present in Script)		
	emptySyringeAtBeginning();

	return he;
}

void PrimeReagentLinesWorkflow::triggerCompletionHandler (HawkeyeError he)
{
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Workflow execution failed!");		
		onWorkflowStateChanged(ePrimeReagentLinesState::prl_Failed, true);
	}
	else
	{
		onWorkflowStateChanged(ePrimeReagentLinesState::prl_Completed, true);
	}
	currentState_ = ePrimeReagentLinesState::prl_Idle;

	Workflow::triggerCompletionHandler(he);
}

void PrimeReagentLinesWorkflow::onWorkflowStateChanged (uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged: " +
		EnumConversion<ePrimeReagentLinesState>::enumToString(static_cast<ePrimeReagentLinesState>(currentState_)));

	if (callback_ != nullptr)
	{
		// Update host with current state.
		auto state = static_cast<ePrimeReagentLinesState>(currentState);
		callback_ (state);
	}
}
