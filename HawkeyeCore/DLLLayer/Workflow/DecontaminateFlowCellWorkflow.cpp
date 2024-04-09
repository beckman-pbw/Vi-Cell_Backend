#include "stdafx.h"

#include "DecontaminateFlowCellWorkflow.hpp"
#include "EnumConversion.hpp"
#include "Fluidics.hpp"
#include "Hardware.hpp"
#include "HawkeyeLogicImpl.hpp"

static const char MODULENAME[] = "DecontaminateFlowCellWorkflow";

const std::map<eDecontaminateFlowCellState, std::string>
EnumConversion<eDecontaminateFlowCellState>::enumStrings<eDecontaminateFlowCellState>::data =
{
	{ eDecontaminateFlowCellState::dfc_Idle, std::string("Idle") },
	{ eDecontaminateFlowCellState::dfc_AspiratingBleach, std::string("AspiratingBleach") },
	{ eDecontaminateFlowCellState::dfc_Dispensing1, std::string("Dispensing1") },
	{ eDecontaminateFlowCellState::dfc_DecontaminateDelay, std::string("DecontaminateDelay") },
	{ eDecontaminateFlowCellState::dfc_Dispensing2, std::string("Dispensing2") },
	{ eDecontaminateFlowCellState::dfc_FlushingBuffer, std::string("FlushingBuffer") },
	{ eDecontaminateFlowCellState::dfc_FlushingAir, std::string("FlushingAir") },
	{ eDecontaminateFlowCellState::dfc_Completed, std::string("Completed") },
	{ eDecontaminateFlowCellState::dfc_Failed, std::string("Failed") },
	{ eDecontaminateFlowCellState::dfc_FindingTube, std::string("FindingTube") },
	{ eDecontaminateFlowCellState::dfc_TfComplete, std::string("Complete") },
	{ eDecontaminateFlowCellState::dfc_TfCancelled, std::string("Cancelled") },
};

DecontaminateFlowCellWorkflow::DecontaminateFlowCellWorkflow(
	flowcell_decontaminate_status_callback_DLL callback)
	: Workflow(Workflow::Type::DecontaminateFlowCell)
	, callback_(callback)
{
	currentState_ = eDecontaminateFlowCellState::dfc_Idle;
}

DecontaminateFlowCellWorkflow::~DecontaminateFlowCellWorkflow()
{ }

HawkeyeError DecontaminateFlowCellWorkflow::execute()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute : <enter>");

	if ( (currentState_ != eDecontaminateFlowCellState::dfc_Idle && currentState_ != eDecontaminateFlowCellState::dfc_TfCancelled)
		|| isBusy() )
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	currentState_ = eDecontaminateFlowCellState::dfc_FindingTube;

	int16_t instrumentType = HawkeyeConfig::Instance().get().instrumentType;
	if (instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		HawkeyeError he = Workflow::execute();
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow!");
			this->triggerCompletionHandler(he);
		}
	}
	else
	{
		auto stageController = Hardware::Instance().getStageController();

		stageController->SetStageProfile([this](bool) -> void {}, true);

		// get the default carrier position asynchronously
		stageController->moveToDefaultStagePosition([this, stageController](bool status) -> void
		{
			if (currentState_ == eDecontaminateFlowCellState::dfc_TfCancelled)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute : Workflow cancelled!");
				this->triggerCompletionHandler(HawkeyeError::eSuccess);
				return;
			}

			currentState_ = eDecontaminateFlowCellState::dfc_TfComplete;

			stageController->SetStageProfile([this](bool) -> void {}, true);

			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute : Failure moving to default carrier position or finding tube");
				this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
				return;
			}

			stageController->ProbeDown([this](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to move probe down");
					this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
					return;
				}

				HawkeyeError he = Workflow::execute();				
				if (he != HawkeyeError::eSuccess)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow!");
					this->triggerCompletionHandler(he);
				}
			});
		});
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute : <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError DecontaminateFlowCellWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "abortExecute : *** Aborting Running Operation ***");

	if (currentState_ != eDecontaminateFlowCellState::dfc_Idle)
	{
		if (currentState_ == eDecontaminateFlowCellState::dfc_FindingTube)
		{
			currentState_ = eDecontaminateFlowCellState::dfc_TfCancelled;
			Hardware::Instance().getStageController()->CancelMove([this](bool)
			{
				triggerCompletionHandler(HawkeyeError::eSuccess);
				Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit> move cancelled");
				return Workflow::abortExecute();
			});
			Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit> tube search cancelled");
			return HawkeyeError::eSuccess;
		}
		triggerCompletionHandler(HawkeyeError::eSuccess);
		Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit> not idle");
		return Workflow::abortExecute();
	}
	triggerCompletionHandler(HawkeyeError::eSuccess);
	Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit>");
	return HawkeyeError::eSuccess;
}

