#include "stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "AuditLog.hpp"
#include "AutofocusDLL.hpp"
#include "CameraAutofocusWorkflow.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "ImageWrapperUtilities.hpp"
#include "LedBase.hpp"
#include "LiveScanningUtilities.hpp"
#include "LoadNudgeExpelWorkflow.hpp"
#include "ReagentPack.hpp"
#include "StageControllerBase.hpp"
#include "TemporaryImageAnalysis.hpp"
#include "CalibrationHistoryDLL.hpp"

static const char MODULENAME[] = "Service";

static const HawkeyeConfig::LedType CameraLampLEDType = HawkeyeConfig::LedType::LED_BrightField;
static std::shared_ptr<TemporaryImageAnalysis> pTemporarySingleShotAnalysis_;
static std::shared_ptr<TemporaryImageAnalysis> pTemporaryContinuousAnalysis_;
static CalibrationState calibrationState_ = CalibrationState::eIdle;
static eImageOutputType serviceAnalysisImagePreference_ = eImageOutputType::eImageAnnotated;

static const int NOMINAL_FLOW_CELL_DEPTH = 63;

//************************************************************************************************************************
// Formula for calculating the flowcell offset (in microns) from the flowcell depth (in microns).
//       FlowcellOffset = (FlowcellDepthInMicrons / (2 * WATER_REFRACTIVE_INDEX)) + 
//                        (FlowcellDepthExperimentalConstant / (2 * WATER_REFRACTIVE_INDEX))
//
// Flowcell offset is considered as the sum of two terms.
// - The first term moves the focus from the bottom of the flowcell to the middle of the flowcell.
// - The second term further moves the focus position by an experimentally derived amount for "better contrast of cells".
//
// Refactoring the above formula gives:
//       FlowcellOffset = (FlowcellDepthExperimentalConstant + FlowcellDepthInMicrons) / (2 * WATER_REFRACTIVE_INDEX)
//************************************************************************************************************************
#define WATER_REFRACTIVE_INDEX 1.33
#define MICRONS_TO_COUNTS(microns) ((microns) * 10.0)
#define COUNTS_TO_MICRONS(counts) ((counts) / 10.0)
double OffsetFromFlowCellDepth (double flowCellDepth)
{
	return MICRONS_TO_COUNTS(HawkeyeConfig::Instance().get().flowCellDepthExperimentalConstant + flowCellDepth) / (2.0 * WATER_REFRACTIVE_INDEX);
}

double FlowCellDepthFromOffset(double offset)
{
	return COUNTS_TO_MICRONS(offset) * (2.0 * WATER_REFRACTIVE_INDEX) - HawkeyeConfig::Instance().get().flowCellDepthExperimentalConstant;
}

/*****************************************************************************/
static void initializeFocusController(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (Hardware::Instance().getFocusController()->IsInitialized())
	{
		callback(true);
		return;
	}
	t_pPTree cfgTree;
	Hardware::Instance().getFocusController()->Init(callback, cfgTree, true, HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::MotorControl));
}

void HawkeyeLogicImpl::svc_CameraFocusAdjust(std::function<void(HawkeyeError)> callback, bool direction_up, bool adjustment_fine)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	// This must be done by an Service or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized, 
			"Service: Camera Focus Adjust"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	pHawkeyeServices_->enqueueInternal(
		[this, loggedInUsername, callback, direction_up, adjustment_fine]()->void
	{
		int32_t currentFocusPos = Hardware::Instance().getFocusController()->Position();
		internal_svc_CameraFocusAdjust(
			loggedInUsername,
			sm_CameraFocusAdjust::cfa_InitializeFocusController, 
			direction_up, 
			adjustment_fine, 
			currentFocusPos, 
			callback);
	});
}

void HawkeyeLogicImpl::internal_svc_CameraFocusAdjust(
	std::string loggedInUsername,
	sm_CameraFocusAdjust state, 
	bool direction_up, 
	bool adjustment_fine, 
	int32_t startFocusPos,
	std::function<void(HawkeyeError)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [=](sm_CameraFocusAdjust nextState)
	{
		pHawkeyeServices_->enqueueInternal([=]()
		{
			internal_svc_CameraFocusAdjust (loggedInUsername, nextState, direction_up, adjustment_fine, startFocusPos, callback);
		});
	};

	auto& focusController = Hardware::Instance().getFocusController();

	switch (state)
	{
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_InitializeFocusController:
		{
			initializeFocusController([onCurrentStateComplete, adjustment_fine](bool initOk)
			{
				if (!initOk)
				{
					Logger::L().Log (MODULENAME, severity_level::critical, "internal_svc_CameraFocusAdjust : cfa_InitializeFocusController :Failed to initialize focus controller!");
					onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Error);
					return;
				}
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_CheckMotorHome);
			});
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_CheckMotorHome:
		{
			// Check if focus motor is already being homed ever after the system power up
			bool isFocusMotorHomed = (SystemStatus::Instance().getData().motor_Focus.flags & eMotorFlags::mfHomed) != 0;
			if (isFocusMotorHomed)
			{
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_DecideMotorMovement);
				return;
			}

			// If focus motor is not being homed  yet, firmware doesn't know about
			// Focus motor position.
			// Since we store the focus motor position "HawkeyeConfig::Instance().get().previousFocusPosition"
			// So we can move focus motor to this position.

			if (HawkeyeConfig::Instance().get().previousFocusPosition == 0)
			{
				// There is no previous focus position available.
				// We need to set focus first using "SetFocus" operation

				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_precondition_notmet, 
					instrument_error::instrument_precondition_instance::focus, 
					instrument_error::severity_level::warning));

				Logger::L().Log (MODULENAME, severity_level::error, "internal_svc_CameraFocusAdjust : cfa_CheckMotorHome : No previous focus position was found!");
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Error);
				return;
			}

			// First do the focus motor home operation
			focusController->FocusHome([onCurrentStateComplete](bool homeOk)
			{
				if (!homeOk)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internal_svc_CameraFocusAdjust : cfa_CheckMotorHome :Failed to home focus motor!");
					onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Error);
					return;
				}
				// Now move the focus motor to "HawkeyeConfig::Instance().get().previousFocusPosition"
				// position
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_MoveToPosition);
			});
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_MoveToPosition:
		{
			int32_t moveToPos = HawkeyeConfig::Instance().get().previousFocusPosition;
			// Moving the focus motor to previous focus position".
			focusController->SetPosition(
				[onCurrentStateComplete, moveToPos](bool posOk, int32_t endPos)
			{
				if (!posOk)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internal_svc_CameraFocusAdjust : cfa_MoveToPosition :Failed to move to position : " + std::to_string(moveToPos));
					onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Error);
					return;
				}

				// Now the focus motor is where it is supposed to be and firmware also knows
				// about it. Now we can perform "step motor operation"
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_DecideMotorMovement);
			}, moveToPos);
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_DecideMotorMovement:
		{
			if (adjustment_fine)
			{
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_StepMotorFine);
			}
			else
			{
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_StepMotorCoarse);
			}
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_StepMotorCoarse:
		{
			auto onStepComplete = [onCurrentStateComplete](bool status, int32_t)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internal_svc_CameraFocusAdjust: cfa_StepMotorCoarse : Failed to step focus motor!");
					onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Error);
					return;
				}
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Complete);
			};

			if (direction_up)
			{
				focusController->FocusStepUpCoarse(onStepComplete);
			}
			else
			{
				focusController->FocusStepDnCoarse(onStepComplete);
			}
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_StepMotorFine:
		{
			auto onStepComplete = [onCurrentStateComplete](bool status, int32_t)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internal_svc_CameraFocusAdjust: cfa_StepMotorFine : Failed to step focus motor!");
					onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Error);
					return;
				}
				onCurrentStateComplete(sm_CameraFocusAdjust::cfa_Complete);
			};

			if (direction_up)
			{
				focusController->FocusStepUpFine(onStepComplete);
			}
			else
			{
				focusController->FocusStepDnFine(onStepComplete);
			}
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_Complete:
		{
			int32_t lastFocusPos = startFocusPos;
			int32_t newFocusPos = focusController->Position();

			// Check if motor has been homed or not.
			bool isFocusMotorHomed = (SystemStatus::Instance().getData().motor_Focus.flags & eMotorFlags::mfHomed) != 0;
			if (!isFocusMotorHomed)
			{
				// If focus motor is not homed yet that means the "focusController" has no idea about the focus motor
				// position. When asked the position initially (just after system restart it will return the the position
				// as "0" and on doing relative move it will return position as "moveStep" (number of steps focus motor was 
				// asked to move either up or down).

				// "HawkeyeConfig::Instance().get().previousFocusPosition" stores the previous known focus position.

				// Actual focus motor position when motor not homed = currentFocusPos + HawkeyeConfig::Instance().get().previousFocusPosition;

				// If focus motor is not homed then get the stored focus position and add it to current focus position
				lastFocusPos += HawkeyeConfig::Instance().get().previousFocusPosition;
				newFocusPos += HawkeyeConfig::Instance().get().previousFocusPosition;
			}

			Logger::L().Log (
				MODULENAME, severity_level::debug1, 
				boost::str(boost::format("svc_CameraFocusAdjust : <%s> with <%s> : Focus position <%d>")
						   % (adjustment_fine ? "adjustment_fine" : "adjustment_coarse")
						   % (direction_up ? "direction_up" : "direction_down")
						   % newFocusPos));

			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_manualfocusoperation, 
				"Camera Focus Adjust",
				std::to_string(lastFocusPos) + std::string(" -> ") + std::to_string(newFocusPos)));

			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess);
			return;
		}
		case HawkeyeLogicImpl::sm_CameraFocusAdjust::cfa_Error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_CameraFocusAdjust: cfa_Error : Focus adjust step failed");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
			return;
		}
	}

	// unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

