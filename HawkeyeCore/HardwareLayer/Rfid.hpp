#pragma once

#include <stdint.h>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "RfidBase.hpp"

class Rfid : public RfidBase
{
public:
	enum class OperationSequence {
		ReadRfidData,
		ReadRfidTag,
	};

	Rfid (std::shared_ptr<CBOService> pCBOService);
	virtual ~Rfid() override;

	virtual void scan (std::function<void(bool)> callback) override;
	virtual void read (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback) override;
	virtual void setValveMap (std::function<void(bool)> callback, ReagentValveMap_t valveMap) override;
	virtual void setTime (std::function<void(bool)> callback) override;
	void logError (uint32_t errorCode);

protected:

private:
	typedef struct {
		size_t numTagsRead;
		size_t numTagsProcessed;
		std::shared_ptr<std::vector<RfidTag_t>> rfidTags;
	} ProcessAsyncData;

	void processAsync        (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback, OperationSequence nextSequence, std::shared_ptr<ProcessAsyncData> processAsyncData);
	void setNextReadSequence (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback, OperationSequence nextSequence, std::shared_ptr<ProcessAsyncData> processAsyncData);
	bool isTagValid (size_t tagNum, const TagStatus& Status);

	size_t numValvesProcessed_;
	size_t numValvesToProcess_;
	ReagentValveMap_t valveMap_;
	std::string logStr_;
};

class RfidOperation : public ControllerBoardOperation::Operation {
public:
	enum eRfidCommand {
		Scan = ReagentCommandRFScanTagMode,
		SetTime = ReagentCommandSetTime,
		SetValveMap = ReagentCommandMapTagValue,
		Reset = ReagentCommandRFReaderReset,     // Only used in RFIDProgrammingStation.
		ProgramTag = ReagentCommandProgTag,      // Only used in RFIDProgrammingStation.
	};

	RfidOperation() {
		Operation::Initialize (&regData_);
		regAddr_ = ReagentRegs;
	}

	RfidOperation (RegisterIds regAddrForOperation) {
		Operation::Initialize (&regData_);
		regAddr_ = regAddrForOperation;
	}

//TODO: example constructor providing ancillary data.
	RfidOperation (RegisterIds regAddrForOperation, uint32_t* regData, size_t regDataLen /* length in bytes */) {
		Operation::Initialize (&regData_);
		regAddr_ = regAddrForOperation;
		Operation::regData_ = (void*)regData;
		lengthInBytes_ = regDataLen;
	}

protected:
	ReagentRegisters regData_;
};

class RfidScanOperation : public RfidOperation {
public:
	RfidScanOperation() {
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(ReagentCommandRFScanTagMode);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class RfidSetTimeOperation : public RfidOperation {
public:
	RfidSetTimeOperation (uint32_t time) {
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(ReagentCommandSetTime);
		regData_.CommandParam = time;
		lengthInBytes_ = sizeof(uint32_t) * 2;
	}
};

class RfidSetValveMapOperation : public RfidOperation {
public:
	RfidSetValveMapOperation (const ReagentValveMap_t& valveMap) {
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(ReagentCommandMapTagValue);
		regData_.CommandParam = valveMap.params;
		lengthInBytes_ = sizeof(uint32_t) * 2;
	}
};

// Read the RFID reader header data from the RFID scan.
class RfidReadOperation : public RfidOperation {
public:
	RfidReadOperation() {
		mode_ = ReadMode;
		lengthInBytes_ = sizeof(uint32_t) * 10;
	}
};


//TODO: incomplete...
// Read the data for a specific RFID tag.
class RfidReadTagDataOperation : public RfidOperation {
public:
	RfidReadTagDataOperation (const RegisterIds registerAddress)
		: RfidOperation(registerAddress) {
		mode_ = ReadMode;
		lengthInBytes_ = sizeof(RfAppdataRegisters);
	}
};
