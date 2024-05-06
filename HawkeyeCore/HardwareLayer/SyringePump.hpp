#pragma once

#include <atomic>

#include "SyringePumpBase.hpp"

class SyringePump : public SyringePumpBase
{
public:

#define SCALE_FACTOR 10

	enum class OperationSequence {
		SetConfiguration,
		SendDirectCommand,
		Initialize,
		ReadVersion,
	};

	SyringePump (std::shared_ptr<CBOService> pCBOService);
	virtual ~SyringePump();

	void initialize(std::function<void(bool)> callback);
	void setPosition (std::function<void(bool)> callback, uint32_t volume, uint32_t speed);
	bool getValve (SyringePumpPort& port);
	void setValve (std::function<void(bool)> callback, SyringePumpPort port, SyringePumpDirection direction);
	bool getPhysicalValve (PhysicalPort& valveNum);
	void setPhysicalValve (std::function<void(bool)> callback, char port, SyringePumpDirection direction);
	bool getPosition(uint32_t& pos);
	std::string getVersion();
	void rotateValve (std::function<void(bool)> callback, uint32_t angle, SyringePumpDirection direction);
	void sendValveCommand(std::function<void(bool)> callback, std::string command);

private:
	void processAsync (std::function<void(bool)> callback, SyringePump::OperationSequence sequence);
	void setNextProcessSequence (std::function<void(bool)> callback, OperationSequence nextSequence);
	void readVersion (std::function<void(bool)> callback);
	void readAndCacheRegisters (std::function<void(bool)> callback);
	void checkReagentUsage (boost::system::error_code ec);
	bool getSyringeConfigCommands();

	boost::optional<std::string> currentSyringeSetCmd_;
	std::shared_ptr<boost::asio::deadline_timer> reagentUsageUpdateCheckTimer_;
};

class SyringePumpOperation : public ControllerBoardOperation::Operation {
public:
	enum eSyringePumpCommand {
		Initialize = 1,
		SetPosition = 2,
		SetValve = 3,
		SendDirect = 4,
		//ClearError = 5,	// not currently used.
	};

	SyringePumpOperation() {
		Operation::Initialize (&regData_);
		regAddr_ = SyringeRegs;
	}

protected:
	SyringeRegisters regData_;
};

class SyringePumpReadRegistersOperation : public SyringePumpOperation {
public:
	SyringePumpReadRegistersOperation() {
		mode_ = ReadMode;
		lengthInBytes_ = sizeof(SyringeRegisters);
	}
};

class SyringePumpInitializeOperation : public SyringePumpOperation {
public:
	SyringePumpInitializeOperation() {
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(SyringePumpOperation::Initialize);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class SyringePumpSetPositionOperation : public SyringePumpOperation {
public:
	SyringePumpSetPositionOperation (uint32_t volume, uint32_t speed) {
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(SyringePumpOperation::SetPosition);
		regData_.CommandParam = volume * SCALE_FACTOR;
		regData_.CommandParam2 = speed * SCALE_FACTOR;
		lengthInBytes_ = sizeof(uint32_t) * 3;
	}
};

class SyringePumpSetValveOperation : public SyringePumpOperation {
public:
	SyringePumpSetValveOperation (PhysicalPort_t physicalPort, SyringePumpDirection direction) {
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(SyringePumpOperation::SetValve);
		regData_.CommandParam = static_cast<uint32_t>(physicalPort);
		regData_.CommandParam2 = static_cast<uint32_t>(direction.Get());
		lengthInBytes_ = sizeof(uint32_t) * 3;
	}
};

class SyringePumpSendDirectOperation : public SyringePumpOperation {
public:
	SyringePumpSendDirectOperation (std::string text, const SyringeRegisters& cachedRegisters) {
		mode_ = WriteMode;
		memcpy_s (&regData_, sizeof(SyringeRegisters), &cachedRegisters, sizeof(SyringeRegisters));
		regData_.Command = static_cast<uint32_t>(SyringePumpOperation::SendDirect);
		DataConversion::convertToCharArray (regData_.Text, sizeof(text), text);
		lengthInBytes_ = sizeof(SyringeRegisters);
	}
};

class SyringePumpReadReagentErrorOperation : public SyringePumpOperation {
public:
	SyringePumpReadReagentErrorOperation() {
		mode_ = ReadMode;
		regAddr_ = ReagentRegs + 8;	// Address of reagent registers error code.
		lengthInBytes_ = sizeof(uint32_t);
	}
};