void HawkeyeLogicImpl::svc_CameraFocusRestoreToSavedLocation(std::function<void(HawkeyeError)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	// This must be done by an Elevated or higher.
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized, 
			"Service: Camera Focus Restore To Saved Location"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	if (HawkeyeConfig::Instance().get().previousFocusPosition == 0)
	{
		// There is no previous focus position available.
		// We need to set focus first using "SetFocus" operation

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::focus, 
			instrument_error::severity_level::warning));

		Logger::L().Log (MODULENAME, severity_level::error, "internal_svc_CameraFocusAdjust : cfa_CheckMotorHome : No previous focus position was found!");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	int32_t currentFocusPos = Hardware::Instance().getFocusController()->Position();
	auto onRestore = [this, loggedInUsername, callback, currentFocusPos](HawkeyeError success, int32_t focusPosition)->void
	{
		if (success != HawkeyeError::eSuccess)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_motor_positionfail, 
				instrument_error::motion_motor_instances::focus, 
				instrument_error::severity_level::error));
		}

		Logger::L().Log (MODULENAME, severity_level::normal,
						boost::str(boost::format("svc_CameraFocusRestoreToSavedLocation : %s - With focus motor position at %f")
								   % (success == HawkeyeError::eSuccess ? "Success" : "Failure")
								   % focusPosition));

		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_manualfocusoperation, 
			"Restore Focus Position",
			std::to_string(currentFocusPos) + std::string(" -> ") + std::to_string(focusPosition)));

		pHawkeyeServices_->enqueueExternal (callback, success);
	};

	initializeFocusController ([this, onRestore, callback](bool initOk)
	{
		if (!initOk)
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "svc_CameraFocusRestoreToSavedLocation: <exit, failed to initialize focus controller!>");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
			return;
		}
		Hardware::Instance().RestoreFocusPositionFromConfig(onRestore, true);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::svc_CameraAutoFocus(
	SamplePosition focusbead_location, 
	CameraAutofocusWorkflow::autofocus_state_callback_t_DLL on_status_change,
	CameraAutofocusWorkflow::countdown_timer_callback_t_DLL on_timer_tick,
	std::function<void(HawkeyeError)> callback)
{
	// This must be done by elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Camera Auto Focus"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
	}
	else
	{		
		eCarrierType type = eCarrierType::ePlate_96;
		if (Hardware::Instance().getStageController()->IsCarouselPresent())
		{
			type = eCarrierType::eCarousel;
		}

		if (!Hardware::Instance().getStageController()->IsStageCalibrated(type))
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus: <exit, Stage is not registered");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eStageNotRegistered);
			return;
		}
	}

	// TODO : Check for hardware fault
	// TODO : Add action to audit trail log.
		
	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	// Check if live scanning is running then stop it
	if (LiveScanningUtilities::Instance().IsBusy())
	{
		if (svc_StopLiveImageFeed() != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_CameraAutoFocus: <exit, failed to stop live image>");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
			return;
		}
	}

	auto wrapOnStateChangeCallback = [this, on_status_change](auto state, auto result) -> void
	{
		// Turn off the live scanning (if active) after "sample settling delay"
		// state is complete.
		if (state == eAutofocusState::af_AcquiringFocusData)
		{
			LiveScanningUtilities::Instance().StopLiveScanning();
		}

		HAWKEYE_ASSERT (MODULENAME, on_status_change);
		on_status_change(state, result);
	};

	auto workflow = std::make_shared<CameraAutofocusWorkflow>(
		SamplePositionDLL(focusbead_location), // This is dummy parameter maybe for future use, because UI doesn't send "focusbead_location" as of now.
		on_timer_tick,
		wrapOnStateChangeCallback);

	WorkflowController::Instance().set_load_execute_async ([this, callback](HawkeyeError success)
	{
		if (success != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_CameraAutoFocus: <exit,workflow set_load_execute_async failed!>");
			pHawkeyeServices_->enqueueExternal (callback, success);
			Hardware::Instance().getStageController()->SetStageProfile([=](bool) -> void {}, false);
			return;
		}

		initializeFocusController ([=](bool initOk)
		{
			if (!initOk)
			{
				Logger::L().Log (MODULENAME, severity_level::critical, "svc_CameraAutoFocus: <exit, failed to initialize focus controller!>");
			}
			pHawkeyeServices_->enqueueExternal (callback, (initOk ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault));
			Hardware::Instance().getStageController()->SetStageProfile([=](bool) -> void {}, false);
		});
	},workflow, Workflow::Type::CameraAutofocus);
}

