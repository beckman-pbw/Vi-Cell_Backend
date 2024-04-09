#include "stdafx.h"

#include <conio.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <boost/date_time/local_time/local_time.hpp>

#include "ChronoUtilities.hpp"
#include "EnumConversion.hpp"
#include "ErrorCode.hpp"
#include "Logger.hpp"
#include "Rfid.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "Rfid";

static constexpr uint32_t RfidTimeout = 10000;

const std::map<Rfid::OperationSequence, std::string>
EnumConversion<Rfid::OperationSequence>::enumStrings<Rfid::OperationSequence>::data =
{
	{ Rfid::OperationSequence::ReadRfidData, std::string("ReadRfidData") },
	{ Rfid::OperationSequence::ReadRfidTag, std::string("ReadRfidTag") },
};

//*****************************************************************************
Rfid::Rfid (std::shared_ptr<CBOService> pCBOService)
	: RfidBase (pCBOService)
{
}

//*****************************************************************************
Rfid::~Rfid()
{
}

//*****************************************************************************
void Rfid::processAsync (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback, OperationSequence sequence, std::shared_ptr<ProcessAsyncData> processAsyncData) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "processAsync : " + EnumConversion<OperationSequence>::enumToString(sequence));

	switch (sequence)
	{
		case OperationSequence::ReadRfidData:
		{
			auto onRfidReadComplete = [this, callback, processAsyncData](ControllerBoardOperation::CallbackData_t cbData) -> void {

				bool status = (cbData.status == ControllerBoardOperation::Success) || !cbData.rxBuf || (cbData.bytesRead == 0);
				if (!status) {
					logError (cbData.errorCode);
				} else {
					ReagentRegisters rfidData;
					uint32_t numRegistersToRead = 10;
					cbData.FromRx ((void*)&rfidData, sizeof(uint32_t) * numRegistersToRead);

					if (rfidData.NumOfTags) {
						processAsyncData->numTagsRead = rfidData.NumOfTags;
						processAsyncData->numTagsProcessed = 0;
						processAsyncData->rfidTags = std::make_shared<std::vector<RfidTag_t>>();
						setNextReadSequence (callback, OperationSequence::ReadRfidTag, processAsyncData);
						return;
					}
				}

				pCBOService_->enqueueExternal (callback, false, processAsyncData->rfidTags);
			};

			// Read the RFID reader header data from the RFID scan.
			auto tid = pCBOService_->CBO()->Query(RfidReadOperation(), onRfidReadComplete);
			if (tid)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("RfidRead, task %d") % (*tid)));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "processAsync::ReadRfidData CBO Query failed");
				pCBOService_->enqueueExternal (callback, false, processAsyncData->rfidTags);
			}

			return;
		}

		case OperationSequence::ReadRfidTag:
		{
			auto onRfidReadTagDataOperationComplete = [this, callback, processAsyncData](ControllerBoardOperation::CallbackData_t cbData) -> void {

				if (cbData.status == ControllerBoardOperation::Success) {
					RfAppdataRegisters rfidData;
					cbData.FromRx ((void*)&rfidData, sizeof(RfAppdataRegisters));

					if (isTagValid (processAsyncData->numTagsProcessed, rfidData.Status)) {

						RfidTag_t rfidTag = { };
						parseTag (processAsyncData->numTagsProcessed, rfidData, rfidTag);
						rfidTag.tagIndex = static_cast<uint16_t>(processAsyncData->numTagsProcessed);
						processAsyncData->rfidTags->push_back (rfidTag);
						processAsyncData->numTagsProcessed++;

						if (processAsyncData->numTagsProcessed == processAsyncData->numTagsRead) {
							pCBOService_->enqueueExternal (callback, true, processAsyncData->rfidTags);
							return;
						}

						setNextReadSequence (callback, OperationSequence::ReadRfidTag, processAsyncData);
						return;
					}

					Logger::L().Log (MODULENAME, severity_level::warning, "RFID tag is not valid");
				} else {
					logError (cbData.errorCode);
				}

				pCBOService_->enqueueExternal (callback, false, processAsyncData->rfidTags);
			};

			uint32_t addressToRead = 
				RegisterIds::ReagentRegs + 
					static_cast<RegisterIds>(ReagentRFTagsAppData + (sizeof(RfAppdataRegisters) * processAsyncData->numTagsProcessed));

			// Read the RFID reader header data from the RFID scan.
			auto tid = pCBOService_->CBO()->Query(RfidReadTagDataOperation(RegisterIds(addressToRead)), onRfidReadTagDataOperationComplete);
			if (tid)
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("ReadTagData, task %d") % (*tid)));
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::error, "processAsync::ReadRfidTag CBO Query failed");
				pCBOService_->enqueueExternal (callback, false, processAsyncData->rfidTags);
			}
			return;
		}

		default:
		{
			HAWKEYE_ASSERT (MODULENAME, false);
			break;
		}
	}

}

