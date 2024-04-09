#pragma once

#include <stdint.h>

#include <functional>
#include <future>
#include <mutex>

#include "DeadlineTimerUtilities.hpp"
#include "HawkeyeError.hpp"
#include "HawkeyeThread.hpp"

// forward declaration
class RunAsync;

#define INVALID_STATE UINT8_MAX
#define NOT_INITIALIZED_STATE INT16_MIN
#define MAX_RETRIES 3

/// <summary>
/// This helper class provides functionality to execute work asynchronous
/// </summary>
class AsyncCommandHelper
{
public:

	/// <summary>
	/// This structure acts as input argument for class/methods using asynchronous execution.	
	/// </summary>
	typedef struct EnumStateParams
	{
		/// <summary>
		/// constructor to create instance of <see cref="EnumStateParams"/>
		/// </summary>
		/// <param name="id">unique id of current asynchronous execution</param>
		/// <param name="onEachEnumStateComplete">Callback to indicate when once single enum state is completed</param>
		/// <param name="canRetryLambda">Callback to check if input state is allowed for retry or not</param>
		EnumStateParams(uint32_t id,
			std::function<void(HawkeyeError, EnumStateParams)> onEachEnumStateComplete,
			std::function<bool(EnumStateParams, uint8_t)> canRetryLambda)
			: id_(id)
			, onEachEnumStateComplete_(onEachEnumStateComplete)
			, canRetryLambda_(canRetryLambda)
		{
			currentState = 0;
			nextState = NOT_INITIALIZED_STATE;
			setComplete = false;
			currentStateRetries = 0;
			maxRetries = MAX_RETRIES;
		}

		/// <summary>
		/// Gets the id of current asynchronous execution
		/// </summary>
		/// <returns>Id of current asynchronous execution</returns>
		uint32_t getId() const
		{
			return id_;
		}

		/// <summary>
		/// Gets the indication if given state can be executed again.
		/// </summary>
		/// <param name="stateId">The input enum state identifier</param>
		/// <returns><c>[true]</c> if current state can be executed again otherwise <c>[false]</c></returns>
		bool canRetry(uint8_t stateId) const
		{
			if (canRetryLambda_ != nullptr)
			{
				return canRetryLambda_(*this, stateId);
			}

			return false;
		}

		/// <summary>
		/// Call this method to indicate enum execution of one state is done.
		/// </summary>
		/// <param name="he"> <c>[HawkeyeError::eSuccess]</c> If state execution is success</param>
		void executeNextState(HawkeyeError he) const
		{
			if (onEachEnumStateComplete_ != nullptr)
			{
				onEachEnumStateComplete_(he, *this);
			}
		}

		/// <summary>
		/// Indicates the current running enum state.
		/// </summary>
		uint8_t currentState;

		/// <summary>
		/// Indicates the next enum state to run when current state <seealso cref="EnumStateParams::currentState"/> is completed
		/// </summary>
		int16_t nextState;

		/// <summary>
		/// Set this to <c>[true]</c> to indicate the enum asynchronous execution is fully completed
		/// </summary>
		bool setComplete;

		/// <summary>
		/// Set this to indicate that current enum state will execute more than once
		/// </summary>
		uint8_t currentStateRetries;

		/// <summary>
		/// Set the maximum number of retries for each enum state
		/// Default value is MAX_RETRIES (3) <see cref="MAX_RETRIES"/>
		/// </summary>
		/// <remarks>
		/// Once the current state retries <see cref="EnumStateParams::currentStateRetries"/> exceeds the
		/// max retries, asynchronous execution will stop will failed status <seealso cref="HawkeyeError::eInvalidArgs"/>
		/// </remarks>
		uint8_t maxRetries;

	private:
		uint32_t id_;

		std::function<void(HawkeyeError, EnumStateParams)> onEachEnumStateComplete_;
		std::function<bool(EnumStateParams, uint8_t)> canRetryLambda_;
	}EnumStateParams;

public:

