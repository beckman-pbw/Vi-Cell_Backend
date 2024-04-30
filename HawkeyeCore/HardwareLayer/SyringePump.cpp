#include "stdafx.h"

#include <string>

#include <boost/algorithm/string.hpp>

#include "EnumConversion.hpp"
#include "ErrorCode.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "Logger.hpp"
#include "Registers.hpp"
#include "SyringePump.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "SyringePump";

// This timeout is longer that the one in HawkeyeConfig due to the delays inherent in the firmware.
static constexpr uint32_t SyringeSMTimeoutMs = 25000;

// Error codes from Controller Board.
static constexpr uint32_t RfidInvalidWriteAttempt = 0x00080500;
static constexpr uint32_t RfidWriteOperationFailed = 0x00026002;
static constexpr uint32_t ReagentPackInvalid = 0x00080501;
static constexpr uint32_t ReagentPackDataEncryptionFailure = 0x00080503;
static constexpr uint32_t RfidTagDataCRCFail = 0x00080504;
static constexpr uint32_t ReagentValveMapNotFound = 0x00080506;
static constexpr uint32_t SyringeOverLoadErrorCode = 0x00080369;

static const std::string SyringeReadVersionDirectCommand = "/1?32";

static size_t numConfigCommandsProcessed_;
static std::vector<std::string> configCommands_;
static std::string sendDirectCommand_;
static bool isNormalDutyCycle_ = true;

const std::map<SyringePump::OperationSequence, std::string>
EnumConversion<SyringePump::OperationSequence>::enumStrings<SyringePump::OperationSequence>::data =
{
	{SyringePump::OperationSequence::SetConfiguration, std::string("SetConfiguration")},
	{SyringePump::OperationSequence::SendDirectCommand, std::string("SendDirectCommand")},
	{SyringePump::OperationSequence::Initialize, std::string("Initialize")},
	{SyringePump::OperationSequence::ReadVersion, std::string("ReadVersion")},
};

//*****************************************************************************
SyringePump::SyringePump (std::shared_ptr<CBOService> pCBOService)
	: SyringePumpBase (pCBOService) {
	reagentUsageUpdateCheckTimer_ = std::make_shared <boost::asio::deadline_timer> (pCBOService_->getInternalIosRef());
}

//*****************************************************************************
SyringePump::~SyringePump() {
	if (reagentUsageUpdateCheckTimer_) {
		reagentUsageUpdateCheckTimer_->cancel();
		reagentUsageUpdateCheckTimer_.reset();
	}

	currentSyringeSetCmd_.reset();
}

