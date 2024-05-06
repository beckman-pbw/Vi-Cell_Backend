#pragma once

#include <stdint.h>
#include <atomic>

#include <opencv2/opencv.hpp>

#include "CompletionHandlerUtilities.hpp"
#include "DeadlineTimerUtilities.hpp"
#include "HawkeyeDirectory.hpp"
#include "SyringePumpPort.hpp"
#include "WorkflowOperation.hpp"

const uint32_t AutoFocusDuration = 45;	// use to determine time required on reagent pack; 30 minutes settling; ~10 minutes focus detect; 2-3 minutes flush

class Workflow
{
public:
	static const std::string AnalysisCmd;
	static const std::string AnalysisParamEnable;
	static const std::string AnalysisParamFilename;
	static const std::string AnalysisParamDatasetName;
	static const std::string CameraCmd;
	static const std::string CameraParamCapture;
	static const std::string CameraParamTakePicture;
	static const std::string CameraParamSetGain;
	static const std::string CameraParamRemoveGainLimits;
	static const std::string CameraAdjustBackgroundIntensity;
	static const std::string FocusCmd;
	static const std::string FocusParamHome;
	static const std::string FocusParamAuto;
	static const std::string FocusParamRunAnalysis;
	static const std::string DataCmd;
	static const std::string DirectoryCmd;
	static const std::string LedCmd;
	static const std::string LedParamSet;
	static const std::string LedUse;
	static const std::string RunScriptCmd;
	static const std::string ProbeCmd;
	static const std::string ProbeParamUp;
	static const std::string ProbeParamDown;
	static const std::string ReagentCmd;
	static const std::string ReagentParamUp;
	static const std::string ReagentParamDown;
	static const std::string ReagentParamPurge;
	static const std::string ReagentParamDoor;
	static const std::string ScriptControlRepeatCmd;
	static const std::string SyringeCmd;
	static const std::string SyringeParamInitialize;
	static const std::string SyringeParamValve;
	static const std::string SyringeParamMove;
	static const std::string SyringeParamRotateValve;
	static const std::string SyringeParamValveCommand;
	static const std::string TimeStampCmd;
	static const std::string TubeCmd;
	static const std::string TubeParamSetHome;
	static const std::string TubeParamHome;
	static const std::string TubeParamCarouselTube;
	static const std::string TubeParamPlateTube;
	static const std::string StopCmd;
	static const std::string WaitCmd;
	static const std::string WaitForKeyPressCmd;
	static const std::string WorkflowStateChangeCmd;

	typedef enum Type : uint8_t
	{
		Sample = 0,
		FlushFlowCell,
		PrimeReagentLines,
		PurgeReagentLines,
		DrainReagentPack,
		DecontaminateFlowCell,
		CameraAutofocus,
		BrightfieldDustSubtract,
		LoadNudgeExpel,
		ReagentLoad,
		ReagentUnload,
	} Type;

	virtual ~Workflow();
	
	virtual HawkeyeError load (std::string filename);
	virtual HawkeyeError load (std::string filename, std::map<std::string, uint32_t> script_control);
	virtual HawkeyeError setScriptControl (std::map<std::string, uint32_t> script_control);

	virtual HawkeyeError execute();
	virtual HawkeyeError execute(uint16_t jumpToState);

	virtual std::string& getImageDirectoryPath() { return imageDirectoryPath_; }
	void setImageCount (uint16_t numImages);
	uint16_t getImageCount();

	void setServices (std::shared_ptr<HawkeyeServices> pServices) {
		pServices_ = pServices;
	}

	virtual HawkeyeError abortExecute();
	virtual bool skipCurrentRunningState();

	virtual uint16_t getCurrentState() { return currentState_; }
	virtual bool isBusy() const { return isBusy_.load(); }
	virtual Type getType() const { return type_; }
	virtual std::string getNameAsString() const { return workflowName_; }

	virtual std::string getWorkFlowScriptFile(uint8_t index) = 0;

	virtual void registerCompletionHandler(Workflow_Callback cb);

	virtual void getTotalFluidVolumeUsage(
		std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
		boost::optional<uint16_t> jumpToState = boost::none);
	
	virtual bool canValidateReagent() const = 0;

protected:

	Workflow (Workflow::Type type);

	virtual void appendProcessing(std::function<ICompletionHandler*()> appendWorklambda);
	virtual void setBusyStatus(bool busy);
	virtual void triggerCompletionHandler(HawkeyeError he);

	size_t getWorkflowOperationIndex(uint16_t jumpToState);

	void getTotalFluidVolumeUsageInternal(
		size_t startIndex, size_t endIndex,
		std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap);

	virtual void emptySyringeAtBeginning(
		SyringePumpDirection::Direction direction = SyringePumpDirection::Direction::Clockwise,
		uint32_t speed = 600);

	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete)
	{ /* no-op by default */ }

	virtual std::function<void(cv::Mat)> getCameraCallback() { return nullptr; }
	virtual void cleaningOnAbort(size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete);
	virtual bool executeCleaningCycleOnTermination(
		size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete);

	std::vector<std::unique_ptr<WorkflowOperation>> workflowOperations_;
	uint16_t currentState_;
	std::atomic<bool> abortOperation_;
	timer_countdown_callback timer_callback_;
	Workflow_Callback wf_cb_;
	boost::optional<std::pair<size_t, size_t>> opCleaningIndex_;
	std::shared_ptr<HawkeyeServices> pServices_;

private:
	HawkeyeError loadInternal(std::string filename);
	bool doLoad (std::string cmd, std::vector<std::string> strParams);
	bool getNextValidScriptLine(std::istream& stream, std::string& line) const;
	bool processLine (const std::string& line, std::istream& stream, boost::filesystem::path parentDirectory);
	void _onWorkflowStateChanged_impl(uint16_t currentState, bool executionComplete);

	void executeInternalAsync(size_t startIndex, size_t endIndex, std::function<void(HawkeyeError)> onComplete);
	void onWorkflowOpComplete(HawkeyeError he, size_t startIndex, size_t endIndex, std::function<void(HawkeyeError)> onComplete);
	
	Type type_;
	std::string imageDirectoryPath_;
	std::atomic<bool> isBusy_;
	std::atomic<size_t> currentworkflowOperationsIndex_;
	std::map<std::string, uint32_t> scriptControlCounts_;
	std::string workflowName_;
};