	/// <summary>
	/// constructor to create instance of <see cref="AsyncCommandHelper"/>
	/// This will initialize the new io_service <see cref="boost::asio::io_context"/> and run it on separate thread
	/// </summary>
	AsyncCommandHelper();

	/// <summary>
	/// constructor to create instance of <see cref="AsyncCommandHelper"/>
	/// </summary>
	/// <param name="pIoService">The shared io_service <see cref="boost::asio::io_context"/> to be used</param>
	/// <remarks>This will not create new io_service</remarks>
	AsyncCommandHelper(std::shared_ptr<boost::asio::io_context> pIoService);

	/// <summary>
	/// Destructor to delete instance and release all resources
	/// </summary>
	virtual ~AsyncCommandHelper();

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
	void postAsync(
		std::function<void(EnumStateParams)> work,
		uint8_t entryState,
		std::function<void(bool)> onWorkComplete);

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
	bool sync_await(
		std::function<void(EnumStateParams)> work,
		uint8_t entryState);

	/// <summary>
	/// Gets the shared instance of deadline timer <see cref="DeadlineTimerUtilities"/> for particular unique id
	/// This unique id generated for each asynchronous enum execution <see cref="EnumStateParams::getId"/>
	/// </summary>
	/// <param name="id">Unique id for asynchronous enum execution</param>
	/// <returns>
	/// The shared instance of deadline timer. Returns empty shared instance if unique id does not exist
	/// </returns>
	std::shared_ptr<DeadlineTimerUtilities> getUniqueTimer(uint32_t id);

	/// <summary>
	/// Gets the shared instance of deadline timer <see cref="DeadlineTimerUtilities"/>
	/// </summary>
	/// <remarks>This instance is unique to each instance of this class</remarks>
	/// <returns>The shared instance of deadline timer.</returns>
	std::shared_ptr<DeadlineTimerUtilities> getTimer();

	/// <summary>
	/// Gets the shared instance of io_serive <see cref="boost::asio::io_context"/> for current instance
	/// </summary>
	/// <returns>The shared instance of io_service</returns>
	std::shared_ptr<boost::asio::io_context> getIoService();

	/// <summary>
	/// Gets the reference instance of io_serive <see cref="boost::asio::io_context"/> for current instance
	/// </summary>
	/// <returns>The reference instance of io_service</returns>
	boost::asio::io_context& getIoServiceRef() const;

	/// <summary>
	/// Helper method to queue multiple asynchronous task with signatures [void(*)(bool)]
	/// and run them in sequence one after another using boost post <see cref="boost::asio::io_context::post"/>
	/// </summary>
	/// <param name="onComplete">Callback to indicate when queue is empty or execution stopped</param>
	/// <param name="asyncTaskList">List of asynchronous tasks</param>
	/// <param name="canExecutionIfAnyTaskFail">
	/// Argument to indicate whether to continue execution when current operation was not successful
	/// <c>[true]</c> Continue execution; otherwise <c>[false]</c>
	/// </param>
	void queueASynchronousTask(
		std::function<void(bool)> onComplete,
		std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList,
		bool canContinueIfAnyTaskFail = true)
	{
		assert (onComplete);

		queueASynchronousTaskInternal<bool>(_plocal_iosvc, onComplete, asyncTaskList, 0, canContinueIfAnyTaskFail, true);
	}

	/// <summary>
	/// Helper method to queue multiple asynchronous task with signatures [void(*)(bool)]
	/// and run them in sequence one after another using boost post <see cref="boost::asio::io_context::post"/>
	/// </summary>
	/// <param name="onComplete">Callback to indicate when queue is empty or execution stopped</param>
	/// <param name="asyncTaskList">List of asynchronous tasks</param>
	/// <param name="canExecutionIfAnyTaskFail">
	/// Argument to indicate whether to continue execution when current operation was not successful
	/// <c>[true]</c> Continue execution; otherwise <c>[false]</c>
	/// </param>
	static void queueASynchronousTask(
		std::shared_ptr<boost::asio::io_context> io_svc,
		std::function<void(bool)> onComplete,
		std::vector<std::function<void(std::function<void(bool)>)>> asyncTaskList,
		bool canContinueIfAnyTaskFail = true)
	{
		assert (io_svc);
		assert (onComplete);

		queueASynchronousTaskInternal<bool>(io_svc, onComplete, asyncTaskList, 0, canContinueIfAnyTaskFail, true);
	}

