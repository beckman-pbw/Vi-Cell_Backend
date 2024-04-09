#pragma once

#include <sstream>
#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/system/error_code.hpp>

#include "FirmwareDownloadCrypto.h"
#include "CBOService.hpp"

#define FW_IDENTIFIER_BOOTLOADER  0x04000000u
#define FW_IDENTIFIER_APPLICATION 0x01000000u
#define FW_IDENTIFIER_MASK        0xFF000000u

#ifndef firmware_progress_update_callback
typedef std::function<void(bool, int16_t)> firmware_progress_update_callback;
#endif

class FirmwareDownload
{
public:
	typedef std::function< void(boost::system::error_code)> t_fimware_completion_cb;

	FirmwareDownload(std::shared_ptr<CBOService> pCBOService);
	~FirmwareDownload();
	void SetFirmwareProgressCallback(firmware_progress_update_callback cb);
	bool InitiateFirmwareUpdate(std::string binFile, std::string hashFile, boost::system::error_code& ec);

	std::string GetFirmwareVersionString(uint32_t firm_ver)
	{
		return  boost::str(boost::format("%d.%d.%d.%d")
						   % ((firm_ver & 0xFF000000) >> 24)
						   % ((firm_ver & 0x00FF0000) >> 16)
						   % ((firm_ver & 0x0000FF00) >> 8)
						   % ((firm_ver & 0x000000FF)));
	}

	uint32_t GetPreviousFirmwareVersion()
	{
		return prev_firmware_ver;
	}

	uint32_t GetCurrentFirmwareVersion()
	{
		return curr_firmware_ver;
	}

private:
	
	enum srecord_type
	{
		SREC_HEADER = '0',
		SREC_DATARECORD16 = '1',
		SREC_DATARECORD24 = '2',
		SREC_DATARECORD32 = '3',
		SREC_COUNT16 = '5',
		SREC_COUNT24 = '6',
		SREC_ENDRECORD32 = '7',
		SREC_ENDRECORD24 = '8',
		SREC_ENDRECORD16 = '9',
		SREC_Invalid = -1
	};

	enum programming_state
	{
		PROGRAMMING = 0,
		PROGRAMMING_DONE_FAILED,
		PROGRAMMING_DONE_SUCCESS
	};

	enum machine_states
	{
		FIRMDOWN_WAIT,
		FIRMDOWN_START,
		FIRMDOWN_READ_FW_TYPE,
		FIRMDOWN_SWITCH_TO_BOOTLOADER,
		FIRMDOWN_ERASE_FLASH,
		FIRMWARE_READ_KEYS,
		FIRMDOWN_GET_RECORD,
		FIRMDOWN_SEND_CHUNK_TO_K70,
		FIRMDOWN_ERROR,
		FIRMDOWN_LAUNCH_APP,
		FIRMDOWN_END
	};

	static auto programming_state_string(programming_state e)
	{
		const std::map<programming_state, std::string> enum_ostring{
			{ PROGRAMMING, "Programming" },
			{ PROGRAMMING_DONE_FAILED, "Failed Programming" },
			{ PROGRAMMING_DONE_SUCCESS, "Successfully Programmed" }
		};
		auto   it = enum_ostring.find(e);
		return it == enum_ostring.end() ? "Out of range" : it->second;
	}

	void run_state_machine();

	bool start_k70_firmware_update(boost::system::error_code& ec);
	bool openBinary(boost::system::error_code& ec);
	void closeall();
	bool extract_line_from_srec(std::string & line, FirmwareDownload::programming_state& programming_state, boost::system::error_code& ec);
	bool read_srecord(std::string & record);
	void reportFwDownloadCompletion (bool finished, int16_t percentage_completeion) const;
	bool set_machine(machine_states state);
	machine_states get_current_state(void) const { return current_machine_state_; }
	bool switch_to_next_state(void);
	bool rerun_current_state(void);
	bool switch_to_error_state(void) { return set_machine(FIRMDOWN_ERROR); }

