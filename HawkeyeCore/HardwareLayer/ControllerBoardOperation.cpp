#include "stdafx.h"

#include "ControllerBoardOperation.hpp"
#include "ErrcHelpers.hpp"
#include "ErrorCode.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "ControllerBoardOperation";
static const uint64_t CBOPollTimerIntervalMillisec = 50;
static uint32_t dumpRunQueueThreshold_ = 15;  // When the Run Queue reaches this number of entries set the debug level to Debug1.
static severity_level originalSeverityLevel_;

//*****************************************************************************
ControllerBoardOperation::ControllerBoardOperation(
	std::shared_ptr<boost::asio::io_context> upstream, const std::string& _serialDevIdentifier, const std::string& _dataDevIdentifier)
	: pServices_(upstream, "ControllerBoardOperation_Thread")
	, pollingTimer_(nullptr)
	, taskindex_(0)
	, pollStatusAvailable_(false)
	, internalFaultActive_(false)
{
	pCBI_ = std::make_shared<ControllerBoardInterface>(pServices_.getInternalIos(), _serialDevIdentifier, _dataDevIdentifier);

	originalSeverityLevel_ = Logger::L().GetLoggingSensitivity();
}

//*****************************************************************************
ControllerBoardOperation::~ControllerBoardOperation()
{
	stopPollTimer();
	pServices_.StopServices();
	pCBI_.reset();
	inputQueue_.clear();
	runQueue_.clear();
}

//*****************************************************************************
void ControllerBoardOperation::Initialize (std::function<void(bool)> completeCallback) 
{
	HAWKEYE_ASSERT (MODULENAME, completeCallback);

	pServices_.enqueueInternal (std::bind(&ControllerBoardOperation::internalInitialize, this, eInitStates::eInitializeCBI, completeCallback));
}

void ControllerBoardOperation::PauseOperation(void)
{
	stopPollTimer();
}

void ControllerBoardOperation::ResumeOperation(void)
{
	startPollTimer();
}

void ControllerBoardOperation::HaltOperation(void)
{
	stopPollTimer();
	pServices_.StopServices();
	pCBI_->Close();
}

//*****************************************************************************
std::shared_ptr<ControllerBoardInterface> ControllerBoardOperation::GetControllerBoardInterface() {
	HAWKEYE_ASSERT (MODULENAME, pCBI_);
	return pCBI_;
}

//*****************************************************************************
boost::optional<uint16_t> ControllerBoardOperation::Query (const Operation& operation, Callback_t cb) {

	//TODO: do input integrity checks...
	
	Logger::L().Log (MODULENAME, severity_level::debug3, "Query");


	// We should call "Query()" method only for reading operation
	HAWKEYE_ASSERT (MODULENAME, operation.GetMode () == Operation::Mode_t::ReadMode);


	if (!pServices_.InternalActive())
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "Query : Not accepting work at this time");
		return boost::none;
	}

	if (operation.GetRegAddr() == operation.InvalidRegisterAddress)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Query : Invalid register address");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_integrity_softwarefault, 
			instrument_error::severity_level::error));
		return boost::none;
	}

	pTask_t task = buildTask(operation, 0, cb);

	pServices_.enqueueInternal([this, task]()->void { executeInternal(task); });
	return task->id;
}

//*****************************************************************************
boost::optional<uint16_t> ControllerBoardOperation::Execute (const Operation& operation, uint32_t timeout_msecs, Callback_t cb) {

	//TODO: do input integrity checks...

	Logger::L().Log (MODULENAME, severity_level::debug3, "Execute");


	// We should call "Execute()" method only for writing operation
	HAWKEYE_ASSERT (MODULENAME, operation.GetMode() != Operation::Mode_t::ReadMode);

	if (!pServices_.InternalActive())
	{
		Logger::L().Log (MODULENAME, severity_level::notification, "Execute : Not accepting work at this time");
		return boost::none;
	}

	if (operation.GetRegAddr() == operation.InvalidRegisterAddress) {
		Logger::L().Log (MODULENAME, severity_level::error, "Execute : Invalid register address");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_integrity_softwarefault, 
			instrument_error::severity_level::error));
		return boost::none;
	}

	pTask_t task = buildTask(operation, timeout_msecs, cb);

	pServices_.enqueueInternal([this, task]() {executeInternal(task); });
	return task->id;
}

//*****************************************************************************
void ControllerBoardOperation::Flush (BoardStatus::StatusBit busyBitToFlush, bool flushRunning)
{
	pServices_.enqueueInternal([this, busyBitToFlush, flushRunning]()
	{
		flushInternal(busyBitToFlush, flushRunning);
	});
}

//*****************************************************************************
ControllerBoardOperation::pTask_t ControllerBoardOperation::buildTask (const Operation& operation, uint32_t timeout_msecs, Callback_t cb)
{
	if (Logger::L().IsOfInterest(severity_level::debug2)) {
		Logger::L().Log (MODULENAME, severity_level::debug2,
						boost::str(boost::format("buildTask: <mode: %s, regAddr: %0u (0x%08.8X)>")
								   % (operation.GetMode() == Operation::ReadMode ? "read" : "write")
								   % operation.GetRegAddr() % operation.GetRegAddr()));
	}

	pTask_t task = std::make_shared<Task_t>();
	{
		std::lock_guard<std::mutex> guard(ti_mutex_);
		task->id = taskindex_;
		if (++taskindex_ > 10000)
			taskindex_ = 0;
	}
	task->mode = operation.GetMode();
	task->regAddr = operation.GetRegAddr();
	task->ioSize = operation.GetLength();
	task->timeout_msecs = timeout_msecs;
	task->cb = cb;
	task->isCompleted = false;
	task->isTimedout = false;

	switch (task->mode) {
		case Operation::WriteMode:
		case Operation::OverridingWriteMode:
		{
			task->busyBitToTest = RegisterToBusyBit(static_cast<RegisterIds>(task->regAddr));
			task->txBuf = operation.ToTx();
			break;
		}
		case Operation::ReadMode:
		{
			task->busyBitToTest = BoardStatus::DoNotCheckAnyBit;
			task->txBuf = nullptr;
			break;
		}
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::critical, "buildTask: invalid task mode!");
			HAWKEYE_ASSERT (MODULENAME, false);
			break;
		}
	}

	task->errorStatusBitToTest = BoardStatusBusyBitToErrorStatusBit(task->busyBitToTest);
	return task;
}

