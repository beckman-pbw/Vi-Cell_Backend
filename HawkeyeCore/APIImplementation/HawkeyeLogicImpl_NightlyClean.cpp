#include "stdafx.h"

#include "AuditLog.hpp"
#include "FlushFlowCellWorkflow.hpp"
#include "HawkeyeLogicImpl.hpp"

static const char MODULENAME[] = "NightCleanCycle";

#define NightCleanRoutineTime_Hrs 24 // 24 hours

std::shared_ptr<boost::asio::deadline_timer> nightCleanTimer_;

//*****************************************************************************
/// <summary>
/// Check if nightly clean is required when the application starts
/// <remarks>
/// This will check when the last time nightly clean was run and if it
/// is more than routine time <see cref="NightCleanRoutineTime_Hrs"/>
/// that means nightly clean is required during system start up
/// </remarks>
/// </summary>
/// <returns><c>[true]</c> if required; otherwise <c>[false]</c></returns>
//*****************************************************************************
static bool isNightlyCleanRequiredOnStartUp()
{
	system_TP lastNCTime_Hrs = HawkeyeConfig::Instance().get().lastNightCleanTime;

	// Skip nightly clean on startup on fresh bootup
	system_TP ZeroTP = {};
	if (lastNCTime_Hrs == ZeroTP)
		return false;

	system_TP now = ChronoUtilities::CurrentTime();

	auto difference = now - lastNCTime_Hrs;
	uint64_t difference_Hrs = std::chrono::duration_cast<std::chrono::hours>(difference).count();

	return difference_Hrs >= NightCleanRoutineTime_Hrs;
}

//*****************************************************************************
// We have set the nightly clean timer for 02:00 AM
// If the system is idle is at that time the nightly clean will start and
// the timer will be set for next day 02:00 AM (24 hours later)

// If during system start, application detects that nightly clean has not been run for
// more than 24 hours (reason could be; system was off, or system was not idle at 02:00 AM so nightly clean got skipped)
// it will start nightly clean as soon as system starts.
//*****************************************************************************
bool HawkeyeLogicImpl::initializeNightlyClean()
{
	// Do not run the "classic" Nightly Clean operation for Shepherd.
	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		return true;
	}
	
	// set up nightly clean cycle
	nightCleanTimer_ = std::make_shared<boost::asio::deadline_timer>(*pLocalIosvc_);
	const auto currentTime = boost::posix_time::second_clock::local_time();
	const boost::posix_time::ptime nightCleanTime(
		currentTime.date(), 
		boost::posix_time::minutes(static_cast<long>(HawkeyeConfig::Instance().get().nightlyCleanMinutesFromMidnight)));

	int32_t totalSeconds = static_cast<int32_t>((nightCleanTime - currentTime).total_seconds());

	if (totalSeconds <= 0)
	{
		totalSeconds += static_cast<int32_t>(boost::posix_time::hours(NightCleanRoutineTime_Hrs).total_seconds());
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "initializeNightlyClean: time to next Nightly Clean cycle: " +
		ChronoUtilities::ConvertToString (ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(totalSeconds), "%Y-%m-%d %H:%M:%S"));

	// Check if nightly clean is required during system start up or not
	if (isNightlyCleanRequiredOnStartUp())
	{
		if ( totalSeconds <= 3600 )	// scheduled nightly clean is less than an hour from when this one is being run.
									// move timer out another 24 hours to prevent duplicate clean request during this one...
		{
			totalSeconds += static_cast<int32_t>( boost::posix_time::hours( NightCleanRoutineTime_Hrs ).total_seconds() );
		}
		
		Logger::L().Log( MODULENAME, severity_level::debug1, "initializeNightlyClean: queuing nightly clean on startup initialization" );
		pHawkeyeServices_->enqueueInternal([this]() -> void
		{
			executeNightCleanCycle();
		});
	}

	nightCleanTimer_->expires_from_now( boost::posix_time::seconds( totalSeconds ) );
	nightCleanTimer_->async_wait( std::bind( &HawkeyeLogicImpl::nightCleanCycleRoutine, this, std::placeholders::_1 ) );

	return true;
}

//*****************************************************************************
void HawkeyeLogicImpl::destroyNightlyClean()
{
	if (nightCleanTimer_)
	{
		nightCleanTimer_->cancel();
		nightCleanTimer_.reset();
	}
}

//*****************************************************************************
void HawkeyeLogicImpl::nightCleanCycleRoutine (const boost::system::error_code& ec)
{
	if (ec || !nightCleanTimer_)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, std::string ("Unable to perform nightly clean cycle : timer was cancelled"));
		return;
	}

	// schedule next run
	{
		// add 24 hour to schedule for next day
		nightCleanTimer_->expires_at (nightCleanTimer_->expires_at() + boost::posix_time::hours(NightCleanRoutineTime_Hrs));
		nightCleanTimer_->async_wait (std::bind(&HawkeyeLogicImpl::nightCleanCycleRoutine, this, std::placeholders::_1));
	}

	executeNightCleanCycle();
}

