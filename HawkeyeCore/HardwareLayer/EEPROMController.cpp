#include "stdafx.h"

#include "EEPROMController.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static std::string MODULENAME = "EEPROMController";


#define ERASE_TIMEOUT_MSECS 20000
#define WRITE_TIMEOUT_MSECS 3000

EEPROMController::EEPROMController (std::shared_ptr<CBOService> pCBOService)
	: next_free_page(0xFFFF),
	cache_valid(false),
	data_loaded(false),
	isEEPROMBusy(false),
	register_cache({})
{
	pCBOService_ = pCBOService;
}

uint32_t EEPROMController::NumPages()
{
	return cache_valid ? (register_cache.ChipSize / EEPROM_PAGE_SIZE) : 0;
}

uint32_t EEPROMController::PagesRemaining()
{
	return (cache_valid && data_loaded) ? (NumPages() - next_free_page) : 0;
}

bool EEPROMController::ReadDataToCache(std::function<void(bool)> onComplete)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "ReadDataToCache <Enter> ");

	HAWKEYE_ASSERT (MODULENAME, onComplete);

	std::lock_guard<std::mutex> lg(mtx_pageandcache);

	if (isEEPROMBusy)
	{
		Logger::L().Log(MODULENAME, severity_level::notification, "ReadDataToCache <Exit - EEPROM BUSY> ");
		return false;
	}
	auto cb_wrapper = [this, onComplete](bool status)->void
	{
		// Release the EEPROM busy flag.
		isEEPROMBusy = false;
		if(!status)
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::controller_general_eepromreaderror,
                instrument_error::cntrlr_general_instance::none,
                instrument_error::severity_level::error));
		pCBOService_->enqueueExternal(onComplete, status);
	};

	// Set the EEPROM busy flag.
	isEEPROMBusy = true;

	pCBOService_->enqueueInternal ([this, cb_wrapper]()
	{
		// Trigger a refresh of the EEPROM data
		this->read(eReadEEPROMStates::eReadConfiguration, 0, cb_wrapper);
	});
	
	Logger::L().Log(MODULENAME, severity_level::debug1, "ReadDataToCache <exit> ");
	return true;
}

void EEPROMController::WriteData (uint16_t data_signature, const std::vector<unsigned char>& data, std::function<void(HawkeyeError)> onComplete)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "WriteData <Enter> ");

	std::lock_guard<std::mutex> lg(mtx_pageandcache);

	if (isEEPROMBusy)
	{
		Logger::L().Log(MODULENAME, severity_level::notification, "WriteData <Exit - EEPROM BUSY> ");
		pCBOService_->enqueueExternal (onComplete, HawkeyeError::eBusy);
	}

	if (!cache_valid || !data_loaded)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "WriteData <Exit - EEPROM data is invalid/not read> ");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::controller_general_eepromreaderror,
            instrument_error::cntrlr_general_instance::none,
            instrument_error::severity_level::error));
		pCBOService_->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
	}

	// Check data and data length before writing
	if (data_signature == 0xFFFF || data.empty() || !isDataLengthValid(data))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "WriteData - Invalid data or signature");
		pCBOService_->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
	}

	// Check the pages remaining
	if (data_map_.size() + 1 > NumPages())
	{
		// Should never happen 
		Logger::L().Log(MODULENAME, severity_level::critical, "WriteData - Out of pages");
//TODO: create and log error code...
		pCBOService_->enqueueExternal (onComplete, HawkeyeError::eHardwareFault);
	}
	
	// Page data shall include signature as well
	std::vector<unsigned char> tmp_page_data(EEPROM_PAGE_SIZE);

	std::copy((unsigned char*)&data_signature, ((unsigned char*)&data_signature) + sizeof(uint16_t), tmp_page_data.begin());
	std::copy(data.begin(), data.end(), tmp_page_data.begin() + sizeof(uint16_t));

	data_map_[data_signature] = tmp_page_data;

	auto cb_wrapper = [this, onComplete](bool status, uint16_t data_signature)-> void 
	{
		// Release the EEPROM busy flag
		isEEPROMBusy = false;
		HawkeyeError he = HawkeyeError::eSuccess;
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::controller_general_eepromwriteerror,
                instrument_error::cntrlr_general_instance::none,
                instrument_error::severity_level::error));

			// Remove the entry from map , if the write operation fails
			data_map_.erase (data_signature);

			he = HawkeyeError::eHardwareFault;
		}

		pCBOService_->enqueueExternal (onComplete, he);
	};

	// Set the EEPROM busy flag
	isEEPROMBusy = true;

	pCBOService_->enqueueInternal ([this, data_signature,tmp_page_data, cb_wrapper]()
	{
		boost::optional<std::pair<uint16_t, std::vector<unsigned char>>> data_to_write = std::make_pair(data_signature, tmp_page_data);

		// Write the given data to EPPROM.
		write (eWriteEEPROMStates::eWriteOnePageData, data_to_write, data_signature, cb_wrapper);
	});

	Logger::L().Log(MODULENAME, severity_level::debug2, "WriteData <exit>");
}

