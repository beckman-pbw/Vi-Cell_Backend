#pragma once

#include <stdint.h>
#include <string>

//*****************************************************************************
class ErrorType {
public:
	enum ErrorTypeMask {
		StateMachineBusy = 0x00010000,
		HardwareError = 0x00020000,
		TimeoutError = 0x00040000,
		CommandFailedError = 0x00080000,
		ProcessingError = 0x00100000,
    };

	ErrorType();
	virtual ~ErrorType();
	
	ErrorType (uint32_t errorCode);
	ErrorType& operator= (ErrorType other);

	bool isSet (ErrorTypeMask mask);
	std::string getAsString();

private:
	uint32_t errorCode_;
};

//*****************************************************************************
class HardwareError {
public:
	enum HardwareErrorMask {
		MotorController = 0x0000,
		SystemCrashed = 0x2000,
		SPIController = 0x3000,
		RS232 = 0x4000,
		I2CController = 0x5000,
		RfReader = 0x6000,
		InternalFlash = 0x7000,
    };

	HardwareError();
	virtual ~HardwareError();

	HardwareError (uint32_t errorCode);
	HardwareError& operator= (HardwareError other);

	bool isSet (HardwareErrorMask bitPosition);
	uint16_t getMotorControllerBitMask();	
	uint16_t getControllerNumber();	// This is only applicable to SPI, RS232, I2C controllers.
	uint16_t getControllerError();	// This is only applicable to SPI, RS232, I2C controllers.
	uint16_t getRfidError();
	std::string getRfidErrorAsString();
	uint16_t getInternalFlashError();
	std::string getAsString();

private:
	uint16_t errorCode_;
};

//*****************************************************************************
class CrashCode {
public:
	enum CrashCodeValue {
		StackOverflow = 2,
		AssertFailed = 3,
		UnexpectedInterruptNMIINT = 4,
		UnexpectedInterruptHardFault = 5,
		UnexpectedInterruptBusFault = 6,
		UnexpectedInterruptUsageFault = 7,
		UnexpectedInterruptSupervisorFault = 8,
		UnexpectedInterruptPendableService = 9,
	};

	CrashCode();
	virtual ~CrashCode();

	CrashCode (uint16_t crashCod);
	CrashCode& operator= (CrashCode other);

	std::string getAsString();

private:
	uint16_t crashCode_;
};

//*****************************************************************************
class TimeoutError {
public:
	enum TimeoutErrorMask {
		RfReader = 0x1000,
		MotorL6470A = 0x2000,
		MotorL6470B = 0x3000,
		LEDCalibration = 0x4000,
		SPIController = 0x5000,
		I2CController = 0x6000,
		PhotoDiode = 0x7000,
		BubbleDetector = 0x8000,
		RS232 = 0x9000,
		Host = 0xA000,
		SyringeUart = 0xB000,
	};

	TimeoutError();
	virtual ~TimeoutError();

	TimeoutError (uint32_t errorCode);
	TimeoutError& operator= (TimeoutError other);

	bool isSet (TimeoutErrorMask mask);
	uint16_t TimeoutError::getLSNibble();
	std::string getAsString();

private:
	uint16_t errorCode_;
};

//*****************************************************************************
class CommandFailedError {
public:
	enum CommandFailedErrorMask {
		InvalidCommandValueReceivedFromHost = 0x0000,
		MotorMoveNotInDeadBand = 0x0001,
		MotorEncoderCoarseHoming = 0x0002,
		ValveDRV104ThermalFault = 0x0003,
		RS232XmitBufToBig = 0x0004,
		RS232RecvBufToSmall = 0x0005,
		InvalidHostRequestLength = 0x00006,
		InvalidHostRequestAddress = 0x0007,
		InvalidHostRequestCRC16 = 0x0008,
		InvalidHostRequestNumBytesToRead = 0x0009,
		InvalidHostRequestLengthAndAddress = 0x000A,
		PhotoDiodeNotFinished = 0x000B,
		InvalidCameraNFPS = 0x000C,
		InvalidCameraCET = 0x000D,
		CameraLEDBusy = 0x000E,
		CameraNonZeroLEDErrorCode = 0x000F,
		LEDOnTimeTooShort = 0x0010,
		InvalidSyringeState = 0x0011,
		InvalidSyringeReply = 0x0012,
		InvalidCommandParameter = 0x0013,
		PeripheralBusyOrError = 0x0014,
		InvalidPersistentMemoryAddress = 0x0015,

