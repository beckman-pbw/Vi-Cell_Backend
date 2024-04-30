#include "stdafx.h"

#include "AuditLog.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "HawkeyeServices.hpp"
#include "ReagentDLL.hpp"
#include "ReagentPack.hpp"
#include "ReagentUnloadWorkflow.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_ReagentPack";

//*****************************************************************************
void HawkeyeLogicImpl::initializeReagentPack (BooleanCallback callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeReagentPack <enter>");

	ReagentPack::Instance().Initialize (pHawkeyeServices_->getInternalIos(), [this, callback](bool status)
	{
		pHawkeyeServices_->enqueueInternal (callback, status);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::GetReagentContainerStatus (HawkeyeErrorCallback callback,
                                                  uint16_t reagent_num,
                                                  ReagentContainerState*& containerState)
{
	containerState = nullptr;

	if (ReagentPack::Instance().isPackUsable())
	{
		auto& containerStatesDLL = ReagentPack::Instance().GetReagentContainerStates();
		if (reagent_num > containerStatesDLL.size() || reagent_num == 0) {
			Logger::L().Log (MODULENAME, severity_level::warning, "GetReagentContainerStatus: <exit, invalid args>");
			pHawkeyeServices_->enqueueInternal  (callback, HawkeyeError::eInvalidArgs);
		} else {
			containerState = new ReagentContainerState[1];
			reagentContainerStateFromDLL (*containerState, containerStatesDLL[reagent_num - 1]);
			pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eSuccess);
		}
		return;
	}

	Logger::L().Log(MODULENAME, severity_level::warning, "GetReagentContainerStatus: <exit not found, " + std::to_string(reagent_num) + ">");
	pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNoneFound);
}

//*****************************************************************************
void HawkeyeLogicImpl::GetReagentContainerStatusAll (std::function<void(HawkeyeError)> callback,
                                                     uint16_t& num_reagents,
                                                     ReagentContainerState*& containerStates)
{
	num_reagents = 0;
	containerStates = nullptr;

	auto& containerStatesDLL = ReagentPack::Instance().GetReagentContainerStates();
	num_reagents = (uint16_t)containerStatesDLL.size();

	if (num_reagents == 0) {
		pHawkeyeServices_->enqueueInternal(callback, HawkeyeError::eNoneFound);
	}
	else {
		containerStates = new ReagentContainerState[num_reagents];

		size_t i = 0;
		for (auto it = containerStatesDLL.begin(); it != containerStatesDLL.end(); ++it)
		{
			reagentContainerStateFromDLL(containerStates[i++], *it);
		}

		pHawkeyeServices_->enqueueInternal(callback, HawkeyeError::eSuccess);
	}
}

//*****************************************************************************
void HawkeyeLogicImpl::GetReagentDefinitions (std::function<void(HawkeyeError)> callback,
                                              uint32_t& num_reagents,
                                              ReagentDefinition*& rd)
{
	num_reagents = 0;
	rd = nullptr;
	
	if (ReagentPack::Instance().isPackUsable ())
	{
		auto& reagentDefinitionsDLL = ReagentPack::Instance().GetReagentDefinitions();
		num_reagents = (uint16_t)reagentDefinitionsDLL.size();
		if (num_reagents == 0)
		{
			pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNoneFound);
			return;
		}
		else
		{
			rd = new ReagentDefinition[num_reagents];

			size_t i = 0;
			for (auto it = reagentDefinitionsDLL.begin(); it != reagentDefinitionsDLL.end(); ++it)
			{
				reagentDefinitionFromDLL (rd[i++], it->second);
			}
			pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eSuccess);
		}
		return;
	}
	pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNoneFound);
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeReagentDefinitions (ReagentDefinition* list, uint32_t num_reagents)
{
	// RegentDefinition doesn't have internal allocations.
	if (list != nullptr)
	{
		delete[] list;
	}
}

