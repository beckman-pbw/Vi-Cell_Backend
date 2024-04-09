#include "stdafx.h"

#include "GetAsStrFunctions.hpp"
#include "Hardware.hpp"
#include "SampleWorkflow.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "SampleWorkflow";

static bool usingACup = false;

SampleWorkflow::SampleWorkflow(
	std::function<void(cv::Mat)> cameraCaptureCallback,
	std::function<void(eSampleStatus, bool)> callback)
	: Workflow(Workflow::Type::Sample)
	, cameraCaptureCallback_(cameraCaptureCallback)
	, callback_(callback)
{ 
	currentState_ = eSampleStatus::eNotProcessed;
}

void SampleWorkflow::UseACup()
{
	usingACup = true;
}

HawkeyeError SampleWorkflow::execute()
{
	if (currentState_ != eSampleStatus::eNotProcessed)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	// set the cleaning cycle parameters
	setCleaningCycleIndices();

	startTime_ = ChronoUtilities::CurrentTime();

	if (usingACup)
	{
		HawkeyeError he = Workflow::execute();
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow!");
			triggerCompletionHandler (he);
		}
	}
	else
	{
		Hardware::Instance().getStageController()->ProbeDown ([this](bool status)
		{
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow : unable to move probe down!");
				triggerCompletionHandler (HawkeyeError::eHardwareFault);
				return;
			}

			HawkeyeError he = Workflow::execute();
			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to execute workflow!");
				triggerCompletionHandler (he);
			}
		});
	}

	return HawkeyeError::eSuccess;
}

void SampleWorkflow::triggerCompletionHandler(HawkeyeError he)
{
	cleanupBeforeExit ([this, he](bool)
	{
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Workflow execution failed!");
			onWorkflowStateChanged (eSampleStatus::eSkip_Error, true);
		}
		else
		{
			// Sample workflow has completed, tell the UI.
			// This does not mean that the sample is complete (data needs to be written).
			// Worklist.cpp code will do that.
		
//NOTE: save for timing testing...		
//			Logger::L ().Log (MODULENAME, severity_level::debug1,
//				boost::str (boost::format ("updateSampleStatus:: currentState_: %s")
//					% getSampleStatusAsStr (static_cast<eSampleStatus>(currentState_))));

			onWorkflowStateChanged (eSampleStatus::eAcquisition_Complete, false);
		}

		usingACup = false;

		const uint32_t MaxAllowedTimeInSec = 110;
		if (!verifySampleTiming (startTime_, ChronoUtilities::CurrentTime(), MaxAllowedTimeInSec))
		{
			Logger::L().Log (MODULENAME, severity_level::warning, 
				boost::str(boost::format("triggerCompletionHandler: <Probe down to Probe up> time is more than : %d seconds") % MaxAllowedTimeInSec));
		}

		currentState_ = eSampleStatus::eNotProcessed;
		Workflow::triggerCompletionHandler(he);
	});
}

HawkeyeError SampleWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "aborting running operation");
	return Workflow::abortExecute();
}

std::string SampleWorkflow::getWorkFlowScriptFile (uint8_t workflowSubType)
{
	switch (workflowSubType)
	{
		case SampleWorkflow::eSampleWorkflowSubType::eNormalWash:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eSampleNormal);
		case SampleWorkflow::eSampleWorkflowSubType::eFastWash:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eSampleFast);
		case SampleWorkflow::eSampleWorkflowSubType::eACup:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eSampleACup);
		case SampleWorkflow::eSampleWorkflowSubType::eACupNoInternalDilution:
			return HawkeyeDirectory::Instance().getWorkFlowScriptFile(HawkeyeDirectory::WorkFlowScriptType::eSampleACupNoDilution);
		default:
			Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile() : Not supported eSamplePostWash type : " + std::to_string(workflowSubType));
			break;
	}

	return std::string();
}

