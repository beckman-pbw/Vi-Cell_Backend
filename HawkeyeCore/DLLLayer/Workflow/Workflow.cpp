#include "stdafx.h"

#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "AppendProcessingWorkflowOperation.hpp"
#include "CameraWorkflowOperation.hpp"
#include "CommandParser.hpp"
#include "FileSystemUtilities.hpp"
#include "FocusWorkflowOperation.hpp"
#include "Hardware.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeError.hpp"
#include "LedWorkflowOperation.hpp"
#include "Logger.hpp"
#include "ProbeWorkflowOperation.hpp"
#include "ReagentWorkflowOperation.hpp"
#include "StateChangeWorkflowOperation.hpp"
#include "SyringePumpBase.hpp"
#include "SyringeWorkflowOperation.hpp"
#include "TimestampWorkflowOperation.hpp"
#include "WaitStateWorkflowOperation.hpp"
#include "Workflow.hpp"

static const char MODULENAME[] = "Workflow";

const std::string Workflow::AnalysisCmd = "A";
const std::string Workflow::AnalysisParamEnable = "E";
const std::string Workflow::AnalysisParamFilename = "F";
const std::string Workflow::AnalysisParamDatasetName = "D";
const std::string Workflow::CameraCmd = "C";
const std::string Workflow::CameraParamCapture = "C";
const std::string Workflow::CameraParamTakePicture = "T";
const std::string Workflow::CameraParamSetGain = "G";
const std::string Workflow::CameraParamRemoveGainLimits = "RGL";
const std::string Workflow::CameraAdjustBackgroundIntensity = "ABI";
const std::string Workflow::FocusCmd = "F";
const std::string Workflow::FocusParamHome = "H";
const std::string Workflow::FocusParamAuto = "A";
const std::string Workflow::FocusParamRunAnalysis = "RA";
const std::string Workflow::DirectoryCmd = "D";
const std::string Workflow::LedCmd = "L";
const std::string Workflow::LedParamSet = "S";
const std::string Workflow::LedUse = "U";
const std::string Workflow::RunScriptCmd = "N";
const std::string Workflow::ProbeCmd = "P";
const std::string Workflow::ProbeParamUp = "U";
const std::string Workflow::ProbeParamDown = "D";
const std::string Workflow::ReagentCmd = "R";
const std::string Workflow::ReagentParamUp = "U";
const std::string Workflow::ReagentParamDown = "D";
const std::string Workflow::ReagentParamPurge = "P";
const std::string Workflow::ReagentParamDoor = "L";
const std::string Workflow::ScriptControlRepeatCmd = "REPEAT";
const std::string Workflow::SyringeCmd = "S";
const std::string Workflow::SyringeParamInitialize = "I";
const std::string Workflow::SyringeParamValve = "V";
const std::string Workflow::SyringeParamMove = "M";
const std::string Workflow::SyringeParamRotateValve = "R";
const std::string Workflow::SyringeParamValveCommand = "VC"; 
const std::string Workflow::TimeStampCmd = "TS";
const std::string Workflow::WaitCmd = "W";
const std::string Workflow::WaitForKeyPressCmd = "K";
const std::string Workflow::WorkflowStateChangeCmd = "SC";


//*****************************************************************************
static std::string getWorkflowName(Workflow::Type& type)
{
	switch (type)
	{
		case Workflow::Sample: { return "Sample"; }
		case Workflow::FlushFlowCell: { return "FlushFlowCell"; }
		case Workflow::PrimeReagentLines: { return "PrimeReagentLines"; }
		case Workflow::DecontaminateFlowCell: { return "DecontaminateFlowCell"; }
		case Workflow::PurgeReagentLines: { return "PurgeReagentLines"; }
		case Workflow::DrainReagentPack: { return "DrainReagentPack"; }
		case Workflow::CameraAutofocus: { return "CameraAutofocus"; }
		case Workflow::BrightfieldDustSubtract: { return "BrightfieldDustSubtract"; }
		case Workflow::LoadNudgeExpel: { return "LoadNudgeExpel"; }
		case Workflow::ReagentLoad: { return "ReagentLoad"; }
		case Workflow::ReagentUnload: { return "ReagentUnload"; }

		default:
			throw std::exception((boost::str(boost::format(
				"Invalid or not supported Workflow type : %d") % type)
				).c_str());
	}
	return std::string();
}

//*****************************************************************************
Workflow::Workflow (Workflow::Type type)
	: type_(type)
{
	currentState_ = 0;
	abortOperation_ = false;
	isBusy_ = false;
	currentworkflowOperationsIndex_ = 0;
	workflowName_ = getWorkflowName(type_);
	timer_callback_ = nullptr;
	scriptControlCounts_.clear();
	opCleaningIndex_ = boost::none;
}