//*****************************************************************************
void ControllerBoardOperation::internalInitialize (eInitStates currentState, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCurrentStateComplete = [this, callback](eInitStates nextState)
	{
		pServices_.enqueueInternal(std::bind(&ControllerBoardOperation::internalInitialize, this, nextState, callback));
	};

	Logger::L().Log (MODULENAME, severity_level::debug3, "internalInitialize: current state is : " + std::to_string((uint16_t)currentState));

	switch (currentState)
	{
		case eInitStates::eInitializeCBI:
		{
			if (!pCBI_)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "internalInitialize: pCBI instance is null");
				onCurrentStateComplete(eInitStates::eError);
				return;
			}
			pCBI_->Initialize([onCurrentStateComplete](bool initStatus)
			{
				if (!initStatus)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitialize: Failed to initialize PCBI");
					onCurrentStateComplete(eInitStates::eError);
					return;
				}
				onCurrentStateComplete(eInitStates::eStartPollTimer);
			});
			return;
		}
		case eInitStates::eStartPollTimer:
		{
			startPollTimer();
			onCurrentStateComplete(eInitStates::eReadVersion);
			return;
		}
		case eInitStates::eReadVersion:
		{
			pCBI_->ReadVersion([onCurrentStateComplete](bool readSuccess)
			{
				if (!readSuccess)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "internalInitialize: Failed to read PCBI version");
					onCurrentStateComplete(eInitStates::eError);
					return;
				}
				onCurrentStateComplete(eInitStates::eWaitForFirstPoll);
			});
			return;
		}
		case eInitStates::eWaitForFirstPoll:
		{
			auto timer = std::make_shared <boost::asio::deadline_timer>(pServices_.getInternalIosRef());
			timer->expires_from_now(boost::posix_time::milliseconds(CBOPollTimerIntervalMillisec));
			timer->async_wait([this, onCurrentStateComplete, timer](boost::system::error_code ec)->void
			{
				// Repeat this state until "pollStatusAvailable_" is available
				onCurrentStateComplete(pollStatusAvailable_ ? eInitStates::eComplete : eInitStates::eWaitForFirstPoll);
			});
			return;
		}
		case eInitStates::eComplete:
		{
			Logger::L().Log(MODULENAME, severity_level::error, "internalInitialize:  CBO initialization complete");
			pServices_.enqueueExternal (callback, true);
			return;
		}
		case eInitStates::eError:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "internalInitialize: Failed to initialize CBO");
			pServices_.enqueueExternal (callback, false);
			return;
		}
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "internalInitialize: Invalid or unhandled eInitStates");
			pServices_.enqueueExternal (callback, false);
			return;
		}
	}

	//Unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void ControllerBoardOperation::executeInternal(pTask_t task) {

	Logger::L().Log (MODULENAME, severity_level::debug2, "executeInternal: <enter>");

	HAWKEYE_ASSERT (MODULENAME, task);

	// If a Overriding Write has arrived, dump all other write/busy-bit activities
	// to make room for this.
	// Enqueue Overriding write directly to RunQueue
	if (task->mode == Operation::OverridingWriteMode) {
		flushInternal(task->busyBitToTest, true);
	}

	// if we can't execute task now then enqueue it to "inputQueue"
	// otherwise execute them immediately
	if (canEnqueueToRunList(*task, true)) {
		pServices_.enqueueInternal([this, task]()->void { Enqueue(task); });
		Logger::L().Log (MODULENAME, severity_level::debug3, "executeInternal: <exit, execute immediately>");
		return;
	}
	inputQueue_.push_back(task);
	dumpInputQueue();

	Logger::L().Log (MODULENAME, severity_level::debug2, "executeInternal: <exit>");
}

//*****************************************************************************
bool ControllerBoardOperation::canEnqueueToRunList(const Task_t& task, bool includeInputQueue)
{
	if (task.mode == Operation::ReadMode)
	{
		return true;
	}

	if (task.mode == Operation::OverridingWriteMode)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "canEnqueueToRunList: <exit with true for OverridingWriteMode>");
		return true;
	}

	// Check if similar command (with same busy bit) is already present in run queue
	for (const auto& item : runQueue_) {
		if (isBusyBitMaskSame(item.second->busyBitToTest, task.busyBitToTest)) {
			return false;
		}
	}

	if (includeInputQueue)
	{
		// Check if similar command (with same busy bit) is already present in input queue
		for (const auto& item : inputQueue_)
		{
			if (isBusyBitMaskSame(item->busyBitToTest, task.busyBitToTest))
			{
				return false;
			}
		}
	}

	// Check if busy bit is set.
	return !boardStatus_.load().isSet(task.busyBitToTest);
}

//*****************************************************************************
// Post a task to the private io_service thread.
// Return an error if the io_service isn't created.
// This method MUST be called from with a mutex lock.
//*****************************************************************************
boost::system::error_code ControllerBoardOperation::Enqueue (pTask_t task) {

	std::string logStr = boost::str(boost::format("Enqueuing task:  taskindex: %d") % task->id);

	if (!pServices_.InternalActive()) {
		// Apparently not accepting new work?
		logStr.append ("   Error: not connected");
		Logger::L().Log (MODULENAME, severity_level::error, logStr);
		return MAKE_ERRC (boost::system::errc::not_connected);
	}

	if (!task) {
		logStr.append ("   Error: invalid task pointer");
		Logger::L().Log (MODULENAME, severity_level::error, logStr);
		return MAKE_ERRC(boost::system::errc::invalid_argument);
	}

	if (runQueue_.find(task->id) != runQueue_.end()) {
		logStr.append("   Error: Trying to add same entry to run queue again <task index>: " + std::to_string(task->id));
		Logger::L().Log (MODULENAME, severity_level::error, logStr);
		HAWKEYE_ASSERT (MODULENAME, false);
	}

	runQueue_[task->id] = task;
	std::string logStr2 = boost::str(boost::format("in Enqueue:  task->id: %d, ioSize: %d") % task->id % task->ioSize);
	Logger::L().Log (MODULENAME, severity_level::debug3, logStr2);
	dumpRunQueue();

	// Push task off to internal IO Service for later processing.
	pServices_.enqueueInternal([this, task]()->void { this->ProcessTask(task); });

	Logger::L().Log (MODULENAME, severity_level::debug2, logStr);

	return MAKE_SUCCESS;
}

