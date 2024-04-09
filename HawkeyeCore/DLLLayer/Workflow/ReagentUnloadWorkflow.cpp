#include "stdafx.h"

#include "DrainReagentPackWorkflow.hpp"
#include "Hardware.hpp"
#include "PurgeReagentLinesWorkflow.hpp"
#include "ReagentUnloadWorkflow.hpp"
#include "Workflow.hpp"


static const char MODULENAME[] = "ReagentUnloadWorkflow";

//*****************************************************************************
ReagentUnloadWorkflow::ReagentUnloadWorkflow(
	std::vector<ReagentContainerUnloadOptionDLL> unloadContainersOptions,
	ReagentPack::reagent_unload_status_callback_DLL stateChangeCallback)
	: unloadContainersOptions_(unloadContainersOptions)
	, stateChangeCallback_(stateChangeCallback)
	, Workflow(Workflow::Type::ReagentUnload)
{
	currentState_ = ReagentUnloadSequence::eULIdle;
}

//*****************************************************************************
ReagentUnloadWorkflow::~ReagentUnloadWorkflow()
{ }

//*****************************************************************************
HawkeyeError ReagentUnloadWorkflow::load(std::string filename)
{
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError ReagentUnloadWorkflow::execute()
{
	setBusyStatus(true);
	pServices_->enqueueInternal([this]() -> void
	{
		executeReagentSequence();
	});
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
std::string ReagentUnloadWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	return std::string();
}

//*****************************************************************************
void ReagentUnloadWorkflow::getTotalFluidVolumeUsage(
	std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
	boost::optional<uint16_t> jumpToState)
{
	syringeVolumeMap.clear();
}

//*****************************************************************************
void ReagentUnloadWorkflow::executeReagentSequence()
{
	queueNextSequence(ReagentUnloadSequence::eULIdle);
}

//*****************************************************************************
void ReagentUnloadWorkflow::queueNextSequence (ReagentUnloadSequence nextSequenceToRun)
{
	if (stateChangeCallback_)
	{
		// Update host about current state is about to be run
		pServices_->enqueueExternal (stateChangeCallback_, nextSequenceToRun);
	}

	pServices_->enqueueInternal(
		std::bind(&ReagentUnloadWorkflow::executeReagentUnloadSequence, this, nextSequenceToRun));
}

//*****************************************************************************
void ReagentUnloadWorkflow::executeReagentUnloadSequence (ReagentUnloadSequence currentSequence)
{
	/*
	Workflow description for UNLOADING the reagent packs.
	Overview:
	The business logic has received a request from the Host to unload the reagent container(s).
	Along with this request are instructions for how to handle fluid in the lines / containers.
	Our final state ends with the containers disengaged and access given to the user.
	1) For each known container, execute the requested handling :
		None
			Proceed to major step 2.
		Drain
			Use the syringe pump to systematically drain the remaining contents of the container into the known location of the Waste tank.A special command to the Controller Board must be used to allow over - draw to clear the "guard volume".
			Ensure "remaining uses" count is at "zero".
			Proceed to major step 2
		Purge
			Use the syringe pump to aspirate AIR and use the AIR to push the remaining fluid volume from the lines back into the reagent containers.A special command to the controller board must be used to allow push - back into the reagents.
			Proceed to major step 2
	2) Ensure RFIDs closed out by Controller board
	3) Retract reagent probes.
	4) Ensure that Controller board reports NO active RFID tags.
	5) Unlatch reagent access door.
	6) Complete.
	*/

	auto& reagentController = Hardware::Instance().getReagentController();
	currentState_ = static_cast<uint16_t>(currentSequence);

	boost::optional<ReagentContainerStateDLL> container;
	if (!unloadContainersOptions_.empty() &&
		ReagentPack::Instance().GetReagentContainerStates().size() > unloadContainersOptions_.back().location_id)
	{
		container = ReagentPack::Instance().GetReagentContainerStates()[unloadContainersOptions_.back().location_id];
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "executeReagentUnloadSequence :: current state is : " + std::to_string(currentState_));

	switch (currentSequence)
	{
		case eULIdle:
		{
			//if (!params_.pReagentData)
			//{
			//	Logger::L().Log (MODULENAME, severity_level::debug1, "container_opt.container_action: No reagent data available!");
			//	queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
			//	return;
			//}

			// If there is no reagent pack installed, then pull up the probes and open the door.
			if (!reagentController->IsPackInstalled())
			{
				queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
				return;
			}

			if (reagentController->IsHome())
			{
				// When the arm is home, then just unlatch the door.
				queueNextSequence(ReagentUnloadSequence::eULUnlatchingDoor);
				return;
			}

			if (unloadContainersOptions_.empty())
			{
				queueNextSequence(ReagentUnloadSequence::eULComplete);
				return;
			}

			const auto container_opt = unloadContainersOptions_.back();
			switch (container_opt.container_action)
			{
				case eULNone:
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "container_opt.container_action: eULNone");
					Logger::L().Log (MODULENAME, severity_level::debug2, "No Unload Option, retract probes and open the door.");

					queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
					return;
				}

				case eULDrainToWaste:
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "container_opt.container_action: eULDrainToWaste");
					Logger::L().Log (MODULENAME, severity_level::debug2, "Drain To Waste Unload Option");

					if (ReagentPack::Instance().isEmpty())
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "container_opt.container_action: Reagent pack is unusable!");
						queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
						return;
					}

					bool isArmDown = Hardware::Instance().getReagentController()->IsDown();
					if (isArmDown)
					{
						queueNextSequence(ReagentUnloadSequence::eULDraining1);
						return;
					}

					Logger::L().Log (MODULENAME, severity_level::debug1, "container_opt.container_action: Reagent arm is not down!");
					bool isArmHome = Hardware::Instance().getReagentController()->IsHome();

					// If reagent arm is not up to home position then put it down
					if (!isArmHome)
					{
						Hardware::Instance().getReagentController()->ArmDown([this](bool status) -> void
						{
							Logger::L().Log (MODULENAME, severity_level::debug1, "ArmDown callback : " + status ? "True" : "False");
							if (status)
							{
								queueNextSequence(ReagentUnloadSequence::eULDraining1);
							}
							else
							{
								queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
							}
						});
						return;
					}

					queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
					return;
				}

				case eULPurgeLinesToContainer:
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "container_opt.container_action: eULPurgeLinesToContainer");
					Logger::L().Log (MODULENAME, severity_level::debug2, "Purge Lines Unload Option");

					bool isArmHome = Hardware::Instance().getReagentController()->IsHome();
					if (isArmHome)
					{
						queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
						return;
					}

					// If reagent arm is not up to home position then put it purge position
					reagentController->ArmPurge([this](bool status) -> void
					{
						ReagentUnloadSequence nextStateInternal = ReagentUnloadSequence::eULPurging1;
						if (!status)
						{
							Logger::L().Log (MODULENAME, severity_level::error, "Reagent unload: Failed to purge the reagent lines");
							nextStateInternal = ReagentUnloadSequence::eULFailed_DrainPurge;
						}
						queueNextSequence(nextStateInternal);
					});
					return;
				}

				default:
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Unknown Unload Option");
					queueNextSequence(ReagentUnloadSequence::eULFailure_StateMachineTimeout);
					return;
				}

			} // End "switch (container_opt.container_action)"

			// Unreachable code
			HAWKEYE_ASSERT (MODULENAME, false);
			return;
		} // End "case eULIdle".

		case eULDraining1:
		case eULDraining2:
		case eULDraining3:
		case eULDraining4:
		case eULDraining5:
		{
			size_t drainBottleIndex = std::numeric_limits<size_t>().max();	//assign some big value
			auto nextSequence = currentSequence;
			if (currentSequence == ReagentUnloadSequence::eULDraining1)
			{
				drainBottleIndex = 0;
				nextSequence = ReagentUnloadSequence::eULDraining2;
			}
			else if (currentSequence == ReagentUnloadSequence::eULDraining2)
			{
				drainBottleIndex = 1;
				nextSequence = ReagentUnloadSequence::eULDraining3;
			}
			else if (currentSequence == ReagentUnloadSequence::eULDraining3)
			{
				drainBottleIndex = 2;
				nextSequence = ReagentUnloadSequence::eULDraining4;
			}
			else if (currentSequence == ReagentUnloadSequence::eULDraining4)
			{
				drainBottleIndex = 3;
				nextSequence = ReagentUnloadSequence::eULDraining5;
			}
			else if (currentSequence == ReagentUnloadSequence::eULDraining5)
			{
				drainBottleIndex = 4;

				// Remove the current item to avoid processing it again
				//Retract Probes ONLY IF we are done with all unloading actions otherwise go to Idle will start next unload action
				unloadContainersOptions_.pop_back();

				nextSequence = unloadContainersOptions_.empty() ? ReagentUnloadSequence::eULRetractingProbes : ReagentUnloadSequence::eULIdle;
			}

			// Check if we can Drain bottle
			// Container is available, Bottle is available, Fluid is available
			if (!container || container->reagent_states.size() <= drainBottleIndex
				|| container->reagent_states[drainBottleIndex].events_remaining <= 0
				|| container->reagent_states[drainBottleIndex].valve_location == SyringePumpPort::Port::InvalidPort)
			{
				queueNextSequence(nextSequence);
				return;
			}

			uint8_t valveLocation = static_cast<uint8_t>(container->reagent_states[drainBottleIndex].valve_location);
			uint16_t eventRemaining = container->reagent_states[drainBottleIndex].events_remaining;

			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("reagentPackUnload : Draining valve location <%d> event remaining <%d>") % (int)valveLocation % eventRemaining));

			DrainReagentPack ([this, nextSequence](bool status) -> void
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload : Failed to perform draining!");
					queueNextSequence(ReagentUnloadSequence::eULFailed_DrainPurge);
				}
				else
				{
					queueNextSequence(nextSequence);
				}
			}, valveLocation, (eventRemaining - 1)); // Repeat count = eventRemaining - 1
			return;
		}
		case eULPurging1:
		case eULPurging2:
		case eULPurging3:
		{
			auto purgeCallback = [this](ePurgeReagentLinesState newstate) -> void
			{
				// We never purge the reagent lines.
				boost::optional<ReagentUnloadSequence> purgeState;
				switch (newstate)
				{
					case dprl_PurgeCleaner1:
						Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload: eULPurging1");
						purgeState = ReagentUnloadSequence::eULPurging1;
						break;

					case dprl_PurgeCleaner2:
						Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload: eULPurging2");
						purgeState = ReagentUnloadSequence::eULPurging2;
						break;

					case dprl_PurgeCleaner3:
						Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload: eULPurging3");
						purgeState = ReagentUnloadSequence::eULPurging3;
						break;

					case dprl_PurgeReagent1:
						Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload: eULPurgingReagent1");
						purgeState = ReagentUnloadSequence::eULPurgingReagent1;
						break;

					case dprl_PurgeDiluent:
						Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload: eULPurgingDiluent");
						purgeState = ReagentUnloadSequence::eULPurgingReagent2;
						break;
				}

				if (purgeState && stateChangeCallback_)
				{
					pServices_->enqueueExternal (stateChangeCallback_, purgeState.get());
				}
			};

			if (!container)
			{
				queueNextSequence(ReagentUnloadSequence::eULRetractingProbes);
				return;
			}

			PurgeReagentLines ([this](bool status) -> void
			{
				if (!status)
				{
					queueNextSequence(ReagentUnloadSequence::eULFailed_DrainPurge);
					return;
				}

				// Remove the current item to avoid processing it again
				//Retract Probes ONLY IF we are done with all unloading actions otherwise go to Idle will start next unload action
				unloadContainersOptions_.pop_back();
				queueNextSequence (unloadContainersOptions_.empty() ? ReagentUnloadSequence::eULRetractingProbes : ReagentUnloadSequence::eULIdle);

			}, purgeCallback, container->position);
			return;
		}

		case eULRetractingProbes:
		{
			reagentController->ArmUp([this](bool status)-> void
			{
				ReagentUnloadSequence nextStateInternal = ReagentUnloadSequence::eULUnlatchingDoor;
				if (!status)
				{
					nextStateInternal = ReagentUnloadSequence::eULFailed_ProbeRetract;
					Logger::L().Log (MODULENAME, severity_level::error, "reagentPackUnload: Failed to retract the reagent arm");
				}

				queueNextSequence(nextStateInternal);
			});
			return;
		}

		case eULUnlatchingDoor:
		{
			reagentController->UnlatchDoor([this](bool status)-> void
			{
				ReagentUnloadSequence nextStateInternal = ReagentUnloadSequence::eULComplete;
				if (!status)
				{
					nextStateInternal = ReagentUnloadSequence::eULFailed_DoorUnlatch;
					Logger::L().Log (MODULENAME, severity_level::error, "reagentPackUnload: Failed to unlatch reagent door");
				}

				queueNextSequence(nextStateInternal);
			});
			return;
		}

		case eULComplete:
		{
			// Check if any entry exist to unload
			if (!unloadContainersOptions_.empty())
			{
				// Make sure the "unloadContainersOptions" is not empty before calling "*.pop_back()" to prevent undefined behavior
				unloadContainersOptions_.pop_back();		// Remove the current item to avoid processing it again

				if (!unloadContainersOptions_.empty())
				{
					queueNextSequence(ReagentUnloadSequence::eULIdle);
					return;
				}
			}

			Logger::L().Log (MODULENAME, severity_level::debug1, "reagentPackUnload: Success!!!");
			onCompleteExecution(true);
			return;
		}

		// Failure termination state.
		case eULFailed_ProbeRetract:
		case eULFailed_DoorUnlatch:
		case eULFailed_DrainPurge:
		case eULFailure_StateMachineTimeout:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "reagentPackUnload: Failure!!!");
			onCompleteExecution(false);
			return;
		}

		default:
		{
			unloadContainersOptions_.clear();
			Logger::L().Log (MODULENAME, severity_level::critical, "reagentPackUnload: Invalid unload sequence option!!!");
			throw std::exception("reagentPackUnload: Invalid unload sequence option!!!");
			return;
		}
	}

	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void ReagentUnloadWorkflow::onCompleteExecution (bool success)
{
	unloadContainersOptions_.clear();
	Workflow::triggerCompletionHandler (success ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);
}

