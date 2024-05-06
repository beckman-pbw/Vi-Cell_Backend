#include "stdafx.h"

#include "CameraAutofocusWorkflow.hpp"
#include "EnumConversion.hpp"
#include "FocusWorkflowOperation.hpp"
#include "Hardware.hpp"

static const char MODULENAME[] = "CameraAutofocusWorkflow";

const std::map<eAutofocusState, std::string>
EnumConversion<eAutofocusState>::enumStrings<eAutofocusState>::data =
{
	{eAutofocusState::af_WaitingForSample, std::string("AutoFocus_State_Waiting_for_Sample")},
	{eAutofocusState::af_PreparingSample, std::string("AutoFocus_State_Preparing_Sample")},
	{eAutofocusState::af_SampleToFlowCell, std::string("AutoFocus_State_Sample_to_Flow_Cell")},
	{eAutofocusState::af_SampleSettlingDelay, std::string("AutoFocus_State_Sample_Settling_Delay")},
	{eAutofocusState::af_AcquiringFocusData, std::string("AutoFocus_State_Acquiring_Focus_Data")},
	{eAutofocusState::af_WaitingForFocusAcceptance, std::string("AutoFocus_State_Waiting_for_Focus_Accept")},
	{eAutofocusState::af_FlushingSample, std::string("AutoFocus_State_Flushing_Sample")},
	{eAutofocusState::af_Cancelling, std::string("AutoFocus_State_Cancelling")},
	{eAutofocusState::af_Idle, std::string("AutoFocus_State_Idle")},
	{eAutofocusState::af_Failed, std::string("AutoFocus_State_Failed")},
	{eAutofocusState::af_Exiting, std::string("AutoFocus_State_Exiting")},
	{eAutofocusState::af_FocusAcceptance, std::string("AutoFocus_State_Focus_Accept")},
	{eAutofocusState::af_FindingTube, std::string("AutoFocus_State_Finding_Tube")},
	{eAutofocusState::af_TfComplete, std::string("AutoFocus_State_Tube_Find_Complete")},
	{eAutofocusState::af_TfCancelled, std::string("AutoFocus_State_Tube_Find_Cancelled")}
};

CameraAutofocusWorkflow::CameraAutofocusWorkflow(
	SamplePositionDLL focusbead_location,
	countdown_timer_callback_t_DLL timer_cb,
	autofocus_state_callback_t_DLL callback)
	: Workflow(Workflow::Type::CameraAutofocus)
	, callback_(callback)
	, focusbead_location_(focusbead_location)
	, timer_cb_(timer_cb)
	, opUserDecision_(boost::none)
	, cancelState_(eAutofocusState::af_Idle)
	, cleanupRun(false)
{
	currentState_ = eAutofocusState::af_Idle;

	HAWKEYE_ASSERT (MODULENAME, timer_cb_);

	timer_callback_ = [this](uint64_t remainingTime, SupportedTimerCountdown tcd)
	{
		this->timer_cb_(static_cast<uint32_t>(remainingTime));
	};
}

CameraAutofocusWorkflow::~CameraAutofocusWorkflow()
{  }

