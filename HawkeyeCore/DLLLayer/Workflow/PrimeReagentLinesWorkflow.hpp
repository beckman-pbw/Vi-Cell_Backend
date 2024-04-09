#pragma once

#include "Fluidics.hpp"
#include "Workflow.hpp"

class PrimeReagentLinesWorkflow : public Workflow
{
public:
	typedef std::function<void (ePrimeReagentLinesState)> prime_reagentlines_callback_DLL;

	PrimeReagentLinesWorkflow (prime_reagentlines_callback_DLL callback);
	virtual ~PrimeReagentLinesWorkflow ();
	virtual HawkeyeError execute() override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual HawkeyeError load (std::string filename) override;
	virtual bool canValidateReagent() const override { return true; }

protected:
	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete) override;

private:
	const prime_reagentlines_callback_DLL callback_;
};