HawkeyeError HawkeyeLogicImpl::svc_CameraAutoFocus_ServiceSkipDelay()
{
	// Skip the autofocus delay state IF the current user is service or higher.
	// No effect if not in the autofocus delay state.

	// This must be done by elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Camera Auto Focus, Skip Delay"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// TODO : Add action to audit trail log.

	if (!WorkflowController::Instance().isOfType(Workflow::Type::CameraAutofocus))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus_ServiceSkipDelay: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().validateCurrentWfOpState(eAutofocusState::af_SampleSettlingDelay))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus_ServiceSkipDelay: <exit, not valid state, not allowed>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	WorkflowController::Instance().getWorkflow()->skipCurrentRunningState();
	return HawkeyeError::eSuccess;
}

void HawkeyeLogicImpl::svc_CameraAutoFocus_FocusAcceptance(eAutofocusCompletion decision, std::function<void(HawkeyeError)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::string loggedInUsername = UserList::Instance().GetConsoleUsername();

	// This must be done by an elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized, 
			"Service: Camera Auto Focus, Focus Acceptance"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

// TODO : Add action to audit trail log.

	if (!WorkflowController::Instance().isOfType(Workflow::Type::CameraAutofocus))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus_FocusAcceptance: <exit, not presently running>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!WorkflowController::Instance().validateCurrentWfOpState(eAutofocusState::af_WaitingForFocusAcceptance))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus_FocusAcceptance: <exit, not valid state, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}
	
	auto wfType = Workflow::Type::CameraAutofocus;
	auto cafWf = WorkflowController::Instance().getWorkflow<CameraAutofocusWorkflow>(wfType);

	if (!cafWf)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus_FocusAcceptance: <exit, invalid workflow instance, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	auto res = cafWf->getAutofocusResult();
	if (!res)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "svc_CameraAutoFocus_FocusAcceptance: <exit, no focus result available!!!>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	auto loc = res->final_af_position;

	auto onMethodCallComplete = [this, loggedInUsername, decision, callback, loc](HawkeyeError success)
	{
		if (success != HawkeyeError::eSuccess)
		{
			// already logged the message for bad argument
			if (success != HawkeyeError::eInvalidArgs)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Service: Focus Acceptance : Failed to execute autofocus workflow");
			}
		}
		else
		{
			if (decision == eAutofocusCompletion::afc_Accept)
			{
				AuditLogger::L().Log (generateAuditWriteData(
					loggedInUsername,
					audit_event_type::evt_autofocusaccepted, 
					boost::str (boost::format ("Service: Camera Auto Focus, Focus Acceptance New focus position: %d") % loc)));
			}
		}
		pHawkeyeServices_->enqueueExternal (callback, success);
	};

	switch (decision)
	{
		case afc_Accept:
		{
			cafWf->setUserDecision(true);
			onMethodCallComplete(HawkeyeError::eSuccess);
			break;
		}
		case afc_Cancel:
		{
			cafWf->setUserDecision(false);
			onMethodCallComplete(HawkeyeError::eSuccess);
			break;
		}
		case afc_RetryOnCurrentCells:	// retry is no longer supported; the option has been removed from the UI
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Service: Focus Acceptance : Invalid decision type");
			onMethodCallComplete(HawkeyeError::eInvalidArgs);
		}
	}
}

HawkeyeError HawkeyeLogicImpl::svc_CameraAutoFocus_Cancel()
{
	// This must be done by elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Camera Auto Focus, Cancel"));
		return HawkeyeError::eNotPermittedByUser;
	}

	Logger::L().Log (MODULENAME, severity_level::normal, "Cancelling the auto focus routine");

	if (!WorkflowController::Instance().isOfType(Workflow::Type::CameraAutofocus))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CameraAutoFocus_Cancel: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

HawkeyeError HawkeyeLogicImpl::svc_GetFlowCellDepthSetting(uint16_t& flow_cell_depth)
{
	flow_cell_depth = 0;

	// This must be done by a logged-in user
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Get FlowCell Depth Setting"));
		return HawkeyeError::eNotPermittedByUser;
	}
	
////TODO: round this stored value?
	flow_cell_depth = static_cast<uint16_t>(HawkeyeConfig::Instance().get().flowCellDepth);

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::svc_SetFlowCellDepthSetting (uint16_t flow_cell_depth) {

	// This must be done by an service engineer or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess) {

		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set FlowCell Depth Setting"));
		return HawkeyeError::eNotPermittedByUser;
	}

	const uint16_t minAllowedFlowcellDepth = 50;
	const uint16_t maxAllowedFlowcellDepth = 100;	

	if (flow_cell_depth < minAllowedFlowcellDepth || flow_cell_depth > maxAllowedFlowcellDepth) {
		return HawkeyeError::eInvalidArgs;
	}


	AuditLogger::L().Log(generateAuditWriteData(
		UserList::Instance().GetConsoleUsername(),
		audit_event_type::evt_flowcelldepthupdate,
		boost::str(boost::format("Flow Cell Depth Set\n\tNew Value: %.3f mm \n\tPrevious Value: %.3f mm")
			% ((double)flow_cell_depth/1000.0)
			% ((double)HawkeyeConfig::Instance().get().flowCellDepth / 1000.0))));

////TODO: should the floating point value be stored?
	HawkeyeConfig::Instance().get().flowCellDepth = static_cast<float>(flow_cell_depth);

	// update the new concentration slope for A-cup and Standard based on this new flow cell depth
	double flow_cell_depth_scaling = (double)NOMINAL_FLOW_CELL_DEPTH / (double)flow_cell_depth;

	status = CalibrationHistoryDLL::Instance().SetConcentrationCalibration_ScaleFromDefault(calibration_type::cal_Concentration, flow_cell_depth_scaling);
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "svc_SetFlowCellDepthSetting: Failed to set Concentration from scaling value");
		return status;
	}
	status = CalibrationHistoryDLL::Instance().SetConcentrationCalibration_ScaleFromDefault(calibration_type::cal_ACupConcentration, flow_cell_depth_scaling);
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "svc_SetFlowCellDepthSetting: Failed to set A-cup Concentration from scaling value");
		return status;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::svc_GetValvePort (char &port)
{
	port = {};
	
	// Get the valve number and Convert to Char
	PhysicalPort_t physicalPort;
	if (!Hardware::Instance().getSyringePump ()->getPhysicalValve (physicalPort) || !SyringePump::FromPhysicalPortToChar (physicalPort, port))
	{
		return HawkeyeError::eHardwareFault;
	}

	return HawkeyeError::eSuccess;
}