//*****************************************************************************
Workflow::~Workflow()
{
	Logger::L().Log (workflowName_, severity_level::normal, "Deleting current Workflow");

	if (Logger::L().IsOfInterest(severity_level::debug3))
	{
		for (const auto& it : workflowOperations_)
		{
			Logger::L().Log (workflowName_, severity_level::debug3, boost::str(boost::format("Deleting %s operation") % it->getTypeAsString()));
		}
	}

	workflowOperations_.clear();
	isBusy_ = false;
}

//*****************************************************************************
HawkeyeError Workflow::load(std::string filename, std::map<std::string, uint32_t> script_control)
{
	setScriptControl(script_control);
	return load(filename);
}

//*****************************************************************************
HawkeyeError Workflow::setScriptControl(std::map < std::string, uint32_t> script_control)
{
	std::swap (scriptControlCounts_, script_control);
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool  Workflow::getNextValidScriptLine (std::istream& stream, std::string& line) const
{
	// Retrieves the next valid (non-blank, non-comment) line from the file
	// Returns false on error/EOF.

	line.clear();

	while (true)
	{
		if (!getline(stream, line))
		{
			Logger::L().Log (workflowName_, severity_level::debug2, "getNextValidScriptLine : unable to fetch new line content!");
			return false;
		}

		boost::algorithm::trim(line);

		// Skip empty lines.
		if (line.length() == 0)
			continue;

		// Skip commented out lines.  Comment character is '!'.
		if (line[0] == '!')
			continue;

		// Got a good line.
		break;
	}

	return !line.empty();
}

//*****************************************************************************
bool Workflow::processLine (const std::string& line, std::istream& stream, boost::filesystem::path fileDirectory)
{
	CommandParser cmdParser;
	cmdParser.parse(",", line);

	std::string cmd = cmdParser.getByIndex(0);
	boost::to_upper(cmd);

	std::stringstream ss;
	ss << "cmd: " << cmd;

	std::vector<std::string> strParams;

	int idx = 1;
	while (true)
	{
		if (cmdParser.hasIndex(idx))
		{
			std::string strTemp = cmdParser.getByIndex(idx);
			boost::algorithm::trim(strTemp);
			strParams.push_back(strTemp);
			ss << ", param" << std::to_string(idx - 1) << ": " << strTemp; // << ", ";
			idx++;
		}
		else
		{
			break;
		}
	}

	Logger::L().Log (workflowName_, severity_level::debug1, ss.str());

	// Repeats: Need to pull the next N lines out and process them Y times.
	if (cmd == Workflow::ScriptControlRepeatCmd)
	{
		// Param 1: Lines; param 2: Iterations
		if (strParams.size() != 2)
		{
			Logger::L().Log (workflowName_, severity_level::error, "Wrong number of arguments to FlowControlRepeat command");
			return false;
		}
		uint32_t linecount = std::stoul(strParams[0]);
		std::string srepetitionHelper = strParams[1];
		uint32_t repetitionCount = 1;
		if ((srepetitionHelper.at(0) >= '0') && (srepetitionHelper.at(0) <= '9'))
		{
			repetitionCount = stoul(srepetitionHelper);
		}
		else
		{
			// Ensure that we have a parameter match for this label
			if (scriptControlCounts_.find(srepetitionHelper) == scriptControlCounts_.end())
			{
				Logger::L().Log (workflowName_, severity_level::error, "No value provided for loop label \"" + srepetitionHelper + "\"");
				return false;
			}
			repetitionCount = scriptControlCounts_[srepetitionHelper];
		}

		std::vector<std::string> nextLines;

		// Load next lines.
		for (uint32_t i = 0; i < linecount; i++)
		{
			std::string line;
			if (!getNextValidScriptLine(stream, line))
			{
				Logger::L().Log (workflowName_, severity_level::error, "Unable to collect enough lines to fulfill the loop request in \"" + line + "\"");
				return false;
			}

			nextLines.push_back (line);
		}

		// Process the lines as required.
		for (uint32_t i = 0; i < repetitionCount; i++)
		{
			for (auto l : nextLines)
			{
				if (!processLine(l, stream, fileDirectory))
				{
					Logger::L().Log (workflowName_, severity_level::error, "Failure while processing script line \"" + l + "\" during Repeat command.");
					return false;
				}
			}
		}

	}

	// RunScript: Load script and insert at this point; then continue.
	if (cmd == Workflow::RunScriptCmd)
	{
		std::string dir = fileDirectory.empty() ? "" : fileDirectory.string() + "\\";
		return (loadInternal(dir + strParams[0]) == HawkeyeError::eSuccess);
	}

	return doLoad (cmd, strParams);
}

//*****************************************************************************
HawkeyeError Workflow::load(std::string filename)
{
	return loadInternal(filename);
}

//*****************************************************************************
// Non virtual protected method to load and decode script file
//NOTE: this code has been copied to CellHealthReagents.cpp and refactored to
// determine the volume of each reagent required for sampling from the ACup.
// Any changes made to this code "may" also need to be made to the
// CellHealthReagents.cpp code.
//*****************************************************************************
HawkeyeError Workflow::loadInternal (std::string filename)
{
	Logger::L().Log (workflowName_, severity_level::normal, "<enter, opening workflow script: " + filename + ">");

	if (isBusy())
	{
		Logger::L().Log (workflowName_, severity_level::error, "system is currently busy running other workflow!");
		return HawkeyeError::eBusy;
	}

	std::stringstream textStream;
	if (!HDA_ReadEncryptedTextFile (filename, textStream))
	{
		Logger::L().Log (workflowName_, severity_level::error, "<exit, failed to decrypt the Workflow script: " + filename + ">");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::workflow_script,
			instrument_error::severity_level::error));
		return HawkeyeError::eStorageFault;
	}

	boost::filesystem::path p(filename);
	auto fileDirectory = p.parent_path();

	std::string line;
	while (getNextValidScriptLine(textStream, line))
	{
		if (!processLine(line, textStream, fileDirectory))
		{
			Logger::L().Log (workflowName_, severity_level::error, "failed to process workflow command: " + line);
			return HawkeyeError::eValidationFailed;
		}

	} // while getNextValidLine

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool Workflow::doLoad (std::string cmd, std::vector<std::string> strParams)
{
	std::unique_ptr<WorkflowOperation> wfOp;
	if (cmd == Workflow::SyringeCmd)
	{
		if (strParams[0] == Workflow::SyringeParamInitialize)
		{
			Logger::L().Log (workflowName_, severity_level::notification, "*** SYRINGE INITAILIZE COMMAND IS UNSUPPORTED ***");
			//wfOp = SyringeWorkflowOperation::BuildInitializeCommand();
		}
		else if (strParams[0] == Workflow::SyringeParamValve)
		{
			SyringePumpPort port = SyringePumpPort::ParamToPort(std::stoul(strParams[1]));
			SyringePumpDirection direction = SyringePumpBase::ParamToDirection(std::stoul(strParams[2]));
			wfOp = SyringeWorkflowOperation::BuildSetValveCommand (port, direction);
		}
		else if (strParams[0] == Workflow::SyringeParamMove)
		{
			wfOp = SyringeWorkflowOperation::BuildMoveCommand (std::stoul(strParams[1]), std::stoul(strParams[2]));
		}
		else if (strParams[0] == Workflow::SyringeParamRotateValve)
		{
			SyringePumpDirection direction = SyringePumpBase::ParamToDirection(std::stoul(strParams[2]));
			wfOp = SyringeWorkflowOperation::BuildRotateValveCommand (std::stoul(strParams[1]), direction);
		}
		else if (boost::to_upper_copy(strParams[0]) == Workflow::SyringeParamValveCommand)
		{
			wfOp = SyringeWorkflowOperation::BuildSendValveCommand (strParams[1]);
		}
	}
	else if (cmd == Workflow::CameraCmd)
	{
		if (strParams[0] == CameraParamSetGain)
		{
			wfOp = CameraWorkflowOperation::BuildSetGainCommand (std::stod(strParams[1]));
		}
		else if (strParams[0] == Workflow::CameraParamTakePicture)
		{
			uint32_t temp = std::stoul(strParams[1]);
			HawkeyeConfig::LedType ledType = (HawkeyeConfig::LedType)temp;
			wfOp = CameraWorkflowOperation::BuildTakePictureCommand (
				ledType, std::stoul(strParams[2]), std::stoul(strParams[3]), getCameraCallback());
		}
		else if (strParams[0] == Workflow::CameraParamCapture)
		{
			uint32_t temp = std::stoul(strParams[4]);
			HawkeyeConfig::LedType ledType = (HawkeyeConfig::LedType)temp;

			// Check if the frame rate has been provided.
			uint8_t frameRate = DefaultFrameRate;
			if (strParams.size() == 8)
			{
				frameRate = static_cast<uint8_t>(std::stoul(strParams[7]));
			}

			wfOp = CameraWorkflowOperation::BuildCaptureCommand(
				std::stoul(strParams[1]),
				std::stoul(strParams[2]),
				std::stoul(strParams[3]),
				ledType,
				static_cast<uint16_t>(std::stoul(strParams[5])),
				static_cast<uint16_t>(std::stoul(strParams[6])),
				frameRate,
				getCameraCallback());
		}
		else if (strParams[0] == Workflow::CameraParamRemoveGainLimits)
		{
			wfOp = CameraWorkflowOperation::BuildRemoveGainLimitsCommand (std::stoul(strParams[1]) != 0 ? true : false);
		}
		else if (strParams[0] == Workflow::CameraAdjustBackgroundIntensity)
		{
			uint32_t temp = std::stoul(strParams[4]);
			HawkeyeConfig::LedType ledType = (HawkeyeConfig::LedType)temp;
			wfOp = CameraWorkflowOperation::BuildCameraAdjustBackgroundIntensityCommand(
				std::stoul(strParams[1]),
				std::stoul(strParams[2]),
				std::stoul(strParams[3]),
				ledType,
				std::stoul(strParams[5]),
				std::stoul(strParams[6])
			);
		}
	}
	else if (cmd == Workflow::FocusCmd)
	{
		if (strParams[0] == Workflow::FocusParamHome)
		{
			wfOp = FocusWorkflowOperation::BuildHomeCommand();
		}
		else if (strParams[0] == Workflow::FocusParamAuto)
		{
			uint32_t temp = std::stoul(strParams[1]);
			HawkeyeConfig::LedType ledType = (HawkeyeConfig::LedType)temp;
			wfOp = FocusWorkflowOperation::BuildAutoCommand (ledType, std::stoul(strParams[2]));
		}
		else if (strParams[0] == Workflow::FocusParamRunAnalysis)
		{
			wfOp = FocusWorkflowOperation::BuildRunAnalysisCommand();
		}
		else
		{
			Logger::L().Log (workflowName_, severity_level::error, "Currently unsupported Focus command: " + strParams[0]);
		}
	}
	else if (cmd == Workflow::LedCmd)
	{
		if (strParams[0] == Workflow::LedParamSet)
		{
			HawkeyeConfig::LedType ledType = (HawkeyeConfig::LedType) std::stoul(strParams[1]);

			// L, S, type, power, simmer, ltcd, ctld, photodiodenum
			wfOp = LedWorkflowOperation::BuildSetCommand(
				ledType,
				std::stof(strParams[2]),
				std::stoul(strParams[3]),
				std::stoul(strParams[4]),
				std::stoul(strParams[5]),
				std::stoul(strParams[6])
			);

		} else if (strParams[0] == Workflow::LedUse) {

			// L, U, <LED_name>
			HawkeyeConfig::LedConfig_t& ledConfig = HawkeyeConfig::Instance().getLedConfigByPositionName (strParams[1]);
			wfOp = LedWorkflowOperation::BuildSetCommand(
				HawkeyeConfig::Instance().positionToLedType(strParams[1]),
				ledConfig.percentPower,
				ledConfig.simmerCurrentVoltage,
				ledConfig.ltcd,
				ledConfig.ctld,
				ledConfig.feedbackPhotodiode);

		} else {
			Logger::L().Log (workflowName_, severity_level::error, "Unknown LED command: " + strParams[0]);
		}
	}
	else if (cmd == Workflow::ReagentCmd)
	{
		Logger::L().Log (workflowName_, severity_level::warning, "Reagent Probe  motor operation is NOT available.");

		if ((strParams[0] == "0") || (strParams[0] == Workflow::ReagentParamUp))
		{
			wfOp = ReagentWorkflowOperation::BuildUpCommand();
		}
		else if ((strParams[0] == "1") || (strParams[0] == Workflow::ReagentParamDown))
		{
			wfOp = ReagentWorkflowOperation::BuildDownCommand();
		}
		else if ((strParams[0] == "2") || (strParams[0] == Workflow::ReagentParamPurge))
		{
			wfOp = ReagentWorkflowOperation::BuildPurgeCommand();
		}
		else if ((strParams[0] == "3") || (strParams[0] == Workflow::ReagentParamDoor))
		{
			wfOp = ReagentWorkflowOperation::BuildDoorLatchCommand();
		}
	}
	else if (cmd == Workflow::DirectoryCmd)
	{
		imageDirectoryPath_ = strParams[0];
		boost::algorithm::trim(imageDirectoryPath_);

		// Make sure that there is a "/" at the end of the string.
		if (imageDirectoryPath_[imageDirectoryPath_.length() - 1] != '/')
		{
			imageDirectoryPath_ += '/';
		}

		// Create the root directory path.
		boost::filesystem::path p(imageDirectoryPath_);
		boost::system::error_code ec;
		boost::filesystem::create_directories(p, ec);
		if (ec == boost::system::errc::success)
		{
			Logger::L().Log (workflowName_, severity_level::normal, "created directory: " + p.generic_string());
		}
		else
		{
			Logger::L().Log (workflowName_, severity_level::error, "failed to create directory: " + p.generic_string() + " : " + ec.message());
			return false;
		}
	}
	else if (cmd == Workflow::WorkflowStateChangeCmd)
	{
		auto wfs = (uint16_t)std::stoul(strParams[0]);
		auto cb = [this](auto ...args) { this->_onWorkflowStateChanged_impl(args...); };
		wfOp = StateChangeWorkflowOperation::Build(cb, wfs);
	}
	else if (cmd == Workflow::WaitCmd)
	{
		auto waitMs = (uint32_t)std::stoul(strParams[0]);
		wfOp = WaitStateWorkflowOperation::Build(waitMs, timer_callback_);
	}
	else if (cmd == Workflow::ProbeCmd)
	{
		if ((strParams[0] == "0") || (strParams[0] == Workflow::ProbeParamUp))
		{
			wfOp = ProbeWorkflowOperation::BuildProbeUpCommand();
		}
		else if ((strParams[0] == "1") || (strParams[0] == Workflow::ProbeParamDown))
		{
			wfOp = ProbeWorkflowOperation::BuildProbeDownCommand();
		}
		else
		{
			Logger::L().Log (workflowName_, severity_level::error, "Unknown Probe command: " + strParams[0]);
		}
	}
	else if (cmd == Workflow::ScriptControlRepeatCmd)
	{
		// This is added to prevent the *UNSUPPORTED SCRIPT COMMAND* from being logged.
	}
	else if (cmd == Workflow::TimeStampCmd)
	{
		const auto comment = strParams[0];
		wfOp = TimestampWorkflowOperation::Build(comment);
	}
	else
	{
		Logger::L().Log (workflowName_, severity_level::warning, "*** UNSUPPORTED SCRIPT COMMAND: " + cmd + " ***");
	}

	if (wfOp)
	{
		workflowOperations_.push_back(std::move(wfOp));
	}
	return true;
}

