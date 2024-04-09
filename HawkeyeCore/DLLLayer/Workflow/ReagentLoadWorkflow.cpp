#include "stdafx.h"

#include "Hardware.hpp"
#include "PrimeReagentLinesWorkflow.hpp"
#include "ReagentLoadWorkflow.hpp"
#include "Workflow.hpp"

static const char MODULENAME[] = "ReagentLoadWorkflow";

ReagentLoadWorkflow::ReagentLoadWorkflow (
	std::function<void (std::function<void (bool)>)> scanAndReadRfidTagsLambda,
	std::function<std::tuple<bool, bool> (void)> isReagentPackValidLambda,
	ReagentPack::reagent_load_status_callback_DLL loadStatusChangeCallback)
	: scanAndReadRfidTagsLambda_(scanAndReadRfidTagsLambda)
	, isReagentPackValidLambda_(isReagentPackValidLambda)
	, loadStatusChangeCallback_(loadStatusChangeCallback)
	, Workflow(Workflow::Type::ReagentLoad)
{
	currentState_ = ReagentLoadSequence::eLIdle;
}

ReagentLoadWorkflow::~ReagentLoadWorkflow()
{ }

HawkeyeError ReagentLoadWorkflow::load (std::string filename)
{
	return HawkeyeError::eSuccess;
}

HawkeyeError ReagentLoadWorkflow::execute()
{
	setBusyStatus(true);
	pServices_->enqueueInternal([this]() -> void
	{
		executeReagentSequence();
	});
	return HawkeyeError::eSuccess;
}

std::string ReagentLoadWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	return std::string();
}

void ReagentLoadWorkflow::getTotalFluidVolumeUsage(
	std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
	boost::optional<uint16_t> jumpToState)
{
	syringeVolumeMap.clear();
}

void ReagentLoadWorkflow::executeReagentSequence()
{
	queueNextSequence(ReagentLoadSequence::eLIdle);
}

void ReagentLoadWorkflow::queueNextSequence (ReagentLoadSequence nextSequenceToRun)
{
	if (loadStatusChangeCallback_)
	{
		// Update host about current state is about to be run
		pServices_->enqueueExternal (loadStatusChangeCallback_, nextSequenceToRun);
	}

	pServices_->enqueueInternal (std::bind (&ReagentLoadWorkflow::executeReagentLoadSequence, this, nextSequenceToRun));
}

