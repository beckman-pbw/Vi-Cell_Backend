#include "stdafx.h"

#include <stdint.h>
#include <string>
#include <sstream>

#include "ErrorCode.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "ErrorCode";


//*****************************************************************************
//*****************************************************************************
ErrorType::ErrorType() {
	errorCode_ = 0;
}

ErrorType::ErrorType (uint32_t errorCode) {
	errorCode_ = errorCode;
}

ErrorType::~ErrorType() {
}

//*****************************************************************************
ErrorType& ErrorType::operator= (ErrorType other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
bool ErrorType::isSet (ErrorTypeMask mask) {
	return (errorCode_ & mask ? true : false);
}

//*****************************************************************************
std::string ErrorType::getAsString() {

	if (isSet(StateMachineBusy)) {
		return "StateMachineBusy";
	}
	if (isSet(HardwareError)) {		
		return "HardwareError";
	}
	if (isSet(TimeoutError)) {
		return "TimeoutError";
	}
	if (isSet(CommandFailedError)) {
		return "CommandFailedError";
	}
	if ( isSet(ProcessingError) ) {
		return "ProcessingError";
	}

	return "Undefined error";
}


//*****************************************************************************
//*****************************************************************************
HardwareError::HardwareError() {
	errorCode_ = 0;
}

HardwareError::HardwareError (uint32_t errorCode) {
	errorCode_ = (uint16_t) (errorCode & 0x0000FFFF);
}

HardwareError::~HardwareError() {
}

//*****************************************************************************
HardwareError& HardwareError::operator= (HardwareError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
bool HardwareError::isSet (HardwareErrorMask mask) {
	return ((errorCode_ & 0xE000) == mask ? true : false);
}

//*****************************************************************************
uint16_t HardwareError::getMotorControllerBitMask() {
	return ((uint16_t)(errorCode_) & 0x1FFF);
}

//*****************************************************************************
// This is only applicable to SPI, RS232, I2C controllers.
//*****************************************************************************
uint16_t HardwareError::getControllerNumber() {
	return ((uint16_t)(errorCode_) & 0x0F00) >> 8;
}

//*****************************************************************************
// This is only applicable to SPI, RS232, I2C controllers.
//*****************************************************************************
uint16_t HardwareError::getControllerError() {
	return (uint16_t)(errorCode_ & 0x000000FF);
}

//*****************************************************************************
uint16_t HardwareError::getRfidError() {
	return errorCode_;
}

//*****************************************************************************
std::string HardwareError::getRfidErrorAsString() {
	//return (uint16_t)(errorCode_ & 0x000000FF);

	RfidError (errorCode_);

	return "";
}

//*****************************************************************************
uint16_t HardwareError::getInternalFlashError() {
	return (uint16_t)(errorCode_ & 0x000000FF);
}

//*****************************************************************************
std::string HardwareError::getAsString() {

	if (isSet(MotorController)) {
		return "MotorControllerError : " + MotorControllerError(getMotorControllerBitMask()).getAsString();
	}
	if (isSet(SystemCrashed)) {
		return boost::str (boost::format ("%d") % CrashCode(getControllerError()).getAsString());
	}
	if (isSet(SPIController)) {
		return boost::str (boost::format ("%d : 0x%02X") % getControllerNumber() % getControllerError());
	}
	if (isSet(RS232)) {
		return boost::str (boost::format ("%d : 0x%02X") % getControllerNumber() % getControllerError());
	}
	if (isSet(I2CController)) {
		return boost::str (boost::format ("%d : 0x%02X") % getControllerNumber() % getControllerError());
	}
	if ((errorCode_ & 0xF000) == RfReader) {
		return boost::str (boost::format ("RfReaderError : %s") % RfidError(errorCode_).getAsString());
	}
	if ((errorCode_ & 0xFF00) == InternalFlash) {
		return "InternalFlashError ";
	}

	return "Undefined error";
}


//*****************************************************************************
//*****************************************************************************
CrashCode::CrashCode() {
	crashCode_ = 0;
}

CrashCode::CrashCode (uint16_t crashCode) {
	crashCode_ = (uint16_t)crashCode;
}

CrashCode::~CrashCode() {
}

//*****************************************************************************
CrashCode& CrashCode::operator= (CrashCode other) {
	crashCode_ = other.crashCode_;
	return *this;
}

//*****************************************************************************
std::string CrashCode::getAsString() {
	if (crashCode_ == StackOverflow) {
		return "StackOverflow";
	}
	if (crashCode_ == AssertFailed) {
		return "AssertFailed";
	}
	if (crashCode_ == UnexpectedInterruptNMIINT) {
		return "UnexpectedInterruptNMIINT";
	}
	if (crashCode_ == UnexpectedInterruptHardFault) {
		return "UnexpectedInterruptHardFault";
	}
	if (crashCode_ == UnexpectedInterruptBusFault) {
		return "UnexpectedInterruptBusFault";
	}
	if (crashCode_ == UnexpectedInterruptUsageFault) {
		return "UnexpectedInterruptUsageFault";
	}
	if (crashCode_ == UnexpectedInterruptSupervisorFault) {
		return "UnexpectedInterruptSupervisorFault";
	}
	if (crashCode_ == UnexpectedInterruptPendableService) {
		return "UnexpectedInterruptPendableService";
	}
	return "Undefined crash code";
}


//*****************************************************************************
//*****************************************************************************
TimeoutError::TimeoutError() {
	errorCode_ = 0;
}

TimeoutError::TimeoutError (uint32_t errorCode) {
	errorCode_ = (uint16_t)errorCode;
}

TimeoutError::~TimeoutError() {
}

//*****************************************************************************
TimeoutError& TimeoutError::operator= (TimeoutError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
bool TimeoutError::isSet (TimeoutErrorMask mask) {
	return ((errorCode_ & 0xFFF0) == mask ? true : false);
}

//*****************************************************************************
uint16_t TimeoutError::getLSNibble() {
	return ((uint16_t)errorCode_ & 0x000F);
}

//*****************************************************************************
std::string TimeoutError::getAsString() {

	if (isSet(RfReader)) {
		return "RfReader";

	}
	if (isSet(MotorL6470A)) {
		return "MotorL6470A : " +
			boost::str (boost::format ("SPI_Id: %d") % getLSNibble());

	}
	if (isSet(MotorL6470B)) {
		return "MotorL6470B : " +
			boost::str (boost::format ("SPI_Id: %d") % getLSNibble());

	}
	if (isSet(LEDCalibration)) {
		return "LEDCalibration";

	}
	if (isSet(SPIController)) {
		return "SPIController : " +
			boost::str (boost::format ("I2C_Id: %d") % (getLSNibble() == 0 ? "motor" : "DtoA"));

	}
	if (isSet(I2CController)) {
		return "I2CController : " +
			boost::str (boost::format ("I2C_Id: %d") % (getLSNibble() == 0 ? "reagent" : "crypto"));

	}
	if (isSet(PhotoDiode)) {
		return "PhotoDiode";

	}
	if (isSet(BubbleDetector)) {
		return "BubbleDetector";

	}
	if (isSet(RS232)) {
		return "RS232";

	}
	if (isSet(Host)) {
		std::string str;

		if ((errorCode_ & 0x000F) == 0) {
			str = "CRC";
		} else if ((errorCode_ & 0x000F) == 1) {
			str = "Header";
		} else if ((errorCode_ & 0x000F) == 2) {
			str = "Transmit";
		} else if ((errorCode_ & 0x000F) == 3) {		
			str = "RcvAck";
		}

		return "Host" + str;

	}
	if (isSet(SyringeUart)) {
		return "SyringeUart : " +
			boost::str (boost::format ("Id: %d") % getLSNibble());

	}
	
	return "Undefined error";
}


//*****************************************************************************
//*****************************************************************************
CommandFailedError::CommandFailedError() {
	errorCode_ = 0;
}

CommandFailedError::CommandFailedError (uint32_t errorCode) {
	errorCode_ = (uint16_t)errorCode;
}

CommandFailedError::~CommandFailedError() {
}

//*****************************************************************************
CommandFailedError& CommandFailedError::operator= (CommandFailedError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
bool CommandFailedError::isSet (CommandFailedErrorMask mask) {

//TODO: if the errorCode_ is 0x01nn then do some more decoding...
	return ((errorCode_ & 0xFFFF) == mask ? true : false);
}

//*****************************************************************************
uint16_t CommandFailedError::getLSByte() {
	return ((uint16_t)errorCode_ & 0x00FF);
}

//*****************************************************************************
std::string CommandFailedError::getAsString() {

	if (isSet(InvalidCommandValueReceivedFromHost)) {
		return "InvalidCommandValueReceivedFromHost";
	}
	if (isSet(MotorMoveNotInDeadBand)) {
		return "MotorMoveNotInDeadBand";
	}
	if (isSet(MotorEncoderCoarseHoming)) {
		return "EncoderCoarseHoming";
	}
	if (isSet(ValveDRV104ThermalFault)) {
		return "ValveDRV104ThermalFault";
	}
	if (isSet(RS232XmitBufToBig)) {
		return "RS232XmitBufToBig";
	}
	if (isSet(RS232RecvBufToSmall)) {
		return "RS232RecvBufToSmall";
	}
	if (isSet(InvalidHostRequestLength)) {
		return "InvalidHostRequestLength";
	}
	if (isSet(InvalidHostRequestAddress)) {
		return "InvalidHostRequestAddress";
	}
	if (isSet(InvalidHostRequestCRC16)) {
		return "InvalidHostRequestCRC16";
	}
	if (isSet(InvalidHostRequestNumBytesToRead)) {
		return "InvalidHostRequestNumBytesToRead";
	}
	if (isSet(InvalidHostRequestLengthAndAddress)) {
		return "InvalidHostRequestLengthAndAddress";
	}
	if (isSet(PhotoDiodeNotFinished)) {
		return "PhotoDiodeNotFinished";
	}
	if (isSet(InvalidCameraNFPS)) {
		return "InvalidCameraNFPS";
	}
	if (isSet(InvalidCameraCET)) {
		return "InvalidCameraCET";
	}
	if (isSet(CameraLEDBusy)) {
		return "CameraLEDBusy";
	}
	if (isSet(CameraNonZeroLEDErrorCode)) {
		return "CameraNonZeroLEDErrorCode";
	}
	if (isSet(LEDOnTimeTooShort)) {
		return "LEDOnTimeTooShort";
	}
	if (isSet(InvalidSyringeState)) {
		return "InvalidSyringeState";
	}
	if (isSet(InvalidSyringeReply)) {
		return "InvalidSyringeReply";
	}
	if (isSet(InvalidCommandParameter)) {
		return "InvalidCommandParameter";
	}
	if (isSet(PeripheralBusyOrError)) {
		return "PeripheralBusyOrError";
	}
	if (isSet(InvalidPersistentMemoryAddress)) {
		return "InvalidPersistentMemoryAddress";
	}
	if (isSet(InvalidPersistentMemoryDataArea)) {
		return "InvalidPersistentMemoryDataArea";
	}
	if (isSet(InvalidPersistentMemoryLength)) {
		return "InvalidPersistentMemoryLength";
	}
	if (isSet(CommandAbortedByHost)) {
		return "CommandAbortedByHost";
	}
	if (isSet(PersistentMemoryWriteVerificaitonFailed)) {
		return "PersistentMemoryWriteVerificaitonFailed";
	}
	if (isSet(InvalidSyringeVolumeOrMaxPosition)) {
		return "InvalidSyringeVolumeOrMaxPosition";
	}
	if (isSet(NotEnoughTimeForLED_CTLD)) {
		return "NotEnoughTimeForLED_CTLD";
	}
	if (isSet(InvalidLED_StabilizationMethod)) {
		return "InvalidLED_StabilizationMethod";
	}
	if (isSet(LEDPulseBeforeMINOFFDone)) {
		return "LEDPulseBeforeMINOFFDone";
	}
	if (isSet(InvalidLED_ISET)) {
		return "InvalidLED_ISET";
	}
	if (isSet(InvalidLED_ADJ)) {
		return "InvalidLED_ADJ";
	}
	if (isSet(invalidPhotoDiodeVREF)) {
		return "invalidPhotoDiodeVREF";
	}
	if (isSet(InvalidLED_FeedbackSensor)) {
		return "InvalidLED_FeedbackSensor";
	}
	if (isSet(MotorMoveAlreadyInProgress)) {
		return "MotorMoveAlreadyInProgress";
	}
	if (isSet(MotorStalled)) {
		return "MotorStalled";
	}

	//TODO: 0x01nn
	//TODO: 0x02nn
	//TODO: 0x03nn
	//TODO: 0x04nn

	if (isSet(InvalidRfidWriteAttempts)) {
		return "InvalidRfidWriteAttempts";
	}
	if (isSet(InvalidReagentPack)) {
		return "InvalidReagentPack";
	}
	if (isSet(RfidTagDataLength)) {
		return "RfidTagDataLength";
	}
	if (isSet(RfidTagDataCryptoError)) {
		return "RfidTagDataCryptoError";
	}
	if (isSet(RfidTagApplicationDataCRC)) {
		return "RfidTageApplicationDataCRC";
	}
	if (isSet(DryIceKeyRead)) {
		return "DryIceKeyRead";
	}
	if (isSet(NoValidReagentMap)) {
		return "NoValidReagentMap";
	}
	if (isSet(InvalidDataLengthForCrypto)) {
		return "InvalidDataLengthForCrypto";
	}
	if (isSet(InvalidCRCforApplicationFirmware)) {
		return "InvalidCRCforApplicationFirmware";
	}
	if (isSet(FlashEraseFailed)) {
		return "FlashEraseFailed";
	}
	if (isSet(InvalidSRecord)) {
		return "InvalidSRecord";
	}
	return "Undefined error";
}


//*****************************************************************************
//*****************************************************************************
#define ISERRORBITSET(bitPosition) ((errorCode_ & (1 << bitPosition)) != 0)

MotorControllerError::MotorControllerError() {
	errorCode_ = 0;
}

MotorControllerError::MotorControllerError (uint16_t errorCode) {
	errorCode_ = errorCode;
}

MotorControllerError::~MotorControllerError() {
}

//*****************************************************************************
MotorControllerError& MotorControllerError::operator= (MotorControllerError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
bool MotorControllerError::isSet (MotorControllerBit bitPosition) {
	return ((errorCode_ & ((uint32_t)1 << bitPosition)) != 0);
}

//*****************************************************************************
std::string MotorControllerError::getAsString() {
	std::stringstream ss;

	if (isSet(ControllerIsBusy)) {
		ss << "ControllerIsBusy";
	} 
	if (isSet(CommandNotPerformed)) {
		if (ss.str().length() > 0) ss << ", ";
		ss << "CommandNotPerformed";
	} 
	if (isSet(InvalidCommand)) {
		if (ss.str().length() > 0) ss << ", ";
		ss << "InvalidCommand";
	}
	if (isSet(UndervoltageLockout)) {
		if (ss.str().length() > 0) ss << ", ";
		ss << "UndervoltageLockout";
	} 
	if (isSet(ThermalWarning)) {
		if (ss.str().length() > 0) ss << ", ";
		ss << "ThermalWarning";
	} 
	if (isSet(ThermalShutdown)) {
		if (ss.str().length() > 0) ss << ", ";
		ss << "ThermalShutdown";
	} 
	if (isSet(OvercurrentDetected)) {
		if (ss.str().length() > 0) ss << ", ";
		ss << "OvercurrentDetected";
	}

	return ss.str();
}


//*****************************************************************************
//*****************************************************************************
MotorSPIId::MotorSPIId() {
	idCode_ = 0;
}

MotorSPIId::MotorSPIId (uint16_t idCode) {
	idCode_ = idCode;
}

MotorSPIId::~MotorSPIId() {
}

//*****************************************************************************
MotorSPIId& MotorSPIId::operator= (MotorSPIId other) {
	idCode_ = other.idCode_;
	return *this;
}

//*****************************************************************************
std::string MotorSPIId::getAsString() {

	if (idCode_ > Motor6) {
		return "Undefined motor ID";
	}

	return boost::str (boost::format ("Motor%d") % idCode_);
}


//*****************************************************************************
//*****************************************************************************
DtoASPIId::DtoASPIId() {
	idCode_ = 0;
}

DtoASPIId::DtoASPIId (uint16_t idCode) {
	idCode_ = idCode;
}

DtoASPIId::~DtoASPIId() {
}

//*****************************************************************************
DtoASPIId& DtoASPIId::operator= (DtoASPIId other) {
	idCode_ = other.idCode_;
	return *this;
}

//*****************************************************************************
std::string DtoASPIId::getAsString() {

	if (idCode_ > DAC3) {
		return "Undefined DAC ID";
	}

	return boost::str (boost::format ("DAC%d") % idCode_);
}


//*****************************************************************************
//*****************************************************************************
RfidReaderOperationError::RfidReaderOperationError() {
	errorCode_ = 0;
}

RfidReaderOperationError::RfidReaderOperationError (uint16_t errorCode) {
	errorCode_ = (uint8_t)(errorCode & 0x00FF);
}

RfidReaderOperationError::~RfidReaderOperationError() {
}

//*****************************************************************************
RfidReaderOperationError& RfidReaderOperationError::operator= (RfidReaderOperationError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
std::string RfidReaderOperationError::getAsString() {

	switch (errorCode_) {
		case InvalidTagType:
			return "InvalidTagType";
		case NoTagInField:
			return "NoTagInField";
		case CollisionDetected:
			return "CollisionDetected";
		case TagDataIntegrityCheckFailed:
			return "TagDataIntegrityCheckFailed";
		case TagBlocked:
			return "TagBlocked";
		case NotAuthenticated:
			return "NotAuthenticated";
		case TagNotInField:
			return "TagNotInField";
		case TagToReaderDataRate:
			return "TagToReaderDataRate";
		case ReaderToTagDataRate:
			return "ReaderToTagDataRate";
		case DecryptTagDataFailed:
			return "DecryptTagDataFailed";
		case InvalidSignature:
			return "InvalidSignature";
		case InvalidAuthenticationKey:
			return "InvalidAuthenticationKey";
		case NoApplicationPresent:
			return "NoApplicationPresent";
		case FileNotFound:
			return "FileNotFound";
		case NoFileSelected:
			return "NoFileSelected";
		case InvalidKeyNumber:
			return "InvalidKeyNumber";
		case InvalidKeyLength:
			return "InvalidKeyLength";
		case Unknown:
			return "Unknown";
		case InvalidCommand:
			return "InvalidCommand";
		case InvalidCRC:
			return "InvalidCRC";
		case InvalidMessageLength:
			return "InvalidMessageLength";
		case InvalidAddress:
			return "InvalidAddress";
		case InvalidFlags:
			return "InvalidFlags";
		case InvalidASCIIByte:
			return "InvalidASCIIByte";
		case InvalidNumberOfBlocks:
			return "InvalidNumberOfBlocks";
		case InvalidDataLength:
			return "InvalidDataLength";
		case NoAntennaDetected:
			return "NoAntennaDetected";
		case InvalidEncoding:
			return "InvalidEncoding";
		case InvalidArgument:
			return "InvalidArgument";
		case InvalidSession:
			return "InvalidSession";
		case CommandNotImplemented:
			return "CommandNotImplemented";
		case OperationFailed:
			return "OperationFailed";
		default:
			return "Undefined error";
	}
}


//*****************************************************************************
//*****************************************************************************
RfidTagOperationError::RfidTagOperationError() {
	errorCode_ = 0;
}

RfidTagOperationError::RfidTagOperationError (uint16_t errorCode) {
	errorCode_ = (uint8_t)(errorCode & 0x00FF);
}

RfidTagOperationError::~RfidTagOperationError() {
}

//*****************************************************************************
RfidTagOperationError& RfidTagOperationError::operator= (RfidTagOperationError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
std::string RfidTagOperationError::getAsString() {

	switch (errorCode_) {
		case CommandNotSupported:
			return "CommandNotSupported";
		case CommandNotRecognizeed:
			return "CommandNotRecognizeed";
		case OptionNotSupported:
			return "OptionNotSupported";
		case UnknownError:
			return "UnknownError";
		case BlockNotAvailable:
			return "BlockNotAvailable";
		case BlockAlreadyLocked:
			return "BlockAlreadyLocked";
		case BlockLocked:
			return "BlockLocked";
		case BlockProgrammingFailed:
			return "BlockProgrammingFailed";
		case BlockLockFailed:
			return "BlockLockFailed";
		default:
			return "Undefined error";
	}
}


//*****************************************************************************
//*****************************************************************************
RfidReaderCommError::RfidReaderCommError() {
	errorCode_ = 0;
}

RfidReaderCommError::RfidReaderCommError (uint16_t errorCode) {
	errorCode_ = (uint8_t)(errorCode & 0x00FF);
}

RfidReaderCommError::~RfidReaderCommError() {
}

//*****************************************************************************
RfidReaderCommError& RfidReaderCommError::operator= (RfidReaderCommError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
std::string RfidReaderCommError::getAsString() {

	switch (errorCode_) {
		case ResponseSOFError:
			return "ResponseSOFError";
		case ResponseLengthError:
			return "ResponseLengthError";
		case ResponseCRCError:
			return "ResponseCRCError";
		case ResponseRIDError:
			return "ResponseRIDError";
		case TagTypeError:
			return "TagTypeError";
		case ResponseDataError:
			return "ResponseDataError";
		case CommTimeout:
			return "CommTimeout";
		case TagCommError:
			return "TagCommError";
		default:
			return "Undefined error";
	}
}


//*****************************************************************************
//*****************************************************************************
RfidError::RfidError() {
	errorCode_ = 0;
}

RfidError::RfidError (uint16_t errorCode) {
	errorCode_ = errorCode & 0x0FFF;
}

RfidError::~RfidError() {
}

//*****************************************************************************
RfidError& RfidError::operator= (RfidError other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
bool RfidError::isSet (RfidErrorMask mask) {
	return ((errorCode_ & 0x0F00) == mask ? true : false);
}

//*****************************************************************************
std::string RfidError::getAsString() {

	switch (errorCode_) {
		case NoTagFound:
			return "NoTagFound";
		case WriteOperationFailed:
			return "WriteOperationFailed";
		case AuthenticationError:
			return "AuthenticationError";
		case InvalidTag:
			return "InvalidTag";
		default:
			if (isSet(ReaderOperationFailed)) {
				return boost::str (boost::format ("ReaderOperationFailed : %s") % RfidReaderOperationError(errorCode_).getAsString());
			}
			if (isSet(TagOperationFailed)) {
				return boost::str (boost::format ("TagOperationFailed : %s") % RfidTagOperationError(errorCode_).getAsString());
			}
			if (isSet(ReaderCommFailed)) {
				return boost::str (boost::format ("ReaderCommFailed : %s") % RfidReaderCommError(errorCode_).getAsString());
			}
			return "Undefined error";
	}
}

//*****************************************************************************
//*****************************************************************************
ErrorCode::ErrorCode() {
	errorCode_ = 0;
}

ErrorCode::ErrorCode (uint32_t errorCode) {
	errorCode_ = errorCode;
}

ErrorCode::~ErrorCode() {
}

//*****************************************************************************
ErrorCode& ErrorCode::operator= (ErrorCode other) {
	errorCode_ = other.errorCode_;
	return *this;
}

//*****************************************************************************
std::string ErrorCode::getAsString() {
	std::stringstream ss;

	ErrorType errorType(errorCode_);

	ss << errorType.getAsString() + " : ";

	if (errorType.isSet(ErrorType::HardwareError)) {
		ss << HardwareError(errorCode_).getAsString();

	} else if (errorType.isSet(ErrorType::TimeoutError)) {
		ss << TimeoutError(errorCode_).getAsString();

	} else if (errorType.isSet(ErrorType::CommandFailedError)) {
		ss << CommandFailedError(errorCode_).getAsString();
	}

	return ss.str();
}

//*****************************************************************************
uint32_t ErrorCode::get() {	
	return errorCode_;
}
