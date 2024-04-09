#pragma once

#include "Workflow.hpp"

class LoadNudgeExpelWorkflow : public Workflow
{
public:
	enum Manual_Sample_Operation
	{
		Load,
		Nudge,
		Expel
	};	

	LoadNudgeExpelWorkflow (Manual_Sample_Operation op);
	virtual ~LoadNudgeExpelWorkflow();
	virtual HawkeyeError load (std::string filename) override;
	virtual HawkeyeError execute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual bool canValidateReagent() const override { return true; }

protected:
	enum Status
	{
		eIdle = 0,
		eBusy = 1,
	};

	virtual void triggerCompletionHandler(HawkeyeError he) override;

private:

	HawkeyeError buildNudgeWorkflow();
	void checkPreconditionBeforeExecute(std::function<void(bool)> callback);
	void cleanupBeforeExit(std::function<void(bool)> callback);

	Manual_Sample_Operation op_;
};
