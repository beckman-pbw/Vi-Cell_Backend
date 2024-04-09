#include "stdafx.h"

#include <atomic>
#include <mutex>

#include "boost/format.hpp"

#include "AsyncCommandHelper.hpp"
#include "DeadlineTimerUtilities.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "AsyncCommandHelper";

/// <summary>
/// This class provide the functionality to execute enum states asynchronous
/// </summary>
class RunAsync
{
public:

	/// <summary>
	/// constructor to create instance of <see cref="RunAsync"/>
	/// </summary>
	/// <param name="pIoService">
	/// The shared instance of io_service <see cref="boost::asio::io_context"/>
	/// </param>
	/// <param name="work">
	/// Functor of Work to be done.
	/// Functor should match the signature [void(*)(AsyncCommandHelper::EnumStateParams)]
	/// </param>
	/// <param name="onComplete_Cb">Callback to indicate when asynchronous enum execution is complete</param>
	/// <param name="id">Unique id of current asynchronous enum execution</param>
	RunAsync(
		std::shared_ptr<boost::asio::io_context> pIoService,
		std::function<void(AsyncCommandHelper::EnumStateParams)> work,
		std::function<void(HawkeyeError, uint32_t)> onComplete_Cb,
		uint32_t id)
		: id_(id)
		, pIoService_(pIoService)
		, work_(work)
		, onComplete_Cb_(onComplete_Cb)
		, isBusy_(false)
	{
		stateRetriesMap_.clear();
	}

	/*RunAsync (const RunAsync& other)
	: pIoService_(other.pIoService_)
	, work_(other.work_)
	, onComplete_Cb_(other.onComplete_Cb_)
	, id_(other.id_)
	, timer_(other.timer_)
	{
	isBusy_ = other.isBusy();
	}

	const RunAsync& operator=(const RunAsync& other)
	{
	this->pIoService_ = other.pIoService_;
	this->work_ = other.work_;
	this->onComplete_Cb_ = other.onComplete_Cb_;
	this->id_ = other.id_;
	this->isBusy_ = other.isBusy();
	this->timer_ = other.timer_;

	return *this;
	}*/

	/// <summary>
	///  Destructor to delete instance and release all resources
	/// </summary>
	~RunAsync()
	{
		if (timer_)
		{
			timer_.reset();
		}
		if (pIoService_.get() != nullptr)
		{
			pIoService_.reset();
		}
		work_ = nullptr;
		onComplete_Cb_ = nullptr;
		isBusy_ = false;
		stateRetriesMap_.clear();
	}

	/// <summary>
	/// The method validates the current enum state parameters <see cref="AsyncCommandHelper::EnumStateParams"/>	/// 
	/// </summary>
	/// <param name="params">Enum state parameter to be validated</param>
	/// <returns><c>[true]</c> if validation is successful; otherwise <c>[false]</c></returns>
	/// <remarks>
	/// The validation is success if the following conditions are met;
	///		current state is less than invalid state <see cref="INVALID_STATE"/>
	///		enum execution is marked as completed <see cref="AsyncCommandHelper::EnumStateParams::setComplete"/>
	///		current state is not been executed more than maximum allowed retries <see cref="AsyncCommandHelper::EnumStateParams::maxRetries"/>
	/// </remarks>
	static bool validateParams(
		const AsyncCommandHelper::EnumStateParams& params)
	{
		return params.currentState < INVALID_STATE
			&& params.setComplete == false
			&& params.currentStateRetries < params.maxRetries;
	}

	/// <summary>
	/// Gets the id of current asynchronous execution
	/// </summary>
	/// <returns>Id of current asynchronous execution</returns>
	uint32_t getId() const { return id_; }

	/// <summary>
	/// The method will start the enum execution with user defined input state
	/// This method will not get called once the execution has started for current instance
	/// </summary>
	/// <param name="entryState">The user defined entry point for enum execution</param>
	/// <returns></returns>
	HawkeyeError runOnce(uint8_t entryState)
	{
		if (isBusy())
		{
			return HawkeyeError::eBusy;
		}

		if (work_ == nullptr)
		{
			return HawkeyeError::eInvalidArgs;
		}

		// Set the busy status for current instance
		setBusy(true);

		auto retryLambda = [this](AsyncCommandHelper::EnumStateParams param, uint8_t stateId) -> bool
		{
			return canRetry(param, stateId);
		};

		// create the instance of AsyncCommandHelper::EnumStateParams with unique id,
		// "callback for each enum state completion",
		// retry logic callback
		auto params = AsyncCommandHelper::EnumStateParams(getId(),
			std::bind(&RunAsync::onTriggerEachEnumStateComplete, this, std::placeholders::_1, std::placeholders::_2),
			retryLambda);

		// set the current state as entry point state
		params.currentState = entryState;

		// set the current execution count to 0;
		this->stateRetriesMap_[params.currentState] = 0;

		return runInternal(params);
	}

