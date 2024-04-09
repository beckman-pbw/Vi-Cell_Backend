#pragma once

#include "ChronoUtilities.hpp"
#include "SampleDefinition.hpp"
#include "Workflow.hpp"

class SampleWorkflow : public Workflow
{
public:
	enum eSampleWorkflowSubType : uint16_t
	{
		eNormalWash = 0,	// The first two here coorespond to the types in eSamplePostWash.
		eFastWash,
		eACup,
		eACupNoInternalDilution,
	};

	SampleWorkflow(
		std::function<void(cv::Mat)> cameraCaptureCallback,
		std::function<void(eSampleStatus, bool)> callback);

	virtual HawkeyeError execute() override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual void getTotalFluidVolumeUsage(
		std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
		boost::optional<uint16_t> jumpToState) override;
	virtual HawkeyeError load (std::string filename) override;
	virtual bool canValidateReagent() const override { return true; }

	static bool verifySampleTiming (const system_TP& startTime, const system_TP& endTime, const uint32_t maxAllowedTimeInSec);
	static void UseACup();

protected:

	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged (uint16_t currentState, bool executionComplete) override;
	virtual std::function<void(cv::Mat)> getCameraCallback() override;

private:
	void setCleaningCycleIndices();
	void cleanupBeforeExit(std::function<void(bool)> callback);

	std::function<void(cv::Mat)> cameraCaptureCallback_;
	std::function<void(eSampleStatus, bool)> callback_;
	system_TP startTime_;
};

