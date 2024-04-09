#include "stdafx.h"

#include "BrightfieldDustSubtractWorkflow.hpp"
#include "EnumConversion.hpp"
#include "FileSystemUtilities.hpp"
#include "Hardware.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "ImageProcessingUtilities.hpp"
#include "ImageWrapperUtilities.hpp"
#include "LegacyData.hpp"

static const char MODULENAME[] = "BrightfieldDustSubtractWorkflow";

const std::map<BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState, std::string>
EnumConversion<BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState>::enumStrings<BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState>::data =
{
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_Idle, std::string("Idle") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_AspiratingBuffer, std::string("AspiratingBuffer") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_DispensingBufferToFlowCell, std::string ("DispensingBufferToFlowCell") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_AcquiringImages, std::string("AcquiringImages") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_ProcessingImages, std::string("ProcessingImages") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval, std::string("WaitingOnUserApproval") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_SettingReferenceImage, std::string("SettingReferenceImage") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_Cancelling, std::string("Cancelling") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_Completed, std::string("Completed") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_Failed, std::string("Failed") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_FindingTube, std::string("FindingTube") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_TfComplete, std::string("TfComplete") },
	{ BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState::bds_TfCancelled, std::string("TfCancelled") },
};

//*****************************************************************************
BrightfieldDustSubtractWorkflow::BrightfieldDustSubtractWorkflow(
	brightfield_dustsubtraction_callback_DLL callback)
	: Workflow(Workflow::Type::BrightfieldDustSubtract)
	, callback_(callback)
{
	currentState_ = eBrightfieldDustSubtractionState::bds_Idle;
	reset();

	cameraCaptureCallback_ = [this](cv::Mat image) -> void
	{
		pServices_->enqueueInternal([=]() { onBDSCameraTrigger(image); });
	};
}

//*****************************************************************************
BrightfieldDustSubtractWorkflow::~BrightfieldDustSubtractWorkflow()
{
	reset();
}

