#include "stdafx.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "ErrcHelpers.hpp"
#include "FTDI_Error_To_Boost.hpp"
#include "OmicronLedControllerBoardInterface.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "OmicronLedControllerBoardInterface";

//*****************************************************************************
OmicronLedControllerBoardInterface::OmicronLedControllerBoardInterface(
	std::shared_ptr<boost::asio::io_context> pUpstream,
	const std::string& _serialDevIdentifier)
	: pUpstream_(pUpstream)
	, deviceIDSerial_(_serialDevIdentifier)
	, deviceIsOpen_Serial_(false)
	, deviceSerialNumber_("")
	, taskindex(0)
{
	boost::trim (deviceIDSerial_);
}

//*****************************************************************************
OmicronLedControllerBoardInterface::~OmicronLedControllerBoardInterface() {

	CloseSerial();
}

//*****************************************************************************
boost::system::error_code OmicronLedControllerBoardInterface::OpenSerial(std::string identifier)
{
	/*
	* Attempt to locate and open the FTDI device whose name matches the provided identifier.
	*  If the identifier matches the tag provided in the constructor (or is blank):
	*    ...and the device is already open: return success immediately
	*    ...and the device is NOT open: attempt to open device.
	*  If the identifier does not match the device name already in use...
	*    ...and the device is already open: close the current device, reset the identifier and attempt to open the device
	*    ...and the device is NOT open: reset the identifier and attempt to open the device
	*  If both the identifier here and the current device name are blank
	*    ...immediately return an error condition.
	*/

	boost::trim (identifier);

	if (identifier.empty() && deviceIDSerial_.empty()) {
		return MAKE_ERRC(boost::system::errc::invalid_argument);
	}

	if (IsSerialOpen()) {

		// This device is already open?  All done, then.
		if (deviceIDSerial_ == identifier || identifier.empty()) {
			return MAKE_SUCCESS;
		}

		// Otherwise, close out existing connection.
		CloseSerial();
	}

	if (!identifier.empty()) {
		deviceIDSerial_ = identifier;
	}

	FT_STATUS fstat = FT_OpenEx((PVOID)deviceIDSerial_.c_str(), FT_OPEN_BY_DESCRIPTION, &deviceSerialHandle_);

	using namespace boost::system::errc;
	boost::system::error_code ec = FTDI_Error_To_Boost(fstat);

	if (!ec) {
		fstat = FT_ResetDevice(deviceSerialHandle_);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBitMode(deviceSerialHandle_, 0, FT_BITMODE_RESET);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_Purge(deviceSerialHandle_, FT_PURGE_RX | FT_PURGE_TX);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetBaudRate(deviceSerialHandle_, 500000);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetDataCharacteristics(deviceSerialHandle_, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetFlowControl(deviceSerialHandle_, FT_FLOW_NONE, 0, 0);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		fstat = FT_SetTimeouts(deviceSerialHandle_, 1100, 1100);
		ec = FTDI_Error_To_Boost(fstat);
		if (ec) return ec;

		deviceIsOpen_Serial_ = true;
		StartLocalIOService();
	}

	return ec;
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::CloseSerial() {
	/*
	* Stop queue processing (if any).
	* Flush queue and handle callbacks.
	* Close device.
	*/
	StopLocalIOService();

	if (deviceIsOpen_Serial_) {
		FT_Close(deviceSerialHandle_);
	}
	deviceIsOpen_Serial_ = false;
}

//*****************************************************************************
boost::system::error_code OmicronLedControllerBoardInterface::Write (t_ptxrxbuf tx, t_xferCallback Cb, std::shared_ptr<boost::asio::io_context> mailbox) {
	if (!tx) 
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Write: Invalid argument tx");
		return MAKE_ERRC(boost::system::errc::invalid_argument);
	}
	if (tx->size() > MaxTxRxSize())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Write: Invalid tx size : " + std::to_string(tx->size()));
		return MAKE_ERRC(boost::system::errc::value_too_large);
	}

	t_pXferTask task(new XferTask);
	task->XType = eWrite;
	task->Callback = Cb;
	task->TXBuf = tx;
	task->RXBuf = NULL;
	task->Mailbox = mailbox;

	return Enqueue(task);
}