void HawkeyeLogicImpl::svc_SetValvePort (std::function<void(HawkeyeError)> callback, const char port, SyringePumpDirection direction)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess) {

		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set Valve Port"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}
	
	if (!SyringePump::isPhysicalPortValid(port)) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
		return;
	}

	Hardware::Instance().getSyringePump()->setPhysicalValve ([this, callback, port](bool status) -> void {
		auto he = status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault;
		pHawkeyeServices_->enqueueExternal (callback, he);
	}, port, direction);
}

static uint32_t getPumpSpeed (PhysicalPort physicalPort) {
	uint32_t speed = 400; // All ports except the FlowCell.

	// Check if port is the FlowCell port.
	if (physicalPort == PhysicalPort::PortH) {
		speed = 100;
	}

	return speed;
}

void HawkeyeLogicImpl::svc_AspirateSample (std::function<void(HawkeyeError)> callback, uint32_t volume)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_AspirateSample: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	uint32_t currentSyringePumpPos = 0;
	PhysicalPort_t physicalPort;

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess) {
		
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Aspirate Sample"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	// Read the Syringe pump position
	auto pSyringePump = Hardware::Instance().getSyringePump();
	if (!(pSyringePump->getPosition (currentSyringePumpPos) && pSyringePump->getPhysicalValve(physicalPort))) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
		return;
	}

	// Check for the maximum volume
	if (volume > 1000 || volume < currentSyringePumpPos) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
		return;
	}

	// Proceed only if valid valve is active.
	// Block the Aspiration if the valve selected is "Waste" and "Flow Cell"
	if (!SyringePump::isAspirationAllowed(physicalPort)) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	Hardware::Instance().getSyringePump()->setPosition ([this, callback, volume](bool status) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_AspirateSample: <exit>");
		auto he = status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault;
		pHawkeyeServices_->enqueueExternal (callback, he);
	}, volume, getPumpSpeed(physicalPort));
}

void HawkeyeLogicImpl::svc_DispenseSample (std::function<void(HawkeyeError)> callback, uint32_t volume)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_DispenseSample: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	uint32_t currentSyringePumpPos = 0;
	PhysicalPort_t physicalPort;

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess) {

		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Dispense Sample"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	auto pSyringePump = Hardware::Instance().getSyringePump();
	// Read the Syringe pump position
	if (!(pSyringePump->getPosition(currentSyringePumpPos) && pSyringePump->getPhysicalValve (physicalPort))) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
		return;
	}

	// Always Current Syringe volume should be more than the requested volume to dispense
	if (volume > currentSyringePumpPos) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
		return;
	}

	if (!SyringePump::isDisPenseAllowed(physicalPort)) {
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	// Dispense
	Hardware::Instance().getSyringePump()->setPosition ([this, callback, volume](bool status) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_DispenseSample: <exit>");
		auto he = status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault;
		pHawkeyeServices_->enqueueExternal (callback, he);
	}, volume, getPumpSpeed(physicalPort));
}

HawkeyeError HawkeyeLogicImpl::svc_GetProbePostion(int32_t &pos)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GetProbePostion: <enter>");

	pos = 0;

	// Validate user permission level.- Any logged-in User
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Get Probe Position"));
		return HawkeyeError::eNotPermittedByUser;
	}

	pos = Hardware::Instance().getStageController()->GetProbePosition();

	return HawkeyeError::eSuccess;
}

void HawkeyeLogicImpl::svc_SetProbePostion(bool up, uint32_t stepsToMove, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_SetProbePostion: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set Probe Position"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	auto wrapCallback = [this, callback](bool status) -> void 
	{
		pHawkeyeServices_->enqueueExternal (callback, status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);
	};

	if (up)
	{
		Hardware::Instance().getStageController()->ProbeStepUp(wrapCallback, stepsToMove);
	} 
	else
	{
		const bool downOnInvalidStagePos = true;
		Hardware::Instance().getStageController()->ProbeStepDn(wrapCallback, stepsToMove, downOnInvalidStagePos);
	}
}

void HawkeyeLogicImpl::svc_MoveProbe (bool up, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_MoveProbe: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Move Probe"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	auto cb_internal = [this, callback](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_MoveProbe: <exit Operation failed>");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_MoveProbe: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess);
	};
	
	if (up)
	{
		Hardware::Instance().getStageController()->ProbeUp(cb_internal);
	} 
	else
	{
		const bool downOnInvalidStagePos = true;
		Hardware::Instance().getStageController()->ProbeDown(cb_internal, downOnInvalidStagePos);
	}
}

HawkeyeError HawkeyeLogicImpl::svc_GetSampleWellPosition(SamplePosition& pos)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GetSampleWellPosition: <enter>");

	pos = SamplePosition();

	// authenticate of user permission level.
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Get Sample Well Position"));
		return HawkeyeError::eNotPermittedByUser;
	}	

	auto samplePosition = Hardware::Instance().getStageController()->GetStagePosition();
	if (!samplePosition.isValid())
	{
		return HawkeyeError::eHardwareFault;
	}

	pos = samplePosition.getSamplePosition();
	return HawkeyeError::eSuccess;
}

void HawkeyeLogicImpl::svc_SetSampleWellPosition(SamplePosition pos, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_SetSampleWellPosition: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set Sample Well Position"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}	

	SamplePositionDLL samplePosition(pos);
	if (!samplePosition.isValid())
	{
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
		return;
	}

	// Dedicated service API for "Move Stage"
	// "MoveStageToServicePosition" method avoids the stage calibration check
	Hardware::Instance().getStageController()->MoveStageToServicePosition([this, callback](bool status)
	{
		pHawkeyeServices_->enqueueExternal (callback, status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);
	}, samplePosition);
}

//*****************************************************************************
void HawkeyeLogicImpl::SyringeRepetition (std::function<void(HawkeyeError)> callback, SyringeRepetitionState state)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [this, callback](SyringeRepetitionState nextState)
	{
		pHawkeyeServices_->enqueueInternal([=]()
		{
			SyringeRepetition(callback, nextState);
		});
	};

	PhysicalPort_t physicalPort;
	Hardware::Instance().getSyringePump()->getPhysicalValve(physicalPort);
	uint32_t speed = getPumpSpeed (physicalPort);

	auto& syringePump = Hardware::Instance().getSyringePump();

	switch (state)
	{
		case SyringeRepetitionState::Empty:
		{
			// Move the syringe to empty position.
			syringePump->setPosition([onCurrentStateComplete](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "SyringeRepetition: Empty: <exit, failed to empty syringe>");
					onCurrentStateComplete(SyringeRepetitionState::Error);
					return;
				}
				onCurrentStateComplete(SyringeRepetitionState::Aspirate);
			}, 0, speed);
			return;
		}

		case SyringeRepetitionState::Aspirate:
		{
			// Aspirate the maximum volume.
			syringePump->setPosition ([onCurrentStateComplete](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "SyringeRepetition: Aspirate: <exit, failed to aspirate>");
					onCurrentStateComplete(SyringeRepetitionState::Error);
					return;
				}
				onCurrentStateComplete(SyringeRepetitionState::Dispense);
			}, 1000, speed);
			return;
		}

		case SyringeRepetitionState::Dispense:
		{
			// Dispense the maximum volume.
			Hardware::Instance().getSyringePump()->setPosition ([onCurrentStateComplete](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "SyringeRepetition: Dispense: <exit, failed to dispense>");
					onCurrentStateComplete(SyringeRepetitionState::Error);
					return;
				}
				onCurrentStateComplete(SyringeRepetitionState::Complete);
			}, 0, speed);
			return;
		}

		case SyringeRepetitionState::Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "SyringeRepetition: Complete: <exit, Syringe repetition completed>");
			pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eSuccess);
			return;
		}

		case SyringeRepetitionState::Error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "SyringeRepetition: Error: <exit, Syringe repetition failed>");
			pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eHardwareFault);
			return;
		}
	}

	// unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

