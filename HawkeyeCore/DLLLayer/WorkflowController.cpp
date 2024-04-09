#include "stdafx.h"

#include "AsyncCommandHelper.hpp"
#include "Hardware.hpp"
#include "HawkeyeServices.hpp"
#include "ReagentPack.hpp"
#include "WorkflowController.hpp"

static const char MODULENAME[] = "WorkflowController";


//*****************************************************************************
void WorkflowController::Initialize (std::shared_ptr<boost::asio::io_context> pUpstream)
{
	isBusy_ = false;
	pWorkflow_.reset();
	pHawkeyeServices_ = std::make_shared<HawkeyeServices>(pUpstream, "WorkflowController_Thread");
}

//*****************************************************************************
WorkflowController::~WorkflowController()
{
	resetWorkflow();
	//pServices_.reset();
}

//*****************************************************************************
bool WorkflowController::isWorkflowBusy() const
{
	// Here we are not creating new workflow just checking the status of existing workflow
	// So pass "nullptr" as input parameter.
	return isWorkflowBusyInternal (nullptr);
}

//*****************************************************************************
HawkeyeError WorkflowController::loadWorkflow (uint8_t workflowSubType) const
{
	if (!pWorkflow_)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "loadWorkflow : no workflow is currently set!");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "loadWorkflow : system is busy performing other operations!");
		return HawkeyeError::eBusy;
	}

	return pWorkflow_->load (pWorkflow_->getWorkFlowScriptFile (workflowSubType));
}

//*****************************************************************************
void WorkflowController::executeWorkflowAsync (
	std::function<void(HawkeyeError)> callback, 
	Workflow::Type type,
	boost::optional<uint16_t> jumpToState)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!pWorkflow_)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "executeWorkflowAsync : no workflow is currently set!");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "executeWorkflowAsync : system is busy performing other operations!");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	//if (pWorkflow_->getType() != type)
	//{
	//	Logger::L().Log (MODULENAME, severity_level::error, "executeWorkflowAsync : trying to execute workflow from different instance!");
	//	enqueueExternal (callback, HawkeyeError::eInvalidArgs);
	//	return;
	//}

	// Get fluidics usage for current workflow
	std::map<SyringePumpPort::Port, uint32_t> requiredVolumes;
	pWorkflow_->getTotalFluidVolumeUsage (requiredVolumes, jumpToState);

	checkReagentPreconditions ([this, callback, jumpToState](bool status)
	{
		if (status)
		{
			if (pWorkflow_->getType() == Workflow::Type::CameraAutofocus)
			{
				if (isReagentNearExpiration(AutoFocusDuration))
				{
					Logger::L().Log (MODULENAME, severity_level::error, "executeWorkflowAsync : reagent expiration check failed!");
					HawkeyeError he = HawkeyeError::eReagentError;
					pHawkeyeServices_->enqueueExternal (callback, he);
					return;
				}
			}
			
			executeInternal (jumpToState);
			pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eSuccess);
			return;
		}
		Logger::L().Log (MODULENAME, severity_level::error, "executeWorkflowAsync : reagent precondition check failed!");

		HawkeyeError he = HawkeyeError::eReagentError;
		if (!Hardware::Instance().getReagentController()->IsDown())
		{
			// Reagent arm not down is not reagent error, so change it to eHardwareFault
			he = HawkeyeError::eHardwareFault;
		}
		pHawkeyeServices_->enqueueExternal (callback, he);
	}, requiredVolumes);
}

//*****************************************************************************
HawkeyeError WorkflowController::setWorkflow (std::shared_ptr<Workflow> newWorkflow)
{
	if (isWorkflowBusyInternal(newWorkflow))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setWorkflow: busy performing another operation");
		if (newWorkflow)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setWorkflow: cannot set workflow: " + newWorkflow->getNameAsString());
		}
		return HawkeyeError::eBusy;
	}

	pWorkflow_ = std::move(newWorkflow);
	pWorkflow_->setServices (pHawkeyeServices_);

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void WorkflowController::validateFluidVolumes (std::map<SyringePumpPort::Port, uint32_t> volumes, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (volumes.empty())
	{
		// No volume to check, then return true
		pHawkeyeServices_->enqueueExternal (callback, true);
		return;
	}

	std::vector<std::function<void(std::function<void(bool)>)>> vTaskList;
	vTaskList.reserve (volumes.size());

	if (Hardware::Instance().getReagentController()->isRfidSim())
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "validateFluidVolumes: in Offline/Simulation mode>");
	}

	for (const auto& item : volumes)
	{
		vTaskList.push_back (std::bind (&WorkflowController::validateFluidVolume, this, std::placeholders::_1, item.first, item.second));
	}

	const bool canContinueIfAnyTaskFail = false;
	AsyncCommandHelper::queueASynchronousTask (pHawkeyeServices_->getInternalIos(), callback, vTaskList, canContinueIfAnyTaskFail);
}

