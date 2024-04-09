#pragma once

#include "ControllerBoardInterface.hpp"
#include "BoardStatus.hpp"
#include "ErrorStatus.hpp"
#include "HawkeyeServices.hpp"
#include "Logger.hpp"
#include "Registers.hpp"
#include "SignalStatus.hpp"
#include "SystemErrors.hpp"
#include "TxRxBuffer.hpp"

// ++++++++++++++++++++++++++
// @TODO - fix this warning
// ++++++++++++++++++++++++++
#pragma warning( push )
#pragma warning( disable : 5208 )

class ControllerBoardOperation
{
public:
	ControllerBoardOperation(
		std::shared_ptr<boost::asio::io_context> upstream, const std::string& _serialDevIdentifier, const std::string& _dataDevIdentifier);
	virtual ~ControllerBoardOperation();

	typedef enum {
		Undefined,
		Success,
		Error,
		Timeout,
		Cancelled,
	} eStatus;

	typedef struct {
		eStatus status;
		ptxrxbuffer_t rxBuf;
		size_t bytesRead;
		ErrorStatus errorStatus;
		uint32_t errorCode;
		void clear() {
			status = Undefined;
			rxBuf = nullptr;
			bytesRead = 0;
			errorStatus = 0;
			errorCode = 0;
		}

		void FromRx (void* data, size_t dataLen) {
			if (!data || !dataLen || !rxBuf || rxBuf->size() > dataLen) {
				HAWKEYE_ASSERT ("ControllerBoardOperation::FromRx", false);
			}
			if (rxBuf && rxBuf->size()) {
				memcpy (data, (uint32_t*)rxBuf->data(), rxBuf->size());
			}
		}

	} CallbackData_t;

	class Operation {
		public:

			/*
			  A "read" operation will execute ASAP (they take no time and have no net effect)
			  A "write" operation will execute as soon as no other "write" operations for the same "busy bit" are active
			  An "overriding write" operation will abort/cancel any other active or pending "write" operations for the same busy bit and then execute ASAP.

			  Overriding Write operations
			     - MUST be supported by the firmware
				    - Most firmware state machines will throw a HOSTCOMM error if a "write" is recieved while another is in progress.
				 - are assumed to be more important than any other request in progress.
			*/
			typedef enum {
				ReadMode,
				WriteMode,
				OverridingWriteMode,
			} Mode_t;

			void Initialize (void* regData) {
				regData_ = regData;
			}

			ptxrxbuffer_t ToTx() const
			{
				if (!regData_)
				{
					Logger::L().Log ("Operation", severity_level::error, "ToTx : NULL register address");
					return{};
				}

				ptxrxbuffer_t txBuf = std::make_shared<txrxbuffer_t>();
				txBuf->resize (lengthInBytes_);
				uint32_t* p = (uint32_t*)txBuf->data();
				memcpy ((void*)p, (void*)regData_, lengthInBytes_);
				return txBuf;
			}

			Mode_t GetMode() const { return mode_; }
			size_t GetLength() const  { return lengthInBytes_; }
			uint32_t GetRegAddr() const  { return regAddr_; }

			const uint32_t InvalidRegisterAddress = (std::numeric_limits<uint32_t>::max)();

		protected:

			Operation()
			{
				regData_ = nullptr;
				regAddr_ = InvalidRegisterAddress;
				lengthInBytes_ = 0;
			}

			Mode_t mode_;
			void*    regData_;
			size_t lengthInBytes_;
			uint32_t regAddr_;
	};

	typedef std::function< void(CallbackData_t)> Callback_t;

	void Initialize(std::function<void(bool)> completeCallback);
	void PauseOperation(void);
	void ResumeOperation(void);
	void HaltOperation(void);

	//TODO: TEMPORARY
	std::shared_ptr<ControllerBoardInterface> GetControllerBoardInterface();
	boost::system::error_code WriteRegister(uint32_t regAddr, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_xferCallback Cb);
	boost::system::error_code ReadRegister(uint32_t regAddr, uint32_t rxLen, ControllerBoardInterface::t_ptxrxbuf rx, ControllerBoardInterface::t_xferCallback Cb);

	boost::optional<uint16_t> Query (
		const Operation& operation,
		Callback_t cb);

	boost::optional<uint16_t> Execute (
		const Operation& operation,
		uint32_t   timeout_msecs,
		Callback_t cb);

