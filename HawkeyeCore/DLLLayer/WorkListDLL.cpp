#include "stdafx.h"

#include <unordered_map>

#include "WorklistDLL_p.hpp"
#include "WorklistDLL.hpp"

#include "AnalysisDefinitionsDLL.hpp"
#include "AuditLog.hpp"
#include "CalibrationHistoryDLL.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "EnumConversion.hpp"
#include "FileSystemUtilities.hpp"
#include "GetAsStrFunctions.hpp"
#include "Hardware.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeResultsDataManager.hpp"
#include "ImageAnalysisUtilities.hpp"
#include "ImageProcessingUtilities.hpp"
#include "ImageWrapperUtilities.hpp"
#include "LiveScanningUtilities.hpp"
#include "Logger.hpp"
#include "QualityControlsDLL.hpp"
#include "ReagentPack.hpp"
#include "SampleLog.hpp"
#include "SampleParametersDLL.hpp"
#include "SampleWorkflow.hpp"
#include "SResultData.hpp"

// CellCounting files.
#include "BlobLabel.h"
#include "CellCounterFactory.h"
#include "CellCountingOutputParams.h"
#include "ErrorCodes.h"
#include "SystemErrors.hpp"
#include "UserList.hpp"
#include "uuid__t.hpp"


static std::string MODULENAME = "Worklist";

static WorklistDLL::sample_status_callback_DLL onSampleStatus_;
static WorklistDLL::sample_image_result_callback_DLL onSampleImageProcessed_;
static WorklistDLL::sample_status_callback_DLL onSampleComplete_;
static WorklistDLL::worklist_completion_callback_DLL onWorklistComplete_;
static std::function<void (void)> cancelCarouselTubeSearchHandler_;
static std::unique_ptr<WorklistDLLPrivate> impl; // Variables related to the functioning of the WorklistDLL class.
static std::shared_ptr<WorklistDLL::WorklistData> worklistData;
static bool autoResume_ = true; // Used to restart if Resume is called before fully paused

static int s_nextImageIndexToAdd = 0;
static int s_processedImageCount = 0;
static int s_expectedImageCount = 0;

const std::map<WorklistDLL::eWorklistDLLState, std::string>
EnumConversion<WorklistDLL::eWorklistDLLState>::enumStrings<WorklistDLL::eWorklistDLLState>::data =
{
	{ WorklistDLL::eWorklistDLLState::eEntryPoint, std::string("EntryPoint") },
	{ WorklistDLL::eWorklistDLLState::eValidateWorklist, std::string("ValidateWorklist") },
	{ WorklistDLL::eWorklistDLLState::eInitializeStage, std::string("InitializeStage") },
	{ WorklistDLL::eWorklistDLLState::eProbeUp, std::string("ProbeUp") },
	{ WorklistDLL::eWorklistDLLState::eComplete, std::string("Complete") },
	{ WorklistDLL::eWorklistDLLState::eError, std::string("Error") },
};

const std::map<WorklistDLL::QueueCarouselState, std::string>
EnumConversion<WorklistDLL::QueueCarouselState>::enumStrings<WorklistDLL::QueueCarouselState>::data =
{
	{ WorklistDLL::QueueCarouselState::eEntryPoint, std::string("EntryPoint") },
	{ WorklistDLL::QueueCarouselState::eSearchForTube, std::string("SearchForTube") },
	{ WorklistDLL::QueueCarouselState::eGotoNextTube, std::string("GotoNextTube") },
	{ WorklistDLL::QueueCarouselState::eCheckIsOrphanSample, std::string("CheckIsOrphanSample") },
	{ WorklistDLL::QueueCarouselState::eHandleOrphanSample, std::string("HandleOrphanSample") },
	{ WorklistDLL::QueueCarouselState::eProcessItem, std::string("ProcessItem") },
	{ WorklistDLL::QueueCarouselState::eWaitAllDone, std::string("eWaitAllDone") },
	{ WorklistDLL::QueueCarouselState::eComplete, std::string("Complete") },
	{ WorklistDLL::QueueCarouselState::eError, std::string("Error") },
};

//*****************************************************************************
WorklistDLL::WorklistDLL (std::shared_ptr<HawkeyeServices> pHawkeyeServices)
{
	impl = std::make_unique<WorklistDLLPrivate>();
	impl->pHawkeyeServices = pHawkeyeServices;
}

//TODO: save for now...
////*****************************************************************************
//static void logSample (std::string title, const SampleDefinitionDLL& sdDLL) {
//	std::stringstream ss;
//
//	ss << std::endl << boost::str(boost::format("\t%s\n") % title);
//	ss << boost::str(boost::format("\tLabel: %s\n") % sdDLL.parameters.label);
//	ss << boost::str(boost::format("\tTag: %s\n") % sdDLL.tag);
//
//	auto position = SamplePositionDLL(sdDLL.position);
//	ss << boost::str(boost::format("\tLocation: %s\n\tCelltype Index: 0x%08X (%d)\n\tSaveEveryNthImage: %d\n\tDilution Factor: %d\n\tPostwash: %d\n")
//		% position.getAsStr()
//		% sdDLL.parameters.celltype.celltype_index
//		% sdDLL.parameters.celltype.celltype_index
//		% sdDLL.parameters.saveEveryNthImage
//		% sdDLL.parameters.dilutionFactor
//		% sdDLL.parameters.postWash
//	);
//
//	ss << boost::str(boost::format("\tBP/QC Name: %s\n") % sdDLL.bp_qc_name);
//
//	ss << boost::str(boost::format("\tAnalysis Index: 0x%04X (%d)\n\tStatus: %d\n")
//		% (int)sdDLL.parameters.analysis.analysis_index
//		% (int)sdDLL.parameters.analysis.analysis_index
//		% sdDLL.status
//	);
//
//	Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());
//}

//*****************************************************************************
static void logSamplesDLL (std::string title, const std::vector<SampleDefinitionDLL> sampleSetSamples) {

	if (Logger::L().IsOfInterest (severity_level::debug1)) {
		Logger::L().Log (MODULENAME, severity_level::debug1,
			"\n**********************************************************\n\t*** " + title + " ***");

		uint32_t idx=0;
		for (const auto& v : sampleSetSamples) {
			SampleDefinitionDLL::Log (boost::str (boost::format ("Sample[%d]") % idx++), v);
		}
	}
}

//*****************************************************************************
static void logSamplesDLL (std::string title, const std::vector<WorklistDLL::SampleIndices> indices) {

	if (Logger::L().IsOfInterest (severity_level::debug1)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, 
			"\n**********************************************************\n\t*** " + title + " ***");
		uint32_t idx = 0;
		for (const auto& v : indices) {
			SampleDefinitionDLL::Log (boost::str (boost::format ("Sample[%d]") % idx++), WorklistDLL::getSampleByIndices(v));
		}
	}
}

//*****************************************************************************
void WorklistDLL::onSampleWorkflowStateChanged (eSampleStatus status, bool executionComplete)
{
	if (executionComplete)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1,
			"onSampleWorkflowStateChanged: workflow complete with state: " + DataConversion::enumToRawValueString(status));
	}

	auto& sample = WorklistDLL::getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices());
	sample.status = status;
	updateSampleStatus (sample);
}

//*****************************************************************************
void WorklistDLL::verifySampleToSampleTiming (const SampleDefinitionDLL& sdDLL)
{
	// Sample to sample timing analysis
	if (impl->sampleToSampleTiming)
	{
		auto previousTime = impl->sampleToSampleTiming.get();
		impl->sampleToSampleTiming = std::make_pair (sdDLL.position, ChronoUtilities::CurrentTime());

		bool canVerifyForCurrentSample = true;
		if (sdDLL.position.isValidForCarousel())
		{
			// if the carousel next item is not just after anther then do not
			// consider it for sample to sample timing analysis
			previousTime.first.incrementColumn();
			canVerifyForCurrentSample = sdDLL.position.equals(previousTime.first);
		}

		const uint32_t MaxAllowedTimeInSec = 120;
		if (canVerifyForCurrentSample &&
			!SampleWorkflow::verifySampleTiming (previousTime.second, impl->sampleToSampleTiming.get().second, MaxAllowedTimeInSec))
		{
			Logger::L().Log (MODULENAME, severity_level::warning, 
				boost::str(boost::format("verifySampleToSampleTiming: <Sample to Sample, time is more than %d seconds when moving Column to %d>")
					% MaxAllowedTimeInSec 
					% sdDLL.position.getColumn()));
		}
	}
	else
	{
		impl->sampleToSampleTiming = std::make_pair (sdDLL.position, ChronoUtilities::CurrentTime());
	}
}

//*****************************************************************************
void WorklistDLL::setSystemStatus (eSystemStatus sysstatus)
{
	SystemStatus::Instance().getData().status = sysstatus;
	Logger::L().Log (MODULENAME, severity_level::debug1, "setSystemStatus: " + std::string(getSystemStatusAsStr(sysstatus)));

}

//*****************************************************************************
void WorklistDLL::addAdditionalSampleInfo (SampleDefinitionDLL& sd)
{
	sd.timestamp = ChronoUtilities::CurrentTime();	// Sampling start time.

	// Update number of images based on cell type index,
	sd.maxImageCount.reset();
	sd.maxImageCount = sd.parameters.celltype.max_image_count;
}

//*****************************************************************************
void WorklistDLL::processSample (SampleDefinitionDLL& sample, sample_completion_callback onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "processSample: <enter>");

	auto triggerPauseProcessing = [this, onComplete]()
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "processSample: Pausing the work");
		const auto he = Pause(true);

		if (he == HawkeyeError::eSuccess)
		{
			if (SystemStatus::Instance().getData().status == eSystemStatus::eIdle)
			{
				setSystemStatus(eSystemStatus::ePausing);
				autoResume_ = false;

				impl->isWorklistRunning = false;
				impl->sampleToSampleTiming = boost::none;	// Reset when current work queue paused
			}

			handlePauseStopForCurrentItem();
			return;
		}
		Logger::L().Log (MODULENAME, severity_level::error, "processSample: triggerPauseProcessing: Failed to pause work queue");
		onComplete (he);
	};

	// Pause queue and report "instrument_storage_nearcapacity" error if the disk space required for one sample is not available.
	if (!isDiskSpaceAvailableForAnalysis (std::vector<SampleDefinitionDLL>(1, sample)))
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_storagenearcapacity,
			instrument_error::instrument_storage_instance::none,
			instrument_error::severity_level::warning));

		impl->pHawkeyeServices->enqueueInternal ([triggerPauseProcessing]() { triggerPauseProcessing(); });
		Logger::L().Log (MODULENAME, severity_level::warning, "processSample: <exit,  Low Disk Space>");
		return;
	}

	// Report "instrument_storage_nearcapacity" WARNING if less than 20% free space on the disk
	uint8_t MIN_FREE_PERCENT = 20;

	if (!atLeastXXPercentDiskFree(MIN_FREE_PERCENT))
	{
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_storage_storagenearcapacity,
			instrument_error::instrument_storage_instance::none,
			instrument_error::severity_level::warning));

		Logger::L().Log(MODULENAME, severity_level::warning, boost::str(boost::format("Less than %d %% disk capacity remaining") % (uint32_t)MIN_FREE_PERCENT));
	}

// Removed for CellHealth since there carousel is not supported.
	//// Pause queue and report "fluidics_general_traycapacity" error if sample discard tray is full.
	//if (Hardware::Instance().getStageController()->IsCarouselPresent() &&
	//	SystemStatus::Instance().getData().sample_tube_disposal_remaining_capacity == 0)
	//{
	//	Logger::L().Log (MODULENAME, severity_level::error, "processSample: <exit,  sample discard tray is full>");
	//	ReportSystemError::Instance().ReportError (BuildErrorInstance(
	//		instrument_error::instrument_precondition_notmet, 
	//		instrument_error::instrument_precondition_instance::wastetubetray_capacity,
	//		instrument_error::severity_level::warning));
	//	impl->pHawkeyeServices->enqueueInternal ([triggerPauseProcessing]() { triggerPauseProcessing(); });
	//	return;
	//}

	int remainUses = SystemStatus::Instance().getData().remainingReagentPackUses;
	if (remainUses == 0)
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "processSample: <exit, - force pause>");
		impl->pHawkeyeServices->enqueueInternal([triggerPauseProcessing]() { triggerPauseProcessing(); });
		return;
	}

	// Pause queue without new error instance if system status is Faulted"
	if (SystemStatus::Instance().getData().status == eSystemStatus::eFaulted)
	{
		impl->pHawkeyeServices->enqueueInternal ([triggerPauseProcessing]() { triggerPauseProcessing (); });
		Logger::L().Log (MODULENAME, severity_level::error, "processSample: <exit,  System health is not Ok!>");
		return;
	}

	auto counting_complete_cb = [this, curSampleIndices = worklistData->samplesToProcess.GetCurrentSampleIndices()] (auto image_num, auto result)
	{
//NOTE: save for timing testing...
		//Logger::L ().Log (MODULENAME, severity_level::debug1, "counting_complete_cb: <enter>");
		impl->pHawkeyeServices->enqueueInternal ([=]()->void {onSampleCellCountingComplete (curSampleIndices, image_num, result); });
		//Logger::L ().Log (MODULENAME, severity_level::debug1, "counting_complete_cb: <exit>");
	};

	// Set up the image analysis algorithm to begin watching for data.
	ImageAnalysisUtilities::reset();
	ImageAnalysisUtilities::setCallback (counting_complete_cb);

	Logger::L().Log (MODULENAME, severity_level::debug2,
		boost::str(boost::format ("processSample:: curSampleIndex: %d") % worklistData->samplesToProcess.CurrentSampleIndex()));

	SampleDefinitionDLL::Log ("Processing sample:", sample);

	// Start setting the ancillary CellCounting configuration parameters that are
	// ultimately sent to *ImageAnalysisUtilities::SetCellCountingConfiguration*.
	CellProcessingParameters_t cellProcessingParameters = {};
	cellProcessingParameters.dilutionFactor = sample.parameters.dilutionFactor;

	// Determine which concentration slope to use.
	if (worklistData->carrier == eCarrierType::eACup)
	{
		cellProcessingParameters.calType = calibration_type::cal_ACupConcentration;
	}
	else
	{
		cellProcessingParameters.calType = calibration_type::cal_Concentration;
	}

	//NOTE: Only one analysis is currently allowed per sample.
	ImageAnalysisUtilities::SetCellCountingParameters (sample.parameters.analysis, sample.parameters.celltype, cellProcessingParameters);

	auto workflow = std::make_shared<SampleWorkflow>(std::bind(&WorklistDLL::onCameraCaptureTrigger, this, std::placeholders::_1),
		[this](auto state, bool execution_complete) { 			
			onSampleWorkflowStateChanged (state, execution_complete); 
		});

	std::map<std::string, uint32_t> script_control;
	script_control["mixing_cycles"] = sample.parameters.analysis.mixing_cycles;
	script_control["aspiration_cycles"] = sample.parameters.celltype.aspiration_cycles;

	if (workflow->setScriptControl(script_control) != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "processSample: <exit, failed to set script control [mixing_cycles, aspiration_cycles] >");
		onComplete (HawkeyeError::eInvalidArgs);
		return;
	}

	// Check if live scanning is running then stop it.
	if (LiveScanningUtilities::Instance().IsBusy())
	{
		if (LiveScanningUtilities::Instance().StopLiveScanning() != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "processSample: <exit, failed to stop live image>");
			onComplete (HawkeyeError::eInvalidArgs);
			return;
		}
	}

	// Find the max image count for this sample and pass the *max_image_count* to the workflow object.
//TODO: the image count in the next statement is always zero.  The workflow file has not been loaded at this point!!!
	uint16_t maxImageCount = workflow->getImageCount();	// get the default image count
	if (sample.maxImageCount)
	{
		maxImageCount = sample.maxImageCount.get();
	}

	s_expectedImageCount = maxImageCount;

	workflow->registerCompletionHandler ([onComplete](HawkeyeError he) -> void
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "processSample: workflow has completed");
			onComplete (he);
		});

	// Verify and report sample to sample timing
	verifySampleToSampleTiming (sample);

	// Set the work queue as running here to override the eSearchingTube that was set for a carousel.
	setSystemStatus (eSystemStatus::eProcessingSample);

	// Update UI with current sample.
	// Trigger sample state callback with "eSampleStatus::eNotProcessed".
	// This is done to notify UI when an orphan sample is found.
	onSampleWorkflowStateChanged (eSampleStatus::eNotProcessed, false);

	uint8_t workflowSubType = static_cast<uint8_t>(sample.parameters.postWash);
	if (HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::eACup)
	{
		// use the "postWash" selection to toggle the correct script for the CHM behavior ("no dilution" is used for VERY low expected concentrations where we need VERY tight CVs).
		switch (sample.parameters.postWash)
		{
			case eSamplePostWash::eFastWash:
				workflowSubType = static_cast<uint8_t>(SampleWorkflow::eSampleWorkflowSubType::eACupNoInternalDilution);
				break;
			case eSamplePostWash::eNormalWash:
			default:
				workflowSubType = static_cast<uint8_t>(SampleWorkflow::eSampleWorkflowSubType::eACup);
				break;
		}

		SampleWorkflow::UseACup();
	}

	WorkflowController::Instance().set_load_execute_async ([this, workflow, maxImageCount, triggerPauseProcessing, onComplete](HawkeyeError he)
		{
			// Check if error is reagent error, in that case pause the queue for user to replace reagent pack.
			if (he == HawkeyeError::eReagentError)
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "processSample: <exit, Invalid or empty reagent pack>");
				impl->pHawkeyeServices->enqueueInternal ([triggerPauseProcessing]() { triggerPauseProcessing(); });
				return;
			}

			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "processSample: <exit, Failed to set sample workflow>");
				onComplete (he);
				return;
			}

			// Find the max image count for the sample and pass the *max_image_count* to the workflow object.
			workflow->setImageCount (maxImageCount);

			impl->imageCollection.reset (new ImageCollection_t());
			
			// Reserve the image collection size with max image count.
			impl->imageCollection->reserve (maxImageCount);

			impl->imageCnt = 0;

			Logger::L().Log (MODULENAME, severity_level::debug1, "processSample: <exit>");

		}, workflow, Workflow::Type::Sample, workflowSubType);
}

