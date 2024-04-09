/*
Interface to a Flash EEPROM chip with the following properties:
	Total Flash = 524288 bytes(512 Kbytes)
	Address space    : 0x00 to 0x80000
	Page size        : 256 bytes
	Sector size      : 4096 bytes(4 Kbytes) (128 Sectors but should check properties at startup)
	Pages per sector : 16 pages each of 256 bytes size

Erase: presently only BULK erasable (20 seconds) implemented, but does support sector erase (3sec)

Cannot write to an address without it being flashed first.
Probably can only write 1 page at a time.

What we're going to do is write a page at a time with each page dedicated to a particular caller-defined two-byte signature.
Whenever we need to write new data, we'll write it to the next open page.
Initialization will read page by page until we reach the first un-written page.  The most recent page for each "signature" will be maintained.
*/

#pragma once

#include "CBOService.hpp"
#include "HawkeyeError.hpp"

#define EEPROM_PAGE_SIZE 256

class EEPROMController
{
public:
	EEPROMController (std::shared_ptr<CBOService> pCBOService);
	virtual ~EEPROMController() = default;
	
	bool     ReadDataToCache(std::function<void(bool)> onCompletionCb);           // Reads EEPROM and gets all current pages.
	void     WriteData(uint16_t data_signature, 
                               const std::vector<unsigned char>& data, 
                               std::function<void(HawkeyeError)> onComplete);             // Data must be 254 bytes or less (first two bytes of page used for signature) Signature cannot be 0xFFFF.
	bool     Erase(std::function<void(bool)>onCompletionCb);
	bool     GetData(uint16_t data_signature, std::vector<unsigned char>& data);  // Retrieve cached copy of most recent data page for the signature.
	uint32_t NumPages();                                                                   // Returns 0: unknown.
	uint32_t PagesRemaining();                                                             // Returns 0: unknown.

private:
	enum class eWriteEEPROMStates: uint16_t
	{
		eWriteOnePageData = 0,
		eEraseCompletely,
		eReWriteCompleteData,
		eWriteData,
		eWriteSuccess,
		eWriteFailed
	};

	enum class eReadEEPROMStates : uint16_t
	{
		eReadConfiguration = 0,
		eSetPage,
		eReadPage,
		eReadSuccess,
		eReadFailed
	};

	std::string toString(eReadEEPROMStates state);
	std::string toString(eWriteEEPROMStates state);
	
	// Write data to EEPROM
	void write (eWriteEEPROMStates writeEEPROMState,
                boost::optional<std::pair<uint16_t, std::vector<unsigned char>>> data_to_write,
                uint16_t data_signature,
                std::function<void(bool, uint16_t)> onCompleteCb);

	// Read contents of EEPROM: Get chip size, then read all pages until we find an empty one.
	void read (eReadEEPROMStates readEEPROMState,
               uint32_t pagenum,
               std::function<void(bool)> onCompleteCb);

	inline uint32_t Pagesize()
	{
		return EEPROM_PAGE_SIZE - 2/*Data signature length*/;
	}

	bool isDataLengthValid(const std::vector<unsigned char>& data)
	{
		return data.size() <= Pagesize();
	}

	// Erase: erase chip
	void eraseEEPROMInternal(std::function<void(bool)> onCompleteCb);
	std::mutex mtx_pageandcache;
	std::atomic<bool> isEEPROMBusy;
	uint32_t next_free_page;

	bool cache_valid;
	bool data_loaded;
	PersistentMemoryRegisters register_cache;

	std::mutex mtx_pageandcache_;
	std::map<uint16_t, std::vector<unsigned char>> data_map_ = {};
	std::shared_ptr<CBOService> pCBOService_;
};

class EEPROMOperation : public ControllerBoardOperation::Operation
{
public:

	EEPROMOperation()
	{
		Operation::Initialize(&regs_);
		regAddr_ = PersistentMemoryRegs;
	}

protected:
	PersistentMemoryRegisters regs_;
};

class EEPROMReadOperation : public EEPROMOperation
{
public:
	EEPROMReadOperation()
	{
		mode_ = ReadMode;
		lengthInBytes_ = sizeof(PersistentMemoryRegisters); 
	}
};

class EEPROMConfigurationReadOperation : public EEPROMOperation
{
public:
	EEPROMConfigurationReadOperation()
	{
		mode_ = ReadMode;
		lengthInBytes_ = PersistentMemoryRegisterOffsets::pmr_Data;                                                // Only config parameter, exclude data
	}
};

class EEPROMSetPageToReadOperation : public EEPROMOperation
{
public:
	EEPROMSetPageToReadOperation(uint32_t pagenum)
	{
		regs_ = {};
		mode_ = WriteMode;
		regs_.Command = PeristentMemoryCommands::ReadEEPROM;
		regs_.Address = pagenum * EEPROM_PAGE_SIZE;
		regs_.Length = EEPROM_PAGE_SIZE;
		lengthInBytes_ = PersistentMemoryRegisterOffsets::pmr_ErrorCode;
	}
};

class EEPROMWriteOperation : public EEPROMOperation
{
public:
	EEPROMWriteOperation(uint32_t next_free_page, std::vector<unsigned char> data)
	{
		mode_ = WriteMode;
		regs_.Command = PeristentMemoryCommands::WriteEEPROM;
		regs_.Address = next_free_page * EEPROM_PAGE_SIZE;
		regs_.Length = EEPROM_PAGE_SIZE;
		regs_.ErrorCode = 0;
		std::copy(data.begin(), data.end(), regs_.Data);
		lengthInBytes_ = sizeof(PersistentMemoryRegisters);
	}
};

//Complete Chip Erase operation
class EEPROMEraseOperation : public EEPROMOperation
{
public:
	EEPROMEraseOperation()
	{
		regs_ = {};
		mode_ = WriteMode;
		regs_.Command = PeristentMemoryCommands::EraseEEPROM;
		regs_.Address = 0;
		regs_.Length = 0;
		lengthInBytes_ = PersistentMemoryRegisterOffsets::pmr_ErrorCode;
	}
};