void HawkeyeLogicImpl::svc_MoveReagentArm(bool up, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_MoveReagentArm: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Move Reagent Arm"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	auto cb_internal = [this, callback](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_MoveReagentArm: <exit Operation failed>");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_MoveReagentArm: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess);
	};

	if (up)
	{
		Hardware::Instance().getReagentController()->ArmHome(cb_internal);
	}
	else
	{
		Hardware::Instance().getReagentController()->ArmDown(cb_internal);
	}
}

void HawkeyeLogicImpl::InitializeCarrier (std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "InitializeCarrier: <enter>");

	HawkeyeError status = UserList::Instance().CheckPermissionAtLeast  (UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: InitializeCarrier"));
		pHawkeyeServices_->enqueueExternal (callback, status);
		return;
	}

	t_pPTree cfgTree;
	Hardware::Instance().getStageController()->Init("", cfgTree, true, HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::MotorControl),
		[this, callback](bool status, bool tubeFoundDuringInit)
			{
				if ( status )
				{
					// NOTE: the ejectSamplestageInternal method will override the angle and holding current if automation is enabled and these parameters are passed...
					ejectSampleStageInternal( AutoSelectEjectAngle, EJECT_HOLDING_CURRENT_OFF,
											  [ this, callback ]( HawkeyeError he )
												{
													if ( HawkeyeError::eSuccess == he )
													{
														Logger::L().Log( MODULENAME, severity_level::debug1, "svc_InitializeCarrier <exit>" );
													}
													else
													{
														ReportSystemError::Instance().ReportError( BuildErrorInstance(
															instrument_error::motion_sampledeck_initerror,
															instrument_error::motion_sample_deck_instances::general,
															instrument_error::severity_level::error ) );
														Logger::L().Log( MODULENAME, severity_level::debug1, "svc_InitializeCarrier: <exit, failed to eject stage after initialization!>" );
													}
													pHawkeyeServices_->enqueueExternal( callback, he );
												});
					return;
				}

				if (tubeFoundDuringInit)
				{
					Logger::L().Log (MODULENAME, severity_level::warning, "InitializeCarrier: Failed to initialize stage controller : Tube found in Carousel while initializing!");
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::motion_sampledeck_tubedetected, 
						instrument_error::motion_sample_deck_instances::carousel, 
						instrument_error::severity_level::warning));
					pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
					return;
				}

				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::motion_sampledeck_initerror, 
					instrument_error::motion_sample_deck_instances::general, 
					instrument_error::severity_level::error));
				Logger::L().Log (MODULENAME, severity_level::debug1, "svc_InitializeCarrier: <exit, failed to initialize stage!>");
				pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
			});
}

HawkeyeError HawkeyeLogicImpl::svc_PerformPlateCalibration(motor_calibration_state_callback_DLL onCalibStateChangeCb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration: <enter>");

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Perform Plate Registration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// Simulation support
	if (onCalibStateChangeCb == nullptr)
	{
		return HawkeyeError::eInvalidArgs;
	}

	// Check if plate is present or not	
	if (Hardware::Instance().getStageController()->IsCarouselPresent())
	{
		// Inform cancelling the calibration
		svc_CancelCalibration(onCalibStateChangeCb);
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	auto cb_internal = [this, onCalibStateChangeCb](bool status, CalibrationState nextstate, CalibrationState informcaller)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_PerformPlateCalibration : <Failed to perform calibration, cancelling the operation>");
			this->svc_CancelCalibration(onCalibStateChangeCb);
			return;
		}

		// On success
		calibrationState_ = nextstate;
		if (informcaller == CalibrationState::eCompleted)
		{
			Hardware::Instance().getStageController()->SetStageProfile([=](bool) -> void {}, false);
		}
		onCalibStateChangeCb(informcaller);
	};

	switch (calibrationState_)
	{
		case CalibrationState::eIdle:
		{
			// Inform Homing Radius and Theta Motors
			onCalibStateChangeCb(CalibrationState::eHomingRadiusTheta);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eHomingRadiusTheta>");

			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, cb_internal]() -> void
			{
					Hardware::Instance().getStageController()->CalibrateMotorsHome(
					std::bind(cb_internal, std::placeholders::_1,
							  CalibrationState::eCalibrateRadius, CalibrationState::eWaitingForRadiusPos));
			});
			break;
		}
		case CalibrationState::eCalibrateRadius:
		{
			// Inform marking radius position
			onCalibStateChangeCb (CalibrationState::eCalibrateRadius);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eCalibrateRadius>");

			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, onCalibStateChangeCb, cb_internal]() -> void
			{
					Hardware::Instance().getStageController()->CalibrateStageRadius([this, onCalibStateChangeCb, cb_internal](bool status)
				{
					if (!status)
					{
						svc_CancelCalibration(onCalibStateChangeCb);
					}
					else
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eCalibrateRadius : Move Probe up!>");
						// Do probe up
						Hardware::Instance().getStageController()->ProbeUp(
							std::bind(cb_internal, std::placeholders::_1,
									  CalibrationState::eCalibrateTheta, CalibrationState::eWaitingForThetaPos));
					}
				});// End of CalibrateStageRadius
			});// End of Post
			break;
		}
		case CalibrationState::eCalibrateTheta:
		{
			// Inform marking radius and theta positions
			onCalibStateChangeCb (CalibrationState::eCalibrateTheta);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eCalibrateTheta>");

			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, onCalibStateChangeCb, cb_internal]() -> void
			{
					Hardware::Instance().getStageController()->CalibrateStageTheta([this, onCalibStateChangeCb, cb_internal](bool status)
				{
					if (!status)
					{
						svc_CancelCalibration(onCalibStateChangeCb);
					}
					else
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eCalibrateTheta : Move Probe up!>");
						// Do probe up
						Hardware::Instance().getStageController()->ProbeUp(
							std::bind(cb_internal, std::placeholders::_1,
									  CalibrationState::eInitialize, CalibrationState::eWaitingForFinish));
					}

				});// End of CalibrateStageTheta
			});// End of Post
			break;
		}
		case CalibrationState::eInitialize:
		{
			// Inform initialization with new values
			onCalibStateChangeCb (CalibrationState::eInitialize);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eInitialize>");
			
			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, onCalibStateChangeCb, cb_internal]() -> void
			{
					Hardware::Instance().getStageController()->FinalizeCalibrateStage(false, [this, onCalibStateChangeCb, cb_internal](bool status)
				{
					if (!status)
					{
						svc_CancelCalibration(onCalibStateChangeCb);
					}
					else
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformPlateCalibration : <State - eInitialize : Select the stage>");
						Hardware::Instance().getStageController()->SelectStage(
							std::bind(cb_internal, std::placeholders::_1,
									  CalibrationState::eIdle, CalibrationState::eCompleted),
							eCarrierType::ePlate_96);
					}

				}); // End of FinalizeCalibrateStage
			});// End of Post
			break;
		}
		default:
		{
			status = HawkeyeError::eInvalidArgs;
			break;
		}
	}

	return status;
}