//*****************************************************************************
void WorklistDLL::worklistCompleted()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "worklistCompleted: <enter>");

	// If the Carousel orphan SampleSet is empty, delete the empty orphan SampleSet.
	if (worklistData->carrier == eCarrierType::eCarousel)
	{
		if (worklistData->sampleSetsDLL[0].samples.size() == 0)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format ("worklistCompleted: <deleting unused Orphan SampleSet: %s>")
					% Uuid::ToStr (worklistData->sampleSetsDLL[0].uuid)));

			DBApi::eQueryResult dbStatus = DBApi::DbRemoveSampleSetByUuid (worklistData->sampleSetsDLL[0].uuid);
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("worklistCompleted: <deletion of DB_SampleSetRecord failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_writeerror,
					instrument_error::instrument_storage_instance::sample,
					instrument_error::severity_level::error));
			}
		}
		else
		{
			// Since the orphan SampleSet is considered to always be available (and running) while the Worklist is running,
			// check the last sample in the orphan SampleSet to see if it is  ? still running ?
			auto& orphanSamples = worklistData->sampleSetsDLL[OrphanSampleSetIndex].samples;
			auto& sample = orphanSamples[orphanSamples.size() - 1];
			updateSampleSetStatusToDB (getSampleSetByIndex (sample.sampleSetIndex), DBApi::eSampleSetStatus::SampleSetComplete);
		}
		// If worklist was stopped, write entry in audit log
		if (impl->isAborted)
		{
			AuditLogger::L().Log(generateAuditWriteData(
				worklistData->runUsername,
				audit_event_type::evt_worklist,
				"Stopped"));
		}
	}

	std::lock_guard<std::mutex> guard (impl->sampleStatusMutex);

	updateWorklistStatusToDB (DBApi::eWorklistStatus::WorklistComplete);

	SampleLogger::L().Log (generateSampleWriteData (*worklistData, worklistData->samplesToProcess.Get()));

	Logger::L().Log (MODULENAME, severity_level::debug1, "worklistCompleted: <exit, Worklist processing completed>");

	onWorklistComplete_ (worklistData->uuid);

	impl->isWorklistRunning = false;
	impl->isWorklistSet = false;
	setSystemStatus (eSystemStatus::eIdle);

	// Hardware cannot determine the difference between the plate and the A-Cup, since there are no direct sensors to detect the PRESENCE of a plate...
	// always ensure the probe is up, the stage is ejected, and holding current is disabled for ALL sample sources...

	// Put the probe up and eject the stage.  This can all be done while the image analysis completes.
	auto stageCompletion = [this](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "worklistCompleted: <exit: error resetting stage work queue profile>");
		}
	};

	auto ejectCompletion = [this, stageCompletion](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "worklistCompleted : <Failed to Eject the Stage>");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_sampledeck_ejectfail,
				instrument_error::motion_sample_deck_instances::general,
				instrument_error::severity_level::warning));
		}

		bool holding = EJECT_HOLDING_CURRENT_OFF;
		// TODO: this block may need to be modified for future use of external config variables
		if ( HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::ePlate_96 )
		{
			holding = EJECT_HOLDING_CURRENT_ON;
		}

		Hardware::Instance().getStageController()->SetStageProfile( stageCompletion, holding );
	};

	auto probeUpCompletion = [this, ejectCompletion, stageCompletion](bool probeStatus)
	{
		if (!probeStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "worklistCompleted: <Failed to move probe up");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_motor_positionfail,
				instrument_error::motion_motor_instances::sample_probe,
				instrument_error::severity_level::warning));

			Hardware::Instance().getStageController()->SetStageProfile (stageCompletion, false);
			return;
		}

		if ( worklistData->carrier == eCarrierType::eCarousel || worklistData->carrier == eCarrierType::ePlate_96 )
		{
			int32_t angle = NoEjectAngle;
			if ( HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::ePlate_96 )
			{
				angle = DfltAutomationEjectAngle;
			}
			Hardware::Instance().getStageController()->EjectStage( ejectCompletion, angle );
		}
	};

	Hardware::Instance().getStageController()->ProbeUp (probeUpCompletion);
}

//*****************************************************************************
//TODO: do we still need this???
//TODO: do we still need this???
//TODO: do we still need this???
// TODO - Need to update these macros with more accurate values 
#define MAX_BF_IMAGE_SIZE_MB 2
#define MAX_FL_IMAGE_SIZE_MB 2
#define MAX_FCS_FILE_SIZE_MB 4
#define MAX_SRESULT_BINARY_FILE_SIZE_MB 12
#define MINIMUM_IMAGES_TO_SAVE 2
bool WorklistDLL::isDiskSpaceAvailableForAnalysis (const std::vector<SampleDefinitionDLL>& sdDLL_list) const
{
	//NOTE : MegaBytes - MB is the unit considered for Memory calculations 
	uint64_t total_required_disk_space = 0;

	for (auto& sdDLL : sdDLL_list)
	{
		uint32_t per_sample_disk_space = 0;

		//NOTE: Test Cases
		// # Images  Nth Image ImagesSaved
		//     0         0           0
		//     0        10           0
		//     0         1           0
		//     0         5           0
		//     1         1           1
		//     1         2           1
		//     2         2           2
		//     2         4           2
		//     4         8           2
		//     8         4           3
		//    10         0           2
		//    11         3           5
		//   100         1         100
		//   100       100           2

		uint16_t images_to_save = 0;

		SampleParametersDLL params = sdDLL.parameters;

		if (params.celltype.max_image_count > 0) {

			if (params.celltype.max_image_count == 1) {

				// Save the only image.
				images_to_save = 1;

			}
			else if (params.saveEveryNthImage <= params.celltype.max_image_count) {

				if (params.saveEveryNthImage > 0) {
					images_to_save = params.celltype.max_image_count / params.saveEveryNthImage;
					if (params.saveEveryNthImage != 1) {
						// Adjust when max_image_count is evenly divisible by saveEveryNthImage.
						if (!(params.celltype.max_image_count % params.saveEveryNthImage)) {
							images_to_save--;
						}
						images_to_save += MINIMUM_IMAGES_TO_SAVE;
					}
				}
				else {
					images_to_save = MINIMUM_IMAGES_TO_SAVE;
				}
			}
			else {
				images_to_save = MINIMUM_IMAGES_TO_SAVE;
			}
		}

		// Space required for bright field images.
		uint32_t bf_image_mem = images_to_save * MAX_BF_IMAGE_SIZE_MB;

		// Space required for Fl images.
		uint32_t fl_image_mem = images_to_save * (uint32_t)params.analysis.fl_illuminators.size() * MAX_FL_IMAGE_SIZE_MB;

		// DiskSpace required for one sample  = 
		//							1. Space required for Bright field images +
		//							2. Space required for FL images +
		//							3. Space required for SResult Bin File +
		//							4. Space required for FCS file 
		per_sample_disk_space = bf_image_mem + fl_image_mem + MAX_SRESULT_BINARY_FILE_SIZE_MB + MAX_FCS_FILE_SIZE_MB;

		total_required_disk_space += per_sample_disk_space;
	}

	return FileSystemUtilities::IsRequiredFreeDiskSpaceAvailable (total_required_disk_space, FileSystemUtilities::eDiskInfoFormat::eMegaBytes);
}


bool WorklistDLL::atLeastXXPercentDiskFree(uint8_t free_percent) const
{
	// Check to see whether the root drive has at least xx% free so that we can warn a user that
	// they will need to do maintenance soon

	double capacity, freespace;
	capacity = FileSystemUtilities::getDiskCapacity(HawkeyeDirectory::Instance().getDriveId(), FileSystemUtilities::eDiskInfoFormat::eMegaBytes);
	freespace = FileSystemUtilities::getDiskFreeSpace(HawkeyeDirectory::Instance().getDriveId(), FileSystemUtilities::eDiskInfoFormat::eMegaBytes);

	// Check to prevent possible divide-by-zero / catch nonsense return values
	if (capacity < 1 || freespace < 1)
		return false;

	double empty_percent = ((double)free_percent / 100.0);

	return (freespace / capacity) > empty_percent;
}


//*****************************************************************************
void WorklistDLL::generateCarouselOrphanSample (const SamplePositionDLL& currentStagePos)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "generateCarouselOrphanSample: <enter>");

	SampleDefinitionDLL orphanSample = {};

	orphanSample.sampleSetUuid = worklistData->sampleSetsDLL[0].uuid;
	orphanSample.sampleDefUuid = {};
	orphanSample.sampleDataUuid = {};
	orphanSample.username = worklistData->runUsername;
	orphanSample.status = eSampleStatus::eNotProcessed;
	orphanSample.position = currentStagePos;
	orphanSample.runUserID = worklistData->runUserUuid;

	std::string curSequenceNum = boost::str (boost::format ("%u")
		% boost::io::group(std::setw(worklistData->sequencingNumberOfDigits), std::setfill('0'), worklistData->sequencingDigit));
	
	orphanSample.parameters = worklistData->orphanParameterSettings;

	orphanSample.parameters.label =
		worklistData->useSequencing
		? (worklistData->sequencingTextFirst ? worklistData->sequencingBaseLabel + curSequenceNum : curSequenceNum + worklistData->sequencingBaseLabel)
		: worklistData->sequencingBaseLabel;

	worklistData->sequencingDigit++;

	orphanSample.sampleSetIndex = 0;	// Always zero for the orphan SampleSet.
	orphanSample.index = static_cast<uint16_t>(worklistData->sampleSetsDLL[0].samples.size());

	auto he = validate_analysis_and_celltype (orphanSample);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "generateCarouselOrphanSample: <exit, this occurred validating the orphan sample settings");
		return;
	}

	DBApi::DB_SampleItemRecord dbSample = {};
	dbSample = orphanSample.ToDbStyle();

	DBApi::eQueryResult dbStatus = DBApi::DbAddSampleSetSample(dbSample, orphanSample.sampleSetUuid);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("generateCarouselOrphanSample: <exit, DB sample write failed, status: %ld>") % (int32_t)dbStatus));

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));
	}

	updateSampleUuids (dbSample, orphanSample);

	// Add sample to the default (orphan) SampleSet.  MUST be done before adding to samplesToProcess indices list!
	worklistData->sampleSetsDLL[0].samples.push_back( orphanSample );

	// Insert after the current sample in the Worklist.
	// It is done this way to ensure that this is the next sample processed.
	worklistData->samplesToProcess.InsertAfterCurrentSampleIndex (getIndicesFromSample(orphanSample));

	Log (worklistData->sampleSetsDLL);

	Logger::L().Log (MODULENAME, severity_level::debug1, "generateCarouselOrphanSample: <exit>");
}

//*****************************************************************************
void WorklistDLL::searchForNextTube (boost::optional<uint32_t> finalTubePosition, std::function<void(bool)> callback)
{
	std::function<void(bool)> onSearchTubeComplete = [this, callback](bool status)
	{
		cancelCarouselTubeSearchHandler_ = nullptr;
		impl->pHawkeyeServices->enqueueInternal (callback, status);
	};

	setSystemStatus (eSystemStatus::eSearchingTube);
	cancelCarouselTubeSearchHandler_ = Hardware::Instance().getStageController()->FindTubePosByPos (finalTubePosition, [](uint32_t) {}/*Returns the Current Position*/, onSearchTubeComplete);
}

// For samples with no tube in defined location, mark them as skipped
void WorklistDLL::skipSample(SampleDefinitionDLL& sample)
{
	sample.status = eSampleStatus::eSkip_Manual;
	updateSampleStatus(sample);

	SampleDefinitionDLL::Log("Skipping sample:", sample);

	onSampleComplete_(sample);
}