void SampleWorkflow::getTotalFluidVolumeUsage(
	std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap, 
	boost::optional<uint16_t> jumpToState)
{
	Workflow::getTotalFluidVolumeUsage(syringeVolumeMap, jumpToState);

	//TODO: For sample workflow cleaning instruction are executed
	// based on reagent information.
	/**************************************************************************************/
	// This code neglect the cleaning cycle fluidics usage from "SampleWorkflow_xxx.txt" script
	// Enable when required
	
	// syringeVolumeMap.clear();
	// if (workflowOperations_.empty())
	// {
	// 	return;
	// }
	// uint32_t startIndex = 0;
	// if (jumpToState)
	// {
	// 	startIndex = getWorkflowOperationIndex(pLocalIoSvc, pHardware, *jumpToState);
	// }
	// uint32_t endIndex = getWorkflowOperationIndex(
	// 	pLocalIoSvc, pHardware, static_cast<uint16_t>(eSampleStatus::eInProcess_Cleaning));
	// 
	// Workflow::getTotalFluidVolumeUsageInternal(startIndex, endIndex, syringeVolumeMap);

	// ToDo : Add the fluidics usage apart from "SampleWorkflow_xxx.txt" script

	/**************************************************************************************/
}

HawkeyeError SampleWorkflow::load(std::string filename)
{
	HawkeyeError he = Workflow::load(filename);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	// Now add the workflow operation (not present in Script)		
	emptySyringeAtBeginning();

	return he;
}

void SampleWorkflow::onWorkflowStateChanged (uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;

	auto state = static_cast<eSampleStatus>(currentState);

	if (callback_)
	{
		// Update host with current state.
		callback_(state, executionComplete);
	}
}

std::function<void(cv::Mat)> SampleWorkflow::getCameraCallback()
{
	// wrap input callback to trigger o workflow asio
	return [this](cv::Mat image)
	{
		pServices_->enqueueInternal(cameraCaptureCallback_, image);
	};
}

void SampleWorkflow::setCleaningCycleIndices()
{
	const auto cleaningState = eSampleStatus::eInProcess_Cleaning;
	size_t startIndex = getWorkflowOperationIndex(cleaningState);
	startIndex++; // Start from cleaning process without updating UI with state change

	size_t endIndex = workflowOperations_.size() - 1;

	opCleaningIndex_ = std::make_pair(startIndex, endIndex);
}

void SampleWorkflow::cleanupBeforeExit(std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// always ensure the probe is up, even for A-Cup.
	Hardware::Instance().getStageController()->ProbeUp( [ this, callback ]( bool isProbeUp ) -> void
		{
			if ( !isProbeUp )
			{
				Logger::L().Log( MODULENAME, severity_level::error, "Failed to move probe up" );
			}

			Logger::L().Log( MODULENAME, severity_level::normal, "Probe up" );

			// Go to next tube position once the workflow is finished so that
			// the current tube will not be processed again
			if ( !usingACup && Hardware::Instance().getStageController()->IsCarouselPresent() )
			{
				Hardware::Instance().getStageController()->GotoNextTube(
					[ = ]( int32_t tubePos ) -> void
					{
						bool status = tubePos > 0 && tubePos <= MaxCarouselTubes;
						if ( !status )
						{
							Logger::L().Log( MODULENAME, severity_level::error, "Failed to go to next tube location" );
						}
						pServices_->enqueueInternal( callback, status );
					} );
			}
			else
			{
				pServices_->enqueueInternal( callback, isProbeUp );
			}
		});
}

bool SampleWorkflow::verifySampleTiming (const system_TP& startTime, const system_TP& endTime, uint32_t maxAllowedTimeInSec)
{
	HAWKEYE_ASSERT (MODULENAME, endTime > startTime);

	auto timeDiff = endTime - startTime;	
	const uint64_t timeTakenSec = std::chrono::duration_cast<std::chrono::seconds>(timeDiff).count();

	Logger::L ().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("verifySampleTiming : Time taken : %d, Max Allowed Time : %d") % timeTakenSec % maxAllowedTimeInSec));
	if (timeTakenSec > maxAllowedTimeInSec)
	{
		Logger::L().Log (MODULENAME, severity_level::warning,
			boost::str (boost::format ("Sample Workflow::verifySampleTiming: <Probe down to Probe up> time (%d secs) is more than %d secs") 
				% timeTakenSec
				% maxAllowedTimeInSec));
		return false;
	}

	return true;
}