	bool isBusy() const { return isBusy_.load(); }

	std::shared_ptr<DeadlineTimerUtilities> getTimer()
	{
		if (work_ != nullptr && !timer_)
		{
			timer_ = std::make_shared<DeadlineTimerUtilities>();
		}
		return timer_;
	}

private:

	/// <summary>
	/// Private method to run the various enum states one after another until complete
	/// </summary>
	/// <param name="params">Input argument containing enum states information</param>
	/// <returns><c>[HawkeyeError::eSuccess]</c> if state execution is success</returns>
	HawkeyeError runInternal(AsyncCommandHelper::EnumStateParams params)
	{
		// Check if enum execution parameters are still valid
		if (work_ == nullptr || validateParams(params) == false)
		{
			return HawkeyeError::eInvalidArgs;
		}

		// If the key doesn't exist then add the key with default "0" count
		if (this->stateRetriesMap_.find(params.currentState) == this->stateRetriesMap_.end())
		{
			this->stateRetriesMap_[params.currentState] = 0;
		}
		// Increment the current state execution count. This will help in identifying if
		// any state is being run more than maximum allowed times
		this->stateRetriesMap_[params.currentState] += 1;

		// Update the current state retries field
		params.currentStateRetries = this->stateRetriesMap_[params.currentState];

		// post the work to boost::asio
		// this workflow does not require the user id, so do not use the transient user technique
		pIoService_->post([=]() -> void
		{
			if (work_)
			{
				work_(params);
			}
		});

		return HawkeyeError::eSuccess;
	}

	/// <summary>
	/// Private method to check if execution is complete
	/// </summary>
	/// <param name="params">Input argument containing enum states information</param>
	/// <returns><c>[true]</c> if complete; otherwise <c>[false]</c> </returns>
	static bool isCompleted(const AsyncCommandHelper::EnumStateParams& params)
	{
		return params.setComplete == true;
	}

	/// <summary>
	/// Gets the indication if given state can be executed again.
	/// </summary>
	/// <param name="params">Input argument containing enum states information</param>
	/// <param name="stateId">The input enum state identifier </param>
	/// <returns><c>[true]</c> if current state can be executed again otherwise <c>[false]</c></returns>
	bool canRetry(AsyncCommandHelper::EnumStateParams params, uint8_t stateId)
	{
		return this->stateRetriesMap_[stateId] < params.maxRetries;
	}

	/// <summary>
	/// Private method to prepare the execution for next run
	/// This method will get called once enum state execution is completed for each state
	/// </summary>
	/// <param name="status">The status of previously run enum state</param>
	/// <param name="params">Input argument containing enum states information</param>
	void onTriggerEachEnumStateComplete(
		HawkeyeError status, AsyncCommandHelper::EnumStateParams params)
	{
		// If status is not success, then stop execution and trigger complete callback with
		// failed status to indicate caller that execution is failed
		if (status != HawkeyeError::eSuccess)
		{
			triggerOnComplete(status);
			return;
		}

		// if execution is completed, then trigger complete callback
		// to indicate caller that execution is completed
		if (isCompleted(params))
		{
			triggerOnComplete(status);
			return;
		}

		// Set the next state as current state which will be executed
		params.currentState = static_cast<uint8_t>(params.nextState);

		// Set the next state to invalid value since next state is not defined yet
		// it should be defined by the work functor
		params.nextState = NOT_INITIALIZED_STATE;

		// reset the current state retries for upcoming state
		params.currentStateRetries = 0;

		// this workflow does not require the user id, so do not use the transient user technique
		pIoService_->post([this, params]()
		{
			if (runInternal(params) != HawkeyeError::eSuccess)
			{
				triggerOnComplete(HawkeyeError::eTimedout);
			}

		});
	}

	/// <summary>
	/// Set the busy status of current instance
	/// </summary>
	/// <param name="state">The busy state to be set</param>
	void setBusy(bool state)
	{
		// Since this is run once operation, so keep the busy state true always
		isBusy_ = true; // state;
	}

