#include "stdafx.h"

#include "DrainReagentPackWorkflow.hpp"
#include "Fluidics.hpp"

static const char MODULENAME[] = "DrainReagentPackWorkflow";

DrainReagentPackWorkflow::DrainReagentPackWorkflow(
	drain_reagentpack_callback_DLL callback, 
	uint32_t repeatCount)
	: Workflow(Workflow::Type::DrainReagentPack)
	, callback_(callback)
	, repeatCount_(repeatCount)
{
	currentState_ = eDrainReagentPackState::drp_Idle;
}

DrainReagentPackWorkflow::~DrainReagentPackWorkflow()
{}

HawkeyeError DrainReagentPackWorkflow::execute()
{
	if (currentState_ != eDrainReagentPackState::drp_Idle)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	return Workflow::execute();
}

HawkeyeError DrainReagentPackWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "*** Aborting Running Operation ***");

	if (currentState_ != eDrainReagentPackState::drp_Idle)
	{
		return Workflow::abortExecute();
	}

	return HawkeyeError::eSuccess;
}

std::string DrainReagentPackWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	const auto valve_port = SyringePumpPort::Port(workflowSubType);
	switch (valve_port)
	{
		case SyringePumpPort::Reagent_1: return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eDrainReagent1);
		case SyringePumpPort::Cleaner_1: return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eDrainCleaner1);
		case SyringePumpPort::Cleaner_2: return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eDrainCleaner2);
		case SyringePumpPort::Cleaner_3: return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eDrainCleaner3);
		
		case SyringePumpPort::InvalidPort:
		case SyringePumpPort::Waste:
		case SyringePumpPort::ACup:
		case SyringePumpPort::FlowCell:
		case SyringePumpPort::Diluent:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile() : Not supported input index : " + std::to_string(workflowSubType));
			break;
		}
	}
	return std::string();
}

HawkeyeError DrainReagentPackWorkflow::load(std::string filename)
{
	// Repeat the same process multiple times (same as repeatCount_ + 1)
	
	// "Workflow::load()" method will read the script and load the operations (to be run) in vector (std::vector<std::shared_ptr<WorkflowOperation>>)
	// Append the processing to same vector

	boost::optional<size_t> wfScriptOpSize = boost::none;
	const uint32_t totalCount = repeatCount_ + 1;
	for (size_t index = 0; index < totalCount; index++)
	{
		HawkeyeError he = Workflow::load(filename);
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "load : failed to load file : " + filename);
			return he;
		}

		if (!wfScriptOpSize)
		{
			wfScriptOpSize = workflowOperations_.size();
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("load : total count <%d> repeated size<%d>") % totalCount % workflowOperations_.size()));
	
	// Check if for-loop has been executed at least 
	// and total workflow operations should match expected operation count
	if (!wfScriptOpSize || (wfScriptOpSize.get() * totalCount) != workflowOperations_.size())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "load : Incorrect workflow operations count w.r.t. repeat count!");
		return HawkeyeError::eValidationFailed;
	}
	return HawkeyeError::eSuccess;
}

void DrainReagentPackWorkflow::getTotalFluidVolumeUsage(
	std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap, 
	boost::optional<uint16_t> jumpToState)
{
	std::map<SyringePumpPort::Port, uint32_t> singleUsageSyringeVolumeMap;
	Workflow::getTotalFluidVolumeUsage(singleUsageSyringeVolumeMap, jumpToState);

	if (repeatCount_ > 0)
	{
		uint32_t totalCount = repeatCount_ + 1;		// Repeat + original
		for (auto& item : singleUsageSyringeVolumeMap)
		{
			item.second *= totalCount;
		}
	}

	syringeVolumeMap = singleUsageSyringeVolumeMap;
}

void DrainReagentPackWorkflow::triggerCompletionHandler(HawkeyeError he)
{
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Workflow execution failed!");
		onWorkflowStateChanged(eDrainReagentPackState::drp_Failed, true);
	}
	else
	{
		onWorkflowStateChanged(eDrainReagentPackState::drp_Completed, true);
	}
	currentState_ = eDrainReagentPackState::drp_Idle;

	Workflow::triggerCompletionHandler(he);
}

void DrainReagentPackWorkflow::onWorkflowStateChanged(uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	auto state = static_cast<eDrainReagentPackState>(currentState);

	if (callback_ != nullptr)
	{
		// Update host with current state.
		callback_(state);
	}
}