//*****************************************************************************
void Workflow::_onWorkflowStateChanged_impl(uint16_t currentState, bool executionComplete)
{
	currentState_ = currentState;
	onWorkflowStateChanged(currentState, executionComplete);
}

//*****************************************************************************
HawkeyeError Workflow::abortExecute()
{
	Logger::L().Log (workflowName_, severity_level::warning, "*** Aborting Running Operation ***");

	abortOperation_ = true;
	if ( !skipCurrentRunningState () )
		return HawkeyeError::eNotPermittedAtThisTime;

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError Workflow::execute()
{
	if (isBusy())
	{
		return HawkeyeError::eBusy;
	}

	if (workflowOperations_.empty())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	pServices_->enqueueInternal([this]()
	{
		setBusyStatus (true);

		executeInternalAsync (0, workflowOperations_.size(), [this](HawkeyeError he)
		{
			triggerCompletionHandler(he);
		});
	});

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError Workflow::execute (uint16_t jumpToState)
{
	if (isBusy())
	{
		return HawkeyeError::eBusy;
	}

	if (workflowOperations_.empty())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	pServices_->enqueueInternal ([this, jumpToState]() -> void
	{
		setBusyStatus(true);
		auto startIndex = getWorkflowOperationIndex(jumpToState);
		executeInternalAsync (startIndex, workflowOperations_.size(), [this](HawkeyeError he)
		{
			triggerCompletionHandler(he);
		});
	});

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
size_t Workflow::getWorkflowOperationIndex(uint16_t jumpToState)
{
	for (size_t index = 0; index < workflowOperations_.size(); index++)
	{
		const auto& workflowOp = workflowOperations_[index];
		if (workflowOp->getType() != WorkflowOperation::Type::StateChange)
		{
			continue;
		}

		uint16_t stateNum = ((StateChangeWorkflowOperation*)workflowOp.get())->getStateNumber();
		if (jumpToState == stateNum)
		{
			return index;
		}
	}

	return std::numeric_limits<size_t>::max();
}

//*****************************************************************************
void Workflow::executeInternalAsync (size_t startIndex, size_t endIndex, std::function<void(HawkeyeError)> onComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	if (startIndex > endIndex || startIndex >= workflowOperations_.size())
	{
		pServices_->enqueueInternal (onComplete, HawkeyeError::eSuccess);
		return;
	}

	if (abortOperation_.load())
	{
		Logger::L().Log (workflowName_, severity_level::normal, boost::str(boost::format("Aborting the current workflow processing as requested!")));
		abortOperation_ = false; // Reset the abort operation flag to false
		cleaningOnAbort(currentworkflowOperationsIndex_, endIndex, onComplete);
		return;
	}

	const auto& workflowOp = workflowOperations_[startIndex];
	currentworkflowOperationsIndex_ = startIndex;
//	Logger::L().Log (workflowName_, severity_level::debug1, boost::str(boost::format("Executing %s operation...") % workflowOp->getTypeAsString()));

#ifdef _DEBUG
	Sleep (100);	// Mimic hardware action...
#endif

	workflowOp->executeAsync (pServices_, std::bind(&Workflow::onWorkflowOpComplete, this, std::placeholders::_1, startIndex, endIndex, onComplete));
}

//*****************************************************************************
void Workflow::onWorkflowOpComplete (HawkeyeError he, size_t startIndex, size_t endIndex, std::function<void(HawkeyeError)> onComplete)
{
	if (he == HawkeyeError::eSuccess)
	{
		pServices_->enqueueInternal (std::bind(&Workflow::executeInternalAsync, this, (startIndex + 1), endIndex, onComplete));
	}
	else
	{
		auto onCleaningComplete = [this, onComplete, he, startIndex](HawkeyeError cleaningError)
		{
			if (cleaningError != HawkeyeError::eSuccess)
			{
				Logger::L().Log (workflowName_, severity_level::error, "onWorkflowOpComplete : Cleaning cycle on failure also failed to complete : " + DataConversion::enumToRawValueString(cleaningError));
			}

			Logger::L().Log (workflowName_, severity_level::error, boost::str(boost::format("Unexpected error occured during operation index %d! Hawkeye Error = %d") % startIndex % (uint32_t)he));
			// trigger callback with original error "he"
			pServices_->enqueueInternal(onComplete, he);
		};

		if (executeCleaningCycleOnTermination(startIndex, endIndex, onCleaningComplete))
		{
			// return since cleaning operation will execute
			return;
		}

		Logger::L().Log (workflowName_, severity_level::error, boost::str(boost::format("Unexpected error occured during operation index %d! Hawkeye Error = %d") % startIndex % (uint32_t)he));
		pServices_->enqueueInternal(onComplete, he);
	}
}

//*****************************************************************************
void Workflow::appendProcessing (std::function<ICompletionHandler*()> appendWorklambda)
{
	auto wfOp = AppendProcessingWorkflowOperation::Build (appendWorklambda);
	workflowOperations_.insert (workflowOperations_.begin() + (currentworkflowOperationsIndex_ + 1), std::move(wfOp));
}

//*****************************************************************************
void Workflow::setBusyStatus(bool busy)
{
	isBusy_ = busy;
}

//*****************************************************************************
bool Workflow::skipCurrentRunningState()
{
	Logger::L().Log (workflowName_, severity_level::warning, "*** Skip Currently Running Operation ***");

	const unsigned long long opIndex = currentworkflowOperationsIndex_.load();
	if (opIndex < workflowOperations_.size())
	{
		const auto& workflowOp = workflowOperations_[opIndex];
		Logger::L().Log (workflowName_, severity_level::normal, boost::str(boost::format("Skipping %s operation...") % workflowOp->getTypeAsString()));
		return workflowOp->cancelOperation();
	}
	
	return false;
}

//*****************************************************************************
void Workflow::registerCompletionHandler(Workflow_Callback cb)
{
	wf_cb_ = cb;
}

//*****************************************************************************
void Workflow::triggerCompletionHandler(HawkeyeError he)
{
	if (wf_cb_ != nullptr)
	{
		pServices_->enqueueExternal (wf_cb_, he);
	}
	
	setBusyStatus(false);
}

//*****************************************************************************
void Workflow::getTotalFluidVolumeUsage (std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap, boost::optional<uint16_t> jumpToState)
{
	syringeVolumeMap.clear();
	if (workflowOperations_.empty())
	{
		return;
	}

	size_t startIndex = 0;
	if (jumpToState)
	{
		startIndex = getWorkflowOperationIndex(jumpToState.get());
	}

	getTotalFluidVolumeUsageInternal (startIndex, workflowOperations_.size(), syringeVolumeMap);
}

//*****************************************************************************
void Workflow::getTotalFluidVolumeUsageInternal (size_t startIndex, size_t endIndex, std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap)
{
	syringeVolumeMap.clear();
	if (startIndex >= endIndex || startIndex >= workflowOperations_.size())
	{
		return;
	}

	uint32_t syringe_current_volume = 0;
	boost::optional<SyringePumpPort::Port> currentPort = boost::none;

	for (size_t index = startIndex; index < endIndex; index++)
	{
		if (index >= workflowOperations_.size())
		{
			syringeVolumeMap.clear();
			break;
		}

		const auto& wfOp = workflowOperations_[index];
		if (!wfOp || wfOp->getType() != WorkflowOperation::Type::SyringePump)
		{
			continue;
		}

		if (wfOp->getOperation() == static_cast<uint8_t>(SyringeWorkflowOperation::SyringeOperation::SetValve))
		{
			auto port = ((SyringeWorkflowOperation*)wfOp.get())->getPort().Get();
			currentPort.reset(port);
			if (syringeVolumeMap.find(*currentPort) == syringeVolumeMap.end())
			{
				syringeVolumeMap[*currentPort] = 0;
			}
			continue;
		}
		
		if (wfOp->getOperation() == static_cast<uint8_t>(SyringeWorkflowOperation::SyringeOperation::Move))
		{
			if (!currentPort)
			{
				continue;
			}
			
			const auto tgtVolume = ((SyringeWorkflowOperation*)wfOp.get())->getTargetVolume();
			if (tgtVolume > syringe_current_volume) // Check for aspiration
			{
				syringeVolumeMap[*currentPort] += tgtVolume - syringe_current_volume;
			}
			syringe_current_volume = tgtVolume;
		}
	}
}

//*****************************************************************************
void Workflow::emptySyringeAtBeginning (SyringePumpDirection::Direction direction, uint32_t speed)
{
	bool isSyringeEmpty = false;
	uint32_t syringePos = 0;
	if (Hardware::Instance().getSyringePump()->getPosition(syringePos))
	{
		isSyringeEmpty = 0 == syringePos;
	}

	if (isSyringeEmpty)
	{
		Logger::L().Log (workflowName_, severity_level::debug1, "emptySyringeAtBeginning : syringe is already empty");
		return;
	}

	for (auto it = workflowOperations_.begin(); it != workflowOperations_.end(); ++it)
	{
		const auto& op = *it;
		if (op->getType() != WorkflowOperation::Type::SyringePump)
			continue;

		// If Syringe initialize command is present then add workflow operation after initialize
		if (op->getOperation() == static_cast<uint8_t>(SyringeWorkflowOperation::SyringeOperation::Initialize))
		{
			++it;
		}

		// Add workflow operation : Move syringe to waste
		auto setCmd = SyringeWorkflowOperation::BuildSetValveCommand (SyringePumpPort::Port::Waste, direction);
		it = workflowOperations_.insert(it, std::move(setCmd));

		// increment to add after the current index
		++it;

		// Add workflow operation : Empty syringe
		auto moveCmd = SyringeWorkflowOperation::BuildMoveCommand (0, speed);
		it = workflowOperations_.insert(it, std::move(moveCmd));

		break;
	}
}

//*****************************************************************************
void Workflow::cleaningOnAbort (size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onCleaningComplete);

	Logger::L().Log (workflowName_, severity_level::normal, "cleaningOnAbort<base> : " + workflowName_);

	if (executeCleaningCycleOnTermination(currentWfOpIndex, endIndex, onCleaningComplete))
	{
		// return since cleaning operation will execute
		return;
	}
	pServices_->enqueueInternal(onCleaningComplete, HawkeyeError::eSuccess);
	return;
}

//*****************************************************************************
bool Workflow::executeCleaningCycleOnTermination (size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onCleaningComplete);

	// Workflow has failed, try running cleaning operation here if possible
	if (!opCleaningIndex_)
	{
		Logger::L().Log (workflowName_, severity_level::debug1, "executeCleaningCycleOnTermination : Cleaning indices not set!");
		return false;
	}

	if (opCleaningIndex_->first <= currentWfOpIndex
		|| opCleaningIndex_->second > endIndex)
	{
		Logger::L().Log (workflowName_, severity_level::debug1, "executeCleaningCycleOnTermination : Unable to execute cleaning cycle!");
		return false;
	}

	// Check if we have executed any syringe move operation till now
	bool foundSyringeMoveOp = false;
	for (size_t index = 0; index <= currentWfOpIndex; index++)
	{
		if (index < workflowOperations_.size())
		{
			const auto& wfo = workflowOperations_[index];
			if(wfo->getType() != WorkflowOperation::Type::SyringePump)
				continue;
			if (wfo->getOperation() != static_cast<uint8_t>(SyringeWorkflowOperation::SyringeOperation::Move))
				continue;

			foundSyringeMoveOp = true;
			break;
		}
	}

	// If no syringe move operation has been executed then no need to clean
	if (!foundSyringeMoveOp)
	{
		Logger::L().Log (workflowName_, severity_level::debug1, "executeCleaningCycleOnTermination : No syringe move operation was done before workflow failed!");
		return false;
	}

	// If tube is not present that means carousel position is changed
	if (Hardware::Instance().getStageController()->IsCarouselPresent()
		&& !Hardware::Instance().getStageController()->IsTubePresent())
	{
		Logger::L().Log (workflowName_, severity_level::debug1, "executeCleaningCycleOnTermination : No tube present for current carousel position");
		return false;
	}

	size_t indexToInsert = opCleaningIndex_->first;

	// Probe has to be down first to make cleaning success
	if (!Hardware::Instance().getStageController()->IsProbeDown())
	{
		Logger::L().Log (workflowName_, severity_level::debug1, "executeCleaningCycleOnTermination : Adding workflow operation to do probe down first");

		// Add workflow operation : Probe down
		auto downCmd = ProbeWorkflowOperation::BuildProbeDownCommand();
		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(downCmd));

		// increment to add after the current index
		indexToInsert++;
	}

	// If syringe is not empty, then move all the fluid to waste first
	bool isSyringeEmpty = false;
	uint32_t syringePos = 0;
	if (Hardware::Instance().getSyringePump()->getPosition(syringePos))
	{
		isSyringeEmpty = 0 == syringePos;
	}
	if (!isSyringeEmpty)
	{
		// Add workflow operation : Move syringe to waste
		auto setCmd = SyringeWorkflowOperation::BuildSetValveCommand (SyringePumpPort::Port::Waste, SyringePumpDirection::Direction::Clockwise);
		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(setCmd));

		// increment to add after the current index
		indexToInsert++;

		// Add workflow operation : Empty syringe
		auto moveCmd = SyringeWorkflowOperation::BuildMoveCommand (0, 400);	// uint32_t speed = 400
		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(moveCmd));

		// increment to add after the current index
		indexToInsert++;
	}

	// Clear sample tube fluid
	{
		
//TODO: is this code CHM specific ???
		// Add workflow operation : Move syringe to waste
		auto setCmd = SyringeWorkflowOperation::BuildSetValveCommand (SyringePumpPort::Port::CHM_ACup, SyringePumpDirection::Direction::Clockwise);

//TODO: this is from Vi-Cell codebase:
//		auto setCmd = SyringeWorkflowOperation::BuildSetValveCommand(SyringePumpPort::Port::Sample, SyringePumpDirection::Direction::Clockwise);


		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(setCmd));

		// increment to add after the current index
		indexToInsert++;

		// Add workflow operation : Aspirate 600 volume from sample tube
		auto moveCmd = SyringeWorkflowOperation::BuildMoveCommand (600, 400);	// uint32_t speed = 400
		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(moveCmd));

		// increment to add after the current index
		indexToInsert++;

		// Add workflow operation : Move syringe to waste
		setCmd = SyringeWorkflowOperation::BuildSetValveCommand (SyringePumpPort::Port::Waste, SyringePumpDirection::Direction::Clockwise);
		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(setCmd));

		// increment to add after the current index
		indexToInsert++;

		// Add workflow operation : Empty syringe
		moveCmd = SyringeWorkflowOperation::BuildMoveCommand (0, 400);	// uint32_t speed = 400
		workflowOperations_.insert(workflowOperations_.begin() + indexToInsert, std::move(moveCmd));

		// increment to add after the current index
		indexToInsert++;
	}

	// Adjust cleaning end index after adding new workflow operations
	size_t difference = indexToInsert - opCleaningIndex_->first;
	opCleaningIndex_->second += difference;

	Logger::L().Log (workflowName_, severity_level::debug1, boost::str(boost::format("Executing cleaning instruction with startIndex<%d> and endIndex<%d>") % opCleaningIndex_->first % opCleaningIndex_->second));
	pServices_->enqueueInternal (std::bind(&Workflow::executeInternalAsync, this, opCleaningIndex_->first, opCleaningIndex_->second, onCleaningComplete));

	// Reset the indices to none so that cleaning doesn't happen again
	opCleaningIndex_ = boost::none;

	return true;
}

//*****************************************************************************
void Workflow::setImageCount(uint16_t numImages)
{
	// Find the CameraCapture operation in the Workflow collection and set the # of images to take.
	for (const auto& it : workflowOperations_)
	{
		if (it->getType() != WorkflowOperation::Type::Camera)
			continue;
		if (it->getOperation() != static_cast<uint8_t>(CameraWorkflowOperation::CameraOperation::Capture))
			continue;

		((CameraWorkflowOperation*)it.get())->setImageCount(numImages);
	}
}

//*****************************************************************************
uint16_t Workflow::getImageCount()
{
	// Find the CameraCapture operation in the Workflow collection and get the # of images to take.
	for (const auto& it : workflowOperations_)
	{
		if (it->getType() != WorkflowOperation::Type::Camera)
			continue;
		if (it->getOperation() != static_cast<uint8_t>(CameraWorkflowOperation::CameraOperation::Capture))
			continue;

		return ((CameraWorkflowOperation*)it.get())->getImageCount();
	}
	return 0;
}
