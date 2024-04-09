#pragma once

#include "PurgeReagentLinesWorkflow.hpp"
#include "ReagentPack.hpp"

class ReagentUnloadWorkflow : public Workflow
{
public:

public:
	ReagentUnloadWorkflow (
		std::vector<ReagentContainerUnloadOptionDLL> unloadContainersOptions,
		ReagentPack::reagent_unload_status_callback_DLL stateChangeCallback);
	~ReagentUnloadWorkflow();

	virtual HawkeyeError load(std::string filename) override;
	virtual HawkeyeError execute() override;
	virtual std::string getWorkFlowScriptFile(uint8_t index) override;
	virtual void getTotalFluidVolumeUsage(
		std::map<SyringePumpPort::Port, uint32_t>& syringeVolumeMap,
		boost::optional<uint16_t> jumpToState = boost::none) override;
	virtual bool canValidateReagent() const override { return false; }

private:
	void executeReagentSequence();
	void queueNextSequence(ReagentUnloadSequence nextSequenceToRun);
	void executeReagentUnloadSequence(ReagentUnloadSequence currentSequence);
	void onCompleteExecution(bool success);

	void DrainReagentPack(
		std::function<void(bool)> callback,
		uint8_t valveLocation, uint16_t eventRemaining);

	void PurgeReagentLines(
		std::function<void(bool)> callback,
		PurgeReagentLinesWorkflow::purge_reagentlines_callback_DLL purgeCallback,
		ReagentContainerPosition position);

	std::vector<ReagentContainerUnloadOptionDLL> unloadContainersOptions_;
	ReagentPack::reagent_unload_status_callback_DLL stateChangeCallback_;
};