//*****************************************************************************
void WorklistDLL::queueCarouselSampleInternal (
	QueueCarouselState currentState, 
	boost::optional<uint32_t> currentTubeLocation,
	std::function<void(HawkeyeError)> callback)
{
	static uint32_t prevTubeLoc = 0;
	uint32_t tubeLoc = prevTubeLoc;

	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [this, callback](QueueCarouselState nextState, boost::optional<uint32_t> currentTubeLocation)
	{
		impl->pHawkeyeServices->enqueueInternal ([=]()
			{
				const auto nextStateNumber = DataConversion::enumToRawVal(nextState);
				// If stopped, goto carousel complete
				if (impl->isAborted)
					queueCarouselSampleInternal(QueueCarouselState::eComplete, currentTubeLocation, callback);		// goto cleanup operation
				else
					queueCarouselSampleInternal (nextState, currentTubeLocation, callback);
			});
	};

	Logger::L().Log (MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: " + EnumConversion<QueueCarouselState>::enumToString(currentState));

	// Get the nearest tube position
	// This will return the valid tube position (1-24) if stage position is valid w.r.t. carousel position
	// If not then it will return the approx. tube position (1-24) to which stage (carousel) can be moved;
	// move will allow small (less then 1/2 tube position) counter-clockwise movement for position refinement
	uint32_t approxTubeLoc = Hardware::Instance().getStageController()->GetNearestTubePosition();

	std::lock_guard<std::mutex> guard (impl->worklistMutex);

	switch (currentState)
	{
		// ..................................................................
		case QueueCarouselState::eEntryPoint:
		{
			// If processing the first sample then reorder work queue items if necessary
			if (worklistData->samplesToProcess.CurrentSampleIndex() == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "queueWorkQueueItemCarouselInternal : Reordering work queue items w.r.t. tube position : " + std::to_string(approxTubeLoc));
				prevTubeLoc = approxTubeLoc;
			}

			// Check if there are any samples in the eSampleStatus::eNotProcessed state. If
			// there are, set the currentTubeLocation to the next sample to be processed. Otherwise
			// fall through and continue searching for a tube.
			if (areAnySamplesNotProcessed())
			{
				auto position = getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices()).position;
				currentTubeLocation = position.getColumn();
				Logger::L().Log(MODULENAME, severity_level::debug1, 
					"queueWorkQueueItemCarouselInternal:: next user defined item to be processed: " + position.getAsStr());
			}

			onCurrentStateComplete (QueueCarouselState::eSearchForTube, currentTubeLocation);
			return;
		}

		// ..................................................................
		case QueueCarouselState::eSearchForTube:
		{
			if (prevTubeLoc != approxTubeLoc)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: current stage position has changed prior to tube search");
				prevTubeLoc = approxTubeLoc;
			}

			// Search for the next tube until one of the conditions listed in the callback have occurred.
			searchForNextTube (currentTubeLocation, [this, tubeLoc, currentState, currentTubeLocation, onCurrentStateComplete](bool isTubePresent)
				{
					static uint32_t missingCount = 0;
					auto currentPos = Hardware::Instance().getStageController()->GetStagePosition();
					Logger::L().Log (MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: current stage position after tube search is : " + currentPos.getAsStr());

					if (!isTubePresent)
					{
						// Case 1: search for tube is cancelled -> Worklist is Paused/Aborted.
						if (!impl->isWorklistRunning)
						{
							handlePauseStopForCurrentItem();
							Logger::L().Log (MODULENAME, severity_level::debug1,
								"queueCarouseSamplelInternal: no tube found, tube search was cancelled because the processing was paused or stopped");
							return;
						}

						// Case 2: # of samples specified == # of samples defined, Worklist is completed.
						if ( isWorklistComplete() || ++missingCount > 3 )		// all defined and previous orphans have been processed;
						{
							// No additional tubes found, check for any missing 'defined' tubes and if found mark as skipped.
							// This can happen when a sample set is defined and one or more consecutive positions at the
							// end of the sample set don't have tubes in them.
							for ( uint16_t idx = 0; idx < worklistData->samplesToProcess.Count(); idx++ )
							{
								auto& sample = getSampleByIndices( worklistData->samplesToProcess.GetSampleIndices( idx ) );
								if ( sample.status == eSampleStatus::eNotProcessed )
								{
									skipSample( sample );
								}
							}

							// If samples were defined then there is image processing left to do.
							if ( worklistData->samplesToProcess.Count() )
							{
								// While images have been collected for all of the samples, there may still be image
								// processing and writing of data to do.  So, set the system state back to processing.
								setSystemStatus( eSystemStatus::eProcessingSample );
								Logger::L().Log( MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: all specified or found samples have been acquired" );
							}
							else
							{ // When no tubes are found, mark the system as Idle.
								impl->isWorklistRunning = false;
								impl->isWorklistSet = false;
								Logger::L().Log( MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: no tubes found..." );
							}

							missingCount = 0;
							onCurrentStateComplete( QueueCarouselState::eComplete, currentTubeLocation );		// goto cleanup operation
							return;
						}

						// Case 3 : In a defined sample tube location, but it's Tube missing; mark as skipped
						// Look through all of the defined samples for one that matches the current position.
						uint16_t sampleIdx = 0;
						if ( worklistData->samplesToProcess.IsSampleDefined( tubeLoc, sampleIdx ) )
						{
							// for a missing defined tube, set the current sample index to the sample index of the missing tube.
							worklistData->samplesToProcess.SetCurrentSampleIndex( sampleIdx );

							auto& sample = getSampleByIndices( worklistData->samplesToProcess.GetSampleIndices( sampleIdx ) );
							if ( sample.status == eSampleStatus::eNotProcessed )
							{
								Logger::L().Log( MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: no tube found for defined location..." );
								skipSample( sample );
							}
						}

						onCurrentStateComplete( QueueCarouselState::eGotoNextTube, currentTubeLocation );
						return;
					}

					// Tube found progress to next state.
					missingCount = 0;
					onCurrentStateComplete (QueueCarouselState::eCheckIsOrphanSample, currentTubeLocation);
				});
			return;
		}

		// ..................................................................
		// Check whether the found tube is a defined or Orphan sample
		case QueueCarouselState::eCheckIsOrphanSample:
		{
			auto currentStagePosition = Hardware::Instance().getStageController()->GetStagePosition();
			uint32_t tubeLoc = currentStagePosition.getColumn();

			// system has found a tube.
			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("queueCarouselSampleInternal: Tube found at position: %d") % tubeLoc));

			// Look through all of the defined samples for one that matches the current position.
			// If no sample is found that matches the current posiiton, it is an orphan.
			uint16_t sampleIdx = 0;
			bool sampleDefined = worklistData->samplesToProcess.IsSampleDefined(tubeLoc, sampleIdx);

			if (!sampleDefined)		// no defined samples at this tube location must be an orphan...
			{
				onCurrentStateComplete (QueueCarouselState::eHandleOrphanSample, currentTubeLocation);
				return;
			}

			// found a sample defined at this tube location; may still be an orphan...

			// check prior samples in samplesToProcess by the intended processing order to see if any were skipped.
			// If so, mark as skipped to provide UI feedback and prevent incorrect orphan handling below.
			// This can happen when a sample set is defined but the tubes for the set are missing,
			// or tubes are missing in between tubes that are present in a set.
			if ( worklistData->samplesToProcess.Count() > 0 )		// if no defined samples in the list, then all samples are orphans
			{
				auto& sampleIndices = worklistData->samplesToProcess.GetSampleIndices( sampleIdx );

				// search back in the list for entries with lower processing order indicators
				for ( size_t listIdx = 0; listIdx < worklistData->samplesToProcess.Count(); ++listIdx )				// search entire list, but only check those with lower processing order than current defined sample
				{
					if ( sampleIdx != listIdx )
					{
						auto& indicesToCheck = worklistData->samplesToProcess.GetSampleIndices( listIdx );
						if ( indicesToCheck.processingOrder < sampleIndices.processingOrder )
						{
							auto& sampleToCheck = getSampleByIndices( worklistData->samplesToProcess.GetSampleIndices( listIdx ) );
							if ( sampleToCheck.status == eSampleStatus::eNotProcessed )
							{
 								skipSample( sampleToCheck );
							}
						}
					}
				}
			}

			// There is a defined sample or an orphan that in this tube location.
			// If it is marked as skipped by the CancelSampleSet operation, then
			// skip this tube and advance to the next tube.
			auto& sampleToProcess = getSampleByIndices( worklistData->samplesToProcess.GetSampleIndices( sampleIdx ) );

			if (sampleToProcess.status == eSampleStatus::eSkip_NotProcessed)		// marked as skipped by sample-set cancel; advance to next tube
			{
				SampleDefinitionDLL::Log("Skipping cancelled sample:", sampleToProcess);
				// This change of status is done here because currently the UI doesn't recognize eSkip_NotProcessed (cancelled),
				// which causes the sample to look like 'complete' if running another sample set after this cancelled one.
				sampleToProcess.status = eSampleStatus::eSkip_Manual;
				onSampleComplete_(sampleToProcess);
				onCurrentStateComplete(QueueCarouselState::eGotoNextTube, currentTubeLocation);
				return;
			}

			// If the sample at this tube location is marked as already processed, then this may be
			// another revolution around the carousel and this tube should be processed as an orphan.
			if (sampleToProcess.status != eSampleStatus::eNotProcessed)
			{
				onCurrentStateComplete(QueueCarouselState::eHandleOrphanSample, currentTubeLocation);
				return;
			}

			worklistData->samplesToProcess.SetCurrentSampleIndex(sampleIdx);
			onCurrentStateComplete (QueueCarouselState::eProcessItem, currentTubeLocation);

			return;
		}

		// ..................................................................
		// Handle orphan sample.
		case QueueCarouselState::eHandleOrphanSample:
		{
			auto currentStagePosition = Hardware::Instance().getStageController()->GetStagePosition();

			prevTubeLoc = currentStagePosition.getColumn();

			Logger::L().Log (MODULENAME, severity_level::debug1, 
				boost::str(boost::format("queueCarouselSampleInternal: No entry found for position %d, using default settings") % currentStagePosition.getColumn()));

			generateCarouselOrphanSample (currentStagePosition);

			SampleDefinitionDLL::Log ("Default sample:", getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices()));

			onCurrentStateComplete (QueueCarouselState::eProcessItem, currentTubeLocation);
			return;
		}

		// ..................................................................
		case QueueCarouselState::eProcessItem:
		{
			// If work queue is not running that means either pause or stop is called by user
			if (!impl->isWorklistRunning)
			{
				handlePauseStopForCurrentItem();
				return;
			}

			auto& sampleToProcess = getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices());
			if (sampleToProcess.status != eSampleStatus::eNotProcessed)
			{
				Logger::L().Log(MODULENAME, severity_level::debug1, "queueCarouseSamplelInternal: sample processed - Search for Tube!");
				onCurrentStateComplete (QueueCarouselState::eGotoNextTube, currentTubeLocation);
				return;
			}

			if (!sampleToProcess.position.isValidForCarousel())
			{
				// Either the carousel work queue item(s) location is invalid
				// or unable to move stage to desired location
				// Stop Worklist and mark all unprocessed samples as "eSampleStatus::eSkip_Error"

				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_precondition_notmet, 
					instrument_error::instrument_precondition_instance::carousel_present, 
					instrument_error::severity_level::warning));

				onCurrentStateComplete(QueueCarouselState::eError, currentTubeLocation);
				return;
			}

			// Update the missing information for current sample to be processed.
			addAdditionalSampleInfo (sampleToProcess);

			impl->pHawkeyeServices->enqueueInternal ([this, &sampleToProcess, callback]() -> void
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("Processing sample @ position %d") 
						% sampleToProcess.position.getColumn()));
					SampleDefinitionDLL::Log ("Current sample:", sampleToProcess);
					processSample (sampleToProcess, callback);
				});
			return;
		}

		// ..................................................................
		case QueueCarouselState::eGotoNextTube:
		{
			Hardware::Instance().getStageController()->GotoNextTube ([=](int32_t tubePos) -> void
				{
					bool status = tubePos > 0 && tubePos <= MaxCarouselTubes;
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "Failed to go to next tube location");
					}
					onCurrentStateComplete (QueueCarouselState::eEntryPoint, currentTubeLocation);
				});
			return;
		}

		// ..................................................................
		case QueueCarouselState::eComplete:
		case QueueCarouselState::eError:
		{
			// "eComplete" only means that data acquisition is done.
			// There are no more samples defined or orhpans to process.
			// This does not include image processing and data writing.

			// Lambda to be called when worklist is complete or faulted.
			auto onCarouselComplete = [this](bool success) -> void
			{
				if (!success)
				{
					setSystemStatus (eSystemStatus::eFaulted);
				}

				// Mark any unprocessed samples as skipped.
				for (auto& sampleIndices : worklistData->samplesToProcess.Get())
				{
					auto& sample = getSampleByIndices (sampleIndices);
					if (sample.status == eSampleStatus::eNotProcessed)
					{
						sample.status = eSampleStatus::eSkip_Error;
						updateSampleStatus (sample);
						SampleDefinitionDLL::Log ("Skipping sample:", sample);
					}
				}

				waitForDataWrapupToComplete();
			};

			bool success = currentState == QueueCarouselState::eComplete;
			onCarouselComplete (success);
			return;
		}

	} // End "switch (currentState)"

	Logger::L().Log (MODULENAME, severity_level::error, "queueCarouseSamplelInternal: unreachable code, application will exit");

	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void WorklistDLL::queuePlateSampleInternal (std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "queuePlateSampleInternal: <enter>");
	HAWKEYE_ASSERT (MODULENAME, callback);

	// For the plate, this will check the sample index to determine if 
	// we are done with all samples. Then just need to wait for sample processing to complete
	if (worklistData->samplesToProcess.IsDone() || isWorklistComplete() || impl->isAborted)
	{
		waitForDataWrapupToComplete();
		return;
	}

	Hardware::Instance().getStageController()->MoveStageToPosition ([this, callback](bool successful_move)
	{
		if (!successful_move)
		{
			// Either the plate work queue item(s) location is invalid
			// or unable to move stage to desired location
			// Stop Worklist and mark all unprocessed samples as "eSampleStatus::eSkip_Error"

			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::motion_sampledeck_positionfail, 
				instrument_error::motion_sample_deck_instances::general, 
				instrument_error::severity_level::error));
				
			worklistData->samplesToProcess.SetCurrentSampleIndex (std::numeric_limits<uint16_t>().max());		// set current work queue index to very high number
			impl->pHawkeyeServices->enqueueInternal ([this, callback]()
				{ 
					queuePlateSampleInternal (callback);
				});

			return;
		}

		// If worklist is not running that means either pause or stop is called by user.
		if (!impl->isWorklistRunning)
		{
			handlePauseStopForCurrentItem();
			return;
		}

		// Update the missing information for current item to be processed.
		SampleDefinitionDLL& sampleToProcess = getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices());
		addAdditionalSampleInfo (sampleToProcess);

		impl->pHawkeyeServices->enqueueInternal ([this, &sampleToProcess, callback]() -> void
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, 
					boost::str (boost::format("Processing sample @ position %d") % sampleToProcess.position.getColumn()));
				SampleDefinitionDLL::Log ("Current sample:", sampleToProcess);
				processSample (sampleToProcess, callback);
			});
	}, getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices()).position);

	Logger::L().Log (MODULENAME, severity_level::debug1, "queuePlateSampleInternal: <exit>");
}

//*****************************************************************************
void WorklistDLL::queueACupSampleInternal (std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "queueACupSampleInternal: <enter>");
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (isWorklistComplete())
	{
		waitForDataWrapupToComplete();
		return;
	}

	// If worklist is not running that means either pause or stop is called by user.
	if (!impl->isWorklistRunning)
	{
		handlePauseStopForCurrentItem();
		return;
	}

	// Update the missing information for current item to be processed.
	SampleDefinitionDLL& sampleToProcess = getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices());
	addAdditionalSampleInfo (sampleToProcess);

	impl->pHawkeyeServices->enqueueInternal ([this, &sampleToProcess, callback]() -> void
		{
			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format("Processing sample @ position %d") % sampleToProcess.position.getColumn()));
			SampleDefinitionDLL::Log ("Current sample:", sampleToProcess);
			processSample (sampleToProcess, callback);
		});

	Logger::L().Log (MODULENAME, severity_level::debug1, "queueACupSampleInternal: <exit>");
}

//*****************************************************************************
void WorklistDLL::onSampleCompleted (HawkeyeError status)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "onSampleCompleted: <enter>");

	auto& currentSample = getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices());

	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onSampleCompleted: Failed to successfully run current sample. Stopping the entire Worklist");
		ReportSystemError::Instance().ReportError (BuildErrorInstance (
			instrument_error::sample_general_processingerror,
			instrument_error::severity_level::warning));

		std::string extra = boost::str (boost::format ("Error while processing sample:\n\tSample: %s\n\tCellType: %s\n\tPosition: %s\n\tTag: %s")
			% currentSample.parameters.label
			% currentSample.parameters.celltype.label
			% currentSample.position.getAsStr ()
			% currentSample.parameters.tag);
		AuditLogger::L().Log (generateAuditWriteData(
			currentSample.username,
			audit_event_type::evt_sampleprocessingerror, 
			extra));

		Stop(); // Stop Worklist execution.

		// Since "Stop()" will trigger the "worklistCompleted" callback
		// So check if Worklist is actually stopped or not.
		if (SystemStatus::Instance().getData().status != eSystemStatus::eStopped)
		{
			setSystemStatus (eSystemStatus::eFaulted);
			worklistCompleted();
		}

		return;
	}

	// This work is done asynchronously.
	wrapupSampleProcessing (currentSample);

	{
	  // This block makes sure that the mutex lock is released before *onSampleComplete_* is called.
	  // This is necessary since the UI callback processing calls *GetSystemStatus*.
		std::lock_guard<std::mutex> guard (impl->worklistMutex);

		// Update sample processing count and sample tube disposal capacity.
		uint32_t totalSamplesProcessed = 0;
		SystemStatus::Instance().incrementSampleProcessingCount (worklistData->carrier, totalSamplesProcessed);

		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (
			boost::format ("onSampleCompleted:: sIndex: %d, Status: %d, totalSamplesProcessed: %d")
			% worklistData->samplesToProcess.CurrentSampleIndex()
			% currentSample.status
			% totalSamplesProcessed));
	}

	if (worklistData->carrier == eCarrierType::eACup)
	{
		// Nothing to do...  just exit the method.
	}
	else
	{
		// For a plate, need to increment the sample index to the next sample.
		// This is not done for the carousel, which uses the tube position to obtain the sample information
		if (worklistData->carrier == eCarrierType::ePlate_96)
		{
			worklistData->samplesToProcess.IncrementCurrentSampleIndex();
		}

		if (impl->isWorklistRunning)
		{
			queueSampleForProcessing();
		}
		else // If work queue is not running that means either it has been paused or stopped
		{
			impl->pHawkeyeServices->enqueueInternal ([this]
				{
					handlePauseStopForCurrentItem();
				});
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "onSampleCompleted: <exit>");
}

//*****************************************************************************
// Basic sample processing (workflow) has completed.
// Now wait for the image processing to complete.
//*****************************************************************************
void WorklistDLL::onSampleProcessingComplete (HawkeyeError he)
{
	if (he != HawkeyeError::eSuccess)
	{
		impl->isWorklistRunning = false;
		onSampleCompleted (he);
		return;
	}

	auto isProcessRunning = [this]() -> bool
	{
		return !ImageAnalysisUtilities::isProcessCompleted();
	};

	auto onComplete = [this](bool status) -> void
	{
		// Turn off the image processing.
		ImageAnalysisUtilities::stopProcess();
		Hardware::Instance ().getStageController()->ResumeMotorPolling();
		onSampleCompleted (status ? HawkeyeError::eSuccess : HawkeyeError::eTimedout);
		impl->deadlineTimer.reset();
	};

	impl->deadlineTimer.reset (new DeadlineTimerUtilities());
	bool timerOk = impl->deadlineTimer->waitRepeat (
		WorkflowController::Instance().getInternalIosRef(), 
		boost::posix_time::seconds(1), 
		onComplete, 
		isProcessRunning, 
		boost::none);

	if (!timerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onSampleProcessingComplete : Failed to start timer!");
		onComplete (false);
	}
}

//*****************************************************************************
void WorklistDLL::waitForDataWrapupToComplete()
{
	auto isProcessRunning = [this]() -> bool
	{
		return !isWorklistComplete();
	};

	auto onComplete = [this](bool status) -> void
	{
		worklistCompleted();
		impl->deadlineTimer.reset();
	};

	impl->deadlineTimer.reset (new DeadlineTimerUtilities());
	bool timerOk = impl->deadlineTimer->waitRepeat (
		WorkflowController::Instance().getInternalIosRef(),
		boost::posix_time::seconds(2),
		onComplete,
		isProcessRunning,
		boost::none);
	if (!timerOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onSampleProcessingComplete : Failed to start timer!");
		onComplete (false);
	}
}