//*****************************************************************************
void SyringePump::initialize (std::function<void(bool)> callback) {
	Logger::L().Log (MODULENAME, severity_level::debug1, "initialize: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);


//NOTE: Currently the SyringePump.cfg file is empty.
//TODO: Deprecating the SyringePump.cfg file.
//      Need to determine whether should be supported in the DB.
//      As of 2020-10-10 this file has not been needed.
	//if (!getSyringeConfigCommands()) {
	//	Logger::L().Log (MODULENAME, severity_level::error, "initialize: <exit, failed to read the Syringe configuration commands>");
	//	ReportSystemError::Instance().ReportError (BuildErrorInstance(
	//		instrument_error::instrument_storage_readerror, 
	//		instrument_error::instrument_storage_instance::syringe_config, 
	//		instrument_error::severity_level::error));
	//	pCBOService_->enqueueInternal (callback, false);
	//	return;
	//}

	auto onInitializeComplete = [this, callback](bool status) -> void
	{
		if (!status)
		{
			  ReportSystemError::Instance().ReportError (BuildErrorInstance(
				  instrument_error::fluidics_syringepump_initerror, 
				  instrument_error::fluidics_syringepump_instances::pump_control,
				  instrument_error::severity_level::error));
		}

		Logger::L().Log (MODULENAME, severity_level::normal, "initialize: <exit, status: " + std::string(status ? "success" : "failure") + ">");
		pCBOService_->enqueueInternal (callback, status);
	};

	// Start off by reading the current SyringePump registers since the Direct command writes all of the registers.
	pCBOService_->getInternalIosRef().post ([this, callback, onInitializeComplete]() -> void {
		readAndCacheRegisters ([this, callback, onInitializeComplete](bool status) -> void {
			setNextProcessSequence (onInitializeComplete, OperationSequence::Initialize);
		});
	});
}

//*****************************************************************************
//TODO: this is not currently called.  If it is needed in the future the data
// will need to come from the DB.
//*****************************************************************************
#ifdef DEPRECATED
bool SyringePump::getSyringeConfigCommands()
{
	std::string configFile = HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::SyringePump);
	std::string encryptedFile = {};
	if (!HDA_GetEncryptedFileName(configFile, encryptedFile))
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "getSyringeConfigCommands: <exit  Failed to get the encrypted file path>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror, 
			instrument_error::instrument_storage_instance::syringe_config, 
			instrument_error::severity_level::error));
		return false;
	}

	if(!HDA_DecryptFile (encryptedFile.c_str(), configFile.c_str()))
	{
		if (!FileSystemUtilities::RestoreFromBackup(HawkeyeDirectory::FileType::SyringePump) ||
			!HDA_DecryptFile (encryptedFile.c_str(), configFile.c_str()))
		{
				Logger::L().Log (MODULENAME, severity_level::critical, "getSyringeConfigCommands: <exit, " + std::string(configFile) + " file not found>");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_readerror, 
					instrument_error::instrument_storage_instance::syringe_config, 
					instrument_error::severity_level::error));
				return false;
		}
	}
	
	std::ifstream ifs (configFile);
	if (!ifs.is_open())
	{
		Logger::L().Log (MODULENAME, severity_level::critical, "getSyringeConfigCommands: <exit, " + std::string(configFile) + " file not found>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror, 
			instrument_error::instrument_storage_instance::syringe_config, 
			instrument_error::severity_level::error));
		return false;
	}

	configCommands_.clear();
	std::string lineString;
	try {
		while (std::getline(ifs, lineString)) {
			if (lineString.empty())
				continue;

			auto pos = lineString.find(':');
			if (pos == std::string::npos) {
				return false;
			}

			std::string command = lineString.substr (0, pos);
			boost::algorithm::trim (command);
			configCommands_.push_back (command);
		}
	}
	catch (std::ios_base::failure &e) {
		auto debugStatement = boost::str(boost::format(" Failed to read Syringe Configuration command line: %s") % e.what());
		Logger::L().Log (MODULENAME, severity_level::debug1, debugStatement);
		std::remove(configFile.c_str());
		return false;
	}

	std::remove(configFile.c_str());
	return true;
}
#endif

//*****************************************************************************
void SyringePump::readAndCacheRegisters (std::function<void(bool)> callback) {
	HAWKEYE_ASSERT (MODULENAME, callback);

	numConfigCommandsProcessed_ = 0;

	// Get the SyringePump registers for use in the SendDirect Operation.
	auto onReadAndCacheRegistersComplete = [this, callback](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug3,
						 boost::str (boost::format ("onReadAndCacheRegistersComplete: <status: %s") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed")));

		if (cbData.status == ControllerBoardOperation::Success && cbData.bytesRead) {
			cbData.FromRx ((void*)&registerCache_, sizeof(registerCache_));
			setNextProcessSequence (callback, OperationSequence::SetConfiguration);
			return;
		}

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::fluidics_syringepump_hardwareerror, 
			instrument_error::fluidics_syringepump_instances::pump_control, 
			instrument_error::severity_level::error));
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("readAndCacheRegisters: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
		pCBOService_->enqueueInternal (callback, false);

	};

	auto tid = pCBOService_->CBO()->Query(SyringePumpReadRegistersOperation(), onReadAndCacheRegistersComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringePumpReadRegisters, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "readAndCacheRegisters::SyringePumpReadRegistersOperation CBO Query failed");
		pCBOService_->enqueueInternal (callback, false);
		return;
	}
}