bool EEPROMController::Erase(std::function<void(bool)> onComplete)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "EraseEEPROM <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	std::lock_guard<std::mutex> lg(mtx_pageandcache);

	if (isEEPROMBusy)
	{
		Logger::L().Log(MODULENAME, severity_level::notification, "WriteData <Exit - EEPROM BUSY> ");
		return false;
	}

	auto cb_wrapper = [this, onComplete](bool status)-> void
	{
		// Release the EEPROM busy flag
		isEEPROMBusy = false;
		if(!status)
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::controller_general_eepromeraseerror,
                instrument_error::cntrlr_general_instance::none,
				instrument_error::severity_level::error));
		pCBOService_->enqueueExternal(onComplete, status);
	};

	// Set the EEPROM busy flag
	isEEPROMBusy = true;

	pCBOService_->enqueueInternal ([this, cb_wrapper]() 
	{
		eraseEEPROMInternal(cb_wrapper);
	});
	
	Logger::L().Log(MODULENAME, severity_level::debug1, "EraseEEPROM <exit> ");
	return true;
}

bool EEPROMController::GetData(uint16_t data_signature, std::vector<unsigned char>& data)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "GetData <Enter> ");

	std::lock_guard<std::mutex> lg(mtx_pageandcache);

	if (!data_loaded)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::controller_general_eepromreaderror,
            instrument_error::cntrlr_general_instance::none,
            instrument_error::severity_level::error));
		Logger::L().Log(MODULENAME, severity_level::critical, "GetData <Exit- EEPROM contents are not read/not initialized>");
		return false;
	}

	data.clear();
	auto page = data_map_.find(data_signature);

	if (page == data_map_.end() || page->second.empty())
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "GetData <Exit- Data not found> ");
		return false;
	}

	// Do not get signature data, hence moving the iterator 2 bytes
	data.resize(page->second.size() - 1); // Note: Resize of (size -1) so that one null character after assigning the contents of one page data(page->second) excluding 2 bytes of signature.
	data.assign(page->second.begin() + 2, page->second.end());

	Logger::L().Log(MODULENAME, severity_level::debug1, "GetData <exit> ");
	return true;
}

