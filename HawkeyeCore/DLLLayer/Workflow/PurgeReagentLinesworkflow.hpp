#pragma once

#include "Workflow.hpp"

class PurgeReagentLinesWorkflow : public Workflow
{
public:
	typedef std::function<void (ePurgeReagentLinesState)> purge_reagentlines_callback_DLL;

	PurgeReagentLinesWorkflow (purge_reagentlines_callback_DLL callback);
	virtual ~PurgeReagentLinesWorkflow();
	virtual HawkeyeError execute() override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile (uint8_t index) override;
	virtual bool canValidateReagent() const override { return false; }

protected:
	virtual void triggerCompletionHandler (HawkeyeError he) override;
	virtual void onWorkflowStateChanged (uint16_t currentState, bool executionComplete) override;

private:
	const purge_reagentlines_callback_DLL callback_;
};