HawkeyeError HawkeyeLogicImpl::svc_PerformCarouselCalibration(motor_calibration_state_callback_DLL onCalibStateChangeCb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformCarouselCalibration: <enter>");

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Perform Carousel Registration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// Simulation support
	if (onCalibStateChangeCb == nullptr)
	{
		return HawkeyeError::eInvalidArgs;
	}

	// Check if carousel is present or not
	if (Hardware::Instance().getStageController()->IsCarouselPresent() == false)
	{
		// Inform cancelling the calibration
		svc_CancelCalibration(onCalibStateChangeCb);
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	auto cb_internal = [this, onCalibStateChangeCb](bool status, CalibrationState nextstate, CalibrationState informcaller)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_PerformCarouselCalibration : <Failed to perform calibration, cancelling the operation>");
			this->svc_CancelCalibration(onCalibStateChangeCb);
			return;
		}

		// On success
		calibrationState_ = nextstate;
		if (informcaller == CalibrationState::eCompleted)
		{
			Hardware::Instance().getStageController()->SetStageProfile([=](bool) -> void {}, false);
		}
		onCalibStateChangeCb(informcaller);
	};

	switch (calibrationState_)
	{
		case CalibrationState::eIdle:
		{
			// Inform Homing Radius and Theta Motors
			onCalibStateChangeCb(CalibrationState::eHomingRadiusTheta);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformCarouselCalibration : <State - eHomingRadiusTheta>");

			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, cb_internal]() -> void
			{
					Hardware::Instance().getStageController()->CalibrateMotorsHome(
					std::bind(cb_internal, std::placeholders::_1,
							  CalibrationState::eCalibrateRadiusTheta, CalibrationState::eWaitingForRadiusThetaPos));
			});
			break;
		}
		case CalibrationState::eCalibrateRadiusTheta:
		{
			// Inform marking radius and theta positions
			onCalibStateChangeCb (CalibrationState::eCalibrateRadiusTheta);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformCarouselCalibration : <State - eCalibrateRadiusTheta :  Radius>");

			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, onCalibStateChangeCb, cb_internal]() -> void
			{
					Hardware::Instance().getStageController()->CalibrateStageRadius([this, onCalibStateChangeCb, cb_internal](bool status)
				{
					if (!status)
					{
						svc_CancelCalibration(onCalibStateChangeCb);
					}
					else
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformCarouselCalibration : <State - eCalibrateRadiusTheta :Theta>");

						Hardware::Instance().getStageController()->CalibrateStageTheta(
							std::bind(cb_internal, std::placeholders::_1,
									  CalibrationState::eInitialize, CalibrationState::eWaitingForFinish));
					}

				});// End of CalibrateStageRadius
			});// End of Post
			break;
		}
		case CalibrationState::eInitialize:
		{
			// Inform initialization with new values
			onCalibStateChangeCb (CalibrationState::eInitialize);
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformCarouselCalibration : <State - eInitialize>");

			// this workflow does not require the user id, so do not use the transient user technique
			pLocalIosvc_->post([this, onCalibStateChangeCb, cb_internal]() -> void
			{
				Hardware::Instance().getStageController()->FinalizeCalibrateStage(false, [this, onCalibStateChangeCb, cb_internal](bool status)
				{
					if (!status)
					{
						svc_CancelCalibration(onCalibStateChangeCb);
					}
					else
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "svc_PerformCarouselCalibration : <State - eInitialize : Select the stage>");
						Hardware::Instance().getStageController()->SelectStage(
							std::bind(cb_internal, std::placeholders::_1, CalibrationState::eIdle, CalibrationState::eCompleted), eCarrierType::eCarousel);
					}

				}); // End of FinalizeCalibrateStage
			});// End of Post
			break;
		}
		default:
		{
			status = HawkeyeError::eInvalidArgs;
			break;
		}
	}

	return status;
}

HawkeyeError HawkeyeLogicImpl::svc_CancelCalibration(motor_calibration_state_callback_DLL onCalibStateChangeCb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CancelCalibration: <enter>");

	// Validate user permission level.- Service Engineer only
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Cancel Registration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// do NOT apply holding current while attempting to register the stage positin calibration info!
	Hardware::Instance().getStageController()->SetStageProfile([=](bool) -> void {}, false);	// false to disable holding current

	if (onCalibStateChangeCb == nullptr)
	{
		return HawkeyeError::eInvalidArgs;
	}

	pHawkeyeServices_->enqueueInternal([this, onCalibStateChangeCb]() -> void
	{
			Hardware::Instance().getStageController()->FinalizeCalibrateStage(true, [this, onCalibStateChangeCb](bool status)
		{
			if (!status)
			{
				//This may not happen, but still log an error if it happens
				Logger::L().Log (MODULENAME, severity_level::warning, "svc_CancelCalibration : Failed to Cancel the calibration!");

				onCalibStateChangeCb(CalibrationState::eFault);
				calibrationState_ = CalibrationState::eIdle;
				return;
			}

			// Do probe up
			Hardware::Instance().getStageController()->ProbeUp([this, onCalibStateChangeCb](bool status)
			{
				if(!status)
					Logger::L().Log (MODULENAME, severity_level::warning, "svc_CancelCalibration : Failed to move probe up!");

				// Initial state of Calibration		
				onCalibStateChangeCb(CalibrationState::eApplyDefaults);
				// Inform cancelling the calibration
				onCalibStateChangeCb(CalibrationState::eFault);
				calibrationState_ = CalibrationState::eIdle;
			});
		}); // End of FinalizeCalibrateStage
	});// End of Post

	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_CancelCalibration: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::svc_SetCameraLampState (bool lamp_on, float intensity_0_to_25, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("svc_SetCameraLampState: <enter, lamp_state: %s, power: %f>") % (lamp_on ? "ON" : "OFF") % intensity_0_to_25));

	HAWKEYE_ASSERT (MODULENAME, callback);

	// This must be done by an service engineer or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set Camera Lamp State"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	LedBase::TriggerMode triggerMode = LedBase::DigitalMode;

	// When lamp_on is set to true then the led is continuously on using the specified intensity value and
	// when lamp_on is set to false then the led is in data acquistion mode (digital trigger mode) using
	// the specified intensity value.
	if (lamp_on) {

		triggerMode = LedBase::ContinousMode;

		// Make sure that Live View is off.
		// When using the BCI LED, Live View conflicts with having the LED ON.
		// Actually, if the BCI LED and Live View are ON at the same time, the ControllerBoard throws
		// errors because it is checking that the LED is off after triggering the camera and LED.
		// This would not occur with the Omicron LED since the ControllerBoard does not control the 
		// power to the Omicron LED.
		if (HawkeyeConfig::Instance().get().brightfieldLedType == HawkeyeConfig::BrightfieldLedType::BCI)
		{
			svc_StopLiveImageFeed();
		}

		if (intensity_0_to_25 < 0.0 || intensity_0_to_25 > 25.0) {
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_powerthreshold,
				instrument_error::imaging_led_instances::brightfield, //NOTE: currently only Brightfield is supported.
				instrument_error::severity_level::notification));

			Logger::L().Log (MODULENAME, severity_level::notification, "svc_SetCameraLampState: <exit, power limited to 25.0%>");
			intensity_0_to_25 = 25;
		}
	}

	Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->setPower (intensity_0_to_25,
		[this, callback](bool status)
	{
		if (!status) {
			Logger::L().Log (MODULENAME, severity_level::error, "svc_SetCameraLampState: <exit, setPower failed>");
		}
		pHawkeyeServices_->enqueueExternal (callback, status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);

	}, triggerMode);

}