//*****************************************************************************
void ControllerBoardOperation::ProcessTask(pTask_t task) {
	/*
	* Called by the private io_service.run() thread.
	*
	* Locate task within map.
	* If task exists, remove it from the map and execute the task.
	*
	* Splitting the execution out lets us release the queue earlier.
	* Tasks are run one at a time.
	*/

	if (!task) {
		Logger::L().Log (MODULENAME, severity_level::error, "\"task\" is nullptr");
		HAWKEYE_ASSERT (MODULENAME, false);
	}

	std::string output = boost::str(boost::format("ProcessTask: <taskID: %d>") % task->id);

	if (Logger::L().IsOfInterest(severity_level::debug2))
	{
		output += boost::str(boost::format("\nMode: %s\nRegister: %d\nLength: %d") % (task->mode == Operation::ReadMode ? "Read":"Write") % task->regAddr % task->ioSize);
	}
	if (Logger::L().IsOfInterest(severity_level::debug3))
	{
		if (task->mode != Operation::ReadMode)
		{
			// lines no longer than 20
			auto x = task->txBuf->begin();
			while (x != task->txBuf->end())
			{
				std::string line = "\n";
				for (uint8_t j = 0; j < 4, x != task->txBuf->end(); j++)
				{
					for (uint8_t i = 0; i < 4, x != task->txBuf->end(); i++)
					{
						line += boost::str(boost::format("%02X ") % (int)*x);
						x++;
					}
					line += " ";
				}
				output += line;
			}

			Logger::L().Log (MODULENAME, severity_level::debug2, output);
		}
	}

	// Only set the timer if a timeout has been specified.
	// Only *execute* operations use a timeout value.
	if (task->timeout_msecs) {
		task->completionTimeoutTimer = std::make_shared <boost::asio::deadline_timer>(pServices_.getInternalIosRef());
		task->completionTimeoutTimer->expires_from_now(boost::posix_time::milliseconds(task->timeout_msecs));
		task->completionTimeoutTimer->async_wait([=](boost::system::error_code ec)->void { this->onCompletionTimeout(ec, task); });
	}

	if (task->mode == Operation::ReadMode) {
		auto ec = pCBI_->ReadRegister(
			task->regAddr, static_cast<uint32_t>(task->ioSize), task->callbackData.rxBuf,
			std::bind(&ControllerBoardOperation::onReadCompletion, this, task, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		if (ec) {
			Logger::L().Log(MODULENAME, severity_level::error, "ProcessTask - ReadRegister failed");
			pServices_.enqueueInternal([this, ec, task]() {onReadCompletion(task, ec, boardStatus_.load().get(), { }, { }); });
		}
	} else // Write mode.
	{
		auto ec = pCBI_->WriteRegister (
			task->regAddr, task->txBuf,
			std::bind(&ControllerBoardOperation::onWriteCompletion, this, task, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		if (ec) {
			Logger::L().Log(MODULENAME, severity_level::error, "ProcessTask - WriteRegister failed");
			pServices_.enqueueInternal([this, ec, task]() {onWriteCompletion(task, ec, boardStatus_.load().get(), { }, { }); });
		}
	}
}

//*****************************************************************************
void ControllerBoardOperation::executeCallbackAndDeleteTask (pTask_t task) {
	if (task->completionTimeoutTimer) {
		task->completionTimeoutTimer->cancel();
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("executeCallbackAndDeleteTask: taskID: %d") % task->id));

	pServices_.enqueueExternal (task->cb, task->callbackData);
	runQueue_.erase(task->id);
}

//*****************************************************************************
// This is probably going to be called before a write operation has finished.
// This only handles really processes *read* completions.
// *onPollStatusCompletion* handles the completion of *write*s.
//*****************************************************************************
void ControllerBoardOperation::onReadCompletion(
	pTask_t task,
	boost::system::error_code ec,
	uint16_t board_status,
	ptxrxbuffer_t tx,
	ptxrxbuffer_t rx) {
	boardStatus_ = board_status;

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str (boost::format ("onReadCompletion: task->id: %d") % task->id));

	// Status and invocation handled elsewhere.  Let it expire.
	if (task->isTimedout) {
		return;
	}

	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::error, "onReadCompletion: " + ec.message());

		task->callbackData.clear();
		task->callbackData.status = Error;

		executeCallbackAndDeleteTask (task);

		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str (boost::format ("onReadCompletion: BoardStatus (0x%04.4X): ") % boardStatus_.load().get()));

	task->callbackData.clear();

	if (task->mode == Operation::ReadMode && rx) {

		uint32_t* p = (uint32_t*)rx->data();
		if (p) {
			task->callbackData.bytesRead = rx->size();
			task->callbackData.rxBuf = rx;
			task->callbackData.status = Success;
			Logger::L().Log (MODULENAME, severity_level::debug3, boost::str (boost::format ("onReadCompletion: task->id: %d") % task->id));

		} else {
			Logger::L().Log (MODULENAME, severity_level::error, "onReadCompletion: no data recvd, BoardStatus: " + boardStatus_.load().getAsString());
			task->callbackData.status = Error;
		}

		executeCallbackAndDeleteTask (task);
	} else {
		// should be calling "onReadCompletion" from a not-a-read!
		task->isCompleted = true;
		HAWKEYE_ASSERT (MODULENAME, false);
	}
}

