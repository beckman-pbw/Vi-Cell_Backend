#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include <ftd2xx.h>

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "BoardStatus.hpp"
#include "ControllerBoardEzPort.hpp"
#include "ErrorStatus.hpp"
#include "HawkeyeThread.hpp"
#include "Registers.hpp"
#include "SignalStatus.hpp"

const int32_t DefaultGpioBaudRate = 256000;
const int32_t ControllerStatusUpdateInterval = 100;

class DLL_CLASS ControllerBoardInterface
{
public:
	/*
	* TxRxBuf implemented as a shared pointer to vector<uchar> because:
	*   1) For Tx, can keep a throwaway buffer alive easily
	*   2) For Rx, size can vary, so we'll just allocate that puppy for you once we learn how big it is.
	*       No need to provide one.
	*   3) Data lengths are implicit in the buffer sizes.  One less parameter to manage.
	*   4) Ownership is easily transferred, disposal is guaranteed.
	*/
	typedef std::vector<unsigned char> t_txrxbuf;
	typedef std::shared_ptr <t_txrxbuf> t_ptxrxbuf;
	typedef std::function< void (boost::system::error_code, uint16_t, t_ptxrxbuf, t_ptxrxbuf)> t_xferCallback;

	ControllerBoardInterface (std::shared_ptr<boost::asio::io_context> upstream, const std::string& _serialDevIdentifier, const std::string& _dataDevIdentifier);
	~ControllerBoardInterface();

	/*
	* Hardware access and configuration.
	*/
	void Initialize (std::function<void (bool)> completeCallback);
	// Base class does not have default arguments
	boost::system::error_code OpenSerial (std::string identifier = "", uint32_t baudRate = DefaultGpioBaudRate);
	boost::system::error_code OpenGPIO (std::string identifier = "");
	bool IsSerialOpen() const { return deviceIsOpen_Serial_; }
	bool IsGPIOOpen() const { return deviceIsOpen_GPIO_; }
	void ReadVersion (std::function<void (bool)> callback);
	std::string GetFirmwareVersion();
	uint32_t GetFirmwareVersionUint32();
	void CloseSerial();
	void CloseGPIO();

	void Close()
	{
		CloseSerial(); CloseGPIO();
	}

	/*
	 * SERIAL INTERFACE
	 *  All serial interaction is asyncronous with the hardware access running
	 *  in a seperate thread.
	 */
	std::size_t MaxTxRxSize() { return 4096; }
	boost::system::error_code WriteRegister (uint32_t regAddr, t_ptxrxbuf tx, t_xferCallback Cb);
	boost::system::error_code ReadRegister (uint32_t regAddr, uint32_t rxLen, t_ptxrxbuf rx, t_xferCallback Cb);
	boost::system::error_code ReadAndClearRegister (uint32_t regAddr, uint32_t rxLen, t_ptxrxbuf rx, t_xferCallback Cb);
	void CancelQueue();

	uint16_t GetBoardHardwareLevel() const;
	bool isReleasedVersion();

	/*
	 * GPIO INTERFACE
	 *  GPIO interaction is SYNCHRONOUS, but it shouldn't take too long.
	 */
	boost::system::error_code SetGPIO (bool p1, bool p2, bool p3);
	boost::system::error_code GetGPIO (bool& p1, bool& p2, bool& p3);
	boost::system::error_code SetGPIO (std::size_t pinNum, bool pinVal);
	boost::system::error_code GetGPIO (std::size_t pinNum, bool& pinVal);

	// Firmware Updates to Attached Hardware.
	bool EnableK70FirmwareUpdate (boost::system::error_code & ec);
	bool DisableK70FirmwareUpdate (boost::system::error_code & ec);
	bool ProgramFirmwareK70 (uint32_t & address, std::vector<uint8_t> & data, boost::system::error_code & ec);
	bool DisconnectFirmwareK70 (boost::system::error_code & ec);
	void StartProcessingQueue() { return StartLocalIOService(); }

	bool HasOmicronLed() const { return hasOmicronLed_; }
	
private:
	enum eXferType
	{
		eRegRead,
		eRegWrite,
		eRegReadAndClear,
	};

	struct XferTask
	{
		eXferType XType;
		uint32_t RegAddr;
		t_ptxrxbuf TXBuf;	// Datalen is implicit in the buffer size.
		t_ptxrxbuf RXBuf;
		uint16_t ReadLen;
		t_xferCallback Callback;
		uint16_t taskindex;
		std::string logstring;
		uint16_t boardStatus;
		boost::system::error_code ec;
	};

	typedef std::shared_ptr<XferTask> t_pXferTask;
	typedef std::map< int, t_pXferTask > t_taskqueue;

	/*

	 * Task management.
	 * These run in a separate thread that's running io_service::run on _plocal_iosvc
	 */
	void ProcessTask_(uint16_t taskid);
	void ProcessTask (t_pXferTask task);
	void ProcessTaskCallback (t_pXferTask task);

	void TaskWrapper(std::string logstring, std::function<void()> cb);
	FILE* task_transaction_log;

	uint16_t TxCRCPosForTask(t_pXferTask task);
	uint16_t TxBufLenForTask(t_pXferTask task);
	t_ptxrxbuf CreateTXBUFForTask(t_pXferTask);
	uint16_t CalculateCRC(t_txrxbuf::iterator start, t_txrxbuf::iterator finish);
	bool VerifyCRC(t_txrxbuf::iterator start, t_txrxbuf::iterator finish);	// Please call this with ALL transmitted bytes INCLUDING the CRC field.
	bool readEEPROM_Description(int device_number, std::string& descrip) const;

	boost::system::error_code Enqueue(t_pXferTask task);
	void ClearQueueWithStatus(boost::system::error_code ec);

	boost::system::error_code OpenHawkeyeDeviceByDescription(std::string identifier, FT_HANDLE& dev_handle);

	/*
	 * FTDI RS232 management;
	 */
	void FlushReadWriteBuffers();

	/*
	 * Private I/O service controls
	 */
	void StartLocalIOService();	//Start from clean slate
	void StopLocalIOService();	// Stop terminally
	void PauseLocalIOService();	// Pause processing of queue
	void ResumeLocalIOService();// Resume processing of prior queue

	// Asynch I/O
	std::shared_ptr<boost::asio::io_context> _pUpstream;
	std::unique_ptr<HawkeyeThread> pCBIThread_;
	std::shared_ptr<EzPort> ez_port_;

	// Serial Device Identification
	std::string deviceSerialNumber_;

	std::string deviceIDSerial_;
	FT_HANDLE deviceSerialHandle_;
	bool deviceIsOpen_Serial_;

	std::string deviceIDGPIO_;
	FT_HANDLE deviceGPIOHandle_;
	bool deviceIsOpen_GPIO_;

	std::atomic<BoardStatus> boardStatus_;
	std::atomic<ErrorStatus> errorStatus_;
	std::atomic<uint32_t> hostCommError_;
	std::atomic<SignalStatus> signalStatus_;

	uint16_t hardware_level_;

	//  Task queuing
	uint16_t taskindex;
	t_taskqueue XFERQueue;
	std::mutex queue_mtx_;
	std::mutex task_process_mtx_;

	std::string firmwareVersion_;
	uint32_t firmwareVersionUint32_;

	bool isReleasedVersion_;
	bool hasOmicronLed_;
};
