#include "stdafx.h"

#include "AuditLog.hpp"
#include "DecontaminateFlowCellWorkflow.hpp"
#include "DrainReagentPackWorkflow.hpp"
#include "FlushFlowCellWorkflow.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "PurgeReagentLinesWorkflow.hpp"

static const char MODULENAME[] = "ReagentFluidics";

//*****************************************************************************
void HawkeyeLogicImpl::StartFlushFlowCell(
	FlushFlowCellWorkflow::flowcell_flush_status_callback_DLL on_status_change,
	HawkeyeErrorCallback callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartFlushFlowCell: <enter>");

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartFlushFlowCell: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	auto workflow = std::make_shared<FlushFlowCellWorkflow>(on_status_change);

	WorkflowController::Instance().set_load_execute_async([this, callback](HawkeyeError success)
	{
		if (success == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetAttributableUserName(),
				audit_event_type::evt_fluidicsflush, 
				"Completed"));
		}
		pHawkeyeServices_->enqueueExternal (callback, success);
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartFlushFlowCell: <exit>");

	}, workflow, Workflow::Type::FlushFlowCell, FlushFlowCellWorkflow::FlushType::eFlushFlowCell);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelFlushFlowCell()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelFlushFlowCell: <enter>");

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::FlushFlowCell))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "CancelFlushFlowCell: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelFlushFlowCell: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetFlushFlowCellState (eFlushFlowCellState& state)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "GetFlushFlowCellState: <enter>");

	state = {};

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::FlushFlowCell))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetFlushFlowCellState: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	state = WorkflowController::Instance().getCurrentWorkflowOpState<eFlushFlowCellState>();

	Logger::L().Log (MODULENAME, severity_level::debug1, "GetFlushFlowCellState: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::StartDecontaminateFlowCell(
	DecontaminateFlowCellWorkflow::flowcell_decontaminate_status_callback_DLL on_status_change, std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartDecontaminateFlowCell: <enter>");

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	eCarrierType type = eCarrierType::ePlate_96;

	int16_t instrumentType = HawkeyeConfig::Instance().get().instrumentType;
	if (instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		type = eCarrierType::eACup;
	}
	else
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

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartDecontaminateFlowCell: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	auto workflow = std::make_shared<DecontaminateFlowCellWorkflow>(on_status_change);

	WorkflowController::Instance().set_load_execute_async([this, callback](HawkeyeError success) -> void
	{
		if (success == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetAttributableUserName(),
				audit_event_type::evt_fluidicsdecontaminate, 
				"Completed"));
		}
		pHawkeyeServices_->enqueueExternal (callback, success);
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartDecontaminateFlowCell: <exit>");

	}, workflow, Workflow::Type::DecontaminateFlowCell, 0);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelDecontaminateFlowCell()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelDecontaminateFlowCell: <enter>");

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::DecontaminateFlowCell))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "CancelDecontaminateFlowCell: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelDecontaminateFlowCell: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetDecontaminateFlowCellState (eDecontaminateFlowCellState& state)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "GetDecontaminateFlowCellState: <enter>");

	state = {};

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::DecontaminateFlowCell))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetDecontaminateFlowCellState: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	state = WorkflowController::Instance().getCurrentWorkflowOpState<eDecontaminateFlowCellState>();

	Logger::L().Log (MODULENAME, severity_level::debug1, "GetDecontaminateFlowCellState: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::StartPrimeReagentLines(
	PrimeReagentLinesWorkflow::prime_reagentlines_callback_DLL on_status_change,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartPrimeReagentLines: <enter>");

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartPrimeReagentLines: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	auto workflow = std::make_shared<PrimeReagentLinesWorkflow>(on_status_change);

	WorkflowController::Instance().set_load_execute_async([this, callback](HawkeyeError success) -> void
	{
		if (success == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				UserList::Instance().GetAttributableUserName(),
				audit_event_type::evt_fluidicsprime, 
				"Completed"));
		}
		pHawkeyeServices_->enqueueExternal (callback, success);
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartPrimeReagentLines: <exit>");

	}, workflow, Workflow::Type::PrimeReagentLines, 0);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelPrimeReagentLines()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelPrimeReagentLines: <enter>");

	if (!WorkflowController::Instance().isOfType(Workflow::Type::PrimeReagentLines))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "CancelPrimeReagentLines: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelPrimeReagentLines: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetPrimeReagentLinesState (ePrimeReagentLinesState& state)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "GetPrimeReagentLinesState: <enter>");

	state = {};

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::PrimeReagentLines))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetPrimeReagentLinesState: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	state = WorkflowController::Instance().getCurrentWorkflowOpState<ePrimeReagentLinesState>();

	Logger::L().Log (MODULENAME, severity_level::debug1, "GetPrimeReagentLinesState: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::StartPurgeReagentLines(
	PurgeReagentLinesWorkflow::purge_reagentlines_callback_DLL on_status_change,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartPurgeReagentLines: <enter>");

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartPurgeReagentLines: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	auto workflow = std::make_shared<PurgeReagentLinesWorkflow>(on_status_change);

	WorkflowController::Instance().set_load_execute_async([this, callback](HawkeyeError success) -> void
		{
			if (success == HawkeyeError::eSuccess)
			{
				AuditLogger::L().Log (generateAuditWriteData(
					UserList::Instance().GetAttributableUserName(),
					audit_event_type::evt_fluidicspurge,
					"Completed"));
			}
			pHawkeyeServices_->enqueueExternal (callback, success);
			Logger::L().Log (MODULENAME, severity_level::debug1, "StartPurgeReagentLines: <exit>");

		}, workflow, Workflow::Type::PurgeReagentLines, 0);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelPurgeReagentLines()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelPurgeReagentLines: <enter>");

	if (!WorkflowController::Instance().isOfType(Workflow::Type::PurgeReagentLines))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "CancelPurgeReagentLines: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelPurgeReagentLines: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

//*****************************************************************************
void HawkeyeLogicImpl::StartCleanFluidics(
	FlushFlowCellWorkflow::flowcell_flush_status_callback_DLL on_status_change,
	HawkeyeErrorCallback callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartCleanFluidics: <enter>");

	std::string loggedInUsername = UserList::Instance().GetAttributableUserName();

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartCleanFluidics: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	uint8_t workflowSubType = FlushFlowCellWorkflow::FlushType::eStandardNightlyClean;
	if (HawkeyeConfig::Instance().get().acupEnabled)
	{
		workflowSubType = FlushFlowCellWorkflow::FlushType::eACupNightlyClean;
	}

	auto workflow = std::make_shared<FlushFlowCellWorkflow>(on_status_change);

	workflow->registerCompletionHandler ([this, workflowSubType, loggedInUsername, callback](HawkeyeError he)
	{
		if (he == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				loggedInUsername,
				audit_event_type::evt_fluidicsnightlyclean,
				"Completed"));
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

		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("StartCleanFluidics: <exit, %d>") % (int)workflowSubType));
	});

	WorkflowController::Instance().set_load_execute_async ([this, loggedInUsername, workflowSubType, callback](HawkeyeError he)
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
			
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess);
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
			
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eReagentError);
		}
	}, workflow, Workflow::Type::FlushFlowCell, workflowSubType);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetCleanFluidicsState(eFlushFlowCellState& state)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "GetCleanFluidicsState: <enter>");

	state = {};

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::FlushFlowCell))
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "GetCleanFluidicsState: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	state = WorkflowController::Instance().getCurrentWorkflowOpState<eFlushFlowCellState>();

	Logger::L().Log(MODULENAME, severity_level::debug1, "GetCleanFluidicsState: <exit>");

	return HawkeyeError::eSuccess;
}


//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelCleanFluidics()
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "CanceCleanFluidics: <enter>");

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::FlushFlowCell))
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "CancelCleanFluidics: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "CancelCleanFluidics: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