//*****************************************************************************
void ControllerBoardOperation::onWriteCompletion(
	pTask_t task,
	boost::system::error_code ec,
	uint16_t board_status,
	ptxrxbuffer_t tx,
	ptxrxbuffer_t rx) {
	boardStatus_ = board_status;

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("onWriteCompletion: task->id: %d, BoardStatus (0x%04.4X)")
																   % task->id % boardStatus_.load().get()));

	// Status and invocation handled elsewhere.  Let it expire.
	if (task->callbackData.status == Cancelled) {
		// If task is cancelled then it means caller has been notified already
		// No need to do anything else now
		return;
	}

	// Status and invocation handled elsewhere.  Let it expire.
	if (task->isTimedout) {
		Logger::L ().Log (MODULENAME, severity_level::debug3, boost::str (boost::format ("onWriteCompletion: timeout, task->id: %d") % task->id));
		return;
	}

	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::error, "onWriteCompletion: " + ec.message() + "(" + std::to_string(ec.value()) + ")");

		task->callbackData.clear();
		task->callbackData.status = Error;

		Logger::L ().Log (MODULENAME, severity_level::debug3, "onWriteCompletion: executing cb");
		executeCallbackAndDeleteTask(task);

		return;
	}

	task->callbackData.clear();
	
	// Check if the command was accepted by the Controller Board.
	if (boardStatus_.load().isSet(BoardStatus::HostCommError)) {
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::controller_general_hostcommerror, 
			instrument_error::cntrlr_general_instance::none, 
			instrument_error::severity_level::error));
		Logger::L().Log (MODULENAME, severity_level::error, "onWriteCompletion callback <HostCommError>: BoardStatus: " + boardStatus_.load().getAsString());

		// we can't clear "HostComm" error
		//ReadAndClearErrorCode (ErrorStatusBitToErrorCodeRegister(ErrorStatus::HostComm));

		task->callbackData.status = Error;
		task->callbackData.errorStatus = ErrorStatus::HostComm;

		Logger::L().Log (MODULENAME, severity_level::debug3, "onWriteCompletion: executing cb");
		executeCallbackAndDeleteTask(task);

		return;
	}

	if (boardStatus_.load().isSet (task->busyBitToTest)) {
		Logger::L ().Log (MODULENAME, severity_level::debug2, boost::str (boost::format ("onWriteCompletion (busy): task->id: %d") % task->id));
		task->isCompleted = true;
		return;
	}

	// If the error status is set, see whether or not the related error bit for this register is set
	if ((!boardStatus_.load().isSet(BoardStatus::Error)) || 
		task->errorStatusBitToTest == ErrorStatus::StatusBit::DoNotCheckAnyBit ||
		errorStatus_.load().isSet(ErrorStatus::ReagentMotor))
	{
		task->callbackData.status = Success;
		executeCallbackAndDeleteTask(task);
		Logger::L ().Log (MODULENAME, severity_level::debug3, boost::str (boost::format ("onWriteCompletion: task->id: %d") % task->id));

		return;
	}

	std::string errmsg = "onWriteCompletion callback <Error>: BoardStatus: " + boardStatus_.load().getAsString();

	auto errorCodeReg = ErrorStatusBitToErrorCodeRegister(task->errorStatusBitToTest);

	ReadAndClearErrorCode(errorCodeReg, [this, task, errmsg](uint32_t err_val) {
		ErrorType et(err_val);
		if ((err_val != 0) && et.isSet(ErrorType::ErrorTypeMask::StateMachineBusy)) {
			task->callbackData.status = Error;
			task->callbackData.errorStatus = errorStatus_;
			task->callbackData.errorCode = err_val;

			executeCallbackAndDeleteTask(task);
			Logger::L().Log (MODULENAME, severity_level::error, (errmsg + "\nErrorStatus: %d" + et.getAsString()));
		} else {
			task->callbackData.status = Success;
			executeCallbackAndDeleteTask(task);
			Logger::L().Log (MODULENAME, severity_level::debug3, "Command completed");
		}
	});
}


//*****************************************************************************
void ControllerBoardOperation::onCompletionTimeout (const boost::system::error_code& ec, pTask_t task) {
	// If task is cancelled then it means caller has been notified already
	// No need to do anything else now
	if (task->callbackData.status == Cancelled) {
		return;
	}

	// Did completion timer get cancelled?
	if (ec == boost::asio::error::operation_aborted) {
		Logger::L().Log (MODULENAME, severity_level::debug3, "onCompletionTimeout: cancelled");
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, boost::str (boost::format ("onCompletionTimeout:  task->id: %d") % task->id));

	task->isTimedout = true;

	Logger::L().Log (MODULENAME, severity_level::error, "onCompletionTimeout: operation timed out");

	task->callbackData.clear();
	task->callbackData.status = Timeout;

	Logger::L ().Log (MODULENAME, severity_level::debug3, "onCompletionTimeout: executing cb");
	executeCallbackAndDeleteTask (task);
}