//*****************************************************************************
/// During the reagent load sequence, single-fluid containers will need to have their location(s) specified.
/// This function must be called for each container when the Load Sequence is in the state ReagentLoadSequence::eWaitingOnContainerLocation;
/// Return values: eSuccess  -  locations accepted and load procedure continuing.
///				   eNotPermittedAtThisTime - function may only be called when the Reagent Load process is in the correct state.
///				   eInvalidArgs - attempted to set a location other than "main bay" for a multi-fluid reagent container; attempted to set a location 
///								  other than "door left" or "door right" for a single-fluid reagent container.
///
///*****************************************************************************
/// THIS API IS NOT CURRENTLY CALLED BY THE USER INTERFACE.
/// COMMENTS INDICATE THAT THIS CODE IS QUESTIONABLE.  AS SUCH, IT IS SET TO ONLY
/// RETURN NotPermittedAtThisTime.
///*****************************************************************************
///
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetReagentContainerLocation (ReagentContainerLocation& location)
{
	return HawkeyeError::eNotPermittedAtThisTime;

	//Logger::L().Log (MODULENAME, severity_level::debug1, "SetReagentContainerLocation: <enter>");

	//// This must be done by a logged-in user for traceability, but no special permissions are necessary.
	//auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast (UserPermissionLevel::eNormal);
	//if (status != HawkeyeError::eSuccess)
	//{
	//	if (status == HawkeyeError::eNotPermittedAtThisTime)
	//		return status;

	//	AuditLogger::L().Log (generateAuditWriteData (UserList::Instance().GetLoggedInUsername(), audit_event_type::evt_notAuthorized, "Set Reagent Container Location"));
	//	return HawkeyeError::eNotPermittedByUser;
	//}

	//auto& containerStatesDLL = ReagentPack::Instance().GetReagentContainerStates();

	//for(size_t index = 0; index < containerStatesDLL.size(); index++)
	//{
	//	auto& it = containerStatesDLL[index];
	//	if(!std::equal(std::begin(it.identifier), std::end(it.identifier), std::begin(location.identifier)))
	//	{
	//		continue;
	//	}

	//	//TODO: For Hunter. Confirm valve mapping between location position and valve position
	//	//TODO: Hunter: Check container fluid count vs. requested position.  Multifluid containers can't be in door, single-fluid can't be in main bay.
	//	uint8_t valve = 0;
	//	switch (location.position)
	//	{
	//		case eMainBay_1:
	//			valve = 0;
	//			break;
	//		case eDoorLeft_2:
	//			valve = SyringePumpPort::ToPhysicalPort (SyringePumpPort::Reagent_1);
	//			break;
	//		case eDoorRight_3:
	//			valve = SyringePumpPort::ToPhysicalPort (SyringePumpPort::Reagent_2);
	//			break;

	//		case eNone:
	//		case eUnknown:
	//		default:
	//		{
	//			Logger::L().Log (MODULENAME, severity_level::error, "SetReagentContainerLocation- Invalid Position");
	//			Logger::L().Log (MODULENAME, severity_level::debug1, "SetReagentContainerLocation: <exit, invalid args>");
	//			return HawkeyeError::eInvalidArgs;
	//		}
	//	}

	//	//TODO: None of this code is FUNCTIONAL!!  Probably need to delete it???  pmills 10/31/2018.
	//	if (pReagentData->reagentValveMaps.find(index) == pReagentData->reagentValveMaps.end())
	//	{
	//		Logger::L().Log (MODULENAME, severity_level::error, "SetReagentContainerLocation: <exit, not valve map found>");
	//		return HawkeyeError::eEntryNotFound;
	//	}

	//	auto& valveMapList = pReagentData->reagentValveMaps[index];
	//	for (size_t vmIndex = 0; vmIndex < valveMapList.size(); vmIndex++)
	//	{
	//		if (valveMapList[vmIndex].parameter.tag_index == *reinterpret_cast<uint8_t*>(it.identifier))
	//		{
	//			valveMapList[vmIndex].parameter.valve_number = valve;
	//			break;
	//		}
	//	}

	//	it.position = location.position;

	//	Logger::L().Log (MODULENAME, severity_level::debug1, "SetReagentContainerLocation: <exit>");
	//	return HawkeyeError::eSuccess;
	//}

	//Logger::L().Log (MODULENAME, severity_level::debug1, "SetReagentContainerLocation: <exit, not found>");

	//return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfReagentContainerState (ReagentContainerState* list, uint32_t num_structs)
{
	if (!list)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "FreeListOfReagentContainerState: <exit, null ptr>");
		return;
	}

	for (uint32_t i = 0; i < num_structs; i++)
	{
		delete[] list[i].lot_information;
		delete[] list[i].bci_part_number;
		FreeListOfReagentState(list[i].reagent_states, list[i].num_reagents);
	}

	delete[] list;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfReagentState (ReagentState* list, uint32_t num_structs)
{
	if (!list)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "FreeReagentState: <exit, null ptr>");
		return;
	}
	for (uint32_t i = 0; i < num_structs; i++)
	{
		delete[] list[i].lot_information;
	}
	delete[] list;
}