//*****************************************************************************
void WorklistDLL::queueSampleForProcessing()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "queueSampleForProcessing: <enter>");

	const auto onComplete = [this](HawkeyeError he) {
		onSampleProcessingComplete (he);
		Logger::L().Log (MODULENAME, severity_level::debug1, "queueSampleForProcessing: <exit>");
	};

	impl->pHawkeyeServices->enqueueInternal ([this, onComplete]()
	{
		switch (worklistData->carrier)
		{
			default:
			case eCarrierType::eCarousel:
				queueCarouselSampleInternal (QueueCarouselState::eEntryPoint, boost::none, onComplete);
				break;
			case eCarrierType::ePlate_96:
				queuePlateSampleInternal (onComplete);
				break;
			case eCarrierType::eACup:
				queueACupSampleInternal (onComplete);
				break;
		}
	});
}

//*****************************************************************************
void WorklistDLL::handlePauseStopForCurrentItem()
{
	switch (SystemStatus::Instance().getData().status)
	{
		case eSystemStatus::eIdle:
		case eSystemStatus::ePaused:
		case eSystemStatus::eStopped:
			return;

		case eSystemStatus::ePausing:

			// For system consistency, do this even if the auto-resume flag is set.
			setSystemStatus(eSystemStatus::ePaused);

			AuditLogger::L().Log(generateAuditWriteData(
				worklistData->runUsername,
				audit_event_type::evt_worklist,
				"Paused"));

			if (autoResume_)
			{
				// We just transitioned to PAUSED but the auto-resume flag is set, so call Resume now
				Logger::L().Log(MODULENAME, severity_level::debug1, "handlePauseStopForCurrentItem: paused -> auto-resume");
				std::function<void(HawkeyeError)> myCBFunc = [=](HawkeyeError err) {};
				Resume(myCBFunc);
				autoResume_ = false;
				return;
			}

			if ( worklistData->carrier == eCarrierType::eCarousel || worklistData->carrier == eCarrierType::ePlate_96 )
			{
				int32_t angle = NoEjectAngle;
				if ( HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::ePlate_96 )
				{
					angle = DfltAutomationEjectAngle;
				}

				// DO NOT modify holding current here!
				Hardware::Instance().getStageController()->EjectStage( [this]( bool status ) -> void
					{
						if (!status)
						{
							Logger::L().Log (MODULENAME, severity_level::warning, "handlePauseStopForCurrentItem: <exit: error ejecting stage after pause>");
						}
					}, angle);
			}

			Logger::L().Log(MODULENAME, severity_level::debug1, "handlePauseStopForCurrentItem: system is paused");
			return;

		case eSystemStatus::eStopping:
			setSystemStatus (eSystemStatus::eStopped);
			Logger::L().Log (MODULENAME, severity_level::debug1, "handlePauseStopForCurrentItem: work queue is stopped");

			AuditLogger::L().Log(generateAuditWriteData(
				worklistData->runUsername,
				audit_event_type::evt_worklist,
				"Stopped"));

			// If current work queue item is not yet processed and "Abort" was called
			// then mark the current item as skip manual and don't process it.
			for (auto& sampleIndices : worklistData->samplesToProcess.Get())
			{
				auto& sample = getSampleByIndices (sampleIndices);

				if (sample.status == eSampleStatus::eNotProcessed)
				{
					sample.status = eSampleStatus::eSkip_Manual;
					updateSampleStatus (sample);

					SampleDefinitionDLL::Log ("Skipping sample:", sample);

					auto text = boost::str (boost::format ("Cancelled Sample: %s\n\tCell Type: %s\n\tPosition: %s\n\tTag: %s")
						% sample.parameters.label
						% sample.parameters.celltype.label
						% sample.position.getAsStr()
						% sample.parameters.tag);
					AuditLogger::L().Log (generateAuditWriteData(
						sample.username,
						audit_event_type::evt_sampleresultcreated,
						text));
				}
			}

			if ( worklistData->carrier == eCarrierType::eCarousel || worklistData->carrier == eCarrierType::ePlate_96 )
			{
				int32_t angle = NoEjectAngle;
				bool holding = EJECT_HOLDING_CURRENT_OFF;
				if ( HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::ePlate_96 )
				{
					angle = DfltAutomationEjectAngle;
					holding = EJECT_HOLDING_CURRENT_ON;
				}

				// Stopping also handles stage holding current.  Pause handles this separately
				Hardware::Instance().getStageController()->EjectStage( [this, holding]( bool status ) -> void
					{
						if (!status)
						{
							Logger::L().Log (MODULENAME, severity_level::warning, "handlePauseStopForCurrentItem: <exit: error ejecting stage after stop>");
						}

						// TODO: this block may need to be modified for future use of external config variables
						if ( HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::ePlate_96 )
						{
							return;
						}

						// Disable stage holding current(if appropriate).  ONLY FOR THE STOP CONDITION!  Pause handles this separately
						Hardware::Instance().getStageController()->SetStageProfile( [ this ]( bool status ) -> void
							{
								if ( !status )
								{
									Logger::L().Log( MODULENAME, severity_level::warning, "handlePauseStopForCurrentItem: <exit: error setting stage work queue profile>" );
								}
							}, holding );

				   }, angle);
			}
			return;
	}

	Logger::L().Log (MODULENAME, severity_level::error, "handlePauseStopForCurrentItem: unreachable code, application will exit");

	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
//TODO: need to know what kind of image is being captured.
//      this is important for Hunter.
//*****************************************************************************
void WorklistDLL::onCameraCaptureTrigger (cv::Mat image)
{
	// keep for debug - Logger::L().Log (MODULENAME, severity_level::debug1, "onCameraCaptureTrigger: <enter>");

	std::lock_guard<std::mutex> guard(impl->worklistMutex);

	// Increment the imageCnt_ at the beginning. This will store the number of times "onCameraCaptureTrigger" is called.
	bool firstImage = (impl->imageCnt == 0);
	impl->imageCnt++;

	if (!impl->imageCollection)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Image collection should have been initialized already, but isn't. Initializing empty collection.");
		impl->imageCollection.reset (new ImageCollection_t());
		// We do not know the max image count, so we do not reserve space in advance.
	}

	if (image.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, "onCameraCaptureTrigger: <exit, error getting image");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_camera_noimage, 
			instrument_error::severity_level::warning));
		return;
	}

	//TODO: how to deal with collecting a set of Hunter images and then
	// passing them to the CellCounting code?
	auto ip = std::make_pair(image, FLImages_t{});
	impl->imageCollection->emplace_back (ip);
	
	if (firstImage)
	{
		//
		// start the image processor and ddd the first image
		// when the processing of this image is complete add the next image
		// when all images are done being collected, add the remaining images. 
		//
		Logger::L().Log(MODULENAME, severity_level::notification, "onCameraCaptureTrigger - ******************  Start image processing ****************");
		s_processedImageCount = 0;
		s_nextImageIndexToAdd = 1;
		s_expectedImageCount = getSampleByIndices(worklistData->samplesToProcess.GetCurrentSampleIndices()).maxImageCount.get();
		ImageAnalysisUtilities::startProcess();
		ImageAnalysisUtilities::addImage(ip);
	}

	if (impl->imageCnt >= static_cast<uint32_t>(s_expectedImageCount))
	{
		if (!impl->imageCollection || impl->imageCollection->empty())
		{
			Logger::L().Log(MODULENAME, severity_level::debug2, "onCameraCaptureTrigger: <exit, empty collection!");
			return;
		}
		impl->pHawkeyeServices->enqueueInternal ([this]()
			{
				Hardware::Instance ().getStageController ()->PauseMotorPolling();
				const int kNUM_THREADS = 4;
				Logger::L().Log(MODULENAME, severity_level::warning, "onCameraCaptureTrigger - ** all images collected ** img proc count " + std::to_string(kNUM_THREADS));
				int cnt = static_cast<int>((*impl->imageCollection).size());
				int lastIndex = (((s_nextImageIndexToAdd + kNUM_THREADS) < cnt - 1)) ? (s_nextImageIndexToAdd + kNUM_THREADS) : (cnt - 1);
				for (; s_nextImageIndexToAdd <= lastIndex; s_nextImageIndexToAdd++)
				{
					ImageAnalysisUtilities::addImage((*impl->imageCollection)[s_nextImageIndexToAdd]);
				}
			});
	}

	// keep for debug - Logger::L().Log (MODULENAME, severity_level::debug1, "onCameraCaptureTrigger: <exit>");
}

//*****************************************************************************
// Check the samples duplicate positions by inserting the appropriate field into
// an std::unordered_map which checks for duplicate keys.
//*****************************************************************************
HawkeyeError WorklistDLL::isSampleValid (bool& reset, SampleDefinitionDLL& sdDLL) {

	typedef std::pair<uint16_t, size_t> LocationMapKey_t;
	typedef std::unordered_map<uint16_t, size_t> LocaltionMap_t;
	std::pair<LocaltionMap_t::iterator, bool> positionRetStat;

	static LocaltionMap_t positionMap;
	static size_t index=0;

	if (reset)
	{
		reset = false;
		index = 0;
		positionMap.clear();
	}

	index++;

	// Verify that the sample position is appropriate for the selected carrier.
	if (worklistData->carrier == eCarrierType::eCarousel)
	{
		if (!sdDLL.position.isValidForCarousel()) {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("isSampleValid: <exit, sample at position %s is incompatible with a carousel.")
					% sdDLL.position.getAsStr ()));
			return HawkeyeError::eInvalidArgs;
		}
	}
	else if (worklistData->carrier == eCarrierType::ePlate_96)
	{
		if (!sdDLL.position.isValidForPlate96()) {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("isSampleValid: <exit, sample at position %s is incompatible with a plate.")
				% sdDLL.position.getAsStr()));
				return HawkeyeError::eInvalidArgs;
		}
	}
	else
	{ // ACup...
		if (!sdDLL.position.isValidForACup()) {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("isSampleValid: <exit, sample at position %s is incompatible with the ACup.")
					% sdDLL.position.getAsStr()));
			return HawkeyeError::eInvalidArgs;
		}
	}

	HawkeyeError he = validate_analysis_and_celltype (sdDLL);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("isSampleValid: <exit, this occurred at sample position %s...>") % sdDLL.position.getAsStr()));
		return he;
	}

	// Verify that the *position* field is unique by inserting the appropriate field into
	// an std::unordered_map which checks for duplicate keys.
	positionRetStat = positionMap.insert (LocationMapKey_t(sdDLL.position.getHash(), index));
	if (!positionRetStat.second) {
		Logger::L().Log (MODULENAME, severity_level::error, 
			boost::str (boost::format("isSampleValid: <exit, sample at position %s has a duplicate sample position.") 
				% sdDLL.position.getAsStr()));
		return HawkeyeError::eInvalidArgs;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// Build a Hawkeye Result object and send it to the UI.
//*****************************************************************************
void WorklistDLL::onSampleCellCountingComplete (SampleIndices curSampleIndices, int imageNum, CellCounterResult::SResult sResult)
{	
	bool sampleDone = false;
	
	// limit the scope of the mutex lock variable
	{
		std::lock_guard<std::mutex> guard(impl->worklistMutex);
		if (s_processedImageCount < s_expectedImageCount)
		{
			s_processedImageCount++;
			if (s_processedImageCount >= s_expectedImageCount)
			{
				// just got the last image
				Logger::L().Log(MODULENAME, severity_level::debug1, "onSampleCellCountingComplete: All images have been processed ...");
			}
		}
		sampleDone = (s_processedImageCount >= s_expectedImageCount);

		// NOTE: save for timing testing...
		// Logger::L().Log(MODULENAME, severity_level::debug1,
		//	"onSampleCellCountingComplete: <enter> ***** image processing done : " + std::to_string(imageNum) + " / " +
		//	std::to_string(s_expectedImageCount) + " next index: " + std::to_string(s_nextImageIndexToAdd) +
		//	" s_processedImageCount " + std::to_string(s_processedImageCount));

		if (s_nextImageIndexToAdd < s_expectedImageCount)
		{
			// keep for debug - Logger::L().Log(MODULENAME, severity_level::debug1, "onSampleCellCountingComplete: add pair");
			ImageAnalysisUtilities::addImage((*impl->imageCollection)[s_nextImageIndexToAdd]);
			s_nextImageIndexToAdd++;
			if (s_nextImageIndexToAdd >= s_expectedImageCount)
			{
				// just started processing the last image
				Logger::L().Log(MODULENAME, severity_level::debug1, "onSampleCellCountingComplete: Last image is now being processed...");
			}
		}

		if (!sampleDone) 
		{
			// Almost done processing - skip to the last image
			if ((imageNum + 12) >= s_expectedImageCount)
				return;
		}

	} // end of lock variable scope => lock is released

	// Adjust for different frame of reference from CellCounting.
	imageNum -= 1;

	// imageNum is now being used as an index, be sure it's within range
	if ((imageNum < 0) || (static_cast<uint32_t>(imageNum) >= impl->imageCnt))
		return;

	// Show the first and last image and many in-between 
	if ((imageNum < 12) || sampleDone || ((imageNum % 4) == 0))
	{
		// image callback code 

		ImageSetWrapperDLL imagesetWrapperDLL = {};

		//NOTE: FL images are not supported at this time.
		//for (size_t index = 0; index < (*impl->imageCollection)[imageNum].second.size(); index++)
		//{
		//	FL_ImageWrapperDLL flWrapper = {};
		//	flWrapper.fl_channel = static_cast<uint16_t>(index);
		//	flWrapper.fl_image.image = (*impl->imageCollection)[imageNum].second[(int)index];
		//	imagesetWrapperDLL.fl_images.push_back(flWrapper);
		//}

		if (!ImageWrapperUtilities::TransformImage(
			impl->imageOutputType,
			(*impl->imageCollection)[imageNum].first,
			imagesetWrapperDLL.brightfield_image.image,
			127,
			sResult,
			imageNum + 1))
		{
			imagesetWrapperDLL.brightfield_image.image = (*impl->imageCollection)[imageNum].first.clone();
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("onSampleCellCountingComplete: image transformation failed, image %d")
					% imageNum));
		}

		const auto sampleDef = getSampleByIndices(curSampleIndices);
		ImageAnalysisUtilities::ApplyConcentrationAdjustmentFactor (sampleDef.parameters.celltype, sResult);

		BasicResultAnswers cumResults = {};
		ImageAnalysisUtilities::getBasicResultAnswers(sResult.Cumulative_results, cumResults);

		//NOTE: Since *imageNum* as passed in is from 1 to N, we subtract one from *imageNum* to access *imageCollection_* (which is a vector).
		//     	Then as *ccResult.map_Image_Results* is a std::map, we add one back to *imageNum* to be able to correctly access the std:map.
		imageNum += 1;

		// keep for debugging...
		//Logger::L().Log(MODULENAME, severity_level::debug1,
		//	boost::str(boost::format("onSampleCellCountingComplete:: nTotalCumulative_Imgs: %d, s_expectedImages: %d")
		//		% cumResults.nTotalCumulative_Imgs
		//		% s_expectedImages
		//	));

		BasicResultAnswers imageResults;
		ImageAnalysisUtilities::getBasicResultAnswers(sResult.map_Image_Results[imageNum], imageResults);
		
		HAWKEYE_ASSERT(MODULENAME, onSampleImageProcessed_);
		impl->pHawkeyeServices->enqueueInternal([=]()
			{
				// keep for debugging...
				//Logger::L().Log(MODULENAME, severity_level::debug1,
				//	boost::str(boost::format("onSampleCellCountingComplete:: ssIndex: %d, index: %d")
				//		% curSampleIndices.sampleSetIndex
				//		% curSampleIndices.sampleIndex
				//	));
				try {
					onSampleImageProcessed_(sampleDef, (uint16_t)imageNum, imagesetWrapperDLL, cumResults, imageResults);
				}
				catch (...) {}
			});
	}

	if (!sampleDone)
	{
		//NOTE: save for timing testing...
		// keep for debug - Logger::L().Log(MODULENAME, severity_level::debug1, "onSampleCellCountingComplete: <exit>");
		return;
	}
	Logger::L().Log(MODULENAME, severity_level::debug1, "onSampleCellCountingComplete: <exit, Sample image processing done!>");
}