std::string DecontaminateFlowCellWorkflow::getWorkFlowScriptFile(uint8_t workflowSubType)
{
	if (workflowSubType > 0)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile : Not supported input index : " + std::to_string(workflowSubType));
		return std::string();
	}
	return  HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eDecontaminateFlowCell);
}

HawkeyeError DecontaminateFlowCellWorkflow::load(std::string filename)
{
	HawkeyeError he = Workflow::load(filename);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "load : failed to load workflow file '" + filename + "'");
		return he;
	}

	// Now add the workflow operation (not present in Script)
	emptySyringeAtBeginning();

	return he;
}

void DecontaminateFlowCellWorkflow::triggerCompletionHandler(HawkeyeError he)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <enter>");

	cleanupBeforeExit([this, he](bool)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : cleanupBeforeExit callback");

		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "triggerCompletionHandler : Workflow execution failed!");
			onWorkflowStateChanged(eDecontaminateFlowCellState::dfc_Failed, true);
		}
		else
		{
			onWorkflowStateChanged(eDecontaminateFlowCellState::dfc_Completed, true);
		}

		currentState_ = eDecontaminateFlowCellState::dfc_Idle;
		Workflow::triggerCompletionHandler(he);
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <exit>");
}

void DecontaminateFlowCellWorkflow::onWorkflowStateChanged(uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged: " + 
		EnumConversion<eDecontaminateFlowCellState>::enumToString(static_cast<eDecontaminateFlowCellState>(currentState_)));

	if ((currentState >= eDecontaminateFlowCellState::dfc_FindingTube) &&
		(currentState <= eDecontaminateFlowCellState::dfc_TfCancelled))
	{
		currentState = eDecontaminateFlowCellState::dfc_Idle;
	}

	if (callback_ != nullptr)
	{
		callback_(static_cast<eDecontaminateFlowCellState>(currentState));
	}
}

void DecontaminateFlowCellWorkflow::cleanupBeforeExit(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "cleanupBeforeExit : <enter>");

	auto stageController = Hardware::Instance().getStageController ();

	std::function<void(bool)> cleanupComplete = [=](bool status) -> void
	{
		bool holding = EJECT_HOLDING_CURRENT_OFF;
		if ( HawkeyeConfig::Instance().get().automationEnabled && !stageController->IsCarouselPresent() )
		{
			holding = EJECT_HOLDING_CURRENT_ON;
		}

		stageController->SetStageProfile([this, status, callback](bool disableOk) -> void
		{
			if (!disableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to disable holding current");
			}
			else
			{
				disableOk = status;
			}
			pServices_->enqueueInternal(callback, disableOk);
		}, holding);
	};

	stageController->ProbeUp([this, stageController, cleanupComplete](bool isProbeUp) -> void
	{
		if (!isProbeUp)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to move probe up");
			pServices_->enqueueInternal(cleanupComplete, isProbeUp);
			return;
		}

		std::function<void(bool)> finalCleanup = [=](bool status) -> void
		{
			int32_t angle = NoEjectAngle;
			if ( HawkeyeConfig::Instance().get().automationEnabled && !stageController->IsCarouselPresent() )
			{
				angle = DfltAutomationEjectAngle;
			}

			stageController->EjectStage([=](bool ejectStatus) -> void
			{
				if (!ejectStatus)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : error ejecting stage");
				}
				pServices_->enqueueInternal(cleanupComplete, status);
			}, angle);
		};

		// NOTE: hardware detection cannot discriminate between the use of the A-Cup and Carousel or plate;
		// if the carousel carrier is present, the workflow will behave as if it was the carrier of choice...
		if (stageController->IsCarouselPresent())
		{
			bool status = false;

			// this switch looks for the expected normal success states;
			// the flow will typically get here with those states set on normal closure;
			switch (currentState_)
			{
				case eDecontaminateFlowCellState::dfc_Idle:
				case eDecontaminateFlowCellState::dfc_Completed:
					status = true;
					break;

				default:
					status = false;
			}

			if ((currentState_ > eDecontaminateFlowCellState::dfc_Idle) &&
				(currentState_ < eDecontaminateFlowCellState::dfc_FindingTube))
			{
				SystemStatus::Instance().decrementSampleTubeCapacityCount();

				Hardware::Instance().getStageController()->GotoNextTube([=](int32_t tubePos) -> void
				{
					bool tubeStatus = tubePos > 0 && tubePos <= MaxCarouselTubes;
					if (!tubeStatus)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to go to next tube location");
					}
					finalCleanup(tubeStatus);
				});
				return;
			}
			finalCleanup(status);
		}
		else
		{
			finalCleanup(isProbeUp);
		}
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "cleanupBeforeExit : <exit>");
}