//*****************************************************************************
void HawkeyeLogicImpl::executeNightCleanCycle()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "executeNightCleanCycle: <enter>");

	std::string loggedInUsername = UserList::Instance().GetAttributableUserName();

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		// check if this is a duplicate nightly clean workflow request;
		// Don't change NC status or return 'busy' status if it is, or it clears the UI NC progress dialog
		bool ncRunning = WorkflowController::Instance().isOfType( Workflow::Type::FlushFlowCell ) &&
			( WorkflowController::Instance().validateCurrentWfOpState( eNightlyCleanStatus::ncsInProgress ) || WorkflowController::Instance().validateCurrentWfOpState( eNightlyCleanStatus::ncsACupInProgress ) );
		if ( ncRunning )
		{
			Logger::L().Log( MODULENAME, severity_level::debug1, "executeNightCleanCycle: <exit, duplicate cleaning request, not allowed>" );
			return;
		}

		updateNightlyCleanStatus (eNightlyCleanStatus::ncsIdle, HawkeyeError::eBusy);
		Logger::L().Log (MODULENAME, severity_level::debug1, "executeNightCleanCycle: <exit, busy, not allowed>");
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_fluidicsnightlyclean, 
			"Skipped; busy"));
		return;
	}

	// Check if workflow is either "CameraAutoFocus" or "BrightfieldDustSubtract" and
	// waiting for user input.
	bool isAutoFocusWf = WorkflowController::Instance().isOfType (Workflow::Type::CameraAutofocus) &&
	                     WorkflowController::Instance().validateCurrentWfOpState(eAutofocusState::af_WaitingForFocusAcceptance);

	bool isBDSWf = WorkflowController::Instance().isOfType (Workflow::Type::BrightfieldDustSubtract) &&
	               WorkflowController::Instance().validateCurrentWfOpState (BrightfieldDustSubtractWorkflow::bds_WaitingOnUserApproval);

	if (isAutoFocusWf || isBDSWf)
	{
		updateNightlyCleanStatus (eNightlyCleanStatus::ncsIdle, HawkeyeError::eBusy);
		Logger::L().Log (MODULENAME, severity_level::debug1, "executeNightCleanCycle: <exit, other workflow running, not allowed>");
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_fluidicsnightlyclean, 
			"Skipped; busy"));
		return;
	}

	uint8_t workflowSubType = FlushFlowCellWorkflow::FlushType::eStandardNightlyClean;
	if (HawkeyeConfig::Instance().get().acupEnabled)
	{
		workflowSubType = FlushFlowCellWorkflow::FlushType::eACupNightlyClean;
	}

	// Pass the callback as "NULL", because we don't need to update UI about nightly clean states
	auto workflow = std::make_shared<FlushFlowCellWorkflow>(nullptr);

	workflow->registerCompletionHandler ([this, workflowSubType, loggedInUsername](HawkeyeError he)
	{
		if (he == HawkeyeError::eSuccess)
		{			
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_fluidicsnightlyclean, 
				"Completed"));

			HawkeyeConfig::Instance().get().lastNightCleanTime = ChronoUtilities::CurrentTime();

			updateNightlyCleanStatus (eNightlyCleanStatus::ncsIdle, he);
		}
		else
		{
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_fluidicsnightlyclean,
				"Error"));
			if (workflowSubType == FlushFlowCellWorkflow::FlushType::eStandardNightlyClean)
			{
				updateNightlyCleanStatus (eNightlyCleanStatus::ncsUnableToPerform, he);
			}
			else
			{
				updateNightlyCleanStatus (eNightlyCleanStatus::ncsACupUnableToPerform, he);
			}
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("executeNightlyClean: <exit, %d>") % (int)workflowSubType));
	});

	WorkflowController::Instance().set_load_execute_async ([this, loggedInUsername, workflowSubType](HawkeyeError he)
	{
		if (he == HawkeyeError::eSuccess)
		{
			if (workflowSubType == FlushFlowCellWorkflow::FlushType::eStandardNightlyClean)
			{
				updateNightlyCleanStatus (eNightlyCleanStatus::ncsInProgress, he);
			}
			else
			{
				updateNightlyCleanStatus (eNightlyCleanStatus::ncsACupInProgress, he);
			}
		}
		else
		{
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_fluidicsnightlyclean,
				"Error"));
			if (workflowSubType == FlushFlowCellWorkflow::FlushType::eStandardNightlyClean)
			{
				updateNightlyCleanStatus (eNightlyCleanStatus::ncsUnableToPerform, he);
			}
			else
			{
				updateNightlyCleanStatus (eNightlyCleanStatus::ncsACupUnableToPerform, he);
			}
		}
	}, workflow, Workflow::Type::FlushFlowCell, workflowSubType);
}

//*****************************************************************************
void HawkeyeLogicImpl::updateNightlyCleanStatus (eNightlyCleanStatus ncs, HawkeyeError he)
{
	SystemStatus::Instance().getData().nightly_clean_cycle = ncs;
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Unable to perform nightly clean cycle: " 
			+ std::string(HawkeyeErrorAsString(he)));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::fluidics_general_nightlyclean,
			instrument_error::cntrlr_general_instance::none,
			instrument_error::severity_level::warning));
	}
	else
	{
		Logger::L ().Log (MODULENAME, severity_level::normal, "Nightly Clean successfully completed");
	}
}
