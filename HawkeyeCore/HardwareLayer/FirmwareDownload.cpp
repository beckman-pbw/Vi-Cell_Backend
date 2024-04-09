#include "stdafx.h"

#include <array>
#include <algorithm>
#include <codecvt>

#include "BoardStatus.hpp"
#include "ErrorCode.hpp"
#include "ErrorStatus.hpp"
#include "FirmwareDownload.hpp"
#include "FTDI_Error_To_Boost.hpp"
#include "HawkeyeConfig.hpp"
#include "Logger.hpp"
#include "Registers.hpp"
#include "SecurityHelpers.hpp"

static const char MODULENAME[] = "FirmwareDownload";

#define FW_DOWNLOAD_RETRY_COUNT   3
#define IV_PASS_CODE              "00000000000000000000000000000000"
#define HASHKEY_PASSCODE_BIN      "11DA8861BFDB43BCBCEDEBB8B536EB3A"
#define FILE_PASSCODE_BIN         "./passcode.bin"
#define FW_DOWNLOAD_SM_TIMEOUT_MS 3000
#define MAX_REC_LEN               80u

//*****************************************************************************
FirmwareDownload::FirmwareDownload(std::shared_ptr<CBOService> pCBOService)
	: pCBOService_(pCBOService),
	_cb_firmware_progress(nullptr),
	_bin_srec_file(""),
	_ioss_srec(""),
	current_machine_state_(FIRMDOWN_WAIT),
	_has_timedout(false),
	_has_completed(false),
	_busyBitToTest(BoardStatus::DoNotCheckAnyBit)
{

	_pfirmware_wait_timer = std::make_shared <boost::asio::deadline_timer>(pCBOService_->getInternalIosRef());
}

FirmwareDownload::~FirmwareDownload()
{
	if (_pfirmware_wait_timer)
		_pfirmware_wait_timer->cancel();
}

