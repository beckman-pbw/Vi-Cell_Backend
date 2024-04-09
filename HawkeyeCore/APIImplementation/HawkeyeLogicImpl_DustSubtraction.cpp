#include "stdafx.h"

#include "AuditLog.hpp"
#include "BrightfieldDustSubtractWorkflow.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "ImageWrapperUtilities.hpp"

static const char MODULENAME[] = "DustSubtraction";

void HawkeyeLogicImpl::StartBrightfieldDustSubtract (BrightfieldDustSubtractWorkflow::brightfield_dustsubtraction_callback_DLL on_status_change, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartBrightfieldDustSubtract: <enter>");

	// This must be done by Elevated User.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Start Bright field Dust Subtract"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	eCarrierType type = eCarrierType::ePlate_96;

	int16_t instrumentType = HawkeyeConfig::Instance().get().instrumentType;
	if (instrumentType != HawkeyeConfig::CellHealth_ScienceModule)
	{
		if (Hardware::Instance().getStageController()->IsCarouselPresent())
		{
			type = eCarrierType::eCarousel;
		}

		if (!Hardware::Instance().getStageController()->IsStageCalibrated(type))
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "StartBrightfieldDustSubtract: <exit, Stage is not registered");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eStageNotRegistered);
			return;
		}
	}

	// ToDo - Check if system is idle {return HawkeyeError::eBusy;}
	// ToDo - Check if system is not able to start the process 
	// at this time (no reagents?) {return HawkeyeError::eNotPermittedAtThisTime;}

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartBrightfieldDustSubtract: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	// Report  error and exit if focus is not done yet
	if (!SystemStatus::Instance().getData().focus_IsFocused)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "StartBrightfieldDustSubtract: <exit, focus operation is not done!>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::focus, 
			instrument_error::severity_level::warning));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	auto workflow = std::make_shared<BrightfieldDustSubtractWorkflow>(on_status_change);

	// Check if live scanning is running then stop it
	if (LiveScanningUtilities::Instance().IsBusy())
	{
		if (svc_StopLiveImageFeed() != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "StartBrightfieldDustSubtract: <exit, failed to stop live image>");
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
			return;
		}
	}

	WorkflowController::Instance().set_load_execute_async ([this, callback](HawkeyeError success)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartBrightfieldDustSubtract: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, success);

	}, workflow, Workflow::Type::BrightfieldDustSubtract);
}

void HawkeyeLogicImpl::AcceptDustReference(bool accepted, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "AcceptDustReference: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	// This must be done by Elevated User.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{
			pHawkeyeServices_->enqueueExternal (callback, status);
			return;
		}
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized, 
			"Accept Dust Reference"));
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedByUser);
		return;
	}

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "AcceptDustReference: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::BrightfieldDustSubtract))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "AcceptDustReference: <exit, not presently running>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!WorkflowController::Instance().validateCurrentWfOpState(BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "AcceptDustReference: <exit, invalid current state, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	auto jumpToState =
		accepted ? BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_SettingReferenceImage
		: BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_Completed;

	WorkflowController::Instance().executeWorkflowAsync([this, loggedInUsername, callback](HawkeyeError success)
	{
		if (success == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_dustsubtractionaccepted, 
				"Success"));
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "AcceptDustReference: Failed to execute bright field dust reference workflow");
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, "AcceptDustReference: <exit>");
		pHawkeyeServices_->enqueueExternal (callback, success);

	},Workflow::Type::BrightfieldDustSubtract, jumpToState);
}

HawkeyeError HawkeyeLogicImpl::CancelBrightfieldDustSubtract()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelBrightfieldDustSubtract: <enter>");

	// This must be done by Elevated User.
	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
		{
			return status;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Cancel Brightfield Dust Subtract"));
		return status;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::BrightfieldDustSubtract))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "CancelBrightfieldDustSubtract: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelBrightfieldDustSubtract: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}