//*****************************************************************************
boost::system::error_code OmicronLedControllerBoardInterface::Enqueue (t_pXferTask task) {
	/*
	* Post a task to the private io_service thread.
	* Return an error if the io_service isn't created.
	*/

	if (!pLocalThread_->IsActive())
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Enqueue : Local io_servie is inactive");
		// Apparently not accepting new work?
		return MAKE_ERRC(boost::system::errc::not_connected);
	}

	if ((task->XType == eWrite && task->TXBuf && task->TXBuf->size() > 40) ||	// 40 = Max - 2, 2 is for the '?' and CR characters.
		(task->XType == eRead && task->ReadLen > 40))
	{
		if (task->XType == eRead && task->ReadLen > 40)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Enqueue : Task read length is too large : eRead" + std::to_string(task->ReadLen));
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Enqueue : Task buffer size is too large : eWrite" + std::to_string(task->TXBuf->size()));
		}

		return MAKE_ERRC(boost::system::errc::file_too_large);
	}

	queue_mtx_.lock();
	XFERQueue[taskindex] = task;

	// Push task off to internal IO Service for later processing.
	//NOTE: "currentTaskIndex = taskindex" is necessary as the taskindex is getting changed soon after the post
	pLocalThread_->Enqueue([this, currentTaskIndex = taskindex](){this->ProcessTask_(currentTaskIndex);});

	if (++taskindex > 10000) {
		taskindex = 0;
	}

	queue_mtx_.unlock();
	return MAKE_SUCCESS;
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::CancelQueue() {
	
	ClearQueueWithStatus(boost::asio::error::operation_aborted);
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::ClearQueueWithStatus (boost::system::error_code ec) {
	std::unique_lock<std::mutex> lock_(queue_mtx_);

	for (t_taskqueue::iterator it = XFERQueue.begin(); it != XFERQueue.end(); it++) {
		if (it->second && it->second->Callback) {

			if (it->second->Mailbox)
				// this workflow does not require the user id, so do not use the transient user technique
				it->second->Mailbox->post(std::bind(it->second->Callback, ec, it->second->TXBuf, it->second->RXBuf));
			else
				// this workflow does not require the user id, so do not use the transient user technique
				pUpstream_->post(std::bind(it->second->Callback, ec, it->second->TXBuf, it->second->RXBuf));
		}
	}
	XFERQueue.clear();
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::ProcessTask_ (uint16_t taskID) {
	/*
	* Called by the private io_service.run() thread.
	*
	* Locate task within map.
	* If task exists, remove it from the map
	*  and execute the task.
	*
	*  Splitting the execution out lets us release
	*  the queue earlier.
	*/
	queue_mtx_.lock();
	if (XFERQueue.find(taskID) == XFERQueue.end()) 
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask_ : Failed to find task id : " + std::to_string(taskID));
		queue_mtx_.unlock();
		return;
	}

	t_pXferTask task = XFERQueue[taskID];
	XFERQueue.erase(taskID);
	queue_mtx_.unlock();

	if (task) {
		boost::system::error_code ec;
		ProcessTask (ec, task);			// boardStatus, task);

		// only work the callback if we're not taking down the system.
		if (ec != boost::system::errc::operation_canceled) {
			if (task->Mailbox)
				// this workflow does not require the user id, so do not use the transient user technique
				task->Mailbox->post(std::bind(task->Callback, ec, task->TXBuf, task->RXBuf));
			else
				// this workflow does not require the user id, so do not use the transient user technique
				pUpstream_->post (std::bind(task->Callback, ec, task->TXBuf, task->RXBuf));
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask_ : operation cancelled error code");
		}
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask_ : Invalid task!");
	}
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::ProcessTask (boost::system::error_code& ec, t_pXferTask task) {
	assert(task);
	ec = MAKE_SUCCESS;


	// Ensure interface is open.
	ec = OpenSerial();
	if (ec != boost::system::errc::success) 
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask : Failed to open serial");
		return;
	}

	t_ptxrxbuf txbuf = CreateTXBUFForTask(task);

	// Send Transmit buffer.
	std::size_t retries_remaining = 3;

	while (retries_remaining > 0) {

		/*
		* Flush out any remaining information in the FTDI buffers that might have been leftover
		*  from an earlier cycle.
		*/
		FlushReadWriteBuffers();

		retries_remaining--;

		DWORD bytesWritten;
		FT_STATUS fstat = FT_Write (deviceSerialHandle_, (VOID*)txbuf->data(), (DWORD)txbuf->size(), &bytesWritten);

		if (fstat != FT_OK || (bytesWritten != txbuf->size()))
		{
			ec = FTDI_Error_To_Boost(fstat);
			Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask : FT_Write : FTDI_Error_To_Boost error : " + ec.message());
			continue;
		}

//TODO: check that the first character is '!'.

		// Read the first byte.
		t_ptxrxbuf rxbuf = std::make_shared<std::vector<char> >();
		DWORD bytesRead;
		
		char oneChar;
		fstat = FT_Read (deviceSerialHandle_, (void*)&oneChar, 1, &bytesRead);
		if (fstat != FT_OK) {
			ec = FTDI_Error_To_Boost (fstat);
			Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask : FT_Read first byte: FTDI_Error_To_Boost error : " + ec.message());
			continue;
		}

		if (bytesRead != 1) {
			Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask : Stream timed out!");
			ec = MAKE_ERRC (boost::system::errc::stream_timeout);
			continue;
		}

		rxbuf->push_back (oneChar);


//TODO: add check for the max response size...
		// Read 1 byte at a time until CR is found.
		while (true) {
			fstat = FT_Read (deviceSerialHandle_, (void*)&oneChar, 1, &bytesRead);
			if (fstat != FT_OK) {
				Logger::L().Log (MODULENAME, severity_level::error, "ProcessTask : FT_Read : FTDI_Error_To_Boost error : " + ec.message());
				ec = FTDI_Error_To_Boost (fstat);
				continue;
			}

			rxbuf->push_back (oneChar);

			if (oneChar == 0x0D) {	// Check for the end of line character (CR).
				break;
			}
		}

		// Eventually get success and copy the RX buffer over into the task.
		if (!task->RXBuf) {
			task->RXBuf = std::make_shared<std::vector<char> >();
		}

		size_t datalen = rxbuf->size() - 2;
		if (datalen > 0) {
			task->RXBuf->resize(datalen);
			auto b = rxbuf->begin() + 1;
			auto e = b + datalen;
			std::copy (b, e, task->RXBuf->begin());

		} else {
			task->RXBuf->clear();
		}
		break;
	}
}

//*****************************************************************************
size_t OmicronLedControllerBoardInterface::TxBufLenForTask (t_pXferTask task) {

	if (!task) {
		return 0;
	}

	std::size_t len = 0;
	len = 2 + task->TXBuf->size();	// +2 for the '?' and CR characters.

	return len;
}

//*****************************************************************************
OmicronLedControllerBoardInterface::t_ptxrxbuf OmicronLedControllerBoardInterface::CreateTXBUFForTask (t_pXferTask task) {
	assert(task);


//TODO: check for a '!" when receiving.


	t_ptxrxbuf txb(new t_txrxbuf());
	txb->resize (TxBufLenForTask(task));

	// Pointer to first character in the vector.
	char* cmd = (char*)(txb->data());

	// Pointer to the last character in the vector.
	char* cr = (char*)(txb->data() + txb->size() - 1);

	*cmd = '?';	// All commands begin with '?'.
	*cr = 0x0D;
	std::copy (task->TXBuf->begin(), task->TXBuf->end(), txb->begin() + 1);

	return txb;
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::FlushReadWriteBuffers() {

	FT_Purge (deviceSerialHandle_, FT_PURGE_RX | FT_PURGE_TX);
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::StartLocalIOService()
{
	StopLocalIOService();
	if (!pLocalThread_)
	{
		pLocalThread_ = std::make_unique<HawkeyeThread>("OmicronLedControllerBoardInterface_Thread");
	}
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::StopLocalIOService()
{
	pLocalThread_.reset();
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::PauseLocalIOService()
{
	if (pLocalThread_)
	{
		pLocalThread_->Pause();
	}
}

//*****************************************************************************
void OmicronLedControllerBoardInterface::ResumeLocalIOService()
{
	if (pLocalThread_)
	{
		pLocalThread_->Resume();
	}
	else
	{
		StartLocalIOService();
	}
}