/********************* Private Internal API's *********************************/
void EEPROMController::write (eWriteEEPROMStates writeEEPROMState, 
                                   boost::optional<std::pair<uint16_t, std::vector<unsigned char>>> data_to_write,
                                   uint16_t data_signature,
                                   std::function<void(bool, uint16_t)> onComplete)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "write <Enter - state: " + toString(writeEEPROMState) + "> ");

	HAWKEYE_ASSERT (MODULENAME, onComplete);

	auto execute_next_state = [this, onComplete](ControllerBoardOperation::CallbackData_t cb_data,
                                                   boost::optional<std::pair<uint16_t, std::vector<unsigned char>>> data_to_write,
                                                   eWriteEEPROMStates writeEEPROMState,
                                                   uint16_t data_signature, 
                                                   bool islastdata = true)->void
	{
		if (cb_data.status != ControllerBoardOperation::eStatus::Success)
			writeEEPROMState = eWriteEEPROMStates::eWriteFailed;

		if (!islastdata)
			return;

		pCBOService_->enqueueInternal ([=]()
		{
			write(writeEEPROMState, data_to_write, data_signature, onComplete);
		});
	};

	switch (writeEEPROMState)
	{
		case eWriteEEPROMStates::eWriteOnePageData:
		{
			ControllerBoardOperation::CallbackData_t cb_data = {};
			cb_data.status = ControllerBoardOperation::eStatus::Success;

			// Out of space on the EEPROM.  Need to erase it.
			if (((next_free_page + 1) * EEPROM_PAGE_SIZE) > register_cache.ChipSize)
			{
				Logger::L().Log(MODULENAME, severity_level::notification, "write : EEPROM is full, erasing the EEPROM and writing fresh. Size:" + std::to_string(next_free_page));
				execute_next_state(cb_data, boost::none, eWriteEEPROMStates::eEraseCompletely, data_signature);
			}
			else
			{
				execute_next_state(cb_data, data_to_write, eWriteEEPROMStates::eWriteData, data_signature);
			}
		}
		break;
		case eWriteEEPROMStates::eEraseCompletely:
		{
			auto erase_eeprom_callback = [this, execute_next_state](ControllerBoardOperation::CallbackData_t cb_data, uint16_t data_signature)
			{
				eWriteEEPROMStates next_state = eWriteEEPROMStates::eReWriteCompleteData;
				if (cb_data.status != ControllerBoardOperation::eStatus::Success)
				{
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::controller_general_eepromeraseerror,
						instrument_error::cntrlr_general_instance::none,
						instrument_error::severity_level::error));
					next_state = eWriteEEPROMStates::eWriteFailed;
				}

				// Make sure to set the next free page after successful erase to zero, as there is nothing in the EEPROM.
				next_free_page = 0; 
				execute_next_state(cb_data, boost::none, next_state, data_signature);
			};

			pCBOService_->CBO()->Execute (EEPROMEraseOperation(),
				ERASE_TIMEOUT_MSECS,
				std::bind(erase_eeprom_callback, std::placeholders::_1, data_signature));
		}
		break;
		case eWriteEEPROMStates::eReWriteCompleteData:
		{
			ControllerBoardOperation::CallbackData_t cb_data = {};
			cb_data.status = ControllerBoardOperation::eStatus::Success;
			execute_next_state(cb_data, boost::none, eWriteEEPROMStates::eWriteData, data_signature);
		}
		break;
		case eWriteEEPROMStates::eWriteData:
		{
			// Write the complete cached data
			if (!data_to_write)
			{
				// Last data signature in the cache
				uint16_t last_data_signature = data_map_.crbegin()->first;
				for (const auto& data : data_map_)
				{
					bool is_last_data = data.first == last_data_signature;
					pCBOService_->CBO()->Execute (EEPROMWriteOperation(next_free_page, data.second),
						WRITE_TIMEOUT_MSECS,
						std::bind(execute_next_state, std::placeholders::_1, boost::none, eWriteEEPROMStates::eWriteSuccess, data.first, is_last_data));
					
					// Make sure the next guy in here gets a new page!
					next_free_page++;
				}
			}
			else// Write only the given single data
			{
				pCBOService_->CBO()->Execute (EEPROMWriteOperation(next_free_page, data_to_write.get().second),
					WRITE_TIMEOUT_MSECS,
					std::bind(execute_next_state, std::placeholders::_1, boost::none, eWriteEEPROMStates::eWriteSuccess, data_to_write.get().first));
				
				// Make sure the next guy in here gets a new page!
				next_free_page++;
			}
		}
		break;
		case eWriteEEPROMStates::eWriteSuccess:
		{
			pCBOService_->enqueueInternal (onComplete, true, data_signature);
			Logger::L().Log(MODULENAME, severity_level::debug2, "write <Exit - Data write to EEPROM is successful>");
		}
		break;
		case eWriteEEPROMStates::eWriteFailed:
		{
			pCBOService_->enqueueInternal (onComplete, false, data_signature);
			Logger::L().Log(MODULENAME, severity_level::error, "write <Exit - Failed to write the EEPROM data>");
		}
		break;
		default:
			//Invalid state, fatal error
			HAWKEYE_ASSERT (MODULENAME, false);
	}

	Logger::L().Log(MODULENAME, severity_level::debug2, "write <Exit- state: " + toString(writeEEPROMState) + "> ");
}