//*****************************************************************************
HawkeyeError BrightfieldDustSubtractWorkflow::execute()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute : <enter>");

	if ( (currentState_ != eBrightfieldDustSubtractionState::bds_Idle && currentState_ != eBrightfieldDustSubtractionState::bds_TfCancelled)
		|| isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

//	reset();
	ImageProcessingUtilities::initialize (pServices_->getInternalIos());

	// set the cleaning cycle parameters
	setCleaningCycleIndices();
	
	currentState_ = eBrightfieldDustSubtractionState::bds_FindingTube;


	int16_t instrumentType = HawkeyeConfig::Instance().get().instrumentType;
	if (instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		HawkeyeError he = Workflow::execute();
		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow!");
			this->triggerCompletionHandler(he);
		}
	}
	else
	{
		auto stageController = Hardware::Instance().getStageController();

		stageController->SetStageProfile([this](bool) -> void {}, true);

		// get the default carrier position asynchronously
		stageController->moveToDefaultStagePosition([this, stageController](bool status) -> void
		{
			if (currentState_ == eBrightfieldDustSubtractionState::bds_TfCancelled)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute : Workflow cancelled!");
				this->triggerCompletionHandler(HawkeyeError::eSuccess);
				return;
			}

			currentState_ = eBrightfieldDustSubtractionState::bds_TfComplete;

			stageController->SetStageProfile([this](bool) -> void {}, true);

			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute : Failure moving to default carrier position or finding tube");
				this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
				return;
			}

			stageController->ProbeDown([this](bool status) -> void
				{
					if (!status)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to move probe down");
						this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
						return;
					}

					HawkeyeError he = Workflow::execute();

					if (he != HawkeyeError::eSuccess)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "execute : Failed to execute workflow!");
						this->triggerCompletionHandler(he);
					}
				});
		});
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute : <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError BrightfieldDustSubtractWorkflow::execute (uint16_t jumpToState)
{
	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("execute: <enter, %s>")
			% EnumConversion<eBrightfieldDustSubtractionState>::enumToString(static_cast<eBrightfieldDustSubtractionState>(jumpToState))));

	if (jumpToState == eBrightfieldDustSubtractionState::bds_Idle)
	{
		return this->execute();
	}

	if (isBusy())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Failed to execute workflow : current instance is busy performing other task!");
		return HawkeyeError::eBusy;
	}

	ImageProcessingUtilities::initialize (pServices_->getInternalIos());

	// set the cleaning cycle parameters
	setCleaningCycleIndices();

	// This method will be called after the user accepts the results.
	// So no need to initialize the carrier or move the location.
	// Add check for fail safe
	if (Hardware::Instance().getCarrier() == eCarrierType::eUnknown
		|| Hardware::Instance().getCarrierPosition().isValid() == false)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Invalid current carrier position!");
		return HawkeyeError::eInvalidArgs;
	}

	auto state = static_cast<eBrightfieldDustSubtractionState>(jumpToState);
	HawkeyeError status = HawkeyeError::eHardwareFault;

	if (state == eBrightfieldDustSubtractionState::bds_SettingReferenceImage)
	{
		if (bdsImage_.get() != nullptr && settingDustImage(*bdsImage_.get()))
		{
			onWorkflowStateChanged(eBrightfieldDustSubtractionState::bds_Completed, false);
			status = HawkeyeError::eSuccess;
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Failed to set dust subtract reference image!");
			status = HawkeyeError::eInvalidArgs;
		}
	}
	else if (state == eBrightfieldDustSubtractionState::bds_Completed)
	{
		onWorkflowStateChanged(eBrightfieldDustSubtractionState::bds_Completed, false);
		status = HawkeyeError::eSuccess;
	}
	else
	{
		Hardware::Instance().getStageController()->ProbeDown([this, jumpToState](bool status)
		{
			if (!status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Failed to move probe down");
				this->triggerCompletionHandler(HawkeyeError::eHardwareFault);
				return;
			}

			HawkeyeError he = Workflow::execute(jumpToState);

			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "execute(state) : Failed to execute workflow!");
				this->triggerCompletionHandler(he);
			}
		});
		return HawkeyeError::eSuccess;
	}
	
	triggerCompletionHandler(status);
	Logger::L().Log (MODULENAME, severity_level::debug1, "execute(state) : <exit>");
	return status;
}

//*****************************************************************************
HawkeyeError BrightfieldDustSubtractWorkflow::abortExecute()
{
	Logger::L().Log (MODULENAME, severity_level::warning, "abortExecute: <enter>");

	if (currentState_ != eBrightfieldDustSubtractionState::bds_Idle)
	{
		if ( currentState_ == eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval )
		{
			currentState_ = eBrightfieldDustSubtractionState::bds_Cancelling;
		}

		if (currentState_ == eBrightfieldDustSubtractionState::bds_FindingTube)
		{
			currentState_ = eBrightfieldDustSubtractionState::bds_TfCancelled;
			Hardware::Instance().getStageController()->CancelMove([this](bool)
			{
				triggerCompletionHandler(HawkeyeError::eSuccess);
				Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute: <exit, move cancelled>");
				return Workflow::abortExecute();
			});

			Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute: <exit, tube search cancelled>");
			return HawkeyeError::eSuccess;
		}

		triggerCompletionHandler(HawkeyeError::eSuccess);
		Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute: <exit, not idle>");
		return Workflow::abortExecute();
	}

	triggerCompletionHandler(HawkeyeError::eSuccess);
	Logger::L().Log (MODULENAME, severity_level::debug1, "abortExecute: <exit>");
	
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
std::string BrightfieldDustSubtractWorkflow::getWorkFlowScriptFile(uint8_t workflowSubType)
{
	if (workflowSubType > 0)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "getWorkFlowScriptFile : Not supported input index : " + std::to_string(workflowSubType));
		return std::string();
	}
	return HawkeyeDirectory::Instance().getWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eDustSubstract);
}