//*****************************************************************************
void WorklistDLL::startWorklistInternal (std::function<void(HawkeyeError)> onComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	eCarrierType carrierType = worklistData->carrier;

	auto quitStartWorklist = [ this, onComplete, carrierType ]( HawkeyeError he )
	{
		// Mark the Worklist as not set so that the "Set" API will work correctly.
		resetWorklist();

		if ( carrierType == eCarrierType::eCarousel || carrierType == eCarrierType::ePlate_96 )
		{
			int32_t angle = NoEjectAngle;
			bool holding = EJECT_HOLDING_CURRENT_OFF;
			if ( HawkeyeConfig::Instance().get().automationEnabled && carrierType == eCarrierType::ePlate_96 )
			{
				angle = DfltAutomationEjectAngle;
				holding = EJECT_HOLDING_CURRENT_ON;
			}

			std::string carrierTypeStr = "";
			switch ( carrierType )
			{
				case eCarrierType::eCarousel:
				{
					carrierTypeStr = "Carousel";
					break;
				}
				case eCarrierType::ePlate_96:
				{
					carrierTypeStr = "Plate";
					break;
				}
				case eCarrierType::eACup:
				{
					carrierTypeStr = "A-cup";
					break;
				}
			}

			auto stageComplete = [ this, onComplete, carrierTypeStr, he ]( bool disableOk )
			{
				if ( !disableOk )
				{
					Logger::L().Log( MODULENAME, severity_level::warning, "startWorklistInternal: <exit: error setting " + carrierTypeStr + " work queue stage profile>" );
				}
				impl->pHawkeyeServices->enqueueExternal( onComplete, he );
			};

			auto ejectComplete = [ this, stageComplete, carrierTypeStr, holding ]( bool ejectStatus )
			{
				if ( !ejectStatus )
				{
					Logger::L().Log( MODULENAME, severity_level::warning, "startWorklistInternal: <exit: error ejecting stage for " + carrierTypeStr + " >" );
				}

				Hardware::Instance().getStageController()->SetStageProfile( stageComplete, holding );
			};

			Hardware::Instance().getStageController()->EjectStage( ejectComplete, angle );
			return;
		}

		impl->pHawkeyeServices->enqueueExternal( onComplete, he );
	};

	auto continueStartWorklist = [this, onComplete, quitStartWorklist]()
	{
		std::lock_guard<std::mutex> guard(impl->worklistMutex);

		ImageAnalysisUtilities::getDustReferenceImage();

		worklistData->timestamp = ChronoUtilities::CurrentTime();

		impl->isWorklistRunning = true;
		impl->sampleToSampleTiming = boost::none;

		// on start, set the initial intended list processing order
		if (worklistData->samplesToProcess.Count() > 0)		// if no defined samples in the list, then all samples will be orphans
		{
			worklistData->samplesToProcess.SetCurrentSampleIndex(0);
			worklistData->samplesToProcess.ResetProcessingListOrder();
		}

		HawkeyeError he = HawkeyeError::eSuccess;

		if (writeWorklistToDB(DBApi::eWorklistStatus::WorklistRunning))
		{
			impl->pHawkeyeServices->enqueueInternal([this]()
				{
					// Enable the writing of any dust reference data.
					// Dust reference images are only written once per Worklist.
					HawkeyeResultsDataManager::Instance().ResetSaveDustReference();
					queueSampleForProcessing();
				});
			impl->pHawkeyeServices->enqueueExternal(onComplete, he);
		}
		else
		{
			// Error is logged and reported by "updateWorklistStatusToDB".
			he = HawkeyeError::eStorageFault;
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror, 
				instrument_error::instrument_storage_instance::none, 
				instrument_error::severity_level::error));
			Stop( false );	
			quitStartWorklist(he);
		}
	};

	switch (carrierType)
	{
		case eCarrierType::eCarousel:
		{
			if (!Hardware::Instance().getStageController()->IsCarouselPresent())
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "startWorklistInternal: <exit, no Carousel detected on carrier>");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_precondition_notmet, 
					instrument_error::instrument_precondition_instance::carousel_present, 
					instrument_error::severity_level::warning));
				
				quitStartWorklist( HawkeyeError::eNotPermittedAtThisTime );
				return;
			}

			if (SystemStatus::Instance().getData().sample_tube_disposal_remaining_capacity == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "startWorklistInternal: <exit,  sample discard tray is full>");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_precondition_notmet, 
					instrument_error::instrument_precondition_instance::wastetubetray_capacity,
					instrument_error::severity_level::warning));

				quitStartWorklist( HawkeyeError::eSpentTubeTray );
				return;
			}

			Hardware::Instance().getStageController()->SetStageProfile([this, onComplete, continueStartWorklist](bool status) -> void
				{
					if (!status)
					{
						Logger::L().Log( MODULENAME, severity_level::warning, "startWorklistInternal: <exit: error setting Carousel work queue stage profile>" );

						// Mark the Worklist as not set so that the "Set" API will work correctly.
						resetWorklist();
						impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eHardwareFault);
						return;
					}
					continueStartWorklist();
				}, true);

			return;
		}

		case eCarrierType::ePlate_96:
		{
			// Make sure that there is something to do in the Worklist (not necessary for carousel).
			// No point in trying to process an empty plate.
			if (worklistData->samplesToProcess.Get().empty())
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "startWorklistInternal: <exit, Worklist is empty>");

				quitStartWorklist( HawkeyeError::eInvalidArgs );
				return;
			}

			Hardware::Instance().getStageController()->IsPlatePresent([this, onComplete, continueStartWorklist, quitStartWorklist](bool status)
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::warning, "No Plate detected on carrier.");
						ReportSystemError::Instance().ReportError (BuildErrorInstance(
							instrument_error::instrument_precondition_notmet, 
							instrument_error::instrument_precondition_instance::plate_present,
							instrument_error::severity_level::warning));

						quitStartWorklist( HawkeyeError::ePlateNotFound );
						return;
					}

					Hardware::Instance().getStageController()->SetStageProfile([this, onComplete, continueStartWorklist](bool status) -> void
						{
							if (!status)
							{
								Logger::L().Log (MODULENAME, severity_level::warning, "startWorklistInternal: <exit: error setting Plate work queue stage profile>");
								impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eHardwareFault);
								return;
							}
							continueStartWorklist();
						}, true);
				});

			return;
		}
		
		case eCarrierType::eACup:
		{
			// explicitly disable holding current when using the A-Cup...
			Hardware::Instance().getStageController()->SetStageProfile( [ this, onComplete, continueStartWorklist ]( bool status ) -> void
				{
					if ( !status )
					{
						Logger::L().Log( MODULENAME, severity_level::warning, "startWorklistInternal: <exit: error setting A-cup work queue stage profile>" );
						impl->pHawkeyeServices->enqueueExternal( onComplete, HawkeyeError::eHardwareFault );
						return;
					}
					continueStartWorklist();
				}, false );

			return;
		}

		default:
			Logger::L().Log (MODULENAME, severity_level::error, "startWorklistInternal : invalid carrier type : " 
				+ std::to_string(static_cast<uint16_t>(worklistData->carrier)));
			impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
			return;
	}

	Logger::L().Log (MODULENAME, severity_level::error, "startWorklistInternal: unreachable code, application will exit");

	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void WorklistDLL::addSampleSetInternal (std::shared_ptr<SampleSetDLL> sampleSet, std::function<void(HawkeyeError)> onComplete, bool isOrphanSampleSet )
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "addSampleSetInternal: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

//TODO: is this needed???
		//// Check for the disk space availability required to process these samples and store the result.
		//if (!isDiskSpaceAvailableForAnalysis(workQueue_.workQueueItems))
		//{
			//ReportSystemError::Instance().ReportError (BuildErrorInstance(
			//	instrument_error::instrument_storage_storagenearcapacity, 
			//	instrument_error::instrument_storage_instance::none, 
			//	instrument_error::severity_level::warning));
		//	Logger::L().Log (MODULENAME, severity_level::error, "Set Work Queue <exit,  Low Disk Space>");
		//	impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		//	return;
		//}

//TODO: From Dennis...
//	Total storage size is pretty closely tied to the size of images.
//	I don't know if the backend calculates storage with respect to data separately from the size of images, 
//	but if it does, then for now, I think we should assume that DB data storage requirements are about 50% bigger.  
//	The size component for images will stay the same. If the calculations for the size of images allows, 
//	be sure it uses disk sector size (should be a minimum of 4096 for NTFS volumes) to calculate the size
//	required to store the image files, since the size on disk will usually be more that the image size.


//TODO: test code to write every SampleSet as a Template.
// Save for testing...
//// To create a collection of templates in the DB for UI testing.
//	{
//		sampleSet->worklistUuid = {};
//		sampleSet->userUuid = worklistData->userUuid;
//
//		DBApi::DB_SampleSetRecord dbSS = sampleSet->ToDbStyle();
//
//		DBApi::eQueryResult dbStatus = DBApi::DbSaveSampleSetTemplate (dbSS);
//		if (dbStatus != DBApi::eQueryResult::QueryOk)
//		{
//			Logger::L().Log (MODULENAME, severity_level::error,
//				boost::str (boost::format ("writeWorklistToDB: <exit, DB Worklist write failed, status: %ld>") % (int32_t)dbStatus));
//		}
//	}


	sampleSet->uuid = {};
	sampleSet->worklistUuid = worklistData->uuid;
	UserList::Instance().GetUserUUIDByName(sampleSet->username, sampleSet->userUuid);
	sampleSet->samplesetIdNum = 0;

	if (sampleSet->samples.size() || isOrphanSampleSet)
	{
#ifdef _DEBUG
		if (sampleSet->name == "Concentration slope")
		{
			// For debugging reduce the number of samples to run.
			// Only reduce the # of sample for the Standard Conc Slope.  ACup is okay.
			if (sampleSet->carrier != eCarrierType::eACup)
			{
				// Remove all sample except for 1, 11 and 16.
				auto iter = sampleSet->samples.begin();
				auto sIter = iter;
				auto eIter = sampleSet->samples.end();
				std::vector<SampleDefinitionDLL>::iterator theEnd;

				Logger::L().Log (MODULENAME, severity_level::debug1, "removing samples for testing concentration slope");

				for (const auto& v : sampleSet->samples)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1,
						boost::str (boost::format ("original:: ssIndex: %d, index: %d")
							% v.sampleSetIndex
							% v.index
						));
				}

				sampleSet->samples.erase (sIter + 17);
				sampleSet->samples.erase (sIter + 16);

				for (const auto& v : sampleSet->samples)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1,
						boost::str (boost::format ("original:: ssIndex: %d, index: %d")
							% v.sampleSetIndex
							% v.index
						));
				}

				sampleSet->samples.erase (sIter + 14);
				sampleSet->samples.erase (sIter + 13);
				sampleSet->samples.erase (sIter + 12);
				sampleSet->samples.erase (sIter + 11);

				for (const auto& v : sampleSet->samples)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1,
						boost::str (boost::format ("original:: ssIndex: %d, index: %d")
							% v.sampleSetIndex
							% v.index
						));
				}

				sampleSet->samples.erase (sIter + 9);
				sampleSet->samples.erase (sIter + 8);
				sampleSet->samples.erase (sIter + 7);
				sampleSet->samples.erase (sIter + 6);
				sampleSet->samples.erase (sIter + 5);
				sampleSet->samples.erase (sIter + 4);
				sampleSet->samples.erase (sIter + 3);
				sampleSet->samples.erase (sIter + 2);
				sampleSet->samples.erase (sIter + 1);

				for (const auto& v : sampleSet->samples)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1,
						boost::str (boost::format ("original:: ssIndex: %d, index: %d")
							% v.sampleSetIndex
							% v.index
						));
				}
			}
		}
#endif

		worklistData->sampleSetsDLL.push_back (*sampleSet);

		logSamplesDLL ("AddSampleSet: new samples", sampleSet->samples);

		// Verify that the SampleSets have the same carrier as the Worklist.
		if (worklistData->carrier != sampleSet->carrier)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "addSampleSetInternal: <exit, SampleSet carrier does not match Worklist carrier>");
			impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
		}

		// Validate the Analysis and Celltype of the samples.
		bool reset = true;
		for (auto& sample : worklistData->sampleSetsDLL.back().samples)
		{
			auto he = isSampleValid (reset, sample);
			if (he != HawkeyeError::eSuccess)
			{	// No error/log message needed, function does it.
				impl->pHawkeyeServices->enqueueExternal (onComplete, he);
				return;
			}
		}

		logSamplesDLL ("AddSampleSet (before adding new SampleSet)", worklistData->samplesToProcess.Get());

		auto samplesIndices = getIndicesVectorFromSamples (sampleSet->samples);
		worklistData->samplesToProcess.Insert (worklistData->carrier, samplesIndices);

		logSamplesDLL ("AddSampleSet (after adding and sorting new SampleSet)", worklistData->samplesToProcess.Get());

		markCarouselSlotsInUseForSimulation (samplesIndices);
	}

	// Write SampleSet to Worklist in DB, if it hasn't already occurred in "writeWorklisttoDB".
	if (Uuid::IsValid (worklistData->uuid))
	{
		sampleSet->worklistUuid = worklistData->uuid;
		addWorklistSampleSet (*sampleSet);
	}

	eSystemStatus currentStatus = SystemStatus::Instance().getData().status;
	if (currentStatus != eSystemStatus::ePaused)
		autoResume_ = true;

	Logger::L().Log (MODULENAME, severity_level::debug1, "addSampleSetInternal: <exit>");

	impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eSuccess);
}

//*****************************************************************************
void WorklistDLL::cancelSampleSetInternal (uint16_t sampleSetIndex, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "cancelSampleSetInternal: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	std::lock_guard<std::mutex> guard (impl->worklistMutex);

	// Only look at the samples that have not been processed.
	for (auto& v : worklistData->samplesToProcess.Get())
	{
		auto& sample = getSampleByIndices (v);
		if (v.sampleSetIndex == sampleSetIndex && sample.status == eNotProcessed)
		{
			sample.status = eSampleStatus::eSkip_NotProcessed;
			updateSampleItemStatusToDB (sample);
		}
	}

	// If sample set is not running, change it's status to complete
	if (!isSampleSetRunning(getSampleSetByIndex(sampleSetIndex)))
		updateSampleSetStatusToDB (getSampleSetByIndex(sampleSetIndex), DBApi::eSampleSetStatus::SampleSetComplete);

	logSamplesDLL ("cancelSampleSetInternal: remaining samplesToProcess", worklistData->samplesToProcess.Get());

	// Plate and Acup samplesets should stop on CancelSet,
	// Carousel should pause
	if ((worklistData->carrier == eCarrierType::eCarousel))
	{
		autoResume_ = false;
		Pause();
	}
	else
		Stop(false);

	AuditLogger::L().Log(generateAuditWriteData(
		worklistData->runUsername,
		audit_event_type::evt_worklist,
		"Sample Set Cancelled"));
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "cancelSampleSetInternal: <exit>");
	
	impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eSuccess);
}