	/// <summary>
	/// Private method to trigger the execution complete callback with status
	/// </summary>
	/// <param name="he">The status of execution</param>
	/// <remarks>if status is <c>[HawkeyeError::eSuccess]</c> then execution successful; otherwise failed</remarks>
	void triggerOnComplete(HawkeyeError he) const
	{
		if (onComplete_Cb_ != nullptr)
		{
			// this workflow does not require the user id, so do not use the transient user technique
			pIoService_->post(std::bind(onComplete_Cb_, he, getId()));
		}
	}

	uint32_t id_;
	std::shared_ptr<boost::asio::io_context> pIoService_;
	std::function<void(AsyncCommandHelper::EnumStateParams)> work_;
	std::function<void(HawkeyeError, uint32_t)> onComplete_Cb_;
	std::shared_ptr<DeadlineTimerUtilities> timer_;
	std::atomic<bool> isBusy_;
	std::map<uint8_t, uint8_t> stateRetriesMap_;
};

/****************************************************************************/

/// <summary>
/// constructor to create instance of <see cref="AsyncCommandHelper"/>
/// This will initialize the new io_service <see cref="boost::asio::io_context"/> and run it on separate thread
/// </summary>
AsyncCommandHelper::AsyncCommandHelper()
{
	pInternalThread_ = std::make_unique<HawkeyeThread>("AsyncCommandHelper_Thread");
	_plocal_iosvc = pInternalThread_->GetIoService();
}

/// <summary>
/// constructor to create instance of <see cref="AsyncCommandHelper"/>
/// </summary>
/// <param name="pIoService">The shared io_service <see cref="boost::asio::io_context"/> to be used</param>
/// <remarks>This will not create new io_service</remarks>
AsyncCommandHelper::AsyncCommandHelper(
	std::shared_ptr<boost::asio::io_context> pIoService)
{ 
	pInternalThread_.reset();
	_plocal_iosvc = std::move(pIoService);
}

/// <summary>
/// Destructor to delete instance and release all resources
/// </summary>
AsyncCommandHelper::~AsyncCommandHelper()
{
	timer_.reset();
	vRunAsync_.clear();
	_plocal_iosvc.reset();
	pInternalThread_.reset();
}

/// <summary>
/// Call this method to run multiple enum states asynchronous
/// This method will post the work to io_service <seealso cref="boost::asio::io_context::post"/> for each enum state
/// until either work is completed/stopped/failed
/// </summary>
/// <param name="work">Work to done</param>
/// <param name="entryState">
/// Entry point of work w.r.t. enum states
/// Default is <c>[0]</c>
/// </param>
/// <param name="onWorkComplete">Callback to indicate when asynchronous execution is complete</param>
void AsyncCommandHelper::postAsync(
	std::function<void(EnumStateParams)> work,
	uint8_t entryState,
	std::function<void(bool)> onWorkComplete)
{
	if (_plocal_iosvc.get() == nullptr)
	{
		if (onWorkComplete != nullptr)
		{
			onWorkComplete(false);
		}
		return;
	}

	// generate the unique id for current enum execution
	auto id = generateId();
	{
		// lock the mutex in current scope
		std::lock_guard<std::mutex> lkg(lockObject_);

		// create heap instance of "RunAsync" and push it vector
		std::shared_ptr<RunAsync> runInstance = std::make_shared<RunAsync>(
			_plocal_iosvc,
			work,
			std::bind(&AsyncCommandHelper::onTriggerComplete, this, std::placeholders::_1, std::placeholders::_2),
			id);
		vRunAsync_.push_back(std::move(runInstance));

		// add entry in map with "work complete" callback
		// this callback will be executed once "RunAsync" has finished executing current enum states
		map_Cb_[id] = onWorkComplete;

		// this workflow does not require the user id, so do not use the transient user technique
		_plocal_iosvc->post([this, entryState]()
		{
			vRunAsync_[vRunAsync_.size() - 1]->runOnce(entryState);
		});
	}
}

/// <summary>
/// Call this method to run multiple enum states synchronously
/// This method will post the work to io_service <seealso cref="boost::asio::io_context::post"/> for each enum state
/// until either work is completed/stopped/failed
/// </summary>
/// <param name="work">Work to done</param>
/// <param name="entryState">
/// Entry point of work w.r.t. enum states
/// Default is <c>[0]</c>
/// </param>
/// <remarks>This method will return once all the required enum states are executed</remarks>
/// <returns>
/// <c>[true]</c> if execution was successful; otherwise <c>[false]</c>
/// </returns>
bool AsyncCommandHelper::sync_await (
	std::function<void(EnumStateParams)> work,
	uint8_t entryState)
{
	std::promise<bool> promise = std::promise<bool>();
	auto future = promise.get_future();

	// this workflow does not require the user id, so do not use the transient user technique
	postAsync(work,	entryState,
		[&promise](bool status) -> void
	{
		promise.set_value(status);
	});

	return future.get();
}

