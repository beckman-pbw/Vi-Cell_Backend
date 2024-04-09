#pragma once

#include "ReagentPack.hpp"
#include "Workflow.hpp"

class ReagentLoadWorkflow : public Workflow
{
public:
	ReagentLoadWorkflow	(
		std::function<void (std::function<void (bool)>)> scanAndReadRfidTagsLambda,
		std::function<std::tuple<bool/*is_expired*/, bool/*is_empty*/> (void)> isReagentPackValidLambda,
		ReagentPack::reagent_load_status_callback_DLL loadStatusChangeCallback
	);
	~ReagentLoadWorkflow();

	virtual HawkeyeError load(std::string filename) override;
	virtual HawkeyeError execute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual void getTotalFluidVolumeUsage(
		std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
		boost::optional<uint16_t> jumpToState = boost::none) override;
	virtual bool canValidateReagent() const override { return false; }

private:

	void executeReagentSequence();
	void queueNextSequence(ReagentLoadSequence nextSequenceToRun);
	void executeReagentLoadSequence(ReagentLoadSequence currentSequence);
	void onCompleteExecution(bool success);

	void checkReagentDoorClosed(std::function<void(bool)> callback);
	void onSequencePrimingFluidLines(std::function<void(ReagentLoadSequence)> callback);

	std::function<void (std::function<void (bool)>)> scanAndReadRfidTagsLambda_;
	std::function<std::tuple<bool, bool> (void)> isReagentPackValidLambda_;
	ReagentPack::reagent_load_status_callback_DLL loadStatusChangeCallback_;
};