//*****************************************************************************
HawkeyeError WorklistDLL::validate_analysis_and_celltype (SampleDefinitionDLL& sdDLL) {

	// Check that the Analysis and CellType are valid.
	uint32_t cellTypeIndex = sdDLL.parameters.celltype.celltype_index;
	if (!CellTypesDLL::getCellTypeByIndex (cellTypeIndex, sdDLL.parameters.celltype)) {

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("validate_analysis_and_celltype: <exit, Celltype index %d (0x%08X) is invalid for default sample parameters>")
				% cellTypeIndex
				% cellTypeIndex));

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::sample_celltype_invalidtype, 
			instrument_error::severity_level::warning));
		return HawkeyeError::eInvalidArgs;
	}

	uint16_t analysisIndex = sdDLL.parameters.analysis.analysis_index;
	if (!AnalysisDefinitionsDLL::GetAnalysisByIndex (analysisIndex, sdDLL.parameters.celltype, sdDLL.parameters.analysis)) {
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("validate_analysis_and_celltype: <exit, Analysis index %d (0x%04X) is invalid for default sample parameters>")
				% analysisIndex
				% analysisIndex));

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::sample_analysis_invalidtype, 
			instrument_error::severity_level::warning));
		return HawkeyeError::eInvalidArgs;
	}


	bool foundIt = false;

	// If *user* is bci_service or an admin or security is *off*, then allow all available analysis and celltypes..
	if (!UserList::Instance().IsUserPermissionAtLeast(sdDLL.username, UserPermissionLevel::eAdministrator))
	{
		// Get the analysis indices available for the logged-in user.
		std::vector<uint32_t> userAnalysisIndices = {};

		HawkeyeError he = UserList::Instance().GetUserAnalysisIndices(sdDLL.username, userAnalysisIndices);
		if ((he != HawkeyeError::eSuccess) || (userAnalysisIndices.size() < 1))
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str(boost::format("validate_analysis_and_celltype: <exit, no analysis found for user (%s)>") % sdDLL.username));
			return HawkeyeError::eInvalidArgs;
		}

		sdDLL.parameters.analysis.analysis_index = static_cast<uint16_t>(userAnalysisIndices.front());	

	}


	// Verify that the analysis indices in the sample are available in the system.
	std::vector<AnalysisDefinitionDLL>& analyses = AnalysisDefinitionsDLL::Get ();
	for (auto analysis : analyses) {
		if (sdDLL.parameters.analysis.analysis_index == analysis.analysis_index) {

			// Verify any illuminators found in this analysis definition are supported by the existing LED configuration.
//TODO: this code to verify the illuminators has not been tested...
//TODO: does the fl_illuminators field ONLY apply to the fluoresecent LEDs (i.e. not the Brightfield)?
			for (size_t m = 0; m < analysis.fl_illuminators.size (); m++) {
				foundIt = false;

				const std::map<HawkeyeConfig::LedType, std::shared_ptr<LedBase>>& ledObjectsMap = Hardware::Instance().getLedObjectsMap();
				for (auto& p : ledObjectsMap) {
					if ((p.second->getConfig ()->illuminatorWavelength == analysis.fl_illuminators[m].illuminator_wavelength_nm) &&
						(p.second->getConfig ()->emissionWavelength == analysis.fl_illuminators[m].emission_wavelength_nm)) {
						foundIt = true;
						break;
					}
				}
				if (!foundIt) {
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("validate_analysis_and_celltype: <exit, sample has an analysis index (0x%04X) with a wavelength set (Illuminator: %d  Emission: %d) or that is not supported>")
							% sdDLL.parameters.analysis.analysis_index
							% sdDLL.parameters.analysis.fl_illuminators[m].illuminator_wavelength_nm
							% sdDLL.parameters.analysis.fl_illuminators[m].emission_wavelength_nm));
					return HawkeyeError::eInvalidArgs;
				}
			}
		}

	} // End "for (auto analysis : analyses)"

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void WorklistDLL::setWorklistInternal (eWorklistDLLState currentState, std::function<void(HawkeyeError)> callback, HawkeyeError errorStatus)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [this, callback](eWorklistDLLState nextState, HawkeyeError he)
	{
		impl->pHawkeyeServices->enqueueInternal (std::bind (&WorklistDLL::setWorklistInternal, this, nextState, callback, he));
	};

	Logger::L().Log (MODULENAME, severity_level::debug1, "setWorklistInternal: " + EnumConversion<eWorklistDLLState>::enumToString(currentState));

	std::lock_guard<std::mutex> guard (impl->worklistMutex);

	switch (currentState)
	{
		case eWorklistDLLState::eEntryPoint:
		{
			// Check Whether the Pack is installed, Arm is down, tags are valid.
			if (!Hardware::Instance().getReagentController()->IsPackInstalled())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <exit, No Reagent pack installed>");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::reagent_pack_nopack, 
					instrument_error::reagent_pack_instance::general, 
					instrument_error::severity_level::warning));
				onCurrentStateComplete(eWorklistDLLState::eError, HawkeyeError::eNotPermittedAtThisTime);
				return;
			}

			if (!ReagentPack::Instance().isPackUsable())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <exit, reagent pack is unusable>");
				onCurrentStateComplete (eWorklistDLLState::eError, HawkeyeError::eNotPermittedAtThisTime);
				return;
			}

			onCurrentStateComplete (eWorklistDLLState::eValidateWorklist, HawkeyeError::eSuccess);
			return;
		}

		case eWorklistDLLState::eValidateWorklist:
		{
			if (worklistData->sampleSetsDLL.size() > 0)
			{
				Log (worklistData->sampleSetsDLL);

				// Validate the carrier of the Worklist.
				switch (worklistData->carrier)
				{
					case eCarrierType::eUnknown:
					default:
						Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <exit, invalid carrier type>");
						impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
						return;

					case eCarrierType::eCarousel:
					{
						// Orphan processing does not have a sample definition at this point.
						// Create a fake sample definition to pass the data to "validate_analysis_and_celltype"
						// to be able to validate the orphan settings.
						SampleDefinitionDLL sdDLL;
						sdDLL.username = worklistData->runUsername;
						sdDLL.parameters = worklistData->orphanParameterSettings;

						auto he = validate_analysis_and_celltype (sdDLL);
						if (he != HawkeyeError::eSuccess)
						{
							Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <exit, this occurred validating the orphan sample settings");
							impl->pHawkeyeServices->enqueueExternal (callback, he);
							return;
						}
						break;
					}

					case eCarrierType::ePlate_96:
					case eCarrierType::eACup:
						// Nothing to do since "OrphanSample" does not apply to plate or ACup.
						break;

				} // End "switch (carrier)"

				// Sort the samples appropriately for the carrier.
				if (worklistData->carrier == eCarrierType::eCarousel)
				{
					// Sort the sample indices to process by column order.
					worklistData->samplesToProcess.CarouselSort();
				}
				else if ( worklistData->carrier == eCarrierType::ePlate_96 )
				{
					// Sort the samples by row or column order.
					worklistData->samplesToProcess.PlateSort (worklistData->precession);
					// Sort the sample sets for plate
					for (auto& v : worklistData->sampleSetsDLL)
					{
						if (!v.samples.empty())
							v.PlateSort(worklistData->precession);
					}
				}

				logSamplesDLL ("setWorklistInternal: samplesToProcess", worklistData->samplesToProcess.Get());

			} // End "if (sampleSetsDLL->size() > 0)"

			if (HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::eACup)
			{
				onCurrentStateComplete (eWorklistDLLState::eComplete, HawkeyeError::eSuccess);
			}
			else
			{
				onCurrentStateComplete (eWorklistDLLState::eInitializeStage, HawkeyeError::eSuccess);
			}
			return;
		}

		case eWorklistDLLState::eInitializeStage:
		{
			// Initialize the specified carrier.
			Hardware::Instance().getStageController()->SelectStage ([this, onCurrentStateComplete](bool status)
			{
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <exit, unable to select the carrier for the Worklist>");
					onCurrentStateComplete (eWorklistDLLState::eError, HawkeyeError::eHardwareFault);
					return;
				}
				onCurrentStateComplete (eWorklistDLLState::eProbeUp, HawkeyeError::eSuccess);
			}, worklistData->carrier);
			return;
		}

		case eWorklistDLLState::eProbeUp:
		{
			// Move probe up
			Hardware::Instance().getStageController()->ProbeUp ([this, onCurrentStateComplete](bool status)
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <exit, Failed to move probe up!>");
						onCurrentStateComplete (eWorklistDLLState::eError, HawkeyeError::eHardwareFault);
						return;
					}
					onCurrentStateComplete (eWorklistDLLState::eComplete, HawkeyeError::eSuccess);
				});
			return;
		}

		case eWorklistDLLState::eComplete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "setWorklistInternal: <exit>");
			if (errorStatus != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::critical, "setWorklistInternal: eComplete : with incorrect error status: " 
					+ std::string(HawkeyeErrorAsString(errorStatus)));
				onCurrentStateComplete (eWorklistDLLState::eError, HawkeyeError::eSoftwareFault);
				return;
			}

			impl->isWorklistSet = true;
			worklistData->status = static_cast<int32_t>(DBApi::eWorklistStatus::WorklistRunning);
			worklistData->acquireSample = true;

			impl->pHawkeyeServices->enqueueExternal (callback, errorStatus);
			return;
		}

		case eWorklistDLLState::eError:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setWorklistInternal: <failed to set Worklist> : " + std::string(HawkeyeErrorAsString(errorStatus)));
			if (errorStatus == HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::critical, "setWorklistInternal: <error, with incorrect error status: " 
					+ std::string(HawkeyeErrorAsString(errorStatus)) + ">");
				HAWKEYE_ASSERT (MODULENAME, false);
			}

			// Mark the Worklist as not set so that the "Set" API will work correctly.
			resetWorklist();

			impl->pHawkeyeServices->enqueueExternal (callback, errorStatus);
			return;
		}

	} // End "switch (currentState)"

	Logger::L().Log (MODULENAME, severity_level::error, std::string("setWorklistInternal: <error, unreachable code, asserting"));

	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void WorklistDLL::setNextSampleSet (Worklist worklist, uint16_t index, std::function<void(HawkeyeError)> callback) {

	impl->pHawkeyeServices->enqueueInternal (std::bind (&WorklistDLL::setSampleSetAsync, this, worklist, index, callback));
}

//*****************************************************************************
void WorklistDLL::setSampleSetAsync (Worklist worklist, uint16_t ssIndex, std::function<void (HawkeyeError)> callback)
{
	if (ssIndex == worklist.numSampleSets)
	{
		impl->pHawkeyeServices->enqueueInternal ([this, callback]() -> void
		{
			setWorklistInternal (eWorklistDLLState::eEntryPoint, callback);
		});
		return;
	}

	bool isOrphanSet = false;

	SampleSet sampleSet = worklist.sampleSets[ssIndex];

	// Since plates do not support orphan samples, do not add the orphan to the Worklist.
	if (sampleSet.index == 0)
	{
		if (sampleSet.carrier == eCarrierType::ePlate_96 ||
			sampleSet.carrier == eCarrierType::eACup)
		{ // Skip over the orphan SampleSet and add the next SampleSet.
			uint16_t index = ssIndex + 1;
			SampleSet sampleSet = worklist.sampleSets[index];
			setNextSampleSet (worklist, index, callback);
			return;
		}

		isOrphanSet = true;
	}

	std::shared_ptr<SampleSetDLL> sampleSetDLL = std::make_shared<SampleSetDLL>(sampleSet);

	addSampleSetInternal (sampleSetDLL, [this, worklist, ssIndex, callback](HawkeyeError he)
	{
		if (he != HawkeyeError::eSuccess)
		{
			impl->pHawkeyeServices->enqueueExternal (callback, he);
			return;
		}

		uint16_t index = ssIndex + 1;
		SampleSet sampleSet = worklist.sampleSets[index];
		setNextSampleSet (worklist, index, callback);
	}, isOrphanSet);
}

//*****************************************************************************
void WorklistDLL::Set (const Worklist& worklist, std::function<void (HawkeyeError)> callback)
{
	impl->isAborted = false;

	if (SystemStatus::Instance().getData().status == eSystemStatus::eFaulted)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Set: <exit, system is in Faulted state>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (impl->isWorklistSet)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Set: <exit, Worklist is already set>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eAlreadyExists);
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, 
		boost::str (boost::format ("Set: <impl->isWorklistRunning: %s>") % (impl->isWorklistRunning ? "true" : "false")));
	Logger::L().Log (MODULENAME, severity_level::debug2,
		boost::str (boost::format ("Set: <WorkflowController::Instance().isWorkflowBusy(): %s>") % (WorkflowController::Instance().isWorkflowBusy() ? "true" : "false")));

	if (impl->isWorklistRunning || WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Set: <exit, Worklist is already running>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eBusy);
		return;
	}

	if (!SystemStatus::Instance().getData().focus_IsFocused)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Set: <exit, not focused>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::focus, 
			instrument_error::severity_level::warning));
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!Hardware::Instance().getStageController()->IsStageCalibrated (worklist.carrier))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Set: <exit, stage is not registered>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eStageNotRegistered);
		return;
	}

	if (!ReagentPack::Instance().isPackUsable())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Set: <exit, reagent pack is unusable>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eReagentError);
		return;
	}

	std::lock_guard<std::mutex> guard (impl->worklistMutex);

	worklistData.reset();
	worklistData = std::make_shared<WorklistData>(worklist);

	resetWorklist();

	worklistData->username = worklist.username;
	worklistData->runUsername = worklist.runUsername;
	if (HawkeyeError::eSuccess != UserList::Instance().GetUserUUIDByName(worklistData->username, worklistData->userUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Set: <exit, no UUID found for " + worklistData->username + ">");
		impl->pHawkeyeServices->enqueueExternal(callback, HawkeyeError::eStorageFault);
		return;
	}

	if (HawkeyeError::eSuccess != UserList::Instance().GetUserUUIDByName(worklistData->runUsername, worklistData->runUserUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Set: <exit, no UUID found for " + worklistData->runUsername + ">");
		impl->pHawkeyeServices->enqueueExternal(callback, HawkeyeError::eStorageFault);
		return;
	}

	if (worklist.numSampleSets > 0)
	{
		// The caller (UI) is not setting the SampleSet carrier correctly when a plate is used.
		for (uint16_t idx = 0; idx < worklist.numSampleSets; idx++)
		{
			worklist.sampleSets[idx].carrier = worklist.carrier;
		}

		worklistData->sampleSetsDLL.reserve (worklist.numSampleSets);

		uint16_t ssIndex = 0;

		setNextSampleSet (worklist, ssIndex, callback);
	}
}

//*****************************************************************************
HawkeyeError WorklistDLL::GetCurrentSample (SampleDefinition*& sd)
{
	std::lock_guard<std::mutex> guard (impl->worklistMutex);

	if (SystemStatus::Instance().getData().status == eSystemStatus::eProcessingSample)
	{
		DataConversion::convert_dllType_to_cType (getSampleByIndices (worklistData->samplesToProcess.GetCurrentSampleIndices()), sd);
		return HawkeyeError::eSuccess;
	}
	else
	{
		sd = nullptr;
		return HawkeyeError::eNotPermittedAtThisTime;
	}
}

//*****************************************************************************
void WorklistDLL::AddSampleSet (const SampleSet& sampleSet, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "AddSampleSet: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	if (SystemStatus::Instance().getData().status == eSystemStatus::eFaulted)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "AddSampleSet: <exit, system is not in OK state>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!impl->isWorklistSet)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "AddSampleSet: <exit, Worklist is NOT set>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eAlreadyExists);
		return;
	}

	if (impl->isWorklistRunning)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "AddSampleSet: <exit, Worklist is running>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eBusy);
		return;
	}

	// Empty SampleSet is not allowed.
	if (sampleSet.numSamples == 0)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "AddSampleSet: <exit, empty SampleSet not allowed>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	std::lock_guard<std::mutex> guard (impl->worklistMutex);

	// Verify that the SampleSets have the same carrier as the Worklist.
	if (sampleSet.carrier != worklistData->carrier)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "AddSampleSet: <exit, SampleSet carrier does not match Worklist carrier>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
		return;
	}

	std::shared_ptr<SampleSetDLL> sampleSetDLL = std::make_shared<SampleSetDLL>(sampleSet);

	sampleSetDLL->uuid = {};

	if (!sufficientReagentForSet(ReagentContainerPosition::eMainBay_1, sampleSet.numSamples))
	{
		// @todo - handle warning for not enough reagents when adding a new set to a running system
		Logger::L().Log(MODULENAME, severity_level::error, "AddSampleSet: <exit, insufficient reagents for new sample set>");
	}


	impl->pHawkeyeServices->enqueueInternal ([this, sampleSetDLL, onComplete]()
		{
			addSampleSetInternal (sampleSetDLL, onComplete);
		});
}

//*****************************************************************************
void WorklistDLL::CancelSampleSet (uint16_t sampleSetIndex, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelSampleSet: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	if (SystemStatus::Instance().getData().status == eSystemStatus::eFaulted)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "CancelSampleSet: <exit, system is not in OK state>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (!impl->isWorklistSet)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "CancelSampleSet: <exit, Worklist is NOT set>");
		impl->pHawkeyeServices->enqueueExternal (onComplete, HawkeyeError::eAlreadyExists);
		return;
	}

	impl->pHawkeyeServices->enqueueInternal ([this, sampleSetIndex, onComplete]()
		{
			cancelSampleSetInternal (sampleSetIndex, onComplete);
		});
}

//*****************************************************************************
//NOTE: not currently used by UI...
//*****************************************************************************
void WorklistDLL::SetImageOutputTypePreference (eImageOutputType image_type)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "SetImageOutputTypePreference: <enter>");
	impl->imageOutputType = image_type;
}

//*****************************************************************************
void WorklistDLL::Start (sample_status_callback_DLL onSampleStatus,
                         sample_image_result_callback_DLL onSampleImageProcessed,
                         sample_status_callback_DLL onSampleComplete,
                         worklist_completion_callback_DLL onWorklistComplete,
                         std::function<void(HawkeyeError)> onCompleteCallback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Start: <enter>");

	// Do not start the Worklist if it is already running.
	if (impl->isWorklistRunning || WorkflowController::Instance().isWorkflowBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Start: <exit, Worklist/Workflow Operation is already running>");
		impl->pHawkeyeServices->enqueueExternal (onCompleteCallback, HawkeyeError::eBusy);
		return;
	}

	onSampleStatus_ = onSampleStatus;
	onSampleImageProcessed_ = onSampleImageProcessed;
	onSampleComplete_ = onSampleComplete;
	onWorklistComplete_ = onWorklistComplete;

	// Set the work queue as running.
	setSystemStatus (eSystemStatus::eProcessingSample);

	impl->pHawkeyeServices->enqueueInternal ([this, onCompleteCallback]()
		{
			startWorklistInternal (onCompleteCallback);
		});

	Logger::L().Log (MODULENAME, severity_level::debug1, "Start: <exit>");
}

//*****************************************************************************
/**
 * The force parameter, if present and true will execute the pausing process
 * regardless of the currentStatus already being ePausing, ePaused, or eIdle.
 * Defaults to false.
 */
