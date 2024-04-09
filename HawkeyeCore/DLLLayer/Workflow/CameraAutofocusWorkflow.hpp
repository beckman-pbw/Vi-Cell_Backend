#pragma once

#include "AutofocusDLL.hpp"
#include "SamplePositionDLL.hpp"
#include "Workflow.hpp"

class CameraAutofocusWorkflow : public Workflow
{
public:
	typedef std::function<void (eAutofocusState, std::shared_ptr<AutofocusResultsDLL>)> autofocus_state_callback_t_DLL;
	typedef std::function<void (uint32_t)> countdown_timer_callback_t_DLL;

	CameraAutofocusWorkflow(
		SamplePositionDLL focusbead_location,
		countdown_timer_callback_t_DLL timer_cb,
		autofocus_state_callback_t_DLL callback);
	virtual ~CameraAutofocusWorkflow();

	virtual HawkeyeError execute() override;
	virtual HawkeyeError execute(uint16_t jumpToState) override;
	virtual HawkeyeError abortExecute() override;
	virtual bool skipCurrentRunningState() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual HawkeyeError load (std::string filename) override;
	virtual bool canValidateReagent() const override { return true; }
	std::shared_ptr<AutofocusResultsDLL> getAutofocusResult();

	void setUserDecision(bool accepted);

protected:
	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete) override;
	virtual bool executeCleaningCycleOnTermination(
		size_t currentWfOpIndex, size_t endIndex, std::function<void(HawkeyeError)> onCleaningComplete) override;

private:
	void updateHost(uint16_t currentState) const;
	void setCleaningCycleIndices();
	void cleanupBeforeExit(std::function<void(bool)> callback);

	autofocus_state_callback_t_DLL callback_;
	SamplePositionDLL focusbead_location_;
	countdown_timer_callback_t_DLL timer_cb_;
	std::shared_ptr<AutofocusResultsDLL> pAutofocusResult_;
	boost::optional<bool> opUserDecision_;
	uint16_t cancelState_;
	bool cleanupRun;
};
