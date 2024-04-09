#pragma once

#include <memory>

#include "Workflow.hpp"
#include "ReagentData.hpp"

class WorkflowController/* : public HawkeyeServices*/
{
public:
	static WorkflowController& Instance()
	{
		static WorkflowController instance;
		return instance;
	}

	WorkflowController() {}
	void Initialize (std::shared_ptr<boost::asio::io_context> pUpstream);
	~WorkflowController();

	// Callback will return the following HawkeyeError
	//    HawkeyeError::eSuccess - Workflow has started executed successfully
	//    HawkeyeError::eReagentError - Reagent related error : Not enough reagent volume available to run
	//....Else - Any other hardware or software errors
	void set_load_execute_async(
		std::function<void(HawkeyeError)> callback,
		std::shared_ptr<Workflow> newWorkflow,
		Workflow::Type type,
		uint8_t index = 0,
		boost::optional<uint16_t> jumpToState = boost::none);
	void executeWorkflowAsync (std::function<void(HawkeyeError)> callback, Workflow::Type type, boost::optional<uint16_t> jumpToState = boost::none);
	bool isWorkflowBusy() const;
	void validateFluidVolumes (std::map<SyringePumpPort::Port, uint32_t> volumes, std::function<void(bool)> callback);
	bool resetWorkflow();
	bool isOfType(Workflow::Type type) const;

	template<typename T>
	T getCurrentWorkflowOpState()
	{
		if (!pWorkflow_)
		{
			return{};
		}
		return static_cast<T>(pWorkflow_->getCurrentState());
	}

	template<typename T>
	bool validateCurrentWfOpState(T state)
	{
		return state == getCurrentWorkflowOpState<T>();
	}

	template<typename WF>
	WF* getWorkflow(Workflow::Type type)
	{
		if (!pWorkflow_ || !isOfType(type))
		{
			return nullptr;
		}
		return static_cast<WF*>(pWorkflow_.get());
	}

	std::shared_ptr<Workflow> getWorkflow() const;

	boost::asio::io_context& getInternalIosRef() const
	{
		return pHawkeyeServices_->getInternalIosRef();
	}

private:
	void validateFluidVolume (std::function<void(bool)> callback, SyringePumpPort::Port Port, uint32_t volume) const;
	void checkReagentPreconditions (std::function<void(bool)> callback, std::map<SyringePumpPort::Port, uint32_t>& requiredVolumes);
	bool isReagentNearExpiration (uint32_t timeRequired);

	void executeInternal (boost::optional<uint16_t> jumpToState = boost::none);
	HawkeyeError loadWorkflow (uint8_t index = 0) const;
	HawkeyeError setWorkflow (std::shared_ptr<Workflow> newWorkflow);
	bool isWorkflowBusyInternal (std::shared_ptr<Workflow> newWorkflow) const;

	bool isBusy_;
	std::shared_ptr<Workflow> pWorkflow_;
	std::shared_ptr<HawkeyeServices> pHawkeyeServices_;
};