//*****************************************************************************
void SyringePump::processAsync (std::function<void(bool)> callback, OperationSequence sequence) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "processAsync : " + EnumConversion<SyringePump::OperationSequence>::enumToString(sequence));

	switch (sequence) {
		case OperationSequence::SetConfiguration:
		{
			auto onSetConfigurationComplete = [this, callback](bool status) -> void {
				Logger::L().Log (MODULENAME, severity_level::debug3, boost::str (boost::format ("processAsync::onSetConfigurationComplete: <status: %s") 
					% (status ? "successful" : "failed")));

				if (status) {
					numConfigCommandsProcessed_++;
					if (numConfigCommandsProcessed_ >= configCommands_.size()) {
						Logger::L().Log (MODULENAME, severity_level::debug3, "processAsync::onSetConfigurationComplete <exit>");
						pCBOService_->enqueueExternal (callback, status);
						return;
					}

					setNextProcessSequence (callback, OperationSequence::SetConfiguration);
					return;
				}

				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::fluidics_syringepump_initerror, 
					instrument_error::fluidics_syringepump_instances::pump_control, 
					instrument_error::severity_level::error));
				pCBOService_->enqueueInternal (callback, status);
			};

			if (configCommands_.size() == 0)
			{
				pCBOService_->enqueueExternal (callback, true);
			}
			else
			{
				sendDirectCommand_ = configCommands_[numConfigCommandsProcessed_];
				Logger::L().Log (MODULENAME, severity_level::debug3, "processAsync::SetConfiguration <enter, command: " + sendDirectCommand_ + ">");
				setNextProcessSequence (onSetConfigurationComplete, OperationSequence::SendDirectCommand);
			}

			return;
		}

		case OperationSequence::SendDirectCommand:
		{
			Logger::L().Log (MODULENAME, severity_level::debug3, "processAsync::SendDirectCommand: <enter, command: " + sendDirectCommand_ + ">");

			auto onSendDirectComplete = [=](ControllerBoardOperation::CallbackData_t cbData) -> void {
				Logger::L().Log (MODULENAME, severity_level::debug3,
								 boost::str (boost::format ("processAsync::onSendDirectComplete: <status: %s") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed")));

				bool status = false;
				if (cbData.status == ControllerBoardOperation::Success) {
					status = true;
				} else {
					// Error is reported in SetConfiguration state if need be
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("processAsync::SendDirectCommand: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
				}

				pCBOService_->enqueueInternal (callback, status);
			};

			auto tid = pCBOService_->CBO()->Execute(SyringePumpSendDirectOperation(sendDirectCommand_, registerCache_), SyringeSMTimeoutMs, onSendDirectComplete);
			if (tid)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringePumpSendDirect, task %d") % (*tid)));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "processAsync::SyringePumpSendDirectOperation CBO Execute failed");
				pCBOService_->enqueueInternal (callback, false);
				return;
			}

			Logger::L().Log (MODULENAME, severity_level::debug3, "processAsync::onSendDirectComplete : <exit>");

			return;
		}

		case OperationSequence::Initialize:
		{
			auto onInitializeComplete = [=](ControllerBoardOperation::CallbackData_t cbData) -> void {
				Logger::L().Log (MODULENAME, severity_level::debug3,
								 boost::str (boost::format ("processAsync::onInitializeComplete: <status: %s>") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed")));

				if (cbData.status != ControllerBoardOperation::Success) {
					Logger::L().Log (MODULENAME, severity_level::error,
						boost::str (boost::format ("processAsync::Initialize: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::fluidics_syringepump_initerror, 
						instrument_error::fluidics_syringepump_instances::pump_control, 
						instrument_error::severity_level::error));
					pCBOService_->enqueueInternal (callback, false);
					return;
				}

				setNextProcessSequence (callback, OperationSequence::ReadVersion);
			};

			auto tid = pCBOService_->CBO()->Execute(SyringePumpInitializeOperation(), SyringeSMTimeoutMs, onInitializeComplete);
			if (tid)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringePumpInitialize, task %d") % (*tid)));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "processAsync::SyringePumpInitializeOperation CBO Execute failed");
				pCBOService_->enqueueInternal (callback, false);
			}
			return;
		}

		case OperationSequence::ReadVersion:
		{
			auto onReadVersion = [this, callback](bool status) -> void
			{
				pCBOService_->enqueueInternal (callback, status);
			};

			readVersion (onReadVersion);
			return;
		}

	} // End "switch (sequence)"

	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void SyringePump::setNextProcessSequence (std::function<void(bool)> callback, OperationSequence nextSequence) {

	pCBOService_->getInternalIosRef().post (std::bind (&SyringePump::processAsync, this, callback, nextSequence));
}

//*****************************************************************************
void SyringePump::checkReagentUsage (boost::system::error_code ec) {

	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::debug3, "checkReagentUsage... exit without running");
		return;
	}

	auto onCheckReagentUsageUpdateComplete = [this](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug3,
			 boost::str (boost::format ("checkReagentUsage::onCheckReagentUsageUpdateComplete: <status: %s>") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed")));

		if (cbData.status == ControllerBoardOperation::Success && cbData.bytesRead) {
			uint32_t errorCode;
			cbData.FromRx ((void*)&errorCode, sizeof(uint32_t));

			// Only check for errors that really matter here.
			if ((errorCode == RfidInvalidWriteAttempt) || (errorCode == RfidWriteOperationFailed) || 
				(errorCode == ReagentPackDataEncryptionFailure) || (errorCode == RfidTagDataCRCFail)) {
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("checkReagentUsage: 0x%08X, %s") % errorCode % ErrorCode(errorCode).getAsString()));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::reagent_pack_updatefail, 
					instrument_error::reagent_pack_instance::general, 
					instrument_error::severity_level::warning));
			}
		}
	};

	if (!pCBOService_->CBO()->Query (SyringePumpReadReagentErrorOperation(), onCheckReagentUsageUpdateComplete)) {
		Logger::L().Log (MODULENAME, severity_level::error, "checkReagentUsage::SyringePumpReadReagentErrorOperation CBO Query failed");
	}
}

