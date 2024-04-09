#include "stdafx.h"

#include "EnumConversion.hpp"
#include "Fluidics.hpp"
#include "FlushFlowCellWorkflow.hpp"

static const char MODULENAME[] = "FlushFlowCellWorkflow";

const std::map<eFlushFlowCellState, std::string>
EnumConversion<eFlushFlowCellState>::enumStrings<eFlushFlowCellState>::data =
{
	{ eFlushFlowCellState::ffc_Idle, std::string("Idle") },
	{ eFlushFlowCellState::ffc_FlushingCleaner, std::string("FlushingCleaner") },
	{ eFlushFlowCellState::ffc_FlushingConditioningSolution, std::string("FlushingConditioningSolution") },
	{ eFlushFlowCellState::ffc_FlushingBuffer, std::string("FlushingBuffer") },
	{ eFlushFlowCellState::ffc_FlushingAir, std::string("FlushingAir") },
	{ eFlushFlowCellState::ffc_Completed, std::string("Completed") },
	{ eFlushFlowCellState::ffc_Failed, std::string("Failed") },
};

//*****************************************************************************
FlushFlowCellWorkflow::FlushFlowCellWorkflow(
	flowcell_flush_status_callback_DLL callback)
	: Workflow(Workflow::Type::FlushFlowCell)
	, callback_(callback)
{
	currentState_ = eFlushFlowCellState::ffc_Idle;
}

//*****************************************************************************
FlushFlowCellWorkflow::~FlushFlowCellWorkflow()
{ }

//*****************************************************************************
HawkeyeError FlushFlowCellWorkflow::execute()
{
	if (currentState_ != eFlushFlowCellState::ffc_Idle)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	return Workflow::execute();
}

//*****************************************************************************
HawkeyeError FlushFlowCellWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "*** Aborting Running Operation ***");

	if (currentState_ != eFlushFlowCellState::ffc_Idle)
	{
		return Workflow::abortExecute();
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
std::string FlushFlowCellWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	FlushType flushType = static_cast<FlushType>(workflowSubType);
	switch (flushType)
	{
		case FlushType::eFlushFlowCell:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eFlushFlowCell);
		case FlushType::eStandardNightlyClean:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eStandardNightlyClean);
		case FlushType::eACupNightlyClean:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eACupNightlyClean);
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile() : Not supported input index : " + std::to_string(workflowSubType));
			break;
		}
	}
	return std::string();
}

//*****************************************************************************
HawkeyeError FlushFlowCellWorkflow::load (std::string filename)
{
	HawkeyeError he = Workflow::load(filename);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	// Now add the workflow operation (not present in Script)		
	emptySyringeAtBeginning();

	return he;
}

//*****************************************************************************
void FlushFlowCellWorkflow::triggerCompletionHandler (HawkeyeError he)
{
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Workflow execution failed!");
		onWorkflowStateChanged(eFlushFlowCellState::ffc_Failed, true);
	}
	else
	{
		onWorkflowStateChanged(eFlushFlowCellState::ffc_Completed, true);
	}
	currentState_ = eFlushFlowCellState::ffc_Idle;

	Workflow::triggerCompletionHandler(he);
}

//*****************************************************************************
void FlushFlowCellWorkflow::onWorkflowStateChanged (uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged: " +
		EnumConversion<eFlushFlowCellState>::enumToString(static_cast<eFlushFlowCellState>(currentState_)));

	if (callback_ != nullptr)
	{
		// Update host with current state.
		auto state = static_cast<eFlushFlowCellState>(currentState);
		callback_(state);
	}
}