	/// <summary>
	/// Static helper method to block the work on current thread until completed/stopped 
	/// </summary>
	/// <remarks>This method will block the thread from which it is been called</remarks>
	/// <param name="work">
	/// Work to be executed.
	/// The work (functor) signature should match [void(*)(bool)]
	/// </param>
	/// <returns><c>[true]</c> if execution was successful; otherwise <c>[false]</c></returns>
	static bool sync_await(std::function<void(std::function<void(bool)>)> work)
	{
		return AsyncCommandHelper::sync_await<bool>(work);
	}

	/// <summary>
	/// Static helper method to block the work on current thread until completed/stopped 
	/// </summary>
	/// <remarks>This method will block the thread from which it is been called</remarks>
	/// <param name="work">
	/// Work to be executed.
	/// The work (functor) signature should match [void(*)(TemplateType)]
	/// </param>
	/// <returns><c>[true]</c> if execution was successful; otherwise <c>[false]</c></returns>
	template <typename T>
	static T sync_await (std::function<void(std::function<void(T)>)> work)
	{
		assert(work);

		std::promise<T> promise = std::promise<T>();
		auto future = promise.get_future();

		work([&promise](T status) -> void
		{
			promise.set_value(status);
		});

		return future.get();
	}

private:

	void onTriggerComplete(HawkeyeError status, uint32_t id);
	uint32_t generateId();
	bool containsId(uint32_t id);

	std::vector<std::shared_ptr<RunAsync>> vRunAsync_;
	std::map<int, std::function<void(bool)>> map_Cb_;
	std::mutex lockObject_;
	/*
	* Asynch I/O
	*/
	std::shared_ptr<boost::asio::io_context> _plocal_iosvc;
	std::unique_ptr<HawkeyeThread> pInternalThread_;
	std::shared_ptr<DeadlineTimerUtilities> timer_;

private:

	template <typename T>
	static void queueASynchronousTaskInternal(
		std::shared_ptr<boost::asio::io_context> io_svc,
		std::function<void(bool)> onComplete,
		std::vector<std::function<void(std::function<void(T)>)>> asyncTaskList,
		size_t currentItemIndex,
		bool canContinueIfAnyTaskFail,
		bool status)
	{
		assert (io_svc);
		assert (onComplete);

		if (currentItemIndex >= asyncTaskList.size() || (!canContinueIfAnyTaskFail && !status))
		{
			// this workflow does not require the user id, so do not use the transient user technique
			io_svc->post([=]() -> void
			{
				onComplete(status);
			});
			return;
		}

		auto currentItemToRun = asyncTaskList[currentItemIndex];
		if (!currentItemToRun)
		{
			// this workflow does not require the user id, so do not use the transient user technique
			io_svc->post([=]() -> void
			{
				queueASynchronousTaskInternal(
					io_svc,
					onComplete,
					asyncTaskList,
					(currentItemIndex + 1),
					canContinueIfAnyTaskFail,
					status);
			});
			return;
		}

		currentItemToRun([=](bool currentStatus) -> void
		{
			// this workflow does not require the user id, so do not use the transient user technique
			io_svc->post([=]() -> void
			{
				queueASynchronousTaskInternal(
					io_svc,
					onComplete,
					asyncTaskList,
					(currentItemIndex + 1),
					canContinueIfAnyTaskFail,
					currentStatus);
			});
		});
	}
};