HawkeyeError WorklistDLL::Pause(const bool force)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Pause: <enter>");
	
	eSystemStatus currentStatus = SystemStatus::Instance().getData().status;

	if ((currentStatus == eSystemStatus::ePausing) && (autoResume_))
	{
		// We were going to auto-resume processing but we are being
		// asked to pause again so don't auto-resume
		autoResume_ = false;
		Logger::L().Log(MODULENAME, severity_level::normal, "Pause: <exit, already pausing, auto-resume flag cleared>");
		return HawkeyeError::eSuccess;
	}

	// If being asked to Pause 
	if (currentStatus == eSystemStatus::ePausing || 
		currentStatus == eSystemStatus::ePaused || 
		currentStatus == eSystemStatus::eIdle)
	{
		Logger::L().Log(MODULENAME, severity_level::normal, "Pause: <exit, system is already Idle, Pausing or Paused>");
		return HawkeyeError::eSuccess;
	}

	if (!force && worklistData->carrier == eCarrierType::eCarousel)
	{
		if (cancelCarouselTubeSearchHandler_)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "Pause: <Canceling carousel tube search>");
			cancelCarouselTubeSearchHandler_();
		}
	}

	setSystemStatus (eSystemStatus::ePausing);
	autoResume_ = false;

	impl->isWorklistRunning = false;
	impl->sampleToSampleTiming = boost::none;	// Reset when current work queue paused

	// when 'Pausing', disable holding current so an operator could manipulate the stage...
	Hardware::Instance().getStageController()->SetStageProfile([this](bool status) -> void
		{
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "PauseQueue: <exit: error resetting stage work queue profile>");
			}
		}, false);

	Logger::L().Log (MODULENAME, severity_level::normal, "Pause: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void WorklistDLL::Resume (std::function<void(HawkeyeError)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Resume: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	if (impl->isWorklistRunning)
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "Resume: <exit, Worklist is already running>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	// Do not allow the aborted Worklist to resume.
	if (impl->isAborted)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Resume: <exit, Aborted work queue can not be resumed>");
		impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	if (SystemStatus::Instance().getData().status == eSystemStatus::ePausing)
	{
		// We are being asked to resume but we are still pausing
		// Set the auto-resume flag so that when we transition from pausing to paused we can call Resume
		Logger::L().Log(MODULENAME, severity_level::normal, "Resume: <exit, system is Pausing, auto-resume is now SET>");
		impl->pHawkeyeServices->enqueueExternal(callback, HawkeyeError::eSuccess);
		autoResume_ = true;
		return;
	}

	auto onComplete = [this, callback](HawkeyeError he)
	{
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "Resume: carrier not found ");
			auto instance = worklistData->carrier == eCarrierType::eCarousel
				? instrument_error::instrument_precondition_instance::carousel_present : instrument_error::instrument_precondition_instance::plate_present;
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_precondition_notmet,
				instance,
				instrument_error::severity_level::warning));
		}
		else
		{
			impl->isWorklistRunning = true;

			Logger::L().Log (MODULENAME, severity_level::normal, "Resume: <exit, Worklist processing resumed>");
			queueSampleForProcessing();
			AuditLogger::L().Log(generateAuditWriteData(
				worklistData->runUsername,
				audit_event_type::evt_worklist,
				"Resumed"));
		}

		impl->pHawkeyeServices->enqueueExternal (callback, he);
	};

	autoResume_ = true;
	switch (worklistData->carrier)
	{
		case eCarrierType::eCarousel:
		{
			// Resuming; make no assumptions about the current carousel presence status.
			HawkeyeError he = HawkeyeError::eSuccess;
			if (!Hardware::Instance().getStageController()->IsCarouselPresent())
			{
				he = HawkeyeError::eNotPermittedAtThisTime;
				impl->pHawkeyeServices->enqueueInternal(std::bind(onComplete, he));
				return;
			}

			Hardware::Instance().getStageController()->SetStageProfile([this, onComplete, he](bool status) -> void
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::warning, "Resume: <exit: error setting Carousel work queue profile>");
						HawkeyeError he = HawkeyeError::eHardwareFault;
					}

					impl->pHawkeyeServices->enqueueInternal(std::bind(onComplete, he));
				}, true);
		}
		break;

		case eCarrierType::ePlate_96:
		{
			// Resuming; make no assumptions about the current plate presence status.
			Hardware::Instance().getStageController()->IsPlatePresent([this, onComplete](bool status)
			{
				HawkeyeError he = HawkeyeError::eSuccess;
				if (!status)
				{
					he = HawkeyeError::eNotPermittedAtThisTime;
					impl->pHawkeyeServices->enqueueInternal(std::bind(onComplete, he));
					return;
				}

				Hardware::Instance().getStageController()->SetStageProfile([this, onComplete, he](bool status) -> void
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::warning, "Resume: <exit: error setting Plate work queue profile>");
						HawkeyeError he = HawkeyeError::eHardwareFault;
					}

					impl->pHawkeyeServices->enqueueInternal(std::bind(onComplete, he));
				}, true);
			});
		}
		break;

		case eCarrierType::eACup:
		{
			if (HawkeyeConfig::Instance().get().automationEnabled && worklistData->carrier == eCarrierType::eACup)
			{
				impl->isWorklistRunning = true;
				Logger::L().Log (MODULENAME, severity_level::normal, "Resume: <exit, Worklist processing resumed>");
				queueSampleForProcessing();
				AuditLogger::L().Log(generateAuditWriteData(
					worklistData->runUsername,
					audit_event_type::evt_worklist,
					"Resumed"));
				impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eSuccess);
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::normal, "Resume: <exit, using ACup carrier w/o automation enabled>");
				impl->pHawkeyeServices->enqueueExternal (callback, HawkeyeError::eInvalidArgs);
			}
		}
		break;

	} // End "switch (worklistData->carrier)"
}

//*****************************************************************************
HawkeyeError WorklistDLL::Stop(bool lock)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <enter>");

	// Stopping the queue resets the current sample to the beginning of the processing list.
	// Processing will stop after the current processing has completed.
	std::unique_lock<std::mutex> uLock(impl->worklistMutex, std::defer_lock);
	if (lock)
	{
		uLock.lock();
	}

	if (!impl->isWorklistRunning)
	{
		if (SystemStatus::Instance().getData().status != eSystemStatus::ePausing && 
			SystemStatus::Instance().getData().status != eSystemStatus::ePaused)
		{
			Logger::L().Log (MODULENAME, severity_level::warning, "Stop: <system is not processing>");
			return HawkeyeError::eNotPermittedAtThisTime;
		}
	}

	if (worklistData->carrier == eCarrierType::eCarousel)
	{
		// Trigger cancel tube search handler.
		if (cancelCarouselTubeSearchHandler_)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <canceling carousel tube search>");
			cancelCarouselTubeSearchHandler_();
		}
	}

	// Keep a record of the aborted Queue, So that if the user tries to resume the same queue we can reject it.
	impl->isAborted = true;
	setSystemStatus (eSystemStatus::eStopping);

	if ( worklistData->samplesToProcess.Count() > 0 )
	{
		// Only look at the samples that have not been processed.
		for (auto& v : worklistData->samplesToProcess.Get())
		{
			auto& sample = getSampleByIndices(v);
			if (sample.status == eNotProcessed)
			{
				sample.status = eSampleStatus::eSkip_Manual;
				updateSampleItemStatusToDB(sample);
				updateSampleStatus(sample);
			}
		}
	}

	// If Worklist is not running then stop the Worklist and mark item skipped immediately
	if (!impl->isWorklistRunning)
	{
		// Ensure that this line of code is called AFTER we set worklistStatus_ to "eStopping"
		// Be careful about trying to condense this check into an earlier "if !isWorklistRunning_" block.
		handlePauseStopForCurrentItem();
		worklistCompleted();
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "Stop: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void WorklistDLL::wrapupSampleProcessing (SampleDefinitionDLL& sampleDef)
{
	Logger::L().Log (MODULENAME, severity_level::normal, "wrapupSampleProcessing: <enter, queueing data write>");

	auto onSaveCompletion = [this](SampleDefinitionDLL sd, bool completion_status)->void
	{
		auto extra = boost::str (boost::format ("Sample: %s\n\tCell Type: %s\n\tPosition: %s\n\tTag: %s")
			% sd.parameters.label
			% sd.parameters.celltype.label
			% sd.position.getAsStr()
			% sd.parameters.tag);
		if (!sd.parameters.bp_qc_name.empty())
		{
			extra += "QC: " + sd.parameters.bp_qc_name + "\n";
		}

		if (!completion_status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "wrapupSampleProcessing: <exit, failed to write the analysis data for sample>" + extra);
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror, 
				instrument_error::instrument_storage_instance::sample, 
				instrument_error::severity_level::error));
			AuditLogger::L().Log (generateAuditWriteData(
				worklistData->runUsername,
				audit_event_type::evt_sampleprocessingerror,
				"Error while processing sample\n\t" + extra));
			return;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			worklistData->runUsername,
			audit_event_type::evt_sampleresultcreated,
			extra));

		// A little kludgyness...
		// The SampleDefinition here is a copy and has no linkage to worklistData->sampleSets.
		// This is to get the object from worklistData->sampleSets.samples so that when the status is updated
		// the status in worklistData->sampleSets.samples is updated, not just a local object.
		auto& indices = getIndicesFromSample (sd);
		auto& sample = getSampleByIndices (indices);

		sample.status = eSampleStatus::eCompleted;
		updateSampleStatus (sample);

		SampleDefinitionDLL::Log ("wrapupSampleProcessing:: <data save completed>", sample);

		// Moved this line from RetrieveSampleRecord to avoid a race condition with wrapupSampleProcessing.
		// The UI was calling RetrieveResultRecord before the data could be written to the DB.
		try
		{
			onSampleComplete_(sample);
		} catch(...)
		{
			Logger::L().Log(MODULENAME, severity_level::error, "wrapupSampleProcessing: onSampleComplete_ exception");
		}

		// ACup processing does not use the sample stage like Carousel and Plate do.  Therefore the ACup processing
		// does not call "worklistCompleted".  Since there is no other processing required we call "worklistCompleted"
		// here to finalize the sample.
		if (worklistData->carrier == eCarrierType::eACup)
		{
			worklistCompleted();
		}

		Logger::L().Log (MODULENAME, severity_level::normal, "wrapupSampleProcessing: <exit>");
	};

	impl->pHawkeyeServices->enqueueInternal ([this, &sampleDef, onSaveCompletion]()
	{
		Logger::L().Log (MODULENAME, severity_level::normal, "wrapupSampleProcessing: <start saving data>");

		// Need to create the DB_SampleRecord and DB_AnalysisRecord here so that its UUIDs can be passed to the UI through the sample complete callback.
		DBApi::DB_SampleRecord dbSampleRecord = {};
		DBApi::DB_AnalysisRecord dbAnalysisRecord = {};
		dbAnalysisRecord.AnalysisUserId = {};
		if (!HawkeyeResultsDataManager::CreateSampleRecord (sampleDef, dbSampleRecord) ||
			!HawkeyeResultsDataManager::CreateAnalysisRecord (dbSampleRecord, dbAnalysisRecord))
		{ // Errors are reported by CreateSampleRecord and CreateAnalysisRecord.
			return;
		}

		sampleDef.sampleDataUuid = dbSampleRecord.SampleId;

		boost::optional<ImageSet_t> dustRef = ImageAnalysisUtilities::GetDustReferenceImageSet();
		if (dustRef.get().first.empty() && dustRef.get().second.empty())
		{
			dustRef = boost::none;
		}

		CellCounterResult::SResult sresult = {};
		ImageAnalysisUtilities::GetSResult (sresult);
		ImageAnalysisUtilities::ApplyConcentrationAdjustmentFactor (sampleDef.parameters.celltype, sresult);

		const QcStatus qcStatus = IsSampleWithinQCLimits (sampleDef.parameters.bp_qc_name, sampleDef.parameters.qc_uuid, sresult);
		
		std::vector<ReagentInfoRecordDLL>& reagentInfo = ReagentPack::Instance().GetReagentInfoRecords();

		// LH6531-5300 : Post warning if bubbles detected during samples
		{
			bool discardedforbubble = false;
			for (auto imgdata : sresult.map_Image_Results)
			{
				if (imgdata.second.eProcessStatus == E_ERRORCODE::eBubbleImage)
				{
					discardedforbubble = true;
					break;
				}
			}
			if (sresult.Cumulative_results.nTotalBubbleCount > 0 || discardedforbubble)
			{
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::sample_cellcounting_bubblewarning,
					instrument_error::severity_level::warning));

				Logger::L().Log(MODULENAME, severity_level::warning, "Bubble detected during sample processing");
			}
		}

		// LH6531-5301 : Post warning if >10 large clusters detected during processing
		{
			const uint32_t LARGECLUSETER_WARNING_LEVEL = 10;
			if (sresult.Cumulative_results.nLargeClusterCount > LARGECLUSETER_WARNING_LEVEL)
			{
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::sample_cellcounting_clusterwarning,
					instrument_error::severity_level::warning));

				Logger::L().Log(MODULENAME, severity_level::warning, boost::str(boost::format(">%d large clusters detected during sample processing") % LARGECLUSETER_WARNING_LEVEL));
			}
		}

		// LH6531-5302 : Post warning if images discarded from processing
		{
			if (sresult.Cumulative_results.nTotalCumulative_Imgs != sresult.map_Image_Results.size())
			{
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::sample_cellcounting_imagedropwarning,
					instrument_error::severity_level::warning));

				Logger::L().Log(MODULENAME, severity_level::warning, "Not all collected images were considered for sample analysis");
			}
		}

		HawkeyeResultsDataManager::Instance().SaveAnalysisData (
			worklistData->uuid,	// This is passed by value since this is an async call.
			sampleDef,
			impl->imageCollection, 
			sresult,
			reagentInfo, 
			dustRef,
			dbSampleRecord,		// This is passed by value since this is an async call.
			dbAnalysisRecord,	// This is passed by value since this is an async call.
			qcStatus,
			onSaveCompletion);

		auto onWritingLegacyDataComplete = [](bool status) {
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "wrapupSampleProcessing: <Failed to Save the legacy cell counting data>");
			}
		};

		if (HawkeyeConfig::Instance().get().generateLegacyCellCountingData)
		{
			auto save_legacy_data_lambda = [=]()
			{
				HawkeyeResultsDataManager::Instance().SaveLegacyCellCountingData(
					impl->imageCollection,
					sampleDef.parameters.label,
					sresult,
					onWritingLegacyDataComplete);
			};
			impl->pHawkeyeServices->enqueueInternal (save_legacy_data_lambda);
		}
	});
}

//*****************************************************************************
bool WorklistDLL::isWorklistComplete()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "isWorklistComplete: <enter>");

	bool allDone = true;
	for (size_t i = 0; i < worklistData->samplesToProcess.Count(); i++)
	{
		auto& sample = getSampleByIndices (worklistData->samplesToProcess.GetSampleIndices(i));
		if ((sample.status != eSampleStatus::eCompleted) &&
			(sample.status != eSampleStatus::eSkip_NotProcessed) &&
			(sample.status != eSampleStatus::eSkip_Manual) &&
			(sample.status != eSampleStatus::eSkip_Error)
			)
		{
			allDone = false;
			break;
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug2, 
		boost::str (boost::format ("isWorklistComplete: <exit, %s>") % (allDone ? "done" : "not done")));

	return allDone;
}

//*****************************************************************************
// Check to see if any samples are in the eNotProcessed state. If so, return 
// true. Else return false.
bool WorklistDLL::areAnySamplesNotProcessed()
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "areAllSamplesProcessing: <enter>");

	bool anySamplesNotProcessed = false;
	for (size_t i = 0; i < worklistData->samplesToProcess.Count(); i++)
	{
		auto& sample = getSampleByIndices (worklistData->samplesToProcess.GetSampleIndices(i));
		if ((sample.status == eSampleStatus::eNotProcessed))
		{
			anySamplesNotProcessed = true;
			break;
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug1,
		boost::str (boost::format ("areAnySamplesNotProcessed: <exit, %s>") % (anySamplesNotProcessed ? "yes" : "no")));

	return anySamplesNotProcessed;
}

//*****************************************************************************
bool WorklistDLL::sufficientReagentForSet(ReagentContainerPosition containerPosition, const int sampleCount) const
{
	int remainUses = SystemStatus::Instance().getData().remainingReagentPackUses;

	// Find the number of samples left to process.
	uint16_t numRemainingSamples = 0;
	for (const auto& v : worklistData->samplesToProcess.Get())
	{
		if (getSampleByIndices (v).status == eSampleStatus::eNotProcessed)
		{
			numRemainingSamples++;
		}
	}

	uint16_t totalSamplesToProcess = numRemainingSamples + static_cast<uint16_t>(sampleCount);
	if (totalSamplesToProcess > remainUses)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
void WorklistDLL::Log (const std::vector<SampleSetDLL>& sampleSets)
{
	std::stringstream ss;

	if ( Logger::L().IsOfInterest( severity_level::debug1 ) )
	{
		ss << "WorklistDLL: " << "\n";
//TODO: list the other fields of the Worklist...
		for ( size_t i = 0; i < sampleSets.size(); i++ )
		{
			SampleSetDLL::Log( ss, boost::str( boost::format( "SampleSet[%d]" ) % i ), sampleSets[i] );
		}
		Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());
	}
}

//*****************************************************************************
void WorklistDLL::resetWorklist()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "resetWorklist: <enter>");

	setSystemStatus (eSystemStatus::eIdle);
	worklistData->sampleSetsDLL.clear();
	worklistData->samplesToProcess.Clear();
	impl->isWorklistSet = false;
	impl->isWorklistRunning = false;
	impl->deadlineTimer.reset();
	worklistData->clear();
	impl->sampleToSampleTiming = boost::none;

	Logger::L().Log (MODULENAME, severity_level::debug1, "resetWorklist: <exit>");
}