//*****************************************************************************
void Rfid::setNextReadSequence (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>>)> callback, OperationSequence nextSequence, std::shared_ptr<ProcessAsyncData> processAsyncData) {

	pCBOService_->getInternalIosRef().post (std::bind (&Rfid::processAsync, this, callback, nextSequence, processAsyncData));
}

//*****************************************************************************
void Rfid::scan (std::function<void(bool)> callback) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "scan: <enter>");

	auto onRfidScanComplete = [=](ControllerBoardOperation::CallbackData_t cbData) -> void {
		Logger::L().Log (MODULENAME, severity_level::debug1, "onRfidScanComplete: <callback>");

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success) {
			status = true;
		} else {
			logError (cbData.errorCode);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "scan: <exit>");
		pCBOService_->enqueueExternal (callback, status);
	};

	auto tid = pCBOService_->CBO()->Execute(RfidScanOperation(), RfidTimeout, onRfidScanComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("RfidScan, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "scan: CBO Execute failed");
		pCBOService_->enqueueExternal (callback, false);
	}
}

//*****************************************************************************
void Rfid::read (std::function<void(bool, std::shared_ptr<std::vector<RfidTag_t>> rfidTags)> callback) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "read: <enter>");

	auto onReadRfidComplete = [this, callback](bool status, std::shared_ptr<std::vector<RfidTag_t>> rfidTags) {
		Logger::L().Log (MODULENAME, severity_level::debug3, "read:onReadRfidComplete: <exit, callback>");
		pCBOService_->enqueueExternal (callback, status, rfidTags);
	};

	std::shared_ptr<ProcessAsyncData> processAsyncData = std::make_shared<ProcessAsyncData>();
	//processAsyncData->rfidTags = std::make_shared<std::vector<RfidTag_t>>();
	pCBOService_->getInternalIosRef().post (std::bind (&Rfid::processAsync, this, onReadRfidComplete, OperationSequence::ReadRfidData, processAsyncData));
}

//*****************************************************************************
//TODO: pass in a vector and then do the same thing as the configuration...
//*****************************************************************************
void Rfid::setValveMap (std::function<void(bool)> callback, ReagentValveMap_t valveMap) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "setValveMap: <enter>");

	auto onSetValveMapComplete = [=](ControllerBoardOperation::CallbackData_t cbData) -> void {

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success) {
			status = true;
		} else {
			logError (cbData.errorCode);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "setValveMap: <exit>");
		pCBOService_->enqueueExternal (callback, status);
	};

	auto tid = pCBOService_->CBO()->Execute(RfidSetValveMapOperation(valveMap), RfidTimeout, onSetValveMapComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("RfidSetValveMap, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setValveMap: CBO Execute failed");
		pCBOService_->enqueueExternal (callback, false);
	}
}