HawkeyeError CameraAutofocusWorkflow::execute()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute : <enter>");
	
	if ((currentState_ != eAutofocusState::af_Idle && currentState_ != eAutofocusState::af_TfCancelled)
		|| isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}
	
	cleanupRun = false;
	onWorkflowStateChanged(eAutofocusState::af_WaitingForSample, false);

	currentState_ = eAutofocusState::af_FindingTube;

	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		// set the cleaning cycle parameters
		setCleaningCycleIndices();

		HawkeyeError he = Workflow::execute();
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow!");
			this->triggerCompletionHandler(he);
		}
	}
	else
	{
		auto& stageController = Hardware::Instance().getStageController();

		stageController->SetStageProfile([this](bool) -> void {}, true);

		// get the default carrier position asynchronously
		stageController->moveToDefaultStagePosition([this, stageController](bool status) -> void
		{
			if (currentState_ == eAutofocusState::af_TfCancelled)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute : Workflow cancelled!");
				this->triggerCompletionHandler(HawkeyeError::eSuccess);
				return;
			}

			currentState_ = eAutofocusState::af_TfComplete;

			stageController->SetStageProfile([this](bool) -> void {}, true);

			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute : Failure moving to default carrier position or finding tube");
				this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
				return;
			}

			// override the input focusbead_location_
			focusbead_location_ = stageController->GetStagePosition();

			// Move the probe down
			stageController->ProbeDown([this](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to move probe down!");
					this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
					return;
				}

				Logger::L ().Log (MODULENAME, severity_level::normal, "Probe down");

				// set the cleaning cycle parameters
				setCleaningCycleIndices();

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

HawkeyeError CameraAutofocusWorkflow::execute(uint16_t jumpToState)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute(state) : <enter> with state: " + std::to_string(jumpToState));

	if (jumpToState == eAutofocusState::af_WaitingForSample || jumpToState == eAutofocusState::af_Idle)
	{
		return execute();
	}

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	if (!focusbead_location_.isValid())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Invalid focus bead location");		
		return HawkeyeError::eInvalidArgs;
	}

	// set the cleaning cycle parameters
	setCleaningCycleIndices();

	Logger::L().Log (MODULENAME, severity_level::debug1, "execute(state) : <exit>");

	return Workflow::execute(jumpToState);
}

HawkeyeError CameraAutofocusWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "abortExecute : *** Aborting Running Operation ***");

	if (currentState_ == eAutofocusState::af_Idle)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit>");
	}
	else
	{
		if ((currentState_ == eAutofocusState::af_FindingTube) ||
			(currentState_ == eAutofocusState::af_TfCancelled))
		{
			currentState_ = eAutofocusState::af_TfCancelled;
			Hardware::Instance().getStageController()->CancelMove([this](bool)
			{
				// not yet into the main workfow processing; must manually call complete operation
				triggerCompletionHandler(HawkeyeError::eSuccess);
				Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit> move cancelled");
				return Workflow::abortExecute();
			});
			Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit> tube search cancelled");
			return HawkeyeError::eSuccess;
		}
		else if (currentState_ == eAutofocusState::af_TfComplete)
		{
			// not yet into the main workfow processing; must manually call complete operation
			triggerCompletionHandler(HawkeyeError::eSuccess);
		}
		Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute : <exit> not idle");
	}
	return Workflow::abortExecute();
}

bool CameraAutofocusWorkflow::skipCurrentRunningState()
{
	auto state = (eAutofocusState)getCurrentState();
	do 
	{		
		if (!Workflow::skipCurrentRunningState())
		{
			state = eAutofocusState::af_Failed;
			continue;
		}
		Logger::L().Log (MODULENAME, severity_level::debug2, "*** Skipping Running Operation ***");
		state = (eAutofocusState)getCurrentState();

	} while (state == eAutofocusState::af_SampleSettlingDelay);
	return Workflow::skipCurrentRunningState();
}

std::string CameraAutofocusWorkflow::getWorkFlowScriptFile(uint8_t workflowSubType)
{
	if (workflowSubType > 0)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile : Not supported input index : " + std::to_string(workflowSubType));
		return std::string();
	}
	return  HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eAFMain);
}

HawkeyeError CameraAutofocusWorkflow::load(std::string filename)
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

void CameraAutofocusWorkflow::setUserDecision(bool accepted)
{
	opUserDecision_ = accepted;
	currentState_ = eAutofocusState::af_FocusAcceptance;
	triggerCompletionHandler(HawkeyeError::eSuccess);
}

