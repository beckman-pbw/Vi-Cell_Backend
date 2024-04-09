#pragma once

#include "Fluidics.hpp"
#include "Workflow.hpp"

class DrainReagentPackWorkflow : public Workflow
{
public:
	typedef std::function<void (eDrainReagentPackState)> drain_reagentpack_callback_DLL;

	DrainReagentPackWorkflow (drain_reagentpack_callback_DLL callback, uint32_t repeatCount = 0);
		// repeatCount : number of additional times the drain has to run.
	virtual ~DrainReagentPackWorkflow() override;

	virtual HawkeyeError execute() override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual HawkeyeError load(std::string filename) override;
	virtual void getTotalFluidVolumeUsage(
		std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
		boost::optional<uint16_t> jumpToState = boost::none) override;
	virtual bool canValidateReagent() const override { return false; }

protected:
	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete) override;

private:
	const drain_reagentpack_callback_DLL callback_;
	const uint32_t repeatCount_;
};