void ReagentLoadWorkflow::executeReagentLoadSequence (ReagentLoadSequence currentSequence)
{
	/*
	//TODO:
	Wait for door latch to report Engaged
	TO after XX seconds -- eLFailure_DoorLatchTimeout
	Wait for reagent sensor to report Engaged
	TO after XX seconds -- eLFailure_ReagentSensorDetect
	Request list of reagent container IDs from CBI
	0 reagents -- eLFailure_NoReagentsDetected
	For each container RFID tag...
		Check authentication
			FAIL: eLFailure_ReagentInvalid
		Read Contents
			FAIL: eLFailure_ReagentInvalid
		Record new Reagent definitions
		Record new Analysis definitions
		Check expiration dates
			FAIL: eLFailure_ReagentExpired
		Check remaining uses
		FAIL: eLFailure_ReagentEmpty
		For each analysis...
		Check available reagents
			FAIL: eLFailure_ReagentInvalid (or TBD - inconsistent data)
		Check parameters against image analysis algorithm
			FAIL: eLFailure_TBD
	At least one container must be multi-fluid
		FAIL: eLFailure_NoWasteDetected
	Vi-CELL BLU should see only a single, 4-reagent container
		FAIL: eLFailure_TBD
	Vi-CELL FL should see a single 3-reagent container and 0..2 single-reagent containers
		FAIL: eLFailure_TBD
	Single-reagent containers detected? (NEW - no response from from UI required)
		BLU reagent will always be a square container and same reagent valve as multi-fluid container.
		FL  reagent will always be a round container and other remaining reagent valve.
	Drive probes down
		TO after XX seconds: eLFailure_ProbeInsert
		FAIL: eLFailure_ProbeInsert
	Ensure "in service date" is written
	Prime fluidics
		???
		FAIL: eLFailure_Fluidics
	SUCCEED eLComplete
	*/

	auto& reagentController = Hardware::Instance().getReagentController();
	currentState_ = static_cast<uint16_t>(currentSequence);

	switch (currentSequence)
	{
		case eLIdle:
		{
			queueNextSequence(ReagentLoadSequence::eLWaitingForDoorLatch);
			return;
		}

		case eLWaitingForDoorLatch:
		{
			checkReagentDoorClosed([this](bool success) -> void
			{
				if (success)
				{
					queueNextSequence(ReagentLoadSequence::eLWaitingForReagentSensor);
				}
				else
				{
					queueNextSequence(ReagentLoadSequence::eLFailure_DoorLatchTimeout);
					Logger::L().Log (MODULENAME, severity_level::warning, "executeReagentLoadSequence: eLFailure_DoorLatchTimeout");
				}
			});
			return;
		}

		case eLWaitingForReagentSensor:
		{
			if (reagentController->IsPackInstalled())
			{
				queueNextSequence(ReagentLoadSequence::eLIdentifyingReagentContainers);
			}
			else
			{
				queueNextSequence(ReagentLoadSequence::eLFailure_NoReagentsDetected);
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::reagent_pack_nopack, 
					instrument_error::reagent_pack_instance::general, 
					instrument_error::severity_level::warning));
			}
			return;
		}

		case eLIdentifyingReagentContainers:
		{
			scanAndReadRfidTagsLambda_ ([this](bool status) -> void {
				if (status) {
					queueNextSequence(ReagentLoadSequence::eLSynchronizingReagentData);
				} else {
					queueNextSequence(ReagentLoadSequence::eLFailure_ReagentInvalid);
				}
			});
			return;
		}

		case eLSynchronizingReagentData:
		{
			auto isReagentPackValid = isReagentPackValidLambda_();
			bool isExpired = std::get<0>(isReagentPackValid);
			bool isEmpty = std::get<1>(isReagentPackValid);
			if (isExpired)
			{
				queueNextSequence(ReagentLoadSequence::eLFailure_ReagentExpired);
			}
			else if (isEmpty)
			{
				queueNextSequence(ReagentLoadSequence::eLFailure_ReagentEmpty);
			}
			else
			{
				queueNextSequence(ReagentLoadSequence::eLInsertingProbes);
			}
			return;
		}

		case eLInsertingProbes:
		{
			// Check if we know where the reagent arm is.
			if ((!reagentController->IsHome()) && !reagentController->IsDown())
			{
				// Make sure the reagent arm is set to home only if the arm is not at down position.
				reagentController->ArmHome([this, currentSequence](bool status) -> void
				{
					auto nextStateInternal = currentSequence;
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "reagentPackLoad: failed to position ArmHome");
						nextStateInternal = ReagentLoadSequence::eLFailure_ProbeInsert;
					}
					queueNextSequence(nextStateInternal);
				});
				return;
			}

			//TODO: retry to put arm down...
			reagentController->ArmDown ([this](bool status)
			{
				if (status && Hardware::Instance().getReagentController()->IsDown())
				{
					queueNextSequence(ReagentLoadSequence::eLPrimingFluidLines);
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error, "reagentPackLoad: failed to position ArmDown");
					queueNextSequence(ReagentLoadSequence::eLFailure_ProbeInsert);
				}
			});
			return;
		}

		case eLPrimingFluidLines:
		{
			this->onSequencePrimingFluidLines([this](ReagentLoadSequence nextSequence) -> void
			{
				queueNextSequence(nextSequence);
			});
			return;
		}

		case eLComplete:
		{
			onCompleteExecution(true);
			return;
		}

		//***ERROR States**//
		case eLFailure_DoorLatchTimeout:
		case eLFailure_ReagentSensorDetect:
		case eLFailure_NoReagentsDetected:
		case eLFailure_NoWasteDetected:
		case eLFailure_ReagentInvalid:
		case eLFailure_ReagentEmpty:
		case eLFailure_ReagentExpired:
		case eLFailure_InvalidContainerLocations:
		case eLFailure_ProbeInsert:
		case eLFailure_Fluidics:
		case eLFailure_StateMachineTimeout:
		{
			onCompleteExecution(false);
			return;
		}

		default:
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "reagentPackLoad: Invalid load sequence option!!!");
			throw std::exception("reagentPackLoad: Invalid load sequence option!!!");
			return;
		}
	}

	HAWKEYE_ASSERT (MODULENAME, false);
}

void ReagentLoadWorkflow::onCompleteExecution(bool success)
{
	Workflow::triggerCompletionHandler(success ? HawkeyeError::eSuccess : HawkeyeError::eHardwareFault);
}

void ReagentLoadWorkflow::checkReagentDoorClosed(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto timer = std::make_shared<DeadlineTimerUtilities>();
	bool success = timer->waitRepeat(
		pServices_->getInternalIosRef(),
		boost::posix_time::milliseconds(250),
		callback,
		[this, timer]() {return !Hardware::Instance().getReagentController()->IsDoorClosed(); },
		boost::posix_time::seconds(5));

	if (!success)
	{
		callback(false);
	}
}

void ReagentLoadWorkflow::onSequencePrimingFluidLines (std::function<void(ReagentLoadSequence)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto prime_wf = std::make_shared<PrimeReagentLinesWorkflow>([](ePrimeReagentLinesState) {});

	// "Capture" prime_wf shared_ptr so that it exist till completion.
	prime_wf->registerCompletionHandler ([this, callback, prime_wf](HawkeyeError he)
		{
			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "reagentPackLoad: failed to perform priming");
				pServices_->enqueueExternal (callback, ReagentLoadSequence::eLFailure_Fluidics);
			}
			else
			{
				pServices_->enqueueExternal (callback, ReagentLoadSequence::eLComplete);
			}
		});

	prime_wf->setServices (pServices_);

	if (prime_wf->load (prime_wf->getWorkFlowScriptFile(0)) != HawkeyeError::eSuccess || 
		prime_wf->execute() != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "reagentPackLoad: failed to perform priming");
		pServices_->enqueueExternal (callback, ReagentLoadSequence::eLFailure_Fluidics);
		return;
	}
}