void EEPROMController::read(eReadEEPROMStates readEEPROMState, uint32_t pagenum, std::function<void(bool)> onComplete)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "read <Enter- state: " + toString(readEEPROMState) + "> ");

	auto execute_next_state = [this, onComplete](eReadEEPROMStates readEEPROMState, uint32_t page_num) 
	{
		pCBOService_->enqueueInternal ([=]() 
		{
			read(readEEPROMState, page_num, onComplete);
		});
	};

	switch (readEEPROMState)
	{
		case eReadEEPROMStates::eReadConfiguration:
		{
			auto callback = [this, execute_next_state](ControllerBoardOperation::CallbackData_t cb_data)
			{
				auto status = cb_data.status == ControllerBoardOperation::eStatus::Success;
				status = status && cb_data.rxBuf->size() == PersistentMemoryRegisterOffsets::pmr_Data;
				if (!status)
				{
					execute_next_state(eReadEEPROMStates::eReadFailed, 0);
					return;
				}

				this->cache_valid = true;
				std::copy(cb_data.rxBuf->begin(), cb_data.rxBuf->end(), (unsigned char*)(&this->register_cache));
				//Set the page number to read : 0 
				execute_next_state(eReadEEPROMStates::eSetPage, 0);
			};

			pCBOService_->CBO()->Query (EEPROMConfigurationReadOperation(), callback);
		}
		break;
		case eReadEEPROMStates::eSetPage:
		{
			auto callback = [this, execute_next_state](ControllerBoardOperation::CallbackData_t cb_data, uint32_t page_num)
			{
				if (cb_data.status != ControllerBoardOperation::eStatus::Success)
				{
					execute_next_state(eReadEEPROMStates::eReadFailed, 0);
					return;
				}

				//Read the page data
				execute_next_state(eReadEEPROMStates::eReadPage, page_num);
			};

			pCBOService_->CBO()->Execute (EEPROMSetPageToReadOperation(pagenum), WRITE_TIMEOUT_MSECS, std::bind(callback, std::placeholders::_1, pagenum));
		}
		break;
		case eReadEEPROMStates::eReadPage:
		{
			auto callback = [this, execute_next_state](ControllerBoardOperation::CallbackData_t cb_data, uint32_t page_num)
			{
				auto status = cb_data.status == ControllerBoardOperation::eStatus::Success;
				status = status && cb_data.rxBuf->size() == sizeof PersistentMemoryRegisters;
				if (!status)
				{
					execute_next_state(eReadEEPROMStates::eReadFailed, 0);
					return;
				}

				std::vector<unsigned char> tmp_buf = {};
				tmp_buf.assign(cb_data.rxBuf->begin() + PersistentMemoryRegisterOffsets::pmr_Data, cb_data.rxBuf->end());

				uint16_t sig = *((uint16_t*)tmp_buf.data());
				
				// Blank?  Done; set this page as next blank.
				if (sig == 0xFFFF)
				{
					this->data_loaded = true;
					this->next_free_page = page_num;
				}
				else
				{
					// Update page cache for signature.
					data_map_[sig].assign(tmp_buf.begin(), tmp_buf.end());
					tmp_buf.clear();

					// If more pages: schedule read else data read complete.
					if ((page_num + 1) < this->NumPages())
					{
						//Set the next page data to read
						execute_next_state(eReadEEPROMStates::eSetPage, page_num + 1);
						return;
					}

					// If no more pages to read, then it means EEPROM is full and read is complete
					this->data_loaded = true;
				}

				tmp_buf.clear();
				execute_next_state(eReadEEPROMStates::eReadSuccess, page_num);
			};

			pCBOService_->CBO()->Query(EEPROMReadOperation(), std::bind(callback, std::placeholders::_1, pagenum));
		}
		break;
		case eReadEEPROMStates::eReadSuccess:
		{
			Logger::L().Log(MODULENAME, severity_level::normal, "read: <Exit Successfully read the EEPROM contents>");
			pCBOService_->enqueueInternal(onComplete, true);
		}
		break;
		case eReadEEPROMStates::eReadFailed:
		{
			Logger::L().Log(MODULENAME, severity_level::error, "read: <Exit Failed to read the EEPROM contents>");
			pCBOService_->enqueueInternal(onComplete, false);
		}
		break;
		default:
			HAWKEYE_ASSERT (MODULENAME, false);
		break;
	}

	Logger::L().Log(MODULENAME, severity_level::debug2, "read <Exit- state: " + toString(readEEPROMState) + "> ");
}

