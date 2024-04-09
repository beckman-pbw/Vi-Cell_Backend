#pragma once

#include <ftd2xx.h>
#include <mutex>
#include <vector>
#include <map>

#include "HawkeyeThread.hpp"

class OmicronLedControllerBoardInterface
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
	typedef std::vector<char> t_txrxbuf;
	typedef std::shared_ptr < t_txrxbuf > t_ptxrxbuf;
	typedef std::function< void(boost::system::error_code, t_ptxrxbuf, t_ptxrxbuf)> t_xferCallback;		//uint16_t, t_ptxrxbuf, t_ptxrxbuf)> t_xferCallback;

private:
	enum eXferType
	{
		eRead,
		eWrite,
	};

	struct XferTask
	{
		eXferType XType;
		t_ptxrxbuf TXBuf;	// Datalen is implicit in the buffer size.
		t_ptxrxbuf RXBuf;
		uint16_t ReadLen;
		t_xferCallback Callback;
		std::shared_ptr<boost::asio::io_context> Mailbox; // If provided, will be used for Post of Callback.  If not, callback will post to "Main" io_service.
	};

	typedef std::shared_ptr<XferTask> t_pXferTask;
	typedef std::map< int, t_pXferTask > t_taskqueue;

	/*
	* Asynch I/O
	*/
	std::unique_ptr<HawkeyeThread> pLocalThread_;
	std::shared_ptr<boost::asio::io_context> pUpstream_;

	/*
	* Serial Device Identification
	*/
	std::string deviceSerialNumber_;

	std::string deviceIDSerial_;
	FT_HANDLE deviceSerialHandle_;
	bool deviceIsOpen_Serial_;

	/*
	* Task queuing
	*/
	uint16_t taskindex;
	t_taskqueue XFERQueue;
	std::mutex queue_mtx_;


public:
	OmicronLedControllerBoardInterface (std::shared_ptr<boost::asio::io_context> iosvc, const std::string& _serialDevIdentifier);
	~OmicronLedControllerBoardInterface();

	/*
	* Hardware access and configuration.
	*/
	boost::system::error_code OpenSerial(std::string identifier = "");
	bool IsSerialOpen() const { return deviceIsOpen_Serial_; }
	void CloseSerial();

	/*
	* SERIAL INTERFACE
	*  All serial interaction is asynchronous with the hardware access running
	*  in a separate thread.
	*/
	static std::size_t MaxTxRxSize() { return 42; }
	boost::system::error_code Write (t_ptxrxbuf tx, t_xferCallback Cb, std::shared_ptr<boost::asio::io_context> mailbox = std::shared_ptr<boost::asio::io_context>());
	void CancelQueue();


private:
	/*
	* Task management.
	* These run in a separate thread that's running io_service::run on _plocal_iosvc
	*/
	void ProcessTask_(uint16_t taskid);
	void ProcessTask (boost::system::error_code& ec, t_pXferTask task);		// uint16_t& boardStatus, t_pXferTask task);

	size_t TxBufLenForTask(t_pXferTask task);
	t_ptxrxbuf CreateTXBUFForTask(t_pXferTask);

	boost::system::error_code Enqueue(t_pXferTask task);
	void ClearQueueWithStatus(boost::system::error_code ec);

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
};