//*****************************************************************************
void Rfid::setTime (std::function<void(bool)> callback) {

	Logger::L().Log (MODULENAME, severity_level::debug3, "setTime: <enter>");

	auto onSetTimeComplete = [=](ControllerBoardOperation::CallbackData_t cbData) -> void {

		bool status = false;
		if (cbData.status == ControllerBoardOperation::Success) {
			status = true;
		} else {
			logError (cbData.errorCode);
		}

		Logger::L().Log (MODULENAME, severity_level::debug3, "setTime: <exit>");
		pCBOService_->enqueueExternal (callback, status);
	};

	boost::posix_time::ptime now_time = boost::posix_time::second_clock().local_time();
	time_t timet = (now_time - boost::posix_time::from_time_t(0)).total_seconds();

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Local time: %s") % ctime(&timet)));

	uint32_t local_since_epoch = static_cast<uint32_t>(timet);

	auto tid = pCBOService_->CBO()->Execute(RfidSetTimeOperation(local_since_epoch), RfidTimeout, onSetTimeComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("RfidSetTime, task %d") % (*tid)));
	}
	else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "setTime: CBO Execute failed");
		pCBOService_->enqueueExternal (callback, false);
	}
}

//*****************************************************************************
// Differentiate between reader errors due to no pack or RFID read error, and
// actual RFID reader hardware errors.
// ASSUME: that the *errorCode* here is an RFID error.
//*****************************************************************************
void Rfid::logError (uint32_t errorCode)
{
	uint16_t rfidErrorCode;
	bool isRfidError = ErrorCode(errorCode).getRfidError(rfidErrorCode);

	Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("0x%08X, %s") % errorCode % ErrorCode(errorCode).getAsString()));

	if (isRfidError && rfidErrorCode == RfidError::NoTagFound)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_nopack, 
			instrument_error::reagent_pack_instance::general, 
			instrument_error::severity_level::warning));
	}
	else if (isRfidError && rfidErrorCode == RfidError::WriteOperationFailed)
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_updatefail, 
			instrument_error::reagent_pack_instance::general, 
			instrument_error::severity_level::warning));
	}
	else if (isRfidError && (rfidErrorCode == RfidError::AuthenticationError || rfidErrorCode == RfidError::InvalidTag))
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_invalid, 
			instrument_error::reagent_pack_instance::general,
			instrument_error::severity_level::warning));
	}
	else if (CommandFailedError(errorCode).isSet(CommandFailedError::RfidTagApplicationDataCRC))
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_loaderror, 
			instrument_error::reagent_pack_instance::general, 
			instrument_error::severity_level::warning));
	}
	else 
	{
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_rfid_hardwareerror,
			instrument_error::reagent_pack_instance::general, //TODO: This line will need to be refactored for Vi-CELL FL.
			instrument_error::severity_level::error));
	}
}

//*****************************************************************************
bool Rfid::isTagValid (size_t tagNum, const TagStatus& tagStatus) {

	if (tagStatus.ProgramStatus == 1 &&
		tagStatus.ValidationStatus == 1 &&
		tagStatus.AuthStatus == 1) {

		return true;

	} else {

		if (tagStatus.ProgramStatus == 0) {
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("tag[%d]: Not Programmed") % tagNum));
		}

		if (tagStatus.AuthStatus == 2) {
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("tag[%d]: Failed to Authenticate") % tagNum));
		}

		if (tagStatus.ValidationStatus == 2) {
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("tag[%d]: Invalid") % tagNum));
		} else {
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("tag[%d]: Error Unknown") % tagNum));
		}

		if ((tagStatus.AuthStatus == 0) || (tagStatus.ValidationStatus == 0)) {
			// This should happen only if the FW reported error is not caught,
			// and we get here and found that the authentication/validation was aborted in FW.
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("tag[%d]: Not Authenticated or validated") % tagNum));
		}

		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::reagent_pack_invalid,
			instrument_error::reagent_pack_instance::general,
			instrument_error::severity_level::warning));

		return false;
	}
}