void CameraAutofocusWorkflow::triggerCompletionHandler(HawkeyeError he)
{
	// Get the current state
	auto curState = (eAutofocusState) currentState_;

	Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <enter> with state: " + EnumConversion<eAutofocusState>::enumToString(curState));

	auto cleanUpLambda = [this, he]()
	{
		if (cleanupRun)
		{
			return;
		}
		
		cleanupRun = true;
		cleanupBeforeExit([=](bool)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : cleanupBeforeExit callback");

			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "triggerCompletionHandler : Workflow execution failed!");
				onWorkflowStateChanged(eAutofocusState::af_Failed, true);
			}

			currentState_ = eAutofocusState::af_Idle;
			Workflow::triggerCompletionHandler(he);
			Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <exit> cleanup");
			cleanupRun = false;

			auto sendExitState = [this]()
			{
				onWorkflowStateChanged(eAutofocusState::af_Idle, true);
			};
			pServices_->enqueueInternal(sendExitState);
		});

		Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <exit> cleanup lambda");
	};

	std::string logStr = "triggerCompletionHandler : set focus operation";
	severity_level logLevel = severity_level::error;

	switch (currentState_)
	{
		case eAutofocusState::af_WaitingForSample:
		case eAutofocusState::af_PreparingSample:
		case eAutofocusState::af_SampleToFlowCell:
		case eAutofocusState::af_SampleSettlingDelay:
		case eAutofocusState::af_FindingTube:
		case eAutofocusState::af_TfComplete:
		case eAutofocusState::af_TfCancelled:
		{
			if (currentState_ == eAutofocusState::af_TfCancelled)
			{
				logStr.append(" cancelled prior to moving the focus motor while searching for tube");
				logLevel = severity_level::debug1;
			}
			else
			{
				logStr.append(" failed prior to moving the focus motor");
			}
			Logger::L().Log (MODULENAME, logLevel, logStr);
			currentState_ = eAutofocusState::af_Exiting;
			break;
		}

		case eAutofocusState::af_AcquiringFocusData:
		case eAutofocusState::af_FlushingSample:
		case eAutofocusState::af_Cancelling:
		case eAutofocusState::af_Failed:
		{
			bool doRestore = false;

			updateHost(currentState_);
			if (currentState_ == eAutofocusState::af_Cancelling)
			{
				logStr.append(" cancelled");
				logLevel = severity_level::debug1;
			}
			else
			{
				logStr.append(" failed");
			}

			// The focus motor is moved only if the "eAutofocusState::af_AcquiringFocusData"
			// state has been executed and the focus motor moved or homed.
			// Check for the last valid executed state prior to the cancel, fail, or completion
			// operation occurred to ensure the restore is performed only when necesary.
			if ((cancelState_ >= eAutofocusState::af_AcquiringFocusData) &&
				(cancelState_ <= eAutofocusState::af_FlushingSample))
			{
				doRestore = true;
				logStr.append(" after moving the focus motor");
			}
			else
			{
				logStr.append(" prior to moving the focus motor");
			}

			Logger::L().Log (MODULENAME, logLevel, logStr);

			currentState_ = eAutofocusState::af_Exiting;
			if (doRestore)
			{
				updateHost(eAutofocusState::af_Cancelling);
				Hardware::Instance().RestoreFocusPositionFromConfig([this, cleanUpLambda](HawkeyeError success, int32_t position)
				{
					Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("triggerCompletionHandler : RestoreFocusPositionFromConfig : %s - With focus motor position at %f")
																					% ((success == HawkeyeError::eSuccess) ? "Success" : "Failure") % position));
					pServices_->enqueueInternal(cleanUpLambda);
				}, true);
				return;
			}
			break;
		}

		case eAutofocusState::af_WaitingForFocusAcceptance:
		{
			// Find Auto focus workflow operation and retrieve result
			for (const auto& it : workflowOperations_)
			{
				if (it->getType() != WorkflowOperation::Type::Focus)
					continue;
				if (it->getOperation() != static_cast<uint8_t>(FocusWorkflowOperation::FocusOperation::Auto))
					continue;

				pAutofocusResult_ = ((FocusWorkflowOperation*) it.get())->getAutofocusResult();
			}

			opCleaningIndex_.reset();	// Reset the cleaning since it has already been done
			updateHost(eAutofocusState::af_WaitingForFocusAcceptance);
			Workflow::triggerCompletionHandler(he);

			Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <exit> waiting for focus acceptance");
			return;
		}

		case eAutofocusState::af_FocusAcceptance:
		{
			if (pAutofocusResult_ && pAutofocusResult_->focus_successful)
			{
				bool accepted = opUserDecision_.get_value_or(false);

				std::string logStr = "triggerCompletionHandler : focus successful: focus position ";
				if (accepted)
				{
					logStr.append(": accepted by user");
				}
				else
				{
					logStr.append(": rejected by user");
				}

				Logger::L().Log (MODULENAME, severity_level::debug1, logStr);

				if (accepted)
				{
					// if auto focus routine was successful AND accepted then save the current focus position
					// to configuration file.
					// if unsuccessful or terminated abruptly or rejected by user then restore to
					// previously saved focus position.

					int32_t final_Af_Pos = pAutofocusResult_->final_af_position;
					if (Hardware::Instance().SaveFocusPositionToConfig(final_Af_Pos) != HawkeyeError::eSuccess)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "triggerCompletionHandler : Failed to save final auto focus position to configuration file : " + std::to_string(final_Af_Pos));
					}
					else
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : saved focus final position to file" + std::to_string(final_Af_Pos));
					}
				}
				else
				{
					updateHost(eAutofocusState::af_Cancelling);
					Hardware::Instance().RestoreFocusPositionFromConfig([this, cleanUpLambda](HawkeyeError success, int32_t position)
					{
						Logger::L().Log (MODULENAME, severity_level::normal, boost::str(boost::format("triggerCompletionHandler : RestoreFocusPositionFromConfig : %s - With focus motor position at %f")
																					   % ((success == HawkeyeError::eSuccess) ? "Success" : "Failure") % position));
						pServices_->enqueueInternal(cleanUpLambda);
					}, true);
					return;
				}
			}
			else
			{
				if (!pAutofocusResult_)
				{
					logStr.append(": result not valid");
				}
				else if (!pAutofocusResult_->focus_successful)
				{
					logStr.append(": focus not successful");
				}
				Logger::L().Log (MODULENAME, severity_level::debug1, logStr);
			}
			currentState_ = eAutofocusState::af_Exiting;
			break;
		}
		case eAutofocusState::af_Idle:
			break;
		case eAutofocusState::af_Exiting:
			return;
	}

	pServices_->enqueueInternal(cleanUpLambda);
}