//*****************************************************************************
// This must be done by a logged-in user for traceability, but no special permissions are necessary.
//*****************************************************************************
void HawkeyeLogicImpl::LoadReagentPack (
	ReagentPack::reagent_load_status_callback_DLL onLoadStatusChange,
	ReagentPack::reagent_load_complete_callback_DLL onLoadComplete,
	std::function<void (HawkeyeError)> callback)
{
	Logger::L ().Log (MODULENAME, severity_level::debug1, "LoadReagentPack: <enter>");

	HAWKEYE_ASSERT (MODULENAME, onLoadStatusChange);
	HAWKEYE_ASSERT (MODULENAME, onLoadComplete);

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	ReagentPack::Instance().Load (onLoadStatusChange, onLoadComplete, callback);
}

//*****************************************************************************
void HawkeyeLogicImpl::UnloadReagentPack (
	ReagentContainerUnloadOption* unloadActions, 
	uint16_t nContainers,
	ReagentPack::reagent_unload_status_callback_DLL onUnloadStatusChange,
	ReagentPack::reagent_unload_complete_callback_DLL onUnloadComplete,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "UnloadReagentPack: <enter>");

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	ReagentPack::Instance().Unload (unloadActions, nContainers, onUnloadStatusChange, onUnloadComplete, callback);
}

//*****************************************************************************
void HawkeyeLogicImpl::StartDrainReagentPack(
	DrainReagentPackWorkflow::drain_reagentpack_callback_DLL on_status_change, 
	uint8_t valve_position, 
	uint32_t repeatCount,
	std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "StartDrainReagentPack: <enter>");

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (WorkflowController::Instance().isWorkflowBusy())

	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "StartDrainReagentPack: <exit, busy, not allowed>");
		pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eBusy);
		return;
	}

	if (!Hardware::Instance().getReagentController()->IsPackInstalled())
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "StartDrainReagentPack: <exit, Reagent pack not installed>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_nopack, 
			instrument_error::reagent_pack_instance::general, 
			instrument_error::severity_level::warning));
		pHawkeyeServices_->enqueueInternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	auto workflow = std::make_shared<DrainReagentPackWorkflow>(on_status_change, repeatCount);

	WorkflowController::Instance().set_load_execute_async([this, callback](HawkeyeError success) -> void
		{
			if (success == HawkeyeError::eSuccess)

			{
				AuditLogger::L().Log (generateAuditWriteData(
					UserList::Instance().GetAttributableUserName(),
//TODO:					UserList::Instance().GetLoggedInUsername(),					
					audit_event_type::evt_fluidicsdrain, 
					"Completed"));
			}
			pHawkeyeServices_->enqueueInternal (callback, success);
			Logger::L().Log (MODULENAME, severity_level::debug1, "StartDrainReagentPack: <exit>");

		}, workflow, Workflow::Type::DrainReagentPack, valve_position);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelDrainReagentPack()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelDrainReagentPack: <enter>");

	// limited to local logins only
	auto status = UserList::Instance().CheckConsoleUserPermissionAtLeast(UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime)
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetAttributableUserName(),
//TODO:					UserList::Instance().GetLoggedInUsername(),					
			audit_event_type::evt_notAuthorized, 
			"Cancel Drain Reagent Pack"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (!WorkflowController::Instance().isOfType(Workflow::Type::DrainReagentPack))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "CancelDrainReagentPack: <exit, not presently running>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelDrainReagentPack: <exit>");

	return WorkflowController::Instance().getWorkflow()->abortExecute();
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetDrainReagentPackState (eDrainReagentPackState& state)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "GetDrainReagentPackState: <enter>");

	state = {};

	if (!WorkflowController::Instance().isOfType(Workflow::Type::DrainReagentPack))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "GetDrainReagentPackState: <exit, not presently running>");
		return HawkeyeError::eNotPermittedByUser;
	}

	state = WorkflowController::Instance().getCurrentWorkflowOpState<eDrainReagentPackState>();

	Logger::L().Log (MODULENAME, severity_level::debug1, "GetDrainReagentPackState: <exit>");
	return HawkeyeError::eSuccess;
}