	ErrorStatus GetErrorStatus() {
		return errorStatus_.load();
	}
	uint32_t GetHostCommError() {
		return hostCommError_.load();
	}
	SignalStatus GetSignalStatus();
	void ReadFirmwareVersion (std::function<void(bool)> callback);
	std::string GetFirmwareVersion();
	uint32_t GetFirmwareVersionUint32();
	BoardStatus GetBoardStatus() {
		return boardStatus_.load();
	}
	bool GetCommsFaulted(){
		return internalFaultActive_;
	}
	bool isReleasedVersion()
	{
		return !pCBI_ ? false : pCBI_->isReleasedVersion();
	}

private:

	typedef struct {
		uint16_t id;
		Operation::Mode_t mode;
		uint32_t regAddr;
		size_t ioSize;
		ptxrxbuffer_t txBuf;
		BoardStatus::StatusBit busyBitToTest;
		ErrorStatus::StatusBit errorStatusBitToTest;
		uint32_t timeout_msecs;
		Callback_t cb;
		bool isCompleted;	// This indicates that the *on(Read/Write)Completion* callback has been executed for this *task*.
		bool isTimedout;	// This indicates that the task has timed out.
		std::string  logstring;
		std::shared_ptr<boost::asio::deadline_timer> completionTimeoutTimer;
		CallbackData_t callbackData;
	} Task_t;

	enum class eInitStates : uint8_t
	{
		eInitializeCBI = 0,
		eStartPollTimer,
		eReadVersion,
		eWaitForFirstPoll,
		eComplete,
		eError,
	};

	typedef std::shared_ptr<Task_t> pTask_t;
	typedef std::map<int, pTask_t> RunQueue_t;
	typedef std::vector<pTask_t> InputQueue_t;

	boost::system::error_code Enqueue (pTask_t operation);
	void ProcessTask (pTask_t task);

	void internalInitialize(eInitStates currentState, std::function<void(bool)> callback);
	void executeInternal(pTask_t task);
	void Flush(BoardStatus::StatusBit busyBitToFlush, bool flushRunning);

	void onReadCompletion(pTask_t task, boost::system::error_code ec, uint16_t board_status, ptxrxbuffer_t tx, ptxrxbuffer_t rx);
	void onWriteCompletion (pTask_t task, boost::system::error_code ec, uint16_t board_status, ptxrxbuffer_t tx, ptxrxbuffer_t rx);
	void onCompletionTimeout (const boost::system::error_code& ec, pTask_t task);
	void PollStatus (const boost::system::error_code& ec);
	void onPollStatusCompletion (boost::system::error_code ec, uint16_t status, ptxrxbuffer_t tx, ptxrxbuffer_t rx);
	void startPollTimer();
	void stopPollTimer();
	void executeCallbackAndDeleteTask (pTask_t task);
	void ReadAndClearErrorCode(ErrorCodeRegister errorcodeRegister, std::function<void(uint32_t)> callback);
	void dumpInputQueue();
	void dumpRunQueue();
	bool canEnqueueToRunList(const Task_t& task, bool includeInputQueue);
	void flushInternal(BoardStatus::StatusBit busyBitToFlush, bool flushRunning);
	void updateCallerOnError(pTask_t task);
	bool isBusyBitMaskSame(BoardStatus::StatusBit src, BoardStatus::StatusBit dst);

	pTask_t buildTask(const Operation& operation, uint32_t timeout_msecs, Callback_t cb);
	static BoardStatus::StatusBit RegisterToBusyBit(RegisterIds regId);
	static ErrorStatus::StatusBit BoardStatusBusyBitToErrorStatusBit(BoardStatus::StatusBit busyBit);
	static ErrorCodeRegister ControllerBoardOperation::ErrorStatusBitToErrorCodeRegister(ErrorStatus::StatusBit statusBit);
	static std::string GetStatusBitAsString(BoardStatus::StatusBit bit);

	// Asynch I/O.
	HawkeyeServices pServices_;
	
	std::shared_ptr<ControllerBoardInterface> pCBI_;

	// Task queuing.
	std::mutex ti_mutex_;
	uint16_t   taskindex_;
	InputQueue_t inputQueue_;
	RunQueue_t runQueue_;

	std::atomic<bool> stopPollingTimer_;
	std::shared_ptr<boost::asio::deadline_timer> pollingTimer_;
	
	std::atomic<ErrorStatus> errorStatus_;
	std::atomic_uint32_t hostCommError_;
	std::atomic<SignalStatus> signalStatus_;
	std::atomic<BoardStatus> boardStatus_;
	bool pollStatusAvailable_;
	bool internalFaultActive_;
};

#pragma warning( pop )