/// <summary>
/// Gets the shared instance of deadline timer <see cref="DeadlineTimerUtilities"/> for particular unique id
/// This unique id generated for each asynchronous enum execution <see cref="EnumStateParams::getId"/>
/// </summary>
/// <param name="id">Unique id for asynchronous enum execution</param>
/// <returns>
/// The shared instance of deadline timer. Returns empty shared instance if unique id does not exist
/// </returns>
std::shared_ptr<DeadlineTimerUtilities> AsyncCommandHelper::getUniqueTimer(uint32_t id)
{
	std::shared_ptr<DeadlineTimerUtilities> timer;
	auto it = std::find_if(
		vRunAsync_.begin(),
		vRunAsync_.end(),
		[id](std::shared_ptr<RunAsync> element) -> bool
	{
		return element->getId() == id;
	});

	if (it != vRunAsync_.end())
	{
		timer = (*it)->getTimer();
	}

	return timer;
}

/// <summary>
/// Gets the shared instance of deadline timer <see cref="DeadlineTimerUtilities"/>
/// </summary>
/// <remarks>This instance is unique to each instance of this class</remarks>
/// <returns>The shared instance of deadline timer.</returns>
std::shared_ptr<DeadlineTimerUtilities> AsyncCommandHelper::getTimer()
{
	if (!timer_)
	{
		timer_ = std::make_shared<DeadlineTimerUtilities>();
	}
	return timer_;
}

/// <summary>
/// Gets the shared instance of io_serive <see cref="boost::asio::io_context"/> for current instance
/// </summary>
/// <returns>The shared instance of io_service</returns>
std::shared_ptr<boost::asio::io_context> AsyncCommandHelper::getIoService()
{
	return _plocal_iosvc;
}

boost::asio::io_context& AsyncCommandHelper::getIoServiceRef() const
{
	return *_plocal_iosvc;
}

/// <summary>
/// Private method to run when asynchronous enum states execution is complete
/// </summary>
/// <param name="status">The status of execution</param>
/// <param name="id">Unique id of asynchronous enum states execution</param>
void AsyncCommandHelper::onTriggerComplete(HawkeyeError status, uint32_t id)
{
	std::function<void(bool)> cb = nullptr;

	{
		// lock the mutex in current scope
		std::lock_guard<std::mutex> lkg(lockObject_);

		// get the store completion callback from "map" with unique id as key
		cb = map_Cb_[id];

		// remove the entry from map for input "unique id"
		map_Cb_.erase(id);

		// loop thru the store instances of "RunAsync" to find the instance with input unique id
		auto it = std::find_if(
			vRunAsync_.begin(),
			vRunAsync_.end(),
			[id](std::shared_ptr<RunAsync> element) -> bool
		{
			return element->getId() == id;
		});

		// if this method is being called and we didn't find input unique id in vector
		// that means something is horribly wrong
		if (it == vRunAsync_.end())
		{
			// Something went wrong.
			return;
		}

		// delete the instance
		it->reset();

		// remove the instance from vector
		// this will make the unique id available for other instance to use
		vRunAsync_.erase(it);
	}

	// update the host about the execution complete
	// this workflow does not require the user id, so do not use the transient user technique
	_plocal_iosvc->post([cb, status]() -> void
	{
		if (cb != nullptr)
		{
			cb(status == HawkeyeError::eSuccess);
		}
	});
}

/// <summary>
/// Private method to generate the unique id for each asynchronous enum states execution
/// </summary>
/// <returns>The unique id</returns>
uint32_t AsyncCommandHelper::generateId()
{
	uint32_t index = 0;
	if (vRunAsync_.empty())
	{
		return index;
	}

	while (true)
	{
		if (containsId(index) == false)
		{
			break;
		}
		index++;
	}

	return index;
}

/// <summary>
/// Private method to check if particular id exist
/// </summary>
/// <param name="id">The input id to be checked</param>
/// <returns><c>[true]</c> if id exist; otherwise <c>[false]</c></returns>
bool AsyncCommandHelper::containsId(uint32_t id)
{
	std::lock_guard<std::mutex> lkg(lockObject_);
	auto it = std::find_if(
		vRunAsync_.begin(),
		vRunAsync_.end(),
		[id](std::shared_ptr<RunAsync> element) -> bool
	{
		return element->getId() == id;
	});

	if (it == vRunAsync_.end())
	{
		return false;
	}
	return true;
}