//*****************************************************************************
void ControllerBoardOperation::PollStatus (const boost::system::error_code& ec) {

	if (stopPollingTimer_ || ec || !pollingTimer_) {
		Logger::L().Log (MODULENAME, severity_level::warning, "PollStatus...Exit without running");
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, "PollStatus...");

	ptxrxbuffer_t rxb = std::make_shared<txrxbuffer_t>();
	uint32_t regId = RegisterIds::ErrorStatus1;
	uint32_t length = sizeof(uint32_t) * 4;

	auto errorStatus = pCBI_->ReadRegister(
		regId, length, rxb, std::bind(&ControllerBoardOperation::onPollStatusCompletion, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

	// If errorStatus is set then CBI will not trigger the callback
	if (errorStatus) {
		Logger::L().Log (MODULENAME, severity_level::warning, "PollStatus...Read Register call failed!");
		// trigger callback manually here
		pServices_.enqueueInternal([=]() {onPollStatusCompletion(errorStatus, boardStatus_.load().get(), { }, { }); });
	}

	if (!stopPollingTimer_) {
		pollingTimer_->expires_from_now (boost::posix_time::milliseconds (CBOPollTimerIntervalMillisec));
		pollingTimer_->async_wait ([this](boost::system::error_code ec2)->void {
			this->PollStatus (ec2);
		});
	}
}

//*****************************************************************************
// Poll the ControllerBoard for BoardStatus, ErrorStatus, HostCommError and SignalStatus.
// Handle successful completion of a ControllerBoardInterface::WriteRegister command.
//*****************************************************************************
void ControllerBoardOperation::onPollStatusCompletion (boost::system::error_code ec, uint16_t board_status, ptxrxbuffer_t tx, ptxrxbuffer_t rx) {
	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::error, "onPollStatusCompletion: " + ec.message());
		//TODO: when this happens, the controller board throws this message: "CRASH 3, ASSERT failed: C:/DEV/Hawkeye/Controller_backup/src/RS232.cpp:189".
		//TODO: need to reset the controller board and recover the processing?
		// 2018-07-31: this has not occurred for 8 -> 10 months.
		// 2023-06 - See LH6531-5892 for a re-appearance of this error.  Because the board is in a fatal state, it's not responding ever, so the 
		//           queue is going to fill up FAST and we'll eventually assert because we start running into duplicated task IDs.
		//           In the meanwhile, we'll have shifted to DBG2 logging and the logs will be filling up FAST.
		//           Need to post a fault upstream and basically shut down the system.

		internalFaultActive_ = true;
		Logger::L().Log(MODULENAME, severity_level::error, "ControllerBoardOperation system set to Faulted state; Signalling for system halt.");

		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::controller_general_hostcommerror,
			instrument_error::cntrlr_general_instance::none,
			instrument_error::severity_level::error));
		return;
	}

	// Set this flag first time the "onPollStatusCompletion" is called
	if (!pollStatusAvailable_)
	{
		pollStatusAvailable_ = true;
	}

	boardStatus_ = board_status;

	//Logger::L().Log (MODULENAME, severity_level::debug3, "onPollStatusCompletion: BoardStatus: " + boardStatus_.load().getAsString());

	uint32_t* p = (uint32_t*)rx->data();
	if (!p) {
		Logger::L().Log (MODULENAME, severity_level::error, "onPollStatusCompletion: no data received, BoardStatus: " + boardStatus_.load().getAsString());
		return;
	}

	uint32_t a, b;
	memcpy (&a, p, sizeof(uint32_t));

	p += 1;
	memcpy (&b, p, sizeof(uint32_t));
	errorStatus_ = ((uint64_t)b << 32) | (uint64_t)a;

	p += 1;
	hostCommError_ = *p;

	p += 1;
	signalStatus_ = *p;

	if (Logger::L().IsOfInterest(severity_level::debug3) && boardStatus_.load().get() != 0) {
		std::string str = boost::str (boost::format("onPollStatusCompletion:\n\tBoardStatus: %s\n\tErrorStatus: %s\n\tHostCommError: 0x%08.8X\n\tSignalStatus: %s")
									  % boardStatus_.load().getAsString()
									  % errorStatus_.load().getAsString()
									  % hostCommError_.load()
									  % signalStatus_.load().getAsString());
		Logger::L().Log (MODULENAME, severity_level::debug2, str);
	}

	// Check each queued operation's busy bit for completion.
	// If completed, fire the operation's callback and delete the entry from the queue.
	RunQueue_t::iterator runQueueIter = runQueue_.begin();
	while (runQueueIter != runQueue_.end()) {
		// Only need to check the busy bit on the write operations, the read operations return immediately.
		// Skip entries not yet accepted by firmware
		// Skip queue entry if it has timed out.
		// Skip entries which has busy bit still set 
		if (runQueueIter->second->mode == Operation::ReadMode
			|| !runQueueIter->second->isCompleted
			|| runQueueIter->second->isTimedout
			|| boardStatus_.load().isSet(runQueueIter->second->busyBitToTest)) {
			++runQueueIter;
			continue;
		}

		//Logger::L().Log (MODULENAME, severity_level::debug3, "boardStatus_: " + boardStatus_.load().getAsString());

		auto task = runQueueIter->second;
		task->completionTimeoutTimer->cancel();

		if (Logger::L().IsOfInterest(severity_level::debug2)) {
			Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("onPollStatusCompletion: task->id: %d") % task->id));
		}

		// Check if BoardStatus error bit is set.
		if (boardStatus_.load().isSet(BoardStatus::Error)
			&& errorStatus_.load().isSet(task->errorStatusBitToTest)) {
			updateCallerOnError(task);
		} else {
			task->callbackData.clear();
			task->callbackData.status = Success;
			pServices_.enqueueExternal (task->cb, task->callbackData);
		}
		runQueueIter = runQueue_.erase(runQueueIter);
	} // End "while (it != runQueue_.end())"

	  // Look for entries in the input queue to move to the run queue.
	  // For each entry in the input queue...
	  // 1) Is there an entry in the run queue that is using the same busy bit as the entry in the input queue.
	  //     True: skip the input queue entry for now.
	  //    False: enqueue the entry from the input queue to the run queue and delete entry from the input queue.

	std::vector<pTask_t> taskedTobeEnqueued;
	InputQueue_t::iterator inputQueueIter = inputQueue_.begin();
	while (inputQueueIter != inputQueue_.end()) {
		// Move read-mode entries directly to runQueue
		// Move entries with "DoNotCheckAnyBit" directly to runQueue
		if (inputQueueIter->get()->mode == Operation::ReadMode
			|| inputQueueIter->get()->busyBitToTest == BoardStatus::StatusBit::DoNotCheckAnyBit)
		{
			Logger::L().Log (MODULENAME, severity_level::debug3, "onPollStatusCompletion: Enqueuing either read mode or [DoNotCheckAnyBit]");
			boost::system::error_code retStat = Enqueue(*inputQueueIter);
			inputQueueIter = inputQueue_.erase(inputQueueIter);
			continue;
		}

		// Check if current task can be enqueued to run queue or not
		bool canEnqueue = this->canEnqueueToRunList(*(inputQueueIter->get()), false);	//False : We are looping thru input queue
		if (canEnqueue)
		{
			for (const auto& item : taskedTobeEnqueued)
			{
				if (isBusyBitMaskSame(item->busyBitToTest, inputQueueIter->get()->busyBitToTest))
				{
					canEnqueue = false;
					break;
				}
			}
		}

		// Skip entries (as of now) which are already present in the runQueue
		// or ready to be queued in to runQueue
		if (!canEnqueue)
		{
			++inputQueueIter;
			continue;
		}

		if (Logger::L().IsOfInterest(severity_level::debug2)) {
			std::string str = boost::str(boost::format("onPollStatusCompletion 2:\n\tBoardStatus: %s\n\tErrorStatus: %s\n\tHostCommError: 0x%08.8X\n\tSignalStatus: %s")
										 % boardStatus_.load().getAsString()
										 % errorStatus_.load().getAsString()
										 % hostCommError_.load()
										 % signalStatus_.load().getAsString());
			Logger::L().Log (MODULENAME, severity_level::debug2, str);
		}

		taskedTobeEnqueued.emplace_back(*inputQueueIter);
		inputQueueIter = inputQueue_.erase(inputQueueIter);
	}
	dumpInputQueue();

	for (auto item : taskedTobeEnqueued) {
		boost::system::error_code retStat = Enqueue(std::move(item));
	}
}

