#pragma once

#include <cstdint>

#include "Fluidics.hpp"
#include "Workflow.hpp"

class FlushFlowCellWorkflow : public Workflow
{
public:
	typedef enum FlushType : uint8_t
	{
		eFlushFlowCell = 0,
		eStandardNightlyClean,
		eACupNightlyClean
	} FlushType;

public:
	typedef std::function<void (eFlushFlowCellState)> flowcell_flush_status_callback_DLL;

	FlushFlowCellWorkflow (flowcell_flush_status_callback_DLL callback);
	virtual ~FlushFlowCellWorkflow();
	virtual HawkeyeError execute() override;
	virtual HawkeyeError abortExecute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual HawkeyeError load (std::string filename) override;
	virtual bool canValidateReagent() const override { return true; }

protected:
	virtual void triggerCompletionHandler(HawkeyeError he) override;
	virtual void onWorkflowStateChanged(uint16_t currentState, bool executionComplete) override;

private:
	const flowcell_flush_status_callback_DLL callback_;
};