		InvalidPersistentMemoryDataArea = 0x0016,
		InvalidPersistentMemoryLength = 0x0017,
		CommandAbortedByHost = 0x0018,
		PersistentMemoryWriteVerificaitonFailed = 0x0019,
		InvalidSyringeVolumeOrMaxPosition = 0x001A,
		NotEnoughTimeForLED_CTLD = 0x001B,
		InvalidLED_StabilizationMethod = 0x001C,
		LEDPulseBeforeMINOFFDone = 0x001D,
		InvalidLED_ISET = 0x001E,
		InvalidLED_ADJ = 0x001F,
		invalidPhotoDiodeVREF = 0x0022,
		InvalidLED_FeedbackSensor = 0x0023,
		MotorMoveAlreadyInProgress = 0x0024,
		MotorStalled = 0x0025,

		//TODO: 0x01nn
		//TODO: 0x02nn
		//TODO: 0x03nn
		//TODO: 0x04nn

		InvalidRfidWriteAttempts = 0x0500,
		InvalidReagentPack = 0x0501,
		RfidTagDataLength = 0x0502,
		RfidTagDataCryptoError = 0x0503,
		RfidTagApplicationDataCRC = 0x0504,
		DryIceKeyRead = 0x0505,
		NoValidReagentMap = 0x0506,
		InvalidDataLengthForCrypto = 0x0601,
		InvalidCRCforApplicationFirmware = 0x0602,
		FlashEraseFailed = 0x0603,
		InvalidSRecord = 0x0604,
	};

	CommandFailedError();
	virtual ~CommandFailedError();

	CommandFailedError (uint32_t errorCode);
	CommandFailedError& operator= (CommandFailedError other);
	
	bool isSet (CommandFailedErrorMask mask);
	uint16_t getLSByte();
	std::string getAsString();

private:
	uint16_t errorCode_;
};

//*****************************************************************************
class MotorControllerError {
public:
	enum MotorControllerBit {
		UnusedBit0 = 0,
		ControllerIsBusy = 1,
		UnusedBit2 = 2,
		UnusedBit3 = 3,
		UnusedBit4 = 4,
		UnusedBit5 = 5,
		UnusedBit6 = 6,
		CommandNotPerformed = 7,
		InvalidCommand = 8,
		UndervoltageLockout = 9,
		ThermalWarning = 10,
		ThermalShutdown = 11,
		OvercurrentDetected = 12,
		// Bits 13 -> 15 are currently not used.
	};

	MotorControllerError();
	virtual ~MotorControllerError();

	MotorControllerError (uint16_t errorCode);
	MotorControllerError& operator= (MotorControllerError other);

	bool isSet (MotorControllerBit bitPosition);
	std::string getAsString();

private:
	uint16_t errorCode_;
};

//*****************************************************************************
class MotorSPIId {
public:
	enum MotorSPIIdValue {
		Motor1 = 0,
		Motor2 = 1,
		Motor3 = 2,
		Motor4 = 3,
		Motor5 = 4,
		Motor6 = 5,
	};

	MotorSPIId();
	virtual ~MotorSPIId();

	MotorSPIId (uint16_t idCode);
	MotorSPIId& operator= (MotorSPIId other);

	std::string getAsString();

private:
	uint16_t idCode_;
};

//*****************************************************************************
class DtoASPIId {
public:
	enum DtoASPIIddValue {
		DAC1 = 0,
		DAC2 = 1,
		DAC3 = 2,
	};

	DtoASPIId();
	virtual ~DtoASPIId();

	DtoASPIId (uint16_t idCode);
	DtoASPIId& operator= (DtoASPIId other);

	std::string getAsString();

private:
	uint16_t idCode_;
};