//*****************************************************************************
void ControllerBoardOperation::ReadAndClearErrorCode(ErrorCodeRegister errorcodeRegister, std::function<void(uint32_t)> callback) {
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto ec = pCBI_->ReadAndClearRegister(
		errorcodeRegister, sizeof(uint32_t), ControllerBoardInterface::t_ptxrxbuf(),
		[this, callback](boost::system::error_code ec, uint16_t board_status, ptxrxbuffer_t tx, ptxrxbuffer_t rx) -> void {
		boardStatus_ = board_status;

		uint32_t errorCode;
		memcpy_s(&errorCode, sizeof(uint32_t), rx->data(), sizeof(uint32_t));

		std::string str = boost::str(boost::format("ReadAndClearErrorCode: <errorCode: 0x%04.4X (%d)>") % errorCode % errorCode);
		Logger::L().Log (MODULENAME, severity_level::debug1, str);

		pServices_.enqueueInternal(std::bind(callback, errorCode));
	});

	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::error, "ReadAndClearErrorCode : failed to execute command");
		uint32_t errorCode = (std::numeric_limits<uint32_t>::max)();
		pServices_.enqueueInternal(std::bind(callback, errorCode));
	}
}

//*****************************************************************************
BoardStatus::StatusBit ControllerBoardOperation::RegisterToBusyBit (RegisterIds regId) {

	switch (regId) {
		case ErrorStatus1:
			return BoardStatus::StatusBit::Error;
		case HostCommError:
			return BoardStatus::StatusBit::HostCommError;
			// Currently Bit2 is not used.
		case Motor1Regs:
			return BoardStatus::StatusBit::Motor1Busy;
		case Motor2Regs:
			return BoardStatus::StatusBit::Motor2Busy;
		case Motor3Regs:
			return BoardStatus::StatusBit::Motor3Busy;
		case Motor4Regs:
			return BoardStatus::StatusBit::Motor4Busy;
		case Motor5Regs:
			return BoardStatus::StatusBit::Motor5Busy;
		case Motor6Regs:
			return BoardStatus::StatusBit::Motor6Busy;
		case Motor7Regs:
			return BoardStatus::StatusBit::Motor7Busy;
		case Motor8Regs:
			return BoardStatus::StatusBit::Motor8Busy;
		case SyringeRegs:
			return BoardStatus::StatusBit::SyringeBusy;
		case PersistentMemoryRegs:
			return BoardStatus::StatusBit::EEPROMBusy;
		case Camera1Regs:
			return BoardStatus::StatusBit::ExposureTimerBusy;
		case ReagentRegs:
			return BoardStatus::StatusBit::ReagentBusy;
		case FwUpdateRegs:
			return BoardStatus::StatusBit::FwUpdateBusy;
		default:
			return BoardStatus::StatusBit::DoNotCheckAnyBit;
	}
}

//*****************************************************************************
ErrorStatus::StatusBit ControllerBoardOperation::BoardStatusBusyBitToErrorStatusBit (BoardStatus::StatusBit busyBit) {

	switch (busyBit) {
		case BoardStatus::StatusBit::Motor1Busy:
			return ErrorStatus::StatusBit::Motor1;
		case BoardStatus::StatusBit::Motor2Busy:
			return ErrorStatus::StatusBit::Motor2;
		case BoardStatus::StatusBit::Motor3Busy:
			return ErrorStatus::StatusBit::Motor3;
		case BoardStatus::StatusBit::Motor4Busy:
			return ErrorStatus::StatusBit::Motor4;
		case BoardStatus::StatusBit::Motor5Busy:
			return ErrorStatus::StatusBit::Motor5;
		case BoardStatus::StatusBit::Motor6Busy:
			return ErrorStatus::StatusBit::Motor6;
		case BoardStatus::StatusBit::Motor7Busy:
			return ErrorStatus::StatusBit::Motor7;
		case BoardStatus::StatusBit::Motor8Busy:
			return ErrorStatus::StatusBit::Motor8;
		case BoardStatus::StatusBit::SyringeBusy:
			return ErrorStatus::StatusBit::Syringe;
		case BoardStatus::StatusBit::ExposureTimerBusy:
			return ErrorStatus::StatusBit::Camera1;
		case BoardStatus::StatusBit::ReagentBusy:
			return ErrorStatus::StatusBit::Reagent;
		case BoardStatus::StatusBit::FwUpdateBusy:
			return ErrorStatus::StatusBit::FwUpdate;
		default:
			return static_cast<ErrorStatus::StatusBit>(0);
	}
}