void EEPROMController::eraseEEPROMInternal(std::function<void(bool)> onComplete)
{
	Logger::L().Log(MODULENAME, severity_level::debug2, "eraseEEPROMInternal <enter> ");

	auto cb_wrapper = [this, onComplete](ControllerBoardOperation::CallbackData_t cb_data)
	{
		if (cb_data.status != ControllerBoardOperation::eStatus::Success)
		{
			Logger::L().Log(MODULENAME, severity_level::error, "readEEPROMContents_readPage_cb :  Failed to erase EEPROM");
			pCBOService_->enqueueInternal(onComplete, false);
			return;
		}

		// Set the free page to zero before start writing
		next_free_page = 0;

		std::lock_guard<std::mutex> lg(mtx_pageandcache);
		data_map_.clear();

		pCBOService_->enqueueInternal(onComplete, true);
	};

	pCBOService_->CBO()->Execute(EEPROMEraseOperation(), ERASE_TIMEOUT_MSECS, cb_wrapper);
	
	Logger::L().Log(MODULENAME, severity_level::debug2, "eraseEEPROMInternal <exit> ");
}

std::string EEPROMController::toString(eReadEEPROMStates state)
{
	switch (state)
	{
		case eReadEEPROMStates::eReadConfiguration:
			return "ReadConfiguration";
		case eReadEEPROMStates::eSetPage:
			return "SetPage";
		case eReadEEPROMStates::eReadPage:
			return "ReadPage";
		case eReadEEPROMStates::eReadFailed:
			return "ReadFailed";
		case eReadEEPROMStates::eReadSuccess:
			return "ReadComplete";
		default:
			return "Invalid State";
	}

	assert(false);
}

std::string EEPROMController::toString(eWriteEEPROMStates state)
{
	switch (state)
	{
		case eWriteEEPROMStates::eWriteOnePageData:
			return "WriteOnePageData";
		case eWriteEEPROMStates::eEraseCompletely:
			return "EraseCompletely";
		case eWriteEEPROMStates::eReWriteCompleteData:
			return "RewriteCompleteData";
		case eWriteEEPROMStates::eWriteData:
			return "WriteEEPROMData";
		case eWriteEEPROMStates::eWriteSuccess:
			return "WriteComplete";
		case eWriteEEPROMStates::eWriteFailed:
			return "WriteFailed";
		default:
			return "Invalid State";
	}

	assert(false);
}