//*****************************************************************************
bool WorkflowController::resetWorkflow()
{
	if (pWorkflow_ && pWorkflow_->isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "resetWorkflow : Workflow is busy, cannot reset workflow : " + pWorkflow_->getNameAsString());
		return false;
	}
	pWorkflow_.reset();
	return true;
}

//*****************************************************************************
bool WorkflowController::isOfType (Workflow::Type type) const
{
	return pWorkflow_ && pWorkflow_->getType() == type;
}

//*****************************************************************************
void WorkflowController::set_load_execute_async(
	std::function<void(HawkeyeError)> callback,
	std::shared_ptr<Workflow> newWorkflow,
	Workflow::Type type,
	uint8_t workflowSubType,
	boost::optional<uint16_t> jumpToState)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (pWorkflow_ && pWorkflow_->isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "set_load_execute_async: workflow is busy");
		pHawkeyeServices_->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	pHawkeyeServices_->enqueueInternal ([=]() -> void
	{
		// Set the workflow.
		auto he = setWorkflow (newWorkflow);
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "set_load_execute_async: failed to set the workflow!");
			pHawkeyeServices_->enqueueExternal (callback, he);
			return;
		}

		// Load the workflow.
		he = loadWorkflow (workflowSubType);
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "set_load_execute_async: failed to load the workflow!");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::workflow_script,
				instrument_error::severity_level::error));
			pHawkeyeServices_->enqueueExternal (callback, he);
			return;
		}

		executeWorkflowAsync (callback, newWorkflow->getType(), jumpToState);
	});
}

//*****************************************************************************
std::shared_ptr<Workflow> WorkflowController::getWorkflow() const
{
	return pWorkflow_;
}

//*****************************************************************************
void WorkflowController::validateFluidVolume (std::function<void(bool)> callback, SyringePumpPort::Port port, uint32_t volume) const
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Hardware::Instance().getReagentController()->isFluidAvailable(callback, port, volume);
}

//*****************************************************************************
void WorkflowController::checkReagentPreconditions (std::function<void(bool)> callback, std::map<SyringePumpPort::Port, uint32_t>& requiredVolumes)
{
	auto reagentController = Hardware::Instance().getReagentController();

	if (pWorkflow_ && pWorkflow_->canValidateReagent())
	{
		// Check Whether the Pack is installed, Arm is down, tags are valid.
		if (!reagentController->IsPackInstalled())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "checkReagentPreconditions : No pack installed");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::reagent_pack_nopack, 
				instrument_error::reagent_pack_instance::general, 
				instrument_error::severity_level::warning));
			pHawkeyeServices_->enqueueInternal (callback, false);
			return;
		}

		if (!ReagentPack::Instance().isPackUsable())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "checkReagentPreconditions : Reagent pack is unusable!");
			pHawkeyeServices_->enqueueExternal (callback, false);
			return;
		}

		if (!reagentController->IsDown())
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "checkReagentPreconditions: Reagent probe arm NOT down");

			// Don't expect the reagent arm be raised all the way up to home if completed reagent pack load sequence.
			if (reagentController->IsHome())
			{
				pHawkeyeServices_->enqueueInternal(callback, false);
				return;
			}

			reagentController->ArmDown ([this, requiredVolumes, callback](bool success) -> void
			{
				if (!success)
				{
					pHawkeyeServices_->enqueueInternal (callback, false);
					return;
				}

				validateFluidVolumes (requiredVolumes, [this, callback](bool status) -> void
				{
					if (!status)
					{
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::reagent_pack_empty,
							instrument_error::reagent_pack_instance::general, 
							instrument_error::severity_level::warning));

						Logger::L().Log (MODULENAME, severity_level::critical,
										boost::str(boost::format("Not enough volume of fluids available to continue operation <Workflow : %s>!") % pWorkflow_->getNameAsString()));
					}
					pHawkeyeServices_->enqueueInternal (callback, status);
				});
			});

			return;
		}
	}

	validateFluidVolumes (requiredVolumes, [this, callback](bool status) -> void
	{
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::reagent_pack_empty, 
				instrument_error::reagent_pack_instance::general, 
				instrument_error::severity_level::warning));

			Logger::L().Log (MODULENAME, severity_level::critical,
							boost::str(boost::format("Not enough volume of fluids available to continue operation <Workflow : %s>!") % pWorkflow_->getNameAsString()));
		}
		pHawkeyeServices_->enqueueInternal (callback, status);
	});

}