//*****************************************************************************
HawkeyeError BrightfieldDustSubtractWorkflow::load(std::string filename)
{
	HawkeyeError he = Workflow::load(filename);	
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "load : failed to load workflow file '" + filename + "'");
		return he;
	}

	// Now add the workflow operation (not present in Script)
	emptySyringeAtBeginning();

	return he;
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::triggerCompletionHandler(HawkeyeError he)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <enter>");

	if (currentState_ == eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval)
	{
		updateHost(eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval);
		Workflow::triggerCompletionHandler(he);
		Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <exit> waiting for user approval");
		return;
	}

	cleanupBeforeExit([this, he](bool)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : cleanupBeforeExit callback");

		if (he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "triggerCompletionHandler : Workflow execution failed!");
			onWorkflowStateChanged(eBrightfieldDustSubtractionState::bds_Failed, true);
		}
		else
		{
			onWorkflowStateChanged(eBrightfieldDustSubtractionState::bds_Idle, true);
		}

		currentState_ = eBrightfieldDustSubtractionState::bds_Idle;
		Workflow::triggerCompletionHandler(he);

		// clear the acquired images and result.
		reset();
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "triggerCompletionHandler : <exit>");
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::processImage (const std::vector<cv::Mat>& acquiredImages, std::function<void(HawkeyeError he)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "processImage: <enter, dust image creation operation - executing>");

	bdsImage_.reset();
	
	if (acquiredImages.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "processImage: <exit, failed to capture any images to process dust subtraction>");
		if (callback)
		{
			callback(HawkeyeError::eHardwareFault);
		}
		return;
	}

	uint16_t imageRequired = getImageCount(); // get the number of images required (as per script)
	if (imageRequired != acquiredImages.size())
	{		
		Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("processImage: <exit, failed to capture required number of images to process dust subtraction, Required: %d, Captured: %d>") % imageRequired % acquiredImages.size()));
		if (callback)
		{
			callback(HawkeyeError::eHardwareFault);
		}
		return;
	}

	bdsImage_.reset(new Mat());

//TODO: !!!
	ImageProcessingUtilities::generateDustImage(acquiredImages, *bdsImage_)
		.whenComplete([this, callback](HawkeyeError he)
	{
		if (bdsImage_->empty() || he != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "processImage: <exit, failed to generate dust reference image>");
			bdsImage_.reset();
			callback(HawkeyeError::eHardwareFault);
		}
		else
		{
			callback(HawkeyeError::eSuccess);
		}

		Logger::L().Log (MODULENAME, severity_level::debug2, "processImage: <exit>");
	});
}

//*****************************************************************************
bool BrightfieldDustSubtractWorkflow::settingDustImage(const cv::Mat& image)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "settingDustImage : <enter>");

	if (image.empty())
	{
		return false;
	}

	std::string dirPath = HawkeyeDirectory::Instance().getDustReferenceImageDir();
	if (FileSystemUtilities::CreateDirectories(dirPath) == false)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "settingDustImage: failed to create destination directory: " + dirPath);
		return false;
	}
	std::string imagePath = HawkeyeDirectory::Instance().getDustReferenceImagePath();

	Logger::L().Log (MODULENAME, severity_level::normal, "settingDustImage: writing " + imagePath);
	if (!HDA_WriteEncryptedImageFile (image, imagePath))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "settingDustImage: Failed to save image to path : " + imagePath);
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::published_errors::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::dust_image, 
			instrument_error::severity_level::error));
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "settingDustImage : <exit>");
	return true;
}

//*****************************************************************************
std::function<void(cv::Mat)> BrightfieldDustSubtractWorkflow::getCameraCallback()
{
	return cameraCaptureCallback_;
}