//*****************************************************************************
#define MAX_SYRINGEPUMP_SPEED 600
//*****************************************************************************
void SyringePump::setPosition (std::function<void(bool)> callback, uint32_t target_volume_uL, uint32_t speed) {
	
	HAWKEYE_ASSERT (MODULENAME, callback);

	//Logger::L().Log (MODULENAME, severity_level::debug1, "setPosition: <enter>");

	if (speed > MAX_SYRINGEPUMP_SPEED) {
		Logger::L().Log (MODULENAME, severity_level::warning, "speed " + std::to_string(speed) + " > " + std::to_string(MAX_SYRINGEPUMP_SPEED) + " using " + std::to_string(MAX_SYRINGEPUMP_SPEED) + "ul/sec");
		speed = MAX_SYRINGEPUMP_SPEED;
	}

	int32_t volume_being_moved = std::abs((int32_t)target_volume_uL - (int32_t)cur_volume_uL_);

	if (Logger::L().IsOfInterest(severity_level::debug1)) {
		Logger::L().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("setPosition: moving to %dul at %dul/sec, current volume: %d, volume being moved: %d")
						% target_volume_uL
						% speed
						% cur_volume_uL_
						% volume_being_moved
			));
	}

	if (volume_being_moved == 0) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "setPosition: Syringe is already at the requested position");
		pCBOService_->enqueueInternal(callback, true);
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, "setPosition: current volume in syringe: " + std::to_string(cur_volume_uL_) + "uL");
	Logger::L().Log (MODULENAME, severity_level::debug3, "setPosition: volume being moved: " + std::to_string(volume_being_moved) + "uL");

	auto onSetPositionComplete = [this, callback, target_volume_uL, volume_being_moved](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug3,
			 boost::str (boost::format ("onSetPositionComplete: <status: %s, error: %s, code: %04X>") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed") % cbData.errorStatus.getAsString() % cbData.errorCode));

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success) {
			status = true;
			cur_volume_uL_ = target_volume_uL;

//TODO: check for Vi-Cell.
			// Update the CellHealth reagent volumes in the database.
			UpdateReagentVolume (curPhysicalPort_, volume_being_moved);

			reagentUsageUpdateCheckTimer_->expires_from_now (boost::posix_time::seconds(2));
			reagentUsageUpdateCheckTimer_->async_wait ([this](boost::system::error_code ec)->void {
				checkReagentUsage (ec);
			});
		} else {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("setPosition: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			if (cbData.errorCode == SyringeOverLoadErrorCode)
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::fluidics_syringepump_overpressure, 
					instrument_error::fluidics_syringepump_instances::pump_control, 
					instrument_error::severity_level::error));
			else
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::fluidics_syringepump_hardwareerror, 
					instrument_error::fluidics_syringepump_instances::pump_control, 
					instrument_error::severity_level::error));
		}
		
		pCBOService_->enqueueInternal (callback, status);
	};

	auto tid = pCBOService_->CBO()->Execute(SyringePumpSetPositionOperation(target_volume_uL, speed), SyringeSMTimeoutMs, onSetPositionComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringePumpSetPosition, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "processAsync::SyringePumpSetPositionOperation CBO Execute failed");
		pCBOService_->enqueueInternal (callback, false);
	}
}