//*****************************************************************************
class RfidReaderOperationError {
public:
	enum RfidReaderOperationErrorCode : uint8_t {
		InvalidTagType = 0x01,
		NoTagInField = 0x02,
		CollisionDetected = 0x03,
		TagDataIntegrityCheckFailed = 0x04,
		TagBlocked = 0x05,
		NotAuthenticated = 0x06,
		TagNotInField = 0x07,
		TagToReaderDataRate = 0x08,
		ReaderToTagDataRate = 0x09,
		DecryptTagDataFailed = 0x0A,
		InvalidSignature = 0x0B,
		InvalidAuthenticationKey = 0x0C,
		NoApplicationPresent = 0x0D,
		FileNotFound = 0x0E,
		NoFileSelected = 0x0F,
		InvalidKeyNumber = 0x10,
		InvalidKeyLength = 0x11,
		Unknown = 0x12,
		InvalidCommand = 0x13,
		InvalidCRC = 0x14,
		InvalidMessageLength = 0x15,
		InvalidAddress = 0x16,
		InvalidFlags = 0x17,
		InvalidASCIIByte = 0x18,
		InvalidNumberOfBlocks = 0x19,
		InvalidDataLength = 0x1A,
		NoAntennaDetected = 0x1B,
		InvalidEncoding = 0x1C,
		InvalidArgument = 0x1D,
		InvalidSession = 0x1E,
		CommandNotImplemented = 0x1F,
		OperationFailed = 0x20,
	};

	RfidReaderOperationError();
	virtual ~RfidReaderOperationError();

	RfidReaderOperationError (uint16_t errorCode);
	RfidReaderOperationError& operator= (RfidReaderOperationError other);

	std::string getAsString();

private:
	uint8_t errorCode_;
};

//*****************************************************************************
class RfidTagOperationError {
public:
	enum RfidTagOperationErrorCode : uint8_t {
		CommandNotSupported = 0x01,
		CommandNotRecognizeed = 0x02,
		OptionNotSupported = 0x03,
		UnknownError = 0x0F,
		BlockNotAvailable = 0x10,
		BlockAlreadyLocked = 0x11,
		BlockLocked = 0x12,
		BlockProgrammingFailed = 0x13,
		BlockLockFailed = 0x14,
	};

	RfidTagOperationError();
	virtual ~RfidTagOperationError();

	RfidTagOperationError (uint16_t errorCode);
	RfidTagOperationError& operator= (RfidTagOperationError other);

	std::string getAsString();

private:
	uint8_t errorCode_;
};

//*****************************************************************************
class RfidReaderCommError {
public:
	enum RfidReaderCommErrorCode : uint8_t {
		ResponseSOFError = 0x01,
		ResponseLengthError = 0x02,
		ResponseCRCError = 0x03,
		ResponseRIDError = 0x04,
		TagTypeError = 0x05,
		ResponseDataError = 0x06,
		CommTimeout = 0x07,
		TagCommError = 0x08,
	};

	RfidReaderCommError();
	virtual ~RfidReaderCommError();

	RfidReaderCommError (uint16_t errorCode);
	RfidReaderCommError& operator= (RfidReaderCommError other);

	std::string getAsString();

private:
	uint8_t errorCode_;
};

//*****************************************************************************
class RfidError {
public:
	enum RfidErrorMask : uint16_t {
		NoTagFound = 0x001,
		WriteOperationFailed = 0x002,
		AuthenticationError = 0x003,
		InvalidTag = 0x004,
		ReaderOperationFailed = 0x100,
		TagOperationFailed = 0x200,
		ReaderCommFailed = 0x300,
	};

	RfidError();
	virtual ~RfidError();

	RfidError (uint16_t errorCode);
	RfidError& operator= (RfidError other);

	bool isSet (RfidErrorMask bitPosition);

	std::string getAsString();

private:
	uint16_t errorCode_;
};

//*****************************************************************************
class ErrorCode {
public:
	ErrorCode ();
	virtual ~ErrorCode();
	
	ErrorCode (uint32_t errorCode);
	ErrorCode& operator= (ErrorCode other);

	uint32_t get();
	std::string getAsString();
	bool getRfidError (uint16_t& rfidErrorCode) {
		uint16_t temp = errorCode_ | (ErrorType::HardwareError | HardwareError::RfReader);
		rfidErrorCode = (uint16_t)(temp & 0x00000FFF);
		return temp ? true : false;
	}

private:
	uint32_t errorCode_;
};