//*****************************************************************************
bool BrightfieldDustSubtractWorkflow::executeCleaningCycleOnTermination(
	size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "executeCleaningCycleOnTermination: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onCleaningComplete);

	// Find out if we can execute cleaning cycle or not
	bool canExecute = Workflow::executeCleaningCycleOnTermination(currentWfOpIndex, endIndex, onCleaningComplete);
	if (canExecute)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "executeCleaningCycleOnTermination: updating host with cancel state");

		// Enqueue immediate next task to update host with "eBrightfieldDustSubtractionState::bds_Cancelling" state
		onWorkflowStateChanged(eBrightfieldDustSubtractionState::bds_Cancelling, false);
	}

	return canExecute;
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::onBDSCameraTrigger(cv::Mat image)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "onBDSCameraTrigger: <enter>");
		
	if (image.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onBDSCameraTrigger: <exit, error getting image>");
		return;
	}

	acquiredImages_.push_back(image);

	bool success = createLegacyImageDataDirectories(static_cast<uint32_t>(acquiredImages_.size()), HawkeyeDirectory::Instance().getDustReferenceImageDir());
	if (!success)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onBDSCameraTrigger: <exit, failed to create directories to save image>");
	}

	std::string dataPath;
	success = success && getLegacyDataPath (HawkeyeConfig::LedType::LED_BrightField, dataPath);

	std::string dataType;
	success = success && getLegacyDataType (HawkeyeConfig::LedType::LED_BrightField, dataType);

	if (success)
	{
		std::string imagePath = boost::str(boost::format("%s\\%s_%d.png") % dataPath % dataType % acquiredImages_.size());

		Logger::L().Log (MODULENAME, severity_level::debug2, "onBDSCameraTrigger: writing " + imagePath);
		if (!HDA_WriteImageToFile(imagePath, image))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "onBDSCameraTrigger: Failed to save image to path : " + imagePath);
			success = false;
		}
	}

	if (!success)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::published_errors::instrument_storage_writeerror, 
			instrument_error::instrument_storage_instance::debug_image,
			instrument_error::severity_level::notification));
		return;
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug2, "onBDSCameraTrigger: <exit>");
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::onWorkflowStateChanged (uint16_t currentState, bool executionComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "onWorkflowStateChanged: " +
		EnumConversion<eBrightfieldDustSubtractionState>::enumToString(static_cast<eBrightfieldDustSubtractionState>(currentState)));

	currentState_ = currentState;

	if (currentState_ == eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval)
	{
		opCleaningIndex_.reset(); // Reset the cleaning since we are doing internal abort waiting for the user acceptance; NOTE not a full abort, don't use abortExecute.
		triggerCompletionHandler( HawkeyeError::eSuccess );
		return;
	}

	if ((currentState >= eBrightfieldDustSubtractionState::bds_FindingTube) &&
		(currentState <= eBrightfieldDustSubtractionState::bds_TfCancelled))
	{
		currentState = eBrightfieldDustSubtractionState::bds_Idle;
	}

	updateHost(currentState);

	// Since there is no workflow operation for processing bright field dust
	// subtract operation, so perform processing here. 
	if (currentState_ == eBrightfieldDustSubtractionState::bds_ProcessingImages)
	{
		auto workLambda = [this]() -> ICompletionHandler*
		{
			auto handler = &CompletionHandler::createInstance();
			auto index = handler->getIndex();
			this->processImage(acquiredImages_, [index](HawkeyeError he) -> void
			{
				CompletionHandler::getInstance(index).onComplete(he);
			});
			return handler;
		};

		appendProcessing(workLambda);
		// Inserted a new workflow operation so updating the cleaning cycle start index
		if (opCleaningIndex_)
		{
			opCleaningIndex_->first += 1;
		}
	}
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::updateHost(uint16_t currentState)
{
	auto state = static_cast<eBrightfieldDustSubtractionState>(currentState);

	cv::Mat localDustRefImage = cv::Mat();
	if (bdsImage_.get() != nullptr)
	{
		localDustRefImage.release();
		localDustRefImage = *bdsImage_.get();
	}

	if (callback_ != nullptr)
	{
		// Update host with current state and result.
		std::vector<ImageWrapperDLL> vec_images{ acquiredImages_.begin(), acquiredImages_.end() };
		callback_(state, ImageWrapperDLL{ localDustRefImage }, (uint16_t)vec_images.size(), vec_images);
	}
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::setCleaningCycleIndices()
{
	const auto cleaningState = eBrightfieldDustSubtractionState::bds_ProcessingImages;
	size_t startIndex = getWorkflowOperationIndex(cleaningState);
	startIndex++; // Start from cleaning process without updating UI with state change

	size_t endIndex = getWorkflowOperationIndex(eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval);
	endIndex--; // End index is just before waiting for user approval

	opCleaningIndex_ = std::make_pair(startIndex, endIndex);
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::cleanupBeforeExit(std::function<void(bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "cleanupBeforeExit : <enter>");
	HAWKEYE_ASSERT (MODULENAME, callback);

	int16_t instrumentType = HawkeyeConfig::Instance().get().instrumentType;
	if (instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		pServices_->enqueueInternal(callback, true);
	}

	auto stageController = Hardware::Instance().getStageController();

	std::function<void(bool)> cleanupComplete = [=](bool status) -> void
	{
		bool holding = EJECT_HOLDING_CURRENT_OFF;
		if ( HawkeyeConfig::Instance().get().automationEnabled && !stageController->IsCarouselPresent() )
		{
			holding = EJECT_HOLDING_CURRENT_ON;
		}

		stageController->SetStageProfile([this, status, callback](bool disableOk) -> void
		{
			if (!disableOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to disable holding current");
			}
			else
			{
				disableOk = status;
			}
			pServices_->enqueueInternal(callback, disableOk);
		}, holding);
	};

	stageController->ProbeUp([this, stageController, cleanupComplete](bool isProbeUp) -> void
	{
		if (!isProbeUp)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to move probe up");
			pServices_->enqueueInternal(cleanupComplete, isProbeUp);
			return;
		}

		std::function<void(bool)> finalCleanup = [=](bool status) -> void
		{
			int32_t angle = NoEjectAngle;
			if ( HawkeyeConfig::Instance().get().automationEnabled && !stageController->IsCarouselPresent() )
			{
				angle = DfltAutomationEjectAngle;
			}

			stageController->EjectStage([=](bool ejectStatus) -> void
			{
				if (!ejectStatus)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : error ejecting stage");
				}
				pServices_->enqueueInternal(cleanupComplete, status);
			}, angle);
		};

		// NOTE: hardware detection cannot discriminate between the use of the A-Cup and Carousel or plate;
		// if the carousel carrier is present, the workflow will behave as if it was the carrier of choice...
		if (stageController->IsCarouselPresent())
		{
			bool status = false;

			// this switch looks for the expected normal success states;
			// the flow will typically get here with those states set on normal closure;
			switch (currentState_)
			{
				case eBrightfieldDustSubtractionState::bds_Idle:
				case eBrightfieldDustSubtractionState::bds_Completed:
					status = true;
					break;

				default:
					status = false;
			}

			if ((currentState_ > eBrightfieldDustSubtractionState::bds_Idle) &&
				(currentState_ < eBrightfieldDustSubtractionState::bds_FindingTube))
			{
				SystemStatus::Instance().decrementSampleTubeCapacityCount();

				stageController->GotoNextTube([=](int32_t tubePos) -> void
				{
					bool tubeStatus = tubePos > 0 && tubePos <= MaxCarouselTubes;
					if (!tubeStatus)
					{
						Logger::L().Log (MODULENAME, severity_level::error, "cleanupBeforeExit : Failed to go to next tube location");
					}
					finalCleanup(tubeStatus);
				});
				return;
			}
			finalCleanup(status);
		}
		else
		{
			finalCleanup(isProbeUp);
		}
	});

	Logger::L().Log (MODULENAME, severity_level::debug1, "cleanupBeforeExit : <exit>");
}

//*****************************************************************************
void BrightfieldDustSubtractWorkflow::reset()
{
	currentState_ = eBrightfieldDustSubtractionState::bds_Idle;

	for (cv::Mat item : acquiredImages_)
	{
		item.release();
	}
	acquiredImages_.clear();

	if (bdsImage_)
	{
		bdsImage_->release();
		bdsImage_.reset();
	}
}