#include "stdafx.h"

#include "Hardware.hpp"
#include "LoadNudgeExpelWorkflow.hpp"
#include "SyringeWorkflowOperation.hpp"

static const char MODULENAME[] = "LoadNudgeExpelWorkflow";

LoadNudgeExpelWorkflow::LoadNudgeExpelWorkflow(
	Manual_Sample_Operation op)
	: Workflow(Workflow::Type::LoadNudgeExpel)
	, op_(op)
{
	currentState_ = LoadNudgeExpelWorkflow::Status::eIdle;
}

LoadNudgeExpelWorkflow::~LoadNudgeExpelWorkflow()
{ }

HawkeyeError LoadNudgeExpelWorkflow::load(std::string filename)
{
	// Nudge workflow doesn't have a execution script because of undefined number of operation
	if (op_ == Manual_Sample_Operation::Nudge)
	{
		// Build the Nudge workflow operations
		HawkeyeError status = buildNudgeWorkflow();
		if (status != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to load workflow : failed to build Nudge workflow!");
		}
		return status;
	}
	return Workflow::load(filename);
}

HawkeyeError LoadNudgeExpelWorkflow::execute()
{
	if (currentState_ != LoadNudgeExpelWorkflow::Status::eIdle)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	checkPreconditionBeforeExecute([this](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to verify precondition before execute");
			this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
			return;
		}

		HawkeyeError he = Workflow::execute();
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow!");
			this->triggerCompletionHandler(he);
		}
	});

	currentState_ = LoadNudgeExpelWorkflow::Status::eBusy;
	return HawkeyeError::eSuccess;
}

void LoadNudgeExpelWorkflow::checkPreconditionBeforeExecute(std::function<void(bool)> callback)
{
	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "Preconditions skipped - Cell Health module");
		pServices_->enqueueInternal(callback, true);
		return;
	}
	
	// For Nudge and Expel workflow stage should be already at valid position
	// Since they are followed by "Load" operation
	if (op_ != Manual_Sample_Operation::Load)
	{
		Hardware::Instance().getStageController()->ProbeDown(callback);
		return;
	}

	// For Load : Move the stage to default position (A1 for plate, First tube found for carousel)
	Hardware::Instance().getStageController()->moveToDefaultStagePosition([this, callback](bool status) -> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to move to default carrier position");
			pServices_->enqueueInternal(callback, false);
			return;
		}

		Hardware::Instance().getStageController()->ProbeDown(callback);
	});
	
}

std::string LoadNudgeExpelWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	auto op = static_cast<Manual_Sample_Operation>(workflowSubType);
	switch (op)
	{
		case LoadNudgeExpelWorkflow::Load:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eLoadSample);
		case LoadNudgeExpelWorkflow::Expel:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eExpelSample);
		case LoadNudgeExpelWorkflow::Nudge:
			return std::string(); // Return empty string here. See LoadNudgeExpelWorkflow::load() method for more details
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile() : Not supported Manual_Sample_Operation type : " + std::to_string(workflowSubType));
			break;
		}
	}
	return std::string();
}

void LoadNudgeExpelWorkflow::triggerCompletionHandler(HawkeyeError he)
{
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Workflow execution failed!");
	}

	cleanupBeforeExit([this, he](bool status)
	{
		currentState_ = LoadNudgeExpelWorkflow::Status::eIdle;
		Workflow::triggerCompletionHandler(he);
	});
}

HawkeyeError LoadNudgeExpelWorkflow::buildNudgeWorkflow()
{
	if (op_ != Manual_Sample_Operation::Nudge)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Failed to build nudge workflow : Invalid Manual_Sample_Operation type");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	workflowOperations_.clear();

	// the amount of sample volume to dispense thru flow cell.
	// default value is 5 uL.
	auto nudgeVolume = HawkeyeConfig::Instance().get().manualSampleNudgeVolume;

	// the syringe speed to dispense sample volume thru flow cell.
	// default value is 3 uL/sec.
	auto nudgeSpeed = HawkeyeConfig::Instance().get().manualSampleNudgeSpeed;

	uint32_t currentVol;
	if (!Hardware::Instance().getSyringePump()->getPosition(currentVol))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to get current syringe position!");
		return HawkeyeError::eHardwareFault;
	}

	if (currentVol == 0)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Current syringe volume is zero : Syringe empty!");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	int32_t targetVol = (int32_t)currentVol - (int32_t)nudgeVolume;
	if (targetVol < 0)
	{
		// empty the syringe even if current available syringe volume
		// is smaller than the expected nudge volume.
		targetVol = 0;
	}

	auto port = SyringePumpPort(SyringePumpPort::FlowCell);
	auto direction = SyringePumpDirection::Clockwise;

	auto setValveCmd = SyringeWorkflowOperation::BuildSetValveCommand (port, direction);
	workflowOperations_.push_back(std::move(setValveCmd));

	auto moveCmd = SyringeWorkflowOperation::BuildMoveCommand (targetVol, nudgeSpeed);
	workflowOperations_.push_back(std::move(moveCmd));

	return HawkeyeError::eSuccess;
}

void LoadNudgeExpelWorkflow::cleanupBeforeExit(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// Do probe up only for "Expel" workflow and do NOT do it for Cell Health Modules
	if ((op_ != Manual_Sample_Operation::Expel) ||
		(HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule))
	{
		pServices_->enqueueInternal(callback, true);
		return;
	}

	Hardware::Instance().getStageController()->ProbeUp(
		[this, callback](bool isProbeUp) -> void
	{
		if (!isProbeUp)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to move probe up");
		}
		pServices_->enqueueInternal(callback, isProbeUp);
	});
}