//*****************************************************************************
bool SyringePump::getValve (SyringePumpPort& port) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "getValve: <enter>");
	port = SyringePumpPort::FromPhysicalPort (curPhysicalPort_);
	Logger::L().Log (MODULENAME, severity_level::debug3, "getValve: " + port.getAsString() + " valve");
	Logger::L().Log (MODULENAME, severity_level::debug3, "getValve: <exit>");

	return true;
}

//*****************************************************************************
// Set the valve position from the system point of view.
// This maps the system valve position to the Physical valve position.
//*****************************************************************************
void SyringePump::setValve (std::function<void(bool)> callback, SyringePumpPort port, SyringePumpDirection direction) {
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "setValve: set " + port.getAsString() + " valve " + direction.getAsString());

	if (port.get() == SyringePumpPort::InvalidPort || direction.get() == SyringePumpDirection::DirectionError) {
		pCBOService_->enqueueInternal (callback, false);
		return;
	}

	PhysicalPort_t physicalPort = SyringePumpPort::ToPhysicalPort(port.get());
	if (physicalPort == curPhysicalPort_) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "setValve: <exit, port already set>");
		pCBOService_->enqueueInternal (callback, true);
		return;
	}

	auto onSetValveComplete = [this, callback, physicalPort, port](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug3,
			boost::str (boost::format ("setValve: onSetValveComplete: <status: %s") % (cbData.status == ControllerBoardOperation::Success ? "successful>" : "failed>")));

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success) {
			curPhysicalPort_ = physicalPort;
			status = true;
		} else {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("setValve: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::fluidics_syringepump_hardwareerror, 
				instrument_error::fluidics_syringepump_instances::valve_control,
				instrument_error::severity_level::error));
		}

		Logger::L().Log (MODULENAME, severity_level::debug2, "setValve: <exit>");

		pCBOService_->enqueueInternal (callback, status);
	};

	auto tid = pCBOService_->CBO()->Execute(SyringePumpSetValveOperation(physicalPort, direction), SyringeSMTimeoutMs, onSetValveComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringePumpSetValve, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setValve::SyringePumpSetValveOperation CBO Execute failed");
		pCBOService_->enqueueInternal (callback, false);
	}
}

//*****************************************************************************
// Reads the Syringe register and fetches the Current set Valve in the pump.
// The valve position is from the syringe pump point of view.
//*****************************************************************************
bool SyringePump::getPhysicalValve (PhysicalPort& valveNum) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "getPhysicalValve: " + std::to_string(curPhysicalPort_));
	valveNum = curPhysicalPort_;
	return true;
}