//*****************************************************************************
void ReagentUnloadWorkflow::DrainReagentPack(
	std::function<void(bool)> callback,
	uint8_t valveLocation, uint16_t eventRemaining)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!Hardware::Instance().getReagentController()->IsPackInstalled())
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "DrainReagentPack: <exit, Reagent pack not installed>");
		pServices_->enqueueExternal (callback, false);
		return;
	}

	auto drain_wf = std::make_shared<DrainReagentPackWorkflow>(nullptr, eventRemaining);

	// "Capture" drain_wf shared_ptr so that it exist till completion
	drain_wf->registerCompletionHandler([this, callback, drain_wf](HawkeyeError he) -> void
	{
		pServices_->enqueueExternal (callback, he == HawkeyeError::eSuccess);
	});

	drain_wf->setServices (pServices_);

	if (drain_wf->load (drain_wf->getWorkFlowScriptFile(valveLocation)) != HawkeyeError::eSuccess ||
		drain_wf->execute() != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "reagentPackUnload: failed to perform draining");
		pServices_->enqueueExternal (callback, false);
		return;
	}
}

//*****************************************************************************
void ReagentUnloadWorkflow::PurgeReagentLines(
	std::function<void(bool)> callback,
	PurgeReagentLinesWorkflow::purge_reagentlines_callback_DLL purgeCallback,
	ReagentContainerPosition position)
{
	auto purge_wf = std::make_shared<PurgeReagentLinesWorkflow>(purgeCallback);

	// "Capture" purge_wf shared_ptr so that it exist till completion
	purge_wf->registerCompletionHandler([this, callback, purge_wf](HawkeyeError he) -> void
	{
		pServices_->enqueueExternal (callback, he == HawkeyeError::eSuccess);
	});

	purge_wf->setServices (pServices_);

	if (purge_wf->load (purge_wf->getWorkFlowScriptFile (static_cast<uint8_t>(position))) != HawkeyeError::eSuccess ||
		purge_wf->execute() != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "reagentPackUnload: failed to perform draining");
		pServices_->enqueueExternal (callback, false);
		return;
	}
}