//*****************************************************************************
bool WorklistDLL::isSampleSetRunning (const SampleSetDLL& sampleSet)
{
	bool retStatus = false;

	// Since orphan Samples may be added at any time, orphan SampleSets cannot be 
	// marked as completed until the Worklist is completely done.
	// The ophan SampleSet is never marked as completed based on whether the Samples
	// in the orphan SampleSet are completed, another orphan sould always be added
	// to the SampleSet.
	// If the orphan SampleSet has Samples in it when "worklistCompleted" is called
	// the orphan SampleSet will then be marked as completed.	
	// So...  always return true to indicate that the orphan SampleSet is alwyas processing.
	if (sampleSet.carrier == eCarrierType::eCarousel && sampleSet.index == OrphanSampleSetIndex)
	{
		return true;
	}

	for (auto& sample : sampleSet.samples)
	{
		switch (sample.status)
		{
			case eNotProcessed:
			case eInProcess_Aspirating:
			case eInProcess_Mixing:
			case eInProcess_ImageAcquisition:
			case eInProcess_Cleaning:
			case eAcquisition_Complete:
				retStatus = true;
				break;
		}
	}

	return retStatus;
}

//*****************************************************************************
bool WorklistDLL::isSampleSetCompleted (const SampleSetDLL& sampleSet)
{
	bool retStatus = true;

	// Since orphan Samples may be added at any time, orphan SampleSets cannot be 
	// marked as completed until the Worklist is completely done.
	// The ophan SampleSet is never marked as completed based on whether the Samples
	// in the orphan SampleSet are completed, another orphan sould always be added
	// to the SampleSet.
	// If the orphan SampleSet has Samples in it when "worklistCompleted" is called
	// the orphan SampleSet will then be marked as completed.
	// So...  always return false to indicate that the orphan SampleSet has not completed processing.
	if (sampleSet.carrier == eCarrierType::eCarousel && sampleSet.index == OrphanSampleSetIndex)
	{
		return false;
	}

	for (auto& sample : sampleSet.samples)
	{
		retStatus = isSampleCompleted (sample);
		if (!retStatus)
		{
			return false;
		}
	}

	return retStatus;
}

//*****************************************************************************
bool WorklistDLL::isSampleCompleted (const SampleDefinitionDLL& sample)
{
	bool retStatus = true;

	switch (sample.status)
	{
		case eNotProcessed:
		case eInProcess_Aspirating:
		case eInProcess_Mixing:
		case eInProcess_ImageAcquisition:
		case eInProcess_Cleaning:
		case eAcquisition_Complete:
			retStatus = false;
	}

	return retStatus;
}

//*****************************************************************************
// Write the Worklist (including any attached sample sets) to the DB.
//*****************************************************************************
bool WorklistDLL::writeWorklistToDB (DBApi::eWorklistStatus dbWorklistStatus)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "writeWorklistToDB: <enter>");

	// If worklist has already been saved, don't write it again.
	if (Uuid::IsValid (worklistData->uuid))
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "writeWorklistToDB: <exit, Worklist already exists, nothing written>");
		return true;
	}

// TODO: ensure the status and acquireData flags are set at the appropriate place in the workflow
	DBApi::DB_WorklistRecord dbWL = WorklistData::ToDbStyle (*worklistData, dbWorklistStatus);

	DBApi::eQueryResult dbStatus = DBApi::DbAddWorklist(dbWL);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("writeWorklistToDB: <exit, DB Worklist write failed, status: %ld>") % (int32_t)dbStatus));

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));

		return false;
	}

	// Populate the local data structures with the returned UUIDs, etc.
	worklistData->uuid = dbWL.WorklistId;

	size_t sampleSetNum = 0;
	for (auto& dbSampleSet : dbWL.SSList)
	{
		updateSampleSetUuids (dbSampleSet, getSampleSetByIndex(worklistData->sampleSetsDLL[sampleSetNum++].index));
	}

	Logger::L().Log( MODULENAME, severity_level::debug2, "writeWorklistToDB: <exit>" );

	return true;
}

//*****************************************************************************
void WorklistDLL::updateSampleSetUuids (const DBApi::DB_SampleSetRecord& dbSampleSet, SampleSetDLL& sampleSet)
{
	sampleSet.worklistUuid = dbSampleSet.WorklistId;
	std::cout << boost::str (boost::format ("worklistUuid: %s\n") % Uuid::ToStr(sampleSet.worklistUuid));
	sampleSet.uuid = dbSampleSet.SampleSetId;
	std::cout << boost::str (boost::format ("sampleSetUuid: %s\n") % Uuid::ToStr(sampleSet.uuid));

	sampleSet.samplesetIdNum = dbSampleSet.SampleSetIdNum;
	sampleSet.uuid = dbSampleSet.SampleSetId;

	uint16_t sIndex = 0;
	for (auto& dbSample : dbSampleSet.SSItemsList)
	{
		SampleDefinitionDLL& sample = sampleSet.samples[sIndex];
		updateSampleUuids (dbSample, sample);
		sIndex++;
	}
}

//*****************************************************************************
void WorklistDLL::updateSampleUuids (const DBApi::DB_SampleItemRecord& dbSample, SampleDefinitionDLL& sample)
{
	sample.sampleSetUuid = dbSample.SampleSetId;
	std::cout << boost::str (boost::format ("sampleSetUuid[%d]: %s\n") % sample.index % Uuid::ToStr(sample.sampleSetUuid));
	sample.sampleDefUuid = dbSample.SampleItemId;
	std::cout << boost::str (boost::format ("sampleDefUuid[%d]: %s\n") % sample.index % Uuid::ToStr(sample.sampleDefUuid));
	sample.sampleDataUuid = dbSample.SampleId;
	std::cout << boost::str (boost::format ("sampleDataUuid[%d]: %s\n") % sample.index % Uuid::ToStr(sample.sampleDataUuid));
}

//*****************************************************************************
// Write the updated WorklistDLL (including any attached SampleSets) to the DB.
//*****************************************************************************
bool WorklistDLL::updateWorklistToDB()
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "updateWorklistToDB: <enter>");

	DBApi::DB_WorklistRecord dbWL = WorklistData::ToDbStyle (*worklistData, static_cast<DBApi::eWorklistStatus>(worklistData->status));

	std::cout << "worklistData->uuid: " << Uuid::ToStr(worklistData->uuid) << std::endl;

	DBApi::eQueryResult dbStatus = DBApi::DbModifyWorklist(dbWL);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format ("updateWorklistToDB: <exit, DB Worklist update failed; query status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));

		return false;
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "updateWorklistToDB: <exit>");

	return true;
}

//*****************************************************************************
// Database: write the updated WorklistDLL status to the DB.
// NOTE that this ONLY updates the worklist object status, and does NOT apply/update
// status for the contained sample set(s) and sample definitions (DB SampleItems)
//*****************************************************************************
bool WorklistDLL::updateWorklistStatusToDB (DBApi::eWorklistStatus wlStatus)
{
	std::cout << "worklistData->uuid: " << Uuid::ToStr(worklistData->uuid) << std::endl;

	DBApi::eQueryResult dbStatus = DBApi::DbSetWorklistStatus(worklistData->uuid, wlStatus);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("updateWorklistStatusToDB: <exit, failed to update Worklist status: %ld>") % (int32_t)dbStatus));

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));

		return false;
	}

	return true;
}

//*****************************************************************************
bool WorklistDLL::addWorklistSampleSet (const SampleSetDLL& sampleSet)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "addWorklistSampleSet: <enter>");

	DBApi::DB_SampleSetRecord dbSS = sampleSet.ToDbStyle();

	std::cout << "worklistData->uuid: " << Uuid::ToStr (worklistData->uuid) << std::endl; 

	DBApi::eQueryResult dbStatus = DBApi::DbAddWorklistSampleSet(dbSS, sampleSet.worklistUuid);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("addWorklistSampleSet: <exit, failed to add SampletSet status: %ld>") % (int32_t)dbStatus));

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));

		return false;
	}

	// Populate the local data structures with the returned UUIDs, etc.
	std::cout << boost::str (boost::format ("SampleSetIndex: %d\n") % sampleSet.index);
	updateSampleSetUuids (dbSS, getSampleSetByIndex(sampleSet.index));

	Logger::L().Log (MODULENAME, severity_level::debug1, "addWorklistSampleSet: <exit>");

	return true;
}

//TODO: not currently used...
////*****************************************************************************
//// Write a SampleSetDLL to the DB, including its contained sample definition objects.
////*****************************************************************************
//bool WorklistDLL::writeSampleSetToDB (SampleSetDLL& sampleSet)
//{
//	Logger::L().Log( MODULENAME, severity_level::debug1, "writeSampleSetToDB: <enter>" );
//
//	DBApi::DB_SampleSetRecord dbSS = sampleSet.ToDbStyle();
//
//	std::cout << "sampleSet.uuid: " << Uuid::ToStr(sampleSet.uuid) << std::endl;
//
//	DBApi::eQueryResult dbStatus = DBApi::DbAddSampleSet(dbSS);
//	if (dbStatus != DBApi::eQueryResult::QueryOk)
//	{
//		Logger::L().Log( MODULENAME, severity_level::error,
//			boost::str( boost::format( "writeSampleSetToDB: <exit, DB SampleSet save failed; query status: %ld>" ) % (int32_t)dbStatus));
//
//		ReportSystemError::Instance().ReportError (BuildErrorInstance(
//			instrument_error::instrument_storage_writeerror,
//			instrument_error::instrument_storage_instance::sample,
//			instrument_error::severity_level::warning));
//
//		return false;
//	}
//
//	updateSampleSet (dbSS, sampleSet);
//
//	Logger::L().Log( MODULENAME, severity_level::debug1, "writeSampleSetToDB: <exit>" );
//
//	return true;
//}

//*****************************************************************************
void WorklistDLL::updateSampleStatus (SampleDefinitionDLL& sample)
{
	std::lock_guard<std::mutex> guard (impl->sampleStatusMutex);

	SampleSetDLL sampleSetDLL = getSampleSetByIndex(sample.sampleSetIndex);
	
	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str (boost::format ("updateSampleStatus:: ssIndex: %d, ssStatus: %d, sIndex: %d, Status: %s")
			% sample.sampleSetIndex
			% (int)sampleSetDLL.status
			% sample.index
			% getSampleStatusAsStr(sample.status)
		));

	updateSampleItemStatusToDB (sample);

	updateSampleSetStatus (sampleSetDLL);

	// Trigger sample complete callback to update UI about skipped item.
	if (onSampleStatus_)
	{
		impl->pHawkeyeServices->enqueueExternal (onSampleStatus_, sample);
	}

#ifdef TEST_REPLACING_REAGENT_PACK
	if (sample.status == eSampleStatus::eAcquisition_Complete)
	{
		void decrementRemainingReagentPackUses();
		decrementRemainingReagentPackUses();
	}
#endif
	
	Logger::L().Log (MODULENAME, severity_level::debug2, "updateSampleStatus: <exit>");
}

//*****************************************************************************
bool WorklistDLL::updateSampleSetStatus (SampleSetDLL& sampleSet)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, 
		boost::str (boost::format ("updateSampleSetStatus: <enter, dbSampleSet status: %d>") 
			% (int)sampleSet.status));

	DBApi::eSampleSetStatus dbSampleSetStatus = DBApi::eSampleSetStatus::NoSampleSetStatus;

	if (isSampleSetCompleted (sampleSet))
	{
		// Do not try and set the SampleSet status to complete more than once.
		// The DB prevents this and throws an error.
		if (sampleSet.status == DBApi::eSampleSetStatus::SampleSetComplete)
		{
			return true;
		}

		if (sampleSet.samples.empty())
		{
			dbSampleSetStatus = DBApi::eSampleSetStatus::SampleSetNotRun;
		}
		else
		{
			// Do not allow the orphan SampleSet to complete here.
			// Completion is handled in "worklistCompleted" method.
			if (sampleSet.carrier != eCarrierType::eCarousel || sampleSet.index != OrphanSampleSetIndex)
			{
				dbSampleSetStatus = DBApi::eSampleSetStatus::SampleSetComplete;
			}
		}
	}
	else
	{
		if (isSampleSetRunning (sampleSet))
		{
			dbSampleSetStatus = DBApi::eSampleSetStatus::SampleSetRunning;
		}
		else
		{
			dbSampleSetStatus = DBApi::eSampleSetStatus::SampleSetActive;
		}
	}

	if (!updateSampleSetStatusToDB (sampleSet, dbSampleSetStatus))
	{ // Method logs any error.
		return false;
	}

	Logger::L().Log( MODULENAME, severity_level::debug2, "updateSampleSetStatus: <exit>" );

	return true;
}

//*****************************************************************************
// Write updated Sample status to the DB.
//*****************************************************************************
bool WorklistDLL::updateSampleItemStatusToDB (SampleDefinitionDLL& sample)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "updateSampleItemStatusToDB: <enter, sampleSetUuid: " + Uuid::ToStr(sample.sampleSetUuid) + ">");
	Logger::L().Log(MODULENAME, severity_level::debug2, "updateSampleItemStatusToDB: <enter, sampleDefUuid: " + Uuid::ToStr(sample.sampleDefUuid) + ">");

	DBApi::eQueryResult dbStatus = DBApi::DbSetSampleSetSampleItemStatus(sample.sampleSetUuid, sample.sampleDefUuid,
		static_cast<DBApi::eSampleItemStatus>(SampleDefinitionDLL::SampleItemStatusToDbStyle(sample.status)));
	if (dbStatus != DBApi::eQueryResult::NoResults)
	{
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str (boost::format ("updateSampleItemStatusToDB: <exit, DB Sample status update failed; query status: %ld>") % (int32_t)dbStatus));

			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::warning));

			return false;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "updateSampleItemStatusToDB: <exit>");

	return true;
}

//*****************************************************************************
bool WorklistDLL::updateSampleSetStatusToDB (SampleSetDLL& sampleSet, DBApi::eSampleSetStatus dbSampleSetStatus)
{
	Logger::L().Log (MODULENAME, severity_level::debug2,
		boost::str (boost::format ("<enter, updateSampleSetStatusToDB: sampleSet: %s, status: %d>") % Uuid::ToStr(sampleSet.uuid) % (int)dbSampleSetStatus));

	// Only write the SampleSet status if it has changed.
	if (sampleSet.status != dbSampleSetStatus)
	{
		sampleSet.status = dbSampleSetStatus;

		DBApi::eQueryResult dbStatus = DBApi::DbSetSampleSetStatus(sampleSet.uuid, dbSampleSetStatus);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str(boost::format("updateSampleSetStatusToDB: <exit, DB SampleSet status update failed; query status: %ld>") % (int32_t)dbStatus));

			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::warning));

			Logger::L ().Log (MODULENAME, severity_level::debug1, "<exit, error>");

			return false;
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug2, "<exit>");

	return true;
}

//*****************************************************************************
bool WorklistDLL::updateSampleDefinitionToDB (SampleDefinitionDLL& sample)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "updateSampleDefinitionToDB: <enter>");

	DBApi::DB_SampleItemRecord dbItem = sample.ToDbStyle();

	DBApi::eQueryResult dbStatus = DBApi::DbModifySampleItem(dbItem);
	if (dbStatus != DBApi::eQueryResult::NoResults)
	{
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str (boost::format ("updateSampleDefinitionToDB: <exit, DB Sample Definition update failed; query status: %ld>") % (int32_t)dbStatus));

			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::warning));

			return false;
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug2, "updateSampleDefinitionToDB: <exit>");

	return true;
}

//*****************************************************************************
SampleSetDLL& WorklistDLL::getSampleSetByIndex (uint16_t ssIndex)
{
	for (auto& sampleSet : worklistData->sampleSetsDLL)
	{
		if (sampleSet.index == ssIndex)
		{
			return sampleSet;
		}
	}
	Logger::L().Log(MODULENAME, severity_level::error, "getSampleSetByIndex: failed to find index: " + std::to_string(ssIndex));
	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
SampleDefinitionDLL& WorklistDLL::getSampleByIndices (const WorklistDLL::SampleIndices indices)
{
	for (auto& sampleSet : worklistData->sampleSetsDLL)
	{
		if (sampleSet.index == indices.sampleSetIndex)
		{
			for (auto& sample : sampleSet.samples)
			{
				if (sample.index == indices.sampleIndex)
				{
					return sample;
				}
			}
		}
	}

	Logger::L().Log(MODULENAME, severity_level::error, "getSampleByIndices: failed");
	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
WorklistDLL::SampleIndices WorklistDLL::getIndicesFromSample (const SampleDefinitionDLL& sample)
{
	return { sample.sampleSetIndex, sample.index };
}

//*****************************************************************************
std::vector<WorklistDLL::SampleIndices> WorklistDLL::getIndicesVectorFromSamples (const std::vector<SampleDefinitionDLL>& samples)
{
	std::vector<WorklistDLL::SampleIndices> collIndices;

	for (auto& v : samples)
	{
		collIndices.push_back (getIndicesFromSample(v));
	}

	return collIndices;
}

//*****************************************************************************
void WorklistDLL::markCarouselSlotsInUseForSimulation (const std::vector<SampleIndices>& indices)
{
	// For simulation, update the slots when samples are added.
	if (!HawkeyeConfig::Instance().get().withHardware && worklistData->carrier == eCarrierType::eCarousel)
	{
		std::vector<bool> carouselSlots(MaxCarouselTubes);

		for (const auto& v : indices)
		{
			auto slotNum = getSampleByIndices (v).position.getColumn() - 1;
			carouselSlots[slotNum] = true;
		}

		Hardware::Instance().getStageController()->setCarsouselSlots (carouselSlots);
	}
}