//*****************************************************************************
// this method assmumes that the pack is installed, has been checked for usability,
// and has the required volume of reagents left.
//*****************************************************************************
bool WorkflowController::isReagentNearExpiration (uint32_t timeRequired)
{
	if (!pWorkflow_ || !pWorkflow_->canValidateReagent())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "isReagentNearExpiration : No workflow or can't validate reagents!");
		return true;
	}

	uint32_t daysLeft = 0;
	uint64_t timeLeft = 0;

	if (!ReagentPack::Instance().isPackUsable())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "isReagentNearExpiration: invalid reagent object or reagent pack is unusable!");
		return true;
	}

	if ( ReagentPack::Instance().isNearExpiration (true, daysLeft, timeRequired) || daysLeft == 0 )
	{
		Logger::L().Log (MODULENAME, severity_level::error, "isReagentNearExpiration : Reagent pack is expired or remaining time insufficient!");
		return true;
	}

	return false;
}

//*****************************************************************************
//TODO: document "jumpToState"...
//*****************************************************************************
void WorkflowController::executeInternal (boost::optional<uint16_t> jumpToState)
{
	pHawkeyeServices_->enqueueInternal ([this, jumpToState]() -> void
	{ 
		if (!jumpToState)
		{
			pWorkflow_->execute();
		}
		else
		{
			pWorkflow_->execute(jumpToState.get());
		}
	});
}

//*****************************************************************************
bool WorkflowController::isWorkflowBusyInternal (std::shared_ptr<Workflow> newWorkflow) const
{
	/*If any workflow is running (or completed), the instance of that workflow will be stored in
	"pWorkflow_" until "SetWorkflow(std::shared_ptr<Workflow> newWorkflow)" is called.
	To avoid running multiple workflows at once we need to check the "Busy" Status of stored "pWorkflow_"
	instance. If it is busy (pWorkflow_->isBusy()) that means some workflow is running and we cannot proceed
	further with setting any new workflow or modifying the existing workflow (e.g. reloading or re-executing)
	Now When we execute work queue (or Sample Workflow), we tend to run multiple samples (one after another)
	In this process we will create separate instance for each "Sample Workflow" until all the samples have been
	processed.
	We should not allow creating any other workflow (except "Sample Workflow") until work queue execution is complete*/

	// Check if we are trying to set new workflow.
	if (newWorkflow != nullptr && newWorkflow.get() != nullptr)
	{
		bool isSampleWf = newWorkflow->getType() == Workflow::Type::Sample; // Check if it is sample workflow
		if (isSampleWf)
		{
			// Check if existing workflow is busy or not.
			return pWorkflow_.get() != nullptr && pWorkflow_->isBusy();
		}

		// Check if existing workflow is busy or not and work queue is not running.
		return (pWorkflow_.get() != nullptr && pWorkflow_->isBusy()) || isBusy_;
	}

	// If we are here that means we not setting new workflow.
	if (pWorkflow_.get() == nullptr)
	{
		return false;
	}

	bool isSampleWf = pWorkflow_->getType() == Workflow::Type::Sample;
	if (isSampleWf)
	{
		// For "Sample" workflow check if existing workflow is busy or not.
		return pWorkflow_->isBusy();
	}

	// For workflow other than "Sample" check if existing workflow is busy or not and work queue is not running.
	return pWorkflow_->isBusy() || isBusy_;
}