	void rebootControllerBoard(void);
	void eraseProgramFlash(void);
	void programSrecToFlash(std::string & rec);
	void launchApplication(void);

	void waitForK70Reboot (const boost::system::error_code& ec);
	void waitForFwApplicationBootup(const boost::system::error_code& ec);
	static uint8_t strhex_to_uint8(const std::string& rec);

	std::shared_ptr<CBOService> pCBOService_;
	firmware_progress_update_callback _cb_firmware_progress;

	std::string  _bin_srec_file;
	std::string  _bin_srec_hashkey;
	std::stringstream _ioss_srec;
	std::ifstream _ifs_bin_srec;
	std::streampos _total_file_size;

	std::stringstream _ioss_pc;
	
	std::shared_ptr<boost::asio::deadline_timer> _pfirmware_wait_timer;
	machine_states current_machine_state_;
	machine_states next_machine_state_;
	t_fimware_completion_cb do_completion_cb_;
	bool _has_timedout;
	bool _has_completed;
	BoardStatus _boardStatus;
	BoardStatus::StatusBit _busyBitToTest;

	//Previous firmware version
	bool prev_fw_version_read;
	uint32_t prev_firmware_ver = 0;

	//Current firmware version
	uint32_t curr_firmware_ver = 0;

};

class FwDownloadOperation : public ControllerBoardOperation::Operation
{
public:
	enum eFwDownloadCommand
	{
		Reboot = 1,
		EraseFlash = 2,
		DownloadFw = 3,
		LauchApp = 4,
		CalculateCRC = 5
	};

protected:
	FwDownloadOperation ()
	{
		Operation::Initialize (&regs_);
		regAddr_ = RegisterIds::FwUpdateRegs;
	}

	FwUpdateRegisters regs_;
};

class FwDownloadReadFwTypeOperation : public ControllerBoardOperation::Operation
{
public:

	FwDownloadReadFwTypeOperation ()
	{
		Operation::Initialize (&regs_);
		regAddr_ = RegisterIds::SwVersion;
	}

protected:
	FwUpdateRegisters regs_;
};

class FwDownloadReadFwVersionOperation : public FwDownloadReadFwTypeOperation
{
public:
	FwDownloadReadFwVersionOperation ()
	{
		mode_ = ReadMode;
		lengthInBytes_ = sizeof (uint32_t);
	}
};

class FwDownloadRebootOperation : public FwDownloadOperation
{
public:
	FwDownloadRebootOperation ()
	{
		mode_ = WriteMode;
		regs_.Command = static_cast<uint32_t>(FwDownloadOperation::Reboot);
		lengthInBytes_ = sizeof (uint32_t);
	}
};

class FwDownloadEraseFlashOperation : public FwDownloadOperation
{
public:
	FwDownloadEraseFlashOperation ()
	{
		mode_ = WriteMode;
		regs_.Command = static_cast<uint32_t>(FwDownloadOperation::EraseFlash);
		regs_.ErrorCode = 0;
		lengthInBytes_ = sizeof (uint32_t) * 2u;
	}
};

class FwDownloadProgramFlashOperation : public FwDownloadOperation
{
public:
	FwDownloadProgramFlashOperation (uint8_t paramLen, uint8_t *cmdParam)
	{
		mode_ = WriteMode;
		regs_.Command = static_cast<uint32_t>(FwDownloadOperation::DownloadFw);
		regs_.ParamLen = paramLen;
		regs_.ErrorCode = 0;
		regs_.AppImageCRC = 0;
		memcpy (regs_.CmdParam, cmdParam, paramLen);
		lengthInBytes_ = (sizeof (uint32_t) * 4u) + paramLen;
	}
};

class FwDownloadLaunchAppOperation : public FwDownloadOperation
{
public:
	FwDownloadLaunchAppOperation ()
	{
		mode_ = WriteMode;
		regs_.Command = static_cast<uint32_t>(FwDownloadOperation::LauchApp);
		regs_.ErrorCode = 0;
		lengthInBytes_ = sizeof (uint32_t) * 2;
	}
};