// This method is called from the workflow when it transitions a state.
// It also may be called by an internal method needing to supply a transitional
// state notification (e.g. tube search states).
// The state reported to the UI may differ from the internal state where the UI
// does not care about a state or have it mapped to a display condition.
void CameraAutofocusWorkflow::onWorkflowStateChanged(uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged: " +
		EnumConversion<eAutofocusState>::enumToString(static_cast<eAutofocusState>(currentState_)));

	// map additional internal enum values to general state condition
	if ((currentState >= eAutofocusState::af_FindingTube) &&
		(currentState <= eAutofocusState::af_TfCancelled))
	{
		if (currentState == eAutofocusState::af_TfCancelled)
		{
			currentState = eAutofocusState::af_Idle;
		}
		else
		{
			currentState = eAutofocusState::af_WaitingForSample;
		}
	}

	// update the last valid operational state used prior to cancellation
	if (currentState_ < eAutofocusState::af_Cancelling)
	{
		cancelState_ = currentState_;
	}

	if (currentState == eAutofocusState::af_FlushingSample)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged : Flushing.");
	}
	else if (currentState == eAutofocusState::af_WaitingForFocusAcceptance ||
		currentState == eAutofocusState::af_FocusAcceptance)
	{
		if (currentState == eAutofocusState::af_WaitingForFocusAcceptance)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "onWorkflowStateChanged : *** Waiting for user acceptance. ***");
		}
		else if (currentState == eAutofocusState::af_FocusAcceptance)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged : received user acceptance.");
		}
		opCleaningIndex_.reset();	// Reset the cleaning since it has already been done
		triggerCompletionHandler(HawkeyeError::eSuccess);
	}

	updateHost(currentState);
}