//*****************************************************************************
// Set the valve position from the syringe pump point of view.
// The valve position here is the Physical valve position.
//*****************************************************************************
void SyringePump::setPhysicalValve (std::function<void(bool)> callback, char physicalPortChar, SyringePumpDirection direction) {
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L ().Log (MODULENAME, severity_level::debug1, "setPhysicalValve: set " + std::string(1, physicalPortChar) + " valve " + direction.getAsString ());

	// Validating the input "A -> H"
	PhysicalPort_t physicalPort;
	if (!ToPhysicalPortFromChar(physicalPortChar, physicalPort)) {
		Logger::L().Log (MODULENAME, severity_level::error, "setPhysicalValve invalid port number received");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::fluidics_syringepump_hardwareerror, 
			instrument_error::fluidics_syringepump_instances::valve_control,
			instrument_error::severity_level::error));
		pCBOService_->enqueueInternal(callback, false);
		return;
	}

	auto onSetValveComplete = [this, callback, physicalPort](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug3,
			boost::str (boost::format ("setPhysicalValve::onSetValveComplete: <status: %s") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed")));

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success) {
			curPhysicalPort_ = static_cast<PhysicalPort_t>(physicalPort);
			status = true;
		} else {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("setPhysicalValve: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::fluidics_syringepump_hardwareerror, 
				instrument_error::fluidics_syringepump_instances::valve_control, 
				instrument_error::severity_level::error));
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "setPhysicalValve: <exit>");

		pCBOService_->enqueueInternal (callback, status);
	};

	auto tid = pCBOService_->CBO()->Execute(SyringePumpSetValveOperation(physicalPort, direction), SyringeSMTimeoutMs, onSetValveComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringePumpSetValve, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setPhysicalValve::SyringePumpSetValveOperation CBO Execute failed");
		pCBOService_->enqueueInternal (callback, false);
	}
}

//*****************************************************************************
// Gets the position of the syringe pump from the controller board.
//*****************************************************************************
bool SyringePump::getPosition (uint32_t & pos) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "getPosition: " + std::to_string(cur_volume_uL_));

	pos = cur_volume_uL_;

	return true;
}

//*****************************************************************************
void SyringePump::rotateValve (std::function<void(bool)> callback, uint32_t angle, SyringePumpDirection direction)
{
	// Data validity checks are done in SyringeWorkflowOperation.

	const auto onRotateComplete = [this, callback](bool status) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("rotateValve: <exit, %s>") % (status ? "success" : "failed")));
		pCBOService_->enqueueInternal (callback, status);
	};

	if (direction.get() == SyringePumpDirection::Clockwise)
	{
		sendDirectCommand_ = boost::str (boost::format ("h27%03d") % angle);
	}
	else
	{
		sendDirectCommand_ = boost::str (boost::format ("h28%03d") % angle);
	}

	setNextProcessSequence (onRotateComplete, OperationSequence::SendDirectCommand);
}

//*****************************************************************************
void SyringePump::sendValveCommand(std::function<void(bool)> callback, std::string command)
{
	// Data validity checks are done in SyringeWorkflowOperation.

	const auto onValveCommandComplete = [this, callback](bool status) -> void {
		Logger::L().Log(MODULENAME, severity_level::debug1, boost::str(boost::format("sendValveCommand: <exit, %s>") % (status ? "success" : "failed")));
		pCBOService_->enqueueInternal(callback, status);
	};

	sendDirectCommand_ = boost::str(boost::format("%s") % command.c_str());

	setNextProcessSequence(onValveCommandComplete, OperationSequence::SendDirectCommand);
}

//*****************************************************************************
std::string SyringePump::getVersion() {

	return version_;
}

//*****************************************************************************
void SyringePump::readVersion (std::function<void(bool)> callback) {
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug3, "readVersion: <enter>");

	auto onReadVersionComplete = [this, callback](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug3,
						 boost::str (boost::format ("readVersion::onReadVersionComplete: <status: %s") % (cbData.status == ControllerBoardOperation::Success ? "successful" : "failed")));

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success && cbData.bytesRead ) 
		{
			cbData.FromRx ((void*)&registerCache_, sizeof (registerCache_));
			version_ = std::string(registerCache_.PumpFirmwareVersion);
			status = true;
			Logger::L().Log (MODULENAME, severity_level::debug1, "readVersion <exit, version: " + version_ + ">");
		} else {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("readVersion: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
		}

		pCBOService_->enqueueInternal (callback, status);
	};

	auto tid = pCBOService_->CBO()->Query(SyringePumpReadRegistersOperation(), onReadVersionComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("SyringeReadVersion, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "readVersion:: CBO Query failed");
		return;
	}
}