//*****************************************************************************
ErrorCodeRegister ControllerBoardOperation::ErrorStatusBitToErrorCodeRegister (ErrorStatus::StatusBit statusBit) {

	switch (statusBit) {
		default:
		case ErrorStatus::StatusBit::HostComm:
			return ErrorCodeRegister::HostCommErrorCode;
		case ErrorStatus::StatusBit::Motor1:
			return ErrorCodeRegister::Motor1ErrorCode;
		case ErrorStatus::StatusBit::Motor2:
			return ErrorCodeRegister::Motor2ErrorCode;
		case ErrorStatus::StatusBit::Motor3:
			return ErrorCodeRegister::Motor3ErrorCode;
		case ErrorStatus::StatusBit::Motor4:
			return ErrorCodeRegister::Motor4ErrorCode;
		case ErrorStatus::StatusBit::Motor5:
			return ErrorCodeRegister::Motor5ErrorCode;
		case ErrorStatus::StatusBit::Motor6:
			return ErrorCodeRegister::Motor6ErrorCode;
		case ErrorStatus::StatusBit::Motor7:
			return ErrorCodeRegister::Motor7ErrorCode;
		case ErrorStatus::StatusBit::Motor8:
			return ErrorCodeRegister::Motor8ErrorCode;
		case ErrorStatus::StatusBit::Fan1:
			return ErrorCodeRegister::Fan1ErrorCode;
		case ErrorStatus::StatusBit::Fan2:
			return ErrorCodeRegister::Fan2ErrorCode;
		case ErrorStatus::StatusBit::LED1:
			return ErrorCodeRegister::LED1ErrorCode;
		case ErrorStatus::StatusBit::LED2:
			return ErrorCodeRegister::LED2ErrorCode;
		case ErrorStatus::StatusBit::LED3:
			return ErrorCodeRegister::LED3ErrorCode;
		case ErrorStatus::StatusBit::LED4:
			return ErrorCodeRegister::LED4ErrorCode;
		case ErrorStatus::StatusBit::LED5:
			return ErrorCodeRegister::LED5ErrorCode;
		case ErrorStatus::StatusBit::LED6:
			return ErrorCodeRegister::LED6ErrorCode;
		case ErrorStatus::StatusBit::PhotoDiode1:
			return ErrorCodeRegister::PhotoDiode1ErrorCode;
		case ErrorStatus::StatusBit::PhotoDiode2:
			return ErrorCodeRegister::PhotoDiode2ErrorCode;
		case ErrorStatus::StatusBit::PhotoDiode3:
			return ErrorCodeRegister::PhotoDiode3ErrorCode;
		case ErrorStatus::StatusBit::PhotoDiode4:
			return ErrorCodeRegister::PhotoDiode4ErrorCode;
		case ErrorStatus::StatusBit::PhotoDiode5:
			return ErrorCodeRegister::PhotoDiode5ErrorCode;
		case ErrorStatus::StatusBit::PhotoDiode6:
			return ErrorCodeRegister::PhotoDiode6ErrorCode;
		case ErrorStatus::StatusBit::DoorLatch:
			return ErrorCodeRegister::DoorLatchErrorCode;
		case ErrorStatus::StatusBit::Toggle2:
			return ErrorCodeRegister::Toggle2ErrorCode;
		case ErrorStatus::StatusBit::Toggle3:
			return ErrorCodeRegister::Toggle3ErrorCode;
		case ErrorStatus::StatusBit::Toggle4:
			return ErrorCodeRegister::Toggle4ErrorCode;
		case ErrorStatus::StatusBit::Camera1:
			return ErrorCodeRegister::Camera1ErrorCode;
			//case ErrorStatus::StatusBit::Camera2:          // These are not currently used and may never be...
			//	return ErrorCodeRegister::Camera2ErrorCode;
			//case ErrorStatus::StatusBit::Camera3:
			//	return ErrorCodeRegister::Camera3ErrorCode;
		case ErrorStatus::StatusBit::Reagent:
			return ErrorCodeRegister::ReagentErrorCode;
		case ErrorStatus::StatusBit::Syringe:
			return ErrorCodeRegister::SyringeErrorCode;
		case ErrorStatus::StatusBit::PersistentMemory:
			return ErrorCodeRegister::PersistentMemoryErrorCode;
		case ErrorStatus::StatusBit::FwUpdate:
			return ErrorCodeRegister::FwUpdateErrorCode;
	}
}

//*****************************************************************************
std::string ControllerBoardOperation::GetStatusBitAsString(BoardStatus::StatusBit bit) {
	auto bStatus = BoardStatus();
	bStatus.setBit(bit);
	return bStatus.getAsString();
}

//*****************************************************************************
void ControllerBoardOperation::startPollTimer() {
	stopPollingTimer_ = false;

	if (!pollingTimer_) {
		pollingTimer_ = std::make_shared <boost::asio::deadline_timer>(pServices_.getInternalIosRef());
	}

	pollingTimer_->expires_from_now(boost::posix_time::milliseconds(1));
	pollingTimer_->async_wait([=](boost::system::error_code ec)->void { this->PollStatus(ec); });
}

//*****************************************************************************
void ControllerBoardOperation::stopPollTimer() {
	stopPollingTimer_ = true;
	if (pollingTimer_) {
		pollingTimer_->cancel();
	}
}
//*****************************************************************************
// These functions are TEMPORARY, only to support Hardware modules directly using CBI
// This function is a pass through to CBI functions, COMPLETELY bypasses the busy-bit checks that protects the queue from unintended COMM errors
boost::system::error_code ControllerBoardOperation::WriteRegister(uint32_t regAddr, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_xferCallback Cb) {
	return pCBI_->WriteRegister(regAddr, tx, [this, Cb](boost::system::error_code ec, uint16_t board_status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx) -> void {
		this->boardStatus_ = board_status;
		pServices_.enqueueExternal (Cb, ec, board_status, tx, rx);
	});
}
//*****************************************************************************
boost::system::error_code ControllerBoardOperation::ReadRegister (uint32_t regAddr, uint32_t rxLen, ControllerBoardInterface::t_ptxrxbuf rx, ControllerBoardInterface::t_xferCallback Cb) {
	return pCBI_->ReadRegister (regAddr, rxLen, rx, [this, Cb](boost::system::error_code ec, uint16_t board_status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx) -> void {
		this->boardStatus_ = board_status;
		pServices_.enqueueExternal (Cb, ec, board_status, tx, rx);
	});
}

//*****************************************************************************
void ControllerBoardOperation::ReadFirmwareVersion (std::function<void(bool)> callback)
{
	pCBI_->ReadVersion([this, callback](bool status)->void
	{
		pServices_.enqueueExternal (callback, status);
	});
}
//*****************************************************************************
std::string ControllerBoardOperation::GetFirmwareVersion() 
{
	return pCBI_->GetFirmwareVersion();
}
//*****************************************************************************
uint32_t ControllerBoardOperation::GetFirmwareVersionUint32()
{
	return pCBI_->GetFirmwareVersionUint32();
}
//*****************************************************************************
void ControllerBoardOperation::dumpInputQueue() {
	if (!inputQueue_.size()) {
		return;
	}

	if (!Logger::L().IsOfInterest(severity_level::debug1)) {
		return;
	}

	std::string output = "dumpInputQueue: inputQueue_.size(): " + std::to_string(inputQueue_.size());
	

	size_t idx = 0;
	for (const auto& it : inputQueue_) {
		//NOTE: the task ID is zero until it is set when the task is transferred to the run queue.
		output += boost::str(boost::format("\n[%3.3d]: %5.5d, %s") % idx++ % it->id % (it->mode == Operation::ReadMode ? "read" : "write"));
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, output);
}