// this method is called from the workflow when it receives an abort request
// Ensure that the cleaning indices are set or cleared appropriately for the aborted state
bool CameraAutofocusWorkflow::executeCleaningCycleOnTermination(size_t currentWfOpIndex, size_t endIndex,
                                                                std::function<void(HawkeyeError)> onCleaningComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onCleaningComplete);

	Logger::L().Log (MODULENAME, severity_level::debug1, "executeCleaningCycleOnTermination : <enter>");

	auto cleaningComplete = [this, onCleaningComplete](HawkeyeError he)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "executeCleaningCycleOnTermination: updating host with cancel state");

		// The focus motor is moved only if the "eAutofocusState::af_AcquiringFocusData"
		// state has been executed and the focus motor moved or homed.
		// Check for the last valid executed state prior to the cancel, fail, or completion
		// operation occurred to ensure the restore is performed only when necesary.

		if ((cancelState_ >= eAutofocusState::af_AcquiringFocusData) &&
			(cancelState_ <= eAutofocusState::af_FlushingSample))
		{
			updateHost(eAutofocusState::af_Cancelling);
		}

		triggerCompletionHandler(he);
		pServices_->enqueueInternal(onCleaningComplete, he);
	};

		// Find out if we can execute cleaning cycle or not
	bool canExecute = Workflow::executeCleaningCycleOnTermination(currentWfOpIndex, endIndex, cleaningComplete);
	if (canExecute)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "executeCleaningCycleOnTermination: updating host with flushing state");
		updateHost(eAutofocusState::af_FlushingSample);
	}
	return canExecute;
}

void CameraAutofocusWorkflow::updateHost(uint16_t currentState) const
{
	// Get the current state
	auto curState = (eAutofocusState) currentState;

	Logger::L().Log (MODULENAME, severity_level::debug1, "updateHost : with state: " + EnumConversion<eAutofocusState>::enumToString(curState) + " (" + std::to_string(currentState) + ")");

	auto state = static_cast<eAutofocusState>(currentState);

	if (callback_ != nullptr)
	{
		// Update host with current state and result.
		callback_(state, pAutofocusResult_);
		Logger::L().Log( MODULENAME, severity_level::debug1, "updateHost : after callback" );
	}
	else
	{
		Logger::L().Log( MODULENAME, severity_level::debug1, "updateHost : callback is nullptr" );
	}
}

void CameraAutofocusWorkflow::setCleaningCycleIndices()
{
	const auto cleaningState = eAutofocusState::af_FlushingSample;
	size_t startIndex = getWorkflowOperationIndex(cleaningState);
	startIndex++; // Start from cleaning process without updating UI with state change

	size_t endIndex = getWorkflowOperationIndex(eAutofocusState::af_WaitingForFocusAcceptance);
	endIndex--;

	if (endIndex < startIndex)
	{
		endIndex = startIndex;
	}

	opCleaningIndex_ = std::make_pair(startIndex, endIndex);
}

void CameraAutofocusWorkflow::cleanupBeforeExit(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "cleanupBeforeExit : <enter>");

	auto stageController = Hardware::Instance().getStageController();

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
			bool status = true;

			// this switch looks for the specific failure cases because the flow will
			// get here with the af_FlushingSample state set on normal closure
			switch (currentState_)
			{
				case eAutofocusState::af_Failed:
				case eAutofocusState::af_Cancelling:
					status = false;
					break;

				default:
					status = true;
			}

			if ((currentState_ > eAutofocusState::af_WaitingForSample) &&
				(currentState_ < eAutofocusState::af_FlushingSample))
			{
				// no need to run decrementSampleTubeCapacityCount() here;
				// it is done in the FocusWorkflowOperation module...
				stageController->GotoNextTube([=](int32_t tubePos) -> void
				{
					bool tubeStatus = tubePos > 0 && tubePos <= MaxCarouselTubes;
					if (!tubeStatus)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to go to next tube location");
					}
					else
					{
						tubeStatus = status;
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

std::shared_ptr<AutofocusResultsDLL> CameraAutofocusWorkflow::getAutofocusResult()
{
	return pAutofocusResult_;
}
