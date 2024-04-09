#pragma once

#include "Fluidics.hpp"
#include "Workflow.hpp"

class DecontaminateFlowCellWorkflow : public Workflow
{
public:
	typedef std::function<void(eDecontaminateFlowCellState)> flowcell_decontaminate_status_callback_DLL;

	DecontaminateFlowCellWorkflow (flowcell_decontaminate_status_callback_DLL  callback);
	virtual ~DecontaminateFlowCellWorkflow();
	virtual HawkeyeError execute() override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual HawkeyeError load (std::string filename) override;
	virtual bool canValidateReagent() const override { return true; }

protected:
	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete) override;

private:
	void cleanupBeforeExit(std::function<void(bool)> callback);

	const flowcell_decontaminate_status_callback_DLL  callback_;
};