//*****************************************************************************
void HawkeyeLogicImpl::svc_SetCameraLampIntensity (float intensity_0_to_100, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("svc_SetCameraLampIntensity: <enter, power: %f>") % intensity_0_to_100));

	HAWKEYE_ASSERT (MODULENAME, callback);

	// This must be done by an service engineer or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Set Camera Lamp Intensity"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	if (intensity_0_to_100 < 0.0 || intensity_0_to_100 > 100.0)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_SetCameraLampIntensity: <exit, invalid args>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
		return;
	}

	Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->getTriggerMode (
		[this, callback, intensity_0_to_100](bool status, LedBase::TriggerMode triggerMode)
	{
		float powerToSet = intensity_0_to_100;

		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "svc_SetCameraLampIntensity: <exit, getTriggerMode failed>");
		}

		if (triggerMode == LedBase::ContinousMode)
		{

			Logger::L().Log (MODULENAME, severity_level::notification, "svc_SetCameraLampIntensity: <Lamp is on continuously>");


			if (powerToSet > 25.0)
			{
				ReportSystemError::Instance().ReportError (BuildErrorInstance (
					instrument_error::imaging_led_powerthreshold,
					instrument_error::imaging_led_instances::brightfield, //NOTE: currently only Brightfield is supported.
					instrument_error::severity_level::notification));

				Logger::L().Log (MODULENAME, severity_level::notification, "svc_SetCameraLampIntensity: <exit, power limited to 25%>");
				powerToSet = 25;
			}
		}

		Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->setPower (powerToSet, [this, callback](bool status) {
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "svc_SetCameraLampIntensity: <exit, setPower failed>");
			}
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_SetCameraLampIntensity: <exit>");
			pHawkeyeServices_->enqueueExternal (callback, status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);
		}, triggerMode);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::svc_GetCameraLampState (bool& lamp_on, float& intensity_0_to_100, std::function<void(HawkeyeError)> callback) {

	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GetCameraLampState: <enter>");

	intensity_0_to_100 = 0.0;

	// This must be done by an service engineer or higher.
	// limited to local logins only
	HawkeyeError status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess) {

		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Get Camera Lamp State"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	auto getPowerCallback = [this, callback, &lamp_on, &intensity_0_to_100](boost::optional<float> power) -> void
	{
		if (power) {
			intensity_0_to_100 = power.get();
		}
		if (intensity_0_to_100 < 0.0f || intensity_0_to_100 > 100.0f)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GetCameraLampState: <exit, invalid args>");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eHardwareFault);
			return;
		}

		//*****************************************************************************
		// triggerMode: sets whether or not the camera is in data acquisition mode
		// or is in service mode (continuously on).
		//  true: data acquisition mode (digital trigger).
		// false: service mode (continuously on).
		//*****************************************************************************
		Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->getTriggerMode ([this, callback, &lamp_on](bool status, LedBase::TriggerMode triggerMode)
		{
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "svc_GetCameraLampState: <exit, getTriggerMode failed>");
			}

			lamp_on = triggerMode == LedBase::DigitalMode ? false : true;

			Logger::L().Log (MODULENAME, severity_level::debug1,
							boost::str (boost::format ("svc_GetCameraLampState: <exit, lamp_state: %s>") % (lamp_on ? "ON" : "OFF")));

			pHawkeyeServices_->enqueueExternal (callback, status ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);
		});
	};

	pHawkeyeServices_->enqueueInternal ([this, getPowerCallback]()
	{
			Hardware::Instance().getLed(HawkeyeConfig::LedType::LED_BrightField)->getPower (getPowerCallback);
	});
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::svc_GenerateSingleShotAnalysis (TemporaryImageAnalysis::temp_analysis_result_callback_DLL callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateSingleShotAnalysis: <enter>");

	// This must be done by an service engineer or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Generate Single Shot Analysis"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (pTemporarySingleShotAnalysis_ && pTemporarySingleShotAnalysis_->isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateSingleShotAnalysis: <exit> : system is busy performing analysis!");
		return HawkeyeError::eBusy;
	}

	pTemporarySingleShotAnalysis_.reset (new TemporaryImageAnalysis(pLocalIosvc_));
	pTemporarySingleShotAnalysis_->setCameraParams (50, CameraLampLEDType);

	pTemporarySingleShotAnalysis_->generateResultsContinuous(
		serviceAnalysisImagePreference_, AnalysisDefinitionsDLL::GetTemporaryAnalysis(), GetTemporaryCellType(),
		[callback](auto&&... args)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateSingleShotAnalysis: OnComplete <enter>");
		pTemporarySingleShotAnalysis_->stopContinuousExecution();
		
		HAWKEYE_ASSERT (MODULENAME, callback);
		callback(std::forward<decltype(args)>(args)...);
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateSingleShotAnalysis: OnComplete <exit>");
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateSingleShotAnalysis: <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::svc_GenerateContinuousAnalysis (TemporaryImageAnalysis::temp_analysis_result_callback_DLL callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateContinuousAnalysis: <enter>");

	// This must be done by an service engineer or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Generate Continuous Analysis"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (pTemporaryContinuousAnalysis_ && pTemporaryContinuousAnalysis_->isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateContinuousAnalysis: <exit> : system is busy performing analysis!");
		return HawkeyeError::eBusy;
	}

	pTemporaryContinuousAnalysis_.reset(new TemporaryImageAnalysis(pLocalIosvc_));
	pTemporaryContinuousAnalysis_->setCameraParams (50, CameraLampLEDType);

	pTemporaryContinuousAnalysis_->generateResultsContinuous(
		serviceAnalysisImagePreference_, AnalysisDefinitionsDLL::GetTemporaryAnalysis(), GetTemporaryCellType(), callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_GenerateContinuousAnalysis: <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::svc_StopContinuousAnalysis()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StopContinuousAnalysis: <enter>");

	// This must be done by an service engineer or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Stop Continuous Analysis"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (pTemporaryContinuousAnalysis_)
	{
		pTemporaryContinuousAnalysis_->stopContinuousExecution();
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StopContinuousAnalysis: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::svc_StartLiveImageFeed (LiveScanningUtilities::service_live_image_callback_DLL liveImageCallback, std::function<void(HawkeyeError)> functionCallback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed: <enter>");

	// This must be done by an elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{ // No logged in user
			pHawkeyeServices_->enqueueExternal (functionCallback, status);
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Start Live Image Feed"));
		pHawkeyeServices_->enqueueExternal (functionCallback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	// ToDo - Check if work queue is running or camera is busy
	// if (busy) { return HawkeyeError::eBusy; }
	// if (cameraBusy) { return HawkeyeError::eBusy; }

	if (LiveScanningUtilities::Instance().IsBusy())
	{
		pHawkeyeServices_->enqueueExternal (functionCallback, HawkeyeError::eSuccess);
		return;
	}

	// ToDo - Check for hardware fault
	// if (hardwareFault) { return HawkeyeError::eHardwareFault; }

	auto isAutoFocusWf = WorkflowController::Instance().isOfType(Workflow::Type::CameraAutofocus) && WorkflowController::Instance().isWorkflowBusy();

	// If the s/w is running as a Shepherd Viability Science Module and is running in simulation
	// do not enable LiveScanning as this will use the images in the ExternalImages folder
	// and mess up the set focus simulation.
	if (!HawkeyeConfig::Instance().get().withHardware &&
		HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule && isAutoFocusWf)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed: <exit 2>");
		pHawkeyeServices_->enqueueExternal (functionCallback, HawkeyeError::eSuccess);
		return;
	}
	
	// If Autofocus workflow processing in progress then live scanning can be
	// turned on only if "eAutofocusState::af_SampleSettlingDelay' state is active.
	if (isAutoFocusWf && !WorkflowController::Instance().validateCurrentWfOpState(eAutofocusState::af_SampleSettlingDelay))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed: <exit, not allowed>");
		pHawkeyeServices_->enqueueExternal (functionCallback, HawkeyeError::eNotPermittedByUser);
		return;
	}


	static bool lamp_on;
	static float intensity_0_to_100;

	auto getLampStateCallback = [this,/* &lamp_on, &intensity_0_to_100,*/ liveImageCallback, functionCallback](HawkeyeError he) {
		//float intensity = intensity_0_to_100; // Save value before it goes out of scope.
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed 1: <intensity_0_to_100: " + std::to_string(intensity_0_to_100) + ">");
		if (lamp_on)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed 2: <intensity_0_to_100: " + std::to_string(intensity_0_to_100) + ">");
			svc_SetCameraLampState (false, intensity_0_to_100, [this, liveImageCallback, functionCallback](HawkeyeError he) {
				if (he == HawkeyeError::eSuccess)
				{
					// Start live scanning
					he = LiveScanningUtilities::Instance().StartLiveScanning(liveImageCallback);
					Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed: <exit 1>");
				}
				pHawkeyeServices_->enqueueExternal (functionCallback, he);
			});
			return;
		}

		// Start live scanning
		he = LiveScanningUtilities::Instance().StartLiveScanning(liveImageCallback);
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StartLiveImageFeed: <exit 2>");
		pHawkeyeServices_->enqueueExternal (functionCallback, he);
	};

	svc_GetCameraLampState (lamp_on, intensity_0_to_100, getLampStateCallback); //[this, lamp_on, intensity_0_to_100, liveImageCallback, functionCallback](HawkeyeError he) {
}

HawkeyeError HawkeyeLogicImpl::svc_StopLiveImageFeed()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StopLiveImageFeed: <enter>");

	// This must be done by an elevated user or higher.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Stop Live Image Feed"));
		return HawkeyeError::eNotPermittedByUser;
	}

	HawkeyeError he = LiveScanningUtilities::Instance().StopLiveScanning();
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_StopLiveImageFeed: <exit>");

	return he;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::svc_ManualSample_Load()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Load: <enter>");

	// This must be done by an service engineer or higher.	
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "svc_ManualSample_Load: <exit - not permitted by user>");

		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Manual Sample, Load"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// ToDo - if (instrumentBusy) { return HawkeyeError::eBusy; }
	// ToDo - if (hardwareFault) { return HawkeyeError::eHardwareFault; }

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Load: <exit, busy, not allowed>");
		return HawkeyeError::eBusy;
	}

	auto workflow = std::make_shared<LoadNudgeExpelWorkflow>(LoadNudgeExpelWorkflow::Manual_Sample_Operation::Load);

	WorkflowController::Instance().set_load_execute_async([this](HawkeyeError success)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Load: <exit>");
	}, workflow, Workflow::Type::LoadNudgeExpel, LoadNudgeExpelWorkflow::Manual_Sample_Operation::Load);

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::svc_ManualSample_Nudge()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Nudge: <enter>");

	/// This must be done by an service engineer or higher.	
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Manual Sample Nudge"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// ToDo - if (instrumentBusy) { return HawkeyeError::eBusy; }
	// ToDo - if (hardwareFault) { return HawkeyeError::eHardwareFault; }

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Nudge: <exit, busy, not allowed>");
		return HawkeyeError::eBusy;
	}

	auto workflow = std::make_shared<LoadNudgeExpelWorkflow>(LoadNudgeExpelWorkflow::Manual_Sample_Operation::Nudge);

	uint32_t currentVol;
	if (!Hardware::Instance().getSyringePump()->getPosition(currentVol))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Nudge: <exit, h/w fault>");
		return HawkeyeError::eHardwareFault;
	}

	// if syringe is empty then return.
	if (currentVol == 0)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Nudge: <exit, not allowed>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	WorkflowController::Instance().set_load_execute_async([this](HawkeyeError success)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Nudge: <exit>");
	}, workflow, Workflow::Type::LoadNudgeExpel, LoadNudgeExpelWorkflow::Manual_Sample_Operation::Nudge);

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::svc_ManualSample_Expel()
{
	// This must be done by an service engineer or higher.	
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eService);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetConsoleUsername(),
			audit_event_type::evt_notAuthorized, 
			"Service: Manual Sample Expel"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// ToDo - if (instrumentBusy) { return HawkeyeError::eBusy; }
	// ToDo - if (hardwareFault) { return HawkeyeError::eHardwareFault; }

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Expel: <exit, busy, not allowed>");
		return HawkeyeError::eBusy;
	}

	auto workflow = std::make_shared<LoadNudgeExpelWorkflow>(LoadNudgeExpelWorkflow::Manual_Sample_Operation::Expel);

	WorkflowController::Instance().set_load_execute_async([this](HawkeyeError success)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "svc_ManualSample_Expel: <exit>");
	}, workflow, Workflow::Type::LoadNudgeExpel, LoadNudgeExpelWorkflow::Manual_Sample_Operation::Expel);

	return HawkeyeError::eSuccess;
}