//*****************************************************************************
void ControllerBoardOperation::dumpRunQueue() {

	static size_t lastQueueSize = UINT_MAX;

	if (lastQueueSize == UINT_MAX) {
		lastQueueSize = runQueue_.size();
	}

	if (!runQueue_.size()) {
		return;
	}

	if (!Logger::L().IsOfInterest(severity_level::debug1)) {
		return;
	}

	if (runQueue_.size() != lastQueueSize) {
		if (runQueue_.size() >= dumpRunQueueThreshold_) {
			Logger::L().SetLoggingSensitivity(severity_level::debug2);

			std::string output = "dumpRunQueue: runQueue_.size(): " + std::to_string(runQueue_.size());

			size_t idx = 0;
			for (const auto& it : runQueue_) {
				output += boost::str(boost::format("\n[%3.3d]: %5.5d, %s,\t%0u")
									 % idx++ % it.second->id % (it.second->mode == Operation::ReadMode ? "read" : "write")
									 % it.second->regAddr);
			}

			Logger::L().Log (MODULENAME, severity_level::debug2, output);

		} else {
			Logger::L().SetLoggingSensitivity(originalSeverityLevel_);
		}
		lastQueueSize = runQueue_.size();
	}
}

//*****************************************************************************
void ControllerBoardOperation::flushInternal(
	BoardStatus::StatusBit busyBitToFlush, bool flushRunning) {
	// Look for entries to remove from the input queue.
	// For each entry in the input queue...
	// 1) Is there an entry in the run queue that is using the same busy bit as the entry in the input queue.
	//     True: skip the input queue entry for now.
	//    False: enqueue the entry from the input queue to the run queue and delete entry from the input queue.
	InputQueue_t::iterator inputQueueIter = inputQueue_.begin();
	while (inputQueueIter != inputQueue_.end()) {
		// Check for specific busy bit here.
		// Do not use motor bit masking (isBusyBitMaskSame())
		if (busyBitToFlush == inputQueueIter->get()->busyBitToTest) {
			// Inform caller of cancellation.
			(*inputQueueIter)->callbackData.clear();
			(*inputQueueIter)->callbackData.status = Cancelled;
			pServices_.enqueueExternal (
				(*inputQueueIter)->cb, (*inputQueueIter)->callbackData);

			inputQueueIter = inputQueue_.erase(inputQueueIter);
		} else {
			++inputQueueIter;
		}
	}

	if (!flushRunning) {
		return;
	}

	RunQueue_t::iterator runQueueIter = runQueue_.begin();
	while (runQueueIter != runQueue_.end()) {
		// Check for specific busy bit here.
		// Do not use motor bit masking (isBusyBitMaskSame())
		if (busyBitToFlush == runQueueIter->second->busyBitToTest) {
			// Inform caller of cancellation.
			runQueueIter->second->callbackData.clear();
			runQueueIter->second->callbackData.status = Cancelled;
			pServices_.enqueueExternal (
				runQueueIter->second->cb, runQueueIter->second->callbackData);

			runQueueIter = runQueue_.erase(runQueueIter);
		} else {
			++runQueueIter;
		}
	}
}

//*****************************************************************************
void ControllerBoardOperation::updateCallerOnError(pTask_t task) {
	HAWKEYE_ASSERT (MODULENAME, task);

	task->callbackData.clear();

	// Send out-of-band command to clear the specified error.
	// This blocks to ensure that the error code is cleared *before* any other queued
	// commands are processed.

	auto errorCodeReg = ErrorStatusBitToErrorCodeRegister(task->errorStatusBitToTest);
	ReadAndClearErrorCode(errorCodeReg, [this, task](uint32_t err_val) {
		task->callbackData.status = Error;
		task->callbackData.errorStatus = errorStatus_;
		task->callbackData.errorCode = err_val;

		// Task is already removed from "runQueue" when "updateCallerOnError" was
		// called.
		// trigger callback to caller
		executeCallbackAndDeleteTask(task);
	});
}

//*****************************************************************************
bool ControllerBoardOperation::isBusyBitMaskSame(
	BoardStatus::StatusBit bit1, BoardStatus::StatusBit bit2) {
	using SB = BoardStatus::StatusBit;
	static auto motorBusybitmask = (1u << SB::Motor1Busy) | (1u << SB::Motor2Busy) | (1u << SB::Motor3Busy)
		| (1u << SB::Motor4Busy) | (1u << SB::Motor5Busy) | (1u << SB::Motor6Busy) | (1u << SB::Motor7Busy)
		| (1u << SB::Motor8Busy);

	if (bit1 == BoardStatus::StatusBit::DoNotCheckAnyBit
		|| bit2 == BoardStatus::StatusBit::DoNotCheckAnyBit) {
		// No logging if any of the bit is "DoNotCheckAnyBit"
		return bit1 == bit2;
	}

	bool isBit1Anymotor = ((1u << bit1) & motorBusybitmask) != 0;
	bool isBit2Anymotor = ((1u << bit2) & motorBusybitmask) != 0;

	// If both are identical then return true
	if ((bit1 == bit2)
		|| ((isBit1Anymotor == isBit2Anymotor) && isBit1Anymotor))
	{
		std::string logStr = "isBusyBitMaskSame : Avoid another write operation for motor when previous one is still not finished : ";
		logStr.append(GetStatusBitAsString(bit1) + " : ");
		logStr.append(GetStatusBitAsString(bit2));
		Logger::L().Log (MODULENAME, severity_level::notification, logStr);
		return true;
	}
	return false;
}

//*****************************************************************************
SignalStatus ControllerBoardOperation::GetSignalStatus() {
	//Logger::L().Log ("ControllerBoardOperation", severity_level::debug1,
	//				 boost::str (boost::format ("GetSignalStatus: 0x%08X, %s")
	//							 % signalStatus_.load().get()
	//							 % signalStatus_.load().getAsString()));
	return signalStatus_.load();
}