//*****************************************************************************
/**
* \brief Register the callback function for the firmware download subsystem to repot the progress of the update.
* \param cb - the function pointer of the callback.
*/
void FirmwareDownload::SetFirmwareProgressCallback (firmware_progress_update_callback cb) 
{
	_cb_firmware_progress = cb;
}
//*****************************************************************************
bool FirmwareDownload::InitiateFirmwareUpdate(std::string binFile, std::string hashFile, boost::system::error_code & ec)
{
	Logger::L().Log (MODULENAME, severity_level::normal, "InitiateFirmwareUpdate : <Enter>");
	boost::filesystem::path pp(hashFile);

	std::ifstream ifsHash;
	ifsHash.open(pp.string());
	if (!ifsHash.is_open())
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::controller_general_firmwareinvalid, 
			instrument_error::cntrlr_general_instance::none, 
			instrument_error::severity_level::error));
		Logger::L().Log (MODULENAME, severity_level::error, "InitiateFirmwareUpdate : <Exit - Failed to open the firmware hash key file>");
		return false;
	}

	std::string file_hash((std::istreambuf_iterator<char>(ifsHash)), (std::istreambuf_iterator<char>()));
	_bin_srec_hashkey = file_hash;
	ifsHash.close();

	_bin_srec_file = binFile;
	if (!openBinary(ec))
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::controller_general_firmwareinvalid,
			instrument_error::cntrlr_general_instance::none, 
			instrument_error::severity_level::error));
		Logger::L().Log (MODULENAME, severity_level::error, "InitiateFirmwareUpdate : <Exit - Invalid/Corrupted firmware file>");
		return false;
	}

	set_machine(FIRMDOWN_START);
	Logger::L().Log (MODULENAME, severity_level::normal, "InitiateFirmwareUpdate : <Exit - Started the firmware update>");
	return true;
}
//**********************************************************************************************************************************************************
bool FirmwareDownload::start_k70_firmware_update(boost::system::error_code& ec)
{
	if (!_ifs_bin_srec.is_open()) 
		return false;
	reportFwDownloadCompletion(false, 0);
	return true;
}
//*****************************************************************************
void FirmwareDownload::closeall()
{
	_ifs_bin_srec.close();
	_ioss_srec.clear();
}
//*****************************************************************************
bool FirmwareDownload::openBinary(boost::system::error_code & ec)
{
	std::string filename = _bin_srec_file;
	boost::filesystem::path p(filename);
	try
	{
		_ifs_bin_srec.open (p.string(), std::ios_base::in | std::ios_base::binary);
		if (!_ifs_bin_srec.is_open()) {
			Logger::L().Log (MODULENAME, severity_level::error, "Unable to open encrypted s-record file :" + _bin_srec_file);
			return false;
		}
		if (!SecurityHelpers::SecurityDecryptFile(_ifs_bin_srec, _ioss_srec, FIRMWARE_ENCRYPTION_VI, _bin_srec_hashkey)) {
			return false;
		}

		_ioss_srec.seekg(0, _ioss_srec.end);
		_total_file_size = _ioss_srec.tellg();
		_ioss_srec.seekg(0, _ioss_srec.beg);
	}
	catch(std::exception const& e)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Unable to open s-record file (exception) :"+_bin_srec_file +" Error:"+e.what());
		ec = MAKE_ERRC(boost::system::errc::io_error);
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "Opened s-record file :" + _bin_srec_file);
	return true;
}
//*****************************************************************************
bool FirmwareDownload::extract_line_from_srec(std::string & line, FirmwareDownload::programming_state& programming_state, boost::system::error_code& ec)
{
	int16_t percentage_completion = 0;
	std::string record;

	float curr_pos;

	line.clear(); //we need to clear from the pervious chunk of data that was sent to EzPort.

	if (!read_srecord(record)) // if there are no more records
	{
		_ifs_bin_srec.close(); // close the file
		if (record.length() == 0)
		{
			programming_state = PROGRAMMING_DONE_SUCCESS; //We are at the end of the file. Previous tests which check if the file was unable to be opened and if the data was valid. 
			return true;
		}
		
		programming_state = PROGRAMMING_DONE_FAILED;
		return false;
	}

	curr_pos = static_cast<float>(_ioss_srec.tellg());
	if (curr_pos != 0 && curr_pos <= _total_file_size)
		percentage_completion = static_cast<int16_t>(curr_pos / _total_file_size * 100);

	if (programming_state == PROGRAMMING)
	{
		reportFwDownloadCompletion(false, percentage_completion);
		Logger::L().Log (MODULENAME, severity_level::debug2, "Programming srec" + record + " - " + std::to_string(percentage_completion) + "% Complete");
	}

	std::string initVector(IV_PASS_CODE);
	if (!SecurityHelpers::SecurityEncryptString(record, line, initVector, _ioss_pc.str()))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to encrypt srec");
		return false;
	}

	return true;
}
//*****************************************************************************
bool FirmwareDownload::read_srecord(std::string & record)
{
	record.clear();

	try
	{
		do // scan for a data record
		{
			std::getline(_ioss_srec, record);
			if (_ioss_srec.eof())
			{
				record.clear();
				break;
			}

			if (record.empty())
				return false;

			if (record.size() < 1 + 1 + 2 + 8 + 2 + 1) // Record size for 32byte srecord :  'S' - 1, Rec type - 1, Length - 2, Address - 8, Chksum - 2, CR - 1
				continue;

			if (record[0] != 'S')
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Illegal record header in: " + std::to_string(record[0]));
				return false;
			}

			if (record[1] == srecord_type::SREC_ENDRECORD32)
			{
				record.clear();
				return false;
			}

			if (record[1] != srecord_type::SREC_DATARECORD32 && 
			    record[1] != srecord_type::SREC_HEADER)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Unsupported Record Type in record : " + std::to_string(record[1]));
				return false;
			}
		} while (record[1] != srecord_type::SREC_DATARECORD32);

		// Validate the checksum
		uint8_t record_length = 0, byte = 0;
		auto strbyte = record.substr(2, 2);
		record_length = strhex_to_uint8(strbyte);

		uint32_t  sum = 0;
		for (int i = 1; i <= record_length; i++)
		{
			strbyte = record.substr(i * 2, 2);
			sum += strhex_to_uint8(strbyte);
		}
		strbyte = record.substr((record_length + 1) * 2, 2);
		if (((sum ^ 0xFF) & 0xFF) != strhex_to_uint8(strbyte))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "S-Record checksum error from: " + record);
			return false;
		}

		return true;
	}
	catch (std::exception const& e)
	{
		if (!_ioss_srec.eof())
			Logger::L().Log (MODULENAME, severity_level::error, "Exception occurred while reading entry from s-record. (" + record + ")\n exception:" + e.what());
	}
	return false;
}
//**********************************************************************************************************************************************************
void FirmwareDownload::rebootControllerBoard(void)
{
	auto onControllerBoardRebootComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
	{
		Logger::L ().Log (MODULENAME, severity_level::debug2, "rebootControllerBoard: <reboot callback>");
		if ( cbData.status == ControllerBoardOperation::Success )
		{
			// Controller board is going throug a reset, DO NOT try to talk.
			pCBOService_->CBO()->PauseOperation ();
			_pfirmware_wait_timer->expires_from_now (boost::posix_time::seconds (3));
			_pfirmware_wait_timer->async_wait (std::bind (&FirmwareDownload::waitForK70Reboot, this, std::placeholders::_1));
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("rebootControllerBoard: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			rerun_current_state ();
		}
	};

	auto tid = pCBOService_->CBO()->Execute(FwDownloadRebootOperation(), FW_DOWNLOAD_SM_TIMEOUT_MS, onControllerBoardRebootComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("FirmwareReboot, task %d") % (*tid)));
	}
	else
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "rebootControllerBoard: CBO Execute failed");
		switch_to_error_state ();
	}
}
//*****************************************************************************
void FirmwareDownload::waitForK70Reboot(const boost::system::error_code& ec)
{
	if ( ec )
	{
		Logger::L ().Log (MODULENAME, severity_level::critical, "MCU bootup wait, timer error" + ec.message ());
		switch_to_error_state ();
	}

	pCBOService_->CBO()->ResumeOperation ();
	switch_to_next_state ();
}
//**********************************************************************************************************************************************************
void FirmwareDownload::launchApplication(void)
{
	auto onAppLaunchComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
	{
		Logger::L ().Log (MODULENAME, severity_level::debug2, "launchApplication: <launch callback>");
		if ( cbData.status == ControllerBoardOperation::Success )
		{
			// Controller board is going throug a reset, DO NOT try to talk.
			pCBOService_->CBO()->PauseOperation ();
			_pfirmware_wait_timer->expires_from_now (boost::posix_time::seconds (3));
			_pfirmware_wait_timer->async_wait (std::bind (&FirmwareDownload::waitForFwApplicationBootup, this, std::placeholders::_1));
		}
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("launchApplication: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			rerun_current_state ();
		}
	};

	auto tid = pCBOService_->CBO()->Execute(FwDownloadLaunchAppOperation(), FW_DOWNLOAD_SM_TIMEOUT_MS, onAppLaunchComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("FirmwareLaunchApp, task %d") % (*tid)));
	}
	else
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "launchApplication: CBO Execute failed");
		switch_to_error_state ();
	}
}
//*****************************************************************************
void FirmwareDownload::waitForFwApplicationBootup (const boost::system::error_code& ec)
{
	if (ec)
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "Application bootup wait, timer error" + ec.message());
		switch_to_error_state();
		return;
	}

	pCBOService_->CBO()->ResumeOperation ();
	switch_to_next_state ();
}
//**********************************************************************************************************************************************************
void FirmwareDownload::eraseProgramFlash(void)
{
	auto onProgramFlashEraseComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
	{
		Logger::L ().Log (MODULENAME, severity_level::debug2, "eraseProgramFlash: <erase callback>");
		if ( cbData.status == ControllerBoardOperation::Success )
			switch_to_next_state ();
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("eraseProgramFlash: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			rerun_current_state ();
		}
	};

	auto tid = pCBOService_->CBO()->Execute(FwDownloadEraseFlashOperation(), FW_DOWNLOAD_SM_TIMEOUT_MS, onProgramFlashEraseComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("FirmwareEraseFlash, task %d") % (*tid)));
	}
	else
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "eraseProgramFlash: CBO Execute failed");
		switch_to_error_state ();
	}
}
//**********************************************************************************************************************************************************
void FirmwareDownload::programSrecToFlash(std::string & rec)
{
	auto onProgramFlashWriteComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
	{
		Logger::L ().Log (MODULENAME, severity_level::debug2, "programSrecToFlash: <program srec callback>");
		if ( cbData.status == ControllerBoardOperation::Success )
			switch_to_next_state ();
		else
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("programSrecToFlash: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			rerun_current_state ();
		}
	};

	char recData[MAX_REC_LEN];
	if ( rec.length () > MAX_REC_LEN )
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "programSrecToFlash: Record length exceeds max");
		switch_to_error_state ();
		return;
	}

	Logger::L ().Log (MODULENAME, severity_level::debug2, "programSrecToFlash: <enter>");
	rec.copy ((char *)recData, rec.length());

	auto tid = pCBOService_->CBO()->Execute(FwDownloadProgramFlashOperation((uint8_t)rec.length(), (uint8_t *)recData), FW_DOWNLOAD_SM_TIMEOUT_MS, onProgramFlashWriteComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("FirmwareDownloadProgramFlash, task %d") % (*tid)));
	}
	else
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "programSrecToFlash: CBO Execute failed");
		switch_to_error_state ();
	}

}
//**********************************************************************************************************************************************************
uint8_t FirmwareDownload::strhex_to_uint8(const std::string& rec)
{
	uint8_t byte;
	byte = static_cast<uint8_t>(std::stoul(rec, nullptr, 16));
	return byte;
}
//*****************************************************************************
void FirmwareDownload::reportFwDownloadCompletion (bool finished, int16_t percentage_completeion) const
{
	if (_cb_firmware_progress)
	{
		pCBOService_->enqueueExternal (std::bind(_cb_firmware_progress, finished, percentage_completeion));
	}
}
//*****************************************************************************
bool FirmwareDownload::set_machine(machine_states state)
{
	current_machine_state_ = state;

	// Push task off to internal IO Service for later processing.
	pCBOService_->enqueueInternal(std::bind(&FirmwareDownload::run_state_machine, this));
	return true;
}
//*****************************************************************************
bool FirmwareDownload::switch_to_next_state(void)
{
	current_machine_state_ = next_machine_state_;

	// Push task off to internal IO Service for later processing.
	pCBOService_->enqueueInternal(std::bind(&FirmwareDownload::run_state_machine, this));
	return true;
}
//*****************************************************************************
bool FirmwareDownload::rerun_current_state(void)
{
	// Push task off to internal IO Service for later processing.
	pCBOService_->enqueueInternal(std::bind(&FirmwareDownload::run_state_machine, this));

	return true;
}
//**********************************************************************************************************************************************************
void FirmwareDownload::run_state_machine()
{
	static programming_state done_programming = PROGRAMMING;
	static std::string srecord;
	static boost::system::error_code ec = MAKE_SUCCESS;
	static uint32_t retry_connect_count = FW_DOWNLOAD_RETRY_COUNT;
	static uint32_t retry_erase_count = FW_DOWNLOAD_RETRY_COUNT;
	static uint32_t retry_launch_count = FW_DOWNLOAD_RETRY_COUNT;
	static uint32_t retry_program_count = FW_DOWNLOAD_RETRY_COUNT;

	switch (current_machine_state_) 
	{
		case FIRMDOWN_WAIT:
			break;

		case FIRMDOWN_START:
			retry_connect_count = FW_DOWNLOAD_RETRY_COUNT;
			retry_erase_count = FW_DOWNLOAD_RETRY_COUNT;
			retry_launch_count = FW_DOWNLOAD_RETRY_COUNT;
			retry_program_count = FW_DOWNLOAD_RETRY_COUNT;

			prev_fw_version_read = false;

			if (!start_k70_firmware_update(ec)) 
				set_machine(FIRMDOWN_ERROR);
			else
				set_machine (FIRMDOWN_READ_FW_TYPE);
			break;

		case FIRMDOWN_READ_FW_TYPE:
		{
			auto onFwVersionRegReadComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
			{
				Logger::L ().Log (MODULENAME, severity_level::debug2, "run_state_machine: <FW version read callback>");
				if ( cbData.status == ControllerBoardOperation::Success )
				{
					uint32_t fwVer;
					cbData.FromRx (&fwVer, sizeof (uint32_t));

					// Existing firmware version (only record it the first time through the code)
					if (!prev_fw_version_read)
					{
						prev_fw_version_read = true;
						prev_firmware_ver = fwVer;
					}

					if ( (fwVer & FW_IDENTIFIER_BOOTLOADER) == FW_IDENTIFIER_BOOTLOADER )
						set_machine (FIRMDOWN_ERASE_FLASH);
					else
						set_machine (FIRMDOWN_SWITCH_TO_BOOTLOADER);
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("run_state_machine:FIRMDOWN_READ_FW_TYPE 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
					set_machine (FIRMDOWN_ERROR);
				}
			};

			auto tid = pCBOService_->CBO()->Query(FwDownloadReadFwVersionOperation(), onFwVersionRegReadComplete);
			if ( tid )
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("run_state_machine: ReadFwVersion, task %d") % (*tid)));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "run_state_machine: CBO Query failed");
				set_machine(FIRMDOWN_ERROR);
			}
			
			break;
		}

		case FIRMDOWN_SWITCH_TO_BOOTLOADER:
			if ( retry_connect_count == 0 )
			{
				Logger::L ().Log (MODULENAME, severity_level::error, "Failed switching to bootloader");
				set_machine (FIRMDOWN_ERROR);
				break;
			}

			Logger::L ().Log (MODULENAME, severity_level::debug1, "Controller Board Reboot. remaining attempts: " + std::to_string (retry_connect_count));
			rebootControllerBoard ();
			next_machine_state_ = FIRMDOWN_READ_FW_TYPE;
			--retry_connect_count;
			break;

		case FIRMDOWN_ERASE_FLASH:
			if (retry_erase_count == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Flash erase. failed: ");
				set_machine(FIRMDOWN_ERROR);
				break;
			}

			next_machine_state_ = FIRMWARE_READ_KEYS;
			Logger::L().Log (MODULENAME, severity_level::debug1, "Flash Erase. remaining attempts: " + std::to_string(retry_erase_count));
			eraseProgramFlash();
			--retry_erase_count;
			break;

		case FIRMWARE_READ_KEYS:
		{
			std::string initVector(IV_PASS_CODE);
			std::ifstream pc;
			std::string   bin_pc_hashkey = HASHKEY_PASSCODE_BIN;

			_ioss_pc.clear();

			pc.open(FILE_PASSCODE_BIN, std::ios_base::in | std::ios_base::binary);
			if (!pc.is_open())
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to open passcode");
				set_machine(FIRMDOWN_ERROR);
				break;
			}

			if (!SecurityHelpers::SecurityDecryptFile(pc, _ioss_pc, FIRMWARE_ENCRYPTION_VI, bin_pc_hashkey))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to decrypt passcode");
				set_machine(FIRMDOWN_ERROR);
				break;
			}

			set_machine(FIRMDOWN_GET_RECORD);
			break;
		}

		case FIRMDOWN_GET_RECORD: 
			if (!extract_line_from_srec(srecord, done_programming, ec)) 
				set_machine(FIRMDOWN_ERROR);
			else
			{
				if (done_programming == PROGRAMMING_DONE_FAILED)
				{
					set_machine(FIRMDOWN_ERROR);
				}
				else if (done_programming == PROGRAMMING)
				{
					retry_program_count = FW_DOWNLOAD_RETRY_COUNT;
					set_machine(FIRMDOWN_SEND_CHUNK_TO_K70);
				}
				else
				{
					_ioss_pc.clear();
					set_machine(FIRMDOWN_LAUNCH_APP);
				}
			}
			break;

		case FIRMDOWN_SEND_CHUNK_TO_K70:
			if (retry_program_count == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Programming failed ");
				set_machine(FIRMDOWN_ERROR);
				break;
			}

			next_machine_state_ = FIRMDOWN_GET_RECORD;
			Logger::L().Log (MODULENAME, severity_level::debug2, "Programming, attempts remaining: " + std::to_string(retry_program_count));
			programSrecToFlash (srecord);
			--retry_program_count;
			break;

		case FIRMDOWN_ERROR:
			Logger::L().Log (MODULENAME, severity_level::error, "Error occurred in firmware update state machine. ec: " + ec.message());
			_ioss_pc.clear();
			reportFwDownloadCompletion(true, -1);
			closeall();
			set_machine(FIRMDOWN_WAIT);
			break;

		case FIRMDOWN_LAUNCH_APP:
			if (retry_launch_count == 0)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to launch controller board application ");
				set_machine(FIRMDOWN_ERROR);
				break;
			}

			next_machine_state_ = FIRMDOWN_END;
			Logger::L().Log (MODULENAME, severity_level::debug1, "Download complete, launching application, attempts remaining : " + std::to_string(retry_launch_count));
			launchApplication();
			--retry_launch_count;
			break;

		case FIRMDOWN_END:
		{
			_ioss_pc.clear();
			pCBOService_->CBO()->ReadFirmwareVersion([this](bool status)->void {}); // This is required for updating the System status structure later


			// Read the firmware version to make sure for one last time , the application firmware booted up successfully.
			auto onAppFwVersionReadComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void
			{
				Logger::L ().Log (MODULENAME, severity_level::debug2, "run_state_machine: <FW version read callback>");
				if ( cbData.status == ControllerBoardOperation::Success )
				{
					closeall ();

					uint32_t fwVer;
					cbData.FromRx (&fwVer, sizeof (uint32_t));

					//Updated firmware version.
					curr_firmware_ver = fwVer;

					Logger::L ().Log (MODULENAME, severity_level::debug1, "Controller Board Firmware version: " 
						+ std::to_string ((fwVer & 0xFF000000) >> 24u)
						+ std::to_string ((fwVer & 0x00FF0000) >> 16u)
						+ std::to_string ((fwVer & 0x0000FF00) >> 8u)
						+ std::to_string (fwVer & 0x000000FF));

					if ( (fwVer & FW_IDENTIFIER_BOOTLOADER) == FW_IDENTIFIER_BOOTLOADER )
					{
						ReportSystemError::Instance().ReportError(
							BuildErrorInstance (instrument_error::controller_general_firmwarebootup, instrument_error::cntrlr_general_instance::none, instrument_error::severity_level::error));
						reportFwDownloadCompletion (true, -1);
					}
					else
						reportFwDownloadCompletion (true, 100);
					Logger::L ().Log (MODULENAME, severity_level::notification, "Controller Board Firmware programming complete, final State: " + programming_state_string (done_programming));
					set_machine (FIRMDOWN_WAIT);
				}
				else
				{
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("run_state_machine:FIRMDOWN_END 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
					set_machine (FIRMDOWN_ERROR);
				}
			};

			auto tid = pCBOService_->CBO()->Query(FwDownloadReadFwVersionOperation(), onAppFwVersionReadComplete);
			if (tid)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("run_state_machine: ReadFwVersion, task %d") % (*tid)));
			}
			else
			{
				Logger::L ().Log (MODULENAME, severity_level::error, "run_state_machine: CBO Query failed");
				set_machine (FIRMDOWN_ERROR);
			}
			break;
		}
		default:
			Logger::L().Log (MODULENAME, severity_level::error, "Unknown state.");
			break;
	}
}
