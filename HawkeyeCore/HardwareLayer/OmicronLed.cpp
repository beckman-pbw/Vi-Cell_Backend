#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include "CommandParser.hpp"
#include "Logger.hpp"
#include "OmicronLed.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "OmicronLed";


//*****************************************************************************
OmicronLed::OmicronLed (std::shared_ptr<CBOService> pCBOService)
	: LedBase (pCBOService)
{
	// Set "pCBOService_->getInternalIos()" as upstream and internal io-service
	// for "OmicronLedControllerBoardCommand"
	pLCBC_ = std::make_shared<OmicronLedControllerBoardCommand>(
		pCBOService_->getInternalIos(),
		pCBOService_->getInternalIos());
}

//*****************************************************************************
OmicronLed::~OmicronLed()
{ }

//*****************************************************************************
// ledType is not used here.
//*****************************************************************************
void OmicronLed::initializeAsync (std::function<void(bool)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::normal, "initializeAsync: <enter>");

	HAWKEYE_ASSERT (MODULENAME, callback);

	auto initComplete1 = [this, callback](bool status) {
		auto initComplete2 = [this, callback](bool status) {
			Logger::L().Log (MODULENAME, severity_level::normal, "initializeAsync: <exit>");
			pCBOService_->enqueueInternal (callback, status);
		};

//TODO: is this really needed???
		if (status) {
			pCBOService_->enqueueInternal ([this, initComplete2]() {
				getAllInfo (LedInfo::SerialNumber, initComplete2);
			});
		} else {
			pCBOService_->enqueueInternal (callback, status);
		}
	};

	pCBOService_->enqueueInternal ([this, initComplete1]()
	{
		initialize (InitStates::EntryPoint, initComplete1);
	});
}

boost::system::error_code OmicronLed::openLedControllerBoard()
{
	return pLCBC_->getLedCBI().OpenSerial(DESC);
}

void OmicronLed::closeLedControllerBoard()
{
	pLCBC_->getLedCBI().CloseSerial();
}

//*****************************************************************************
void OmicronLed::initialize (InitStates currentState, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCompleteCurrentState = [this, callback](InitStates nextState)
	{
		pCBOService_->enqueueInternal([=]() { initialize(nextState, callback); });
	};

	switch (currentState)
	{
		case InitStates::EntryPoint:
		{
			boost::system::error_code ec = openLedControllerBoard();
			if (ec) {
				Logger::L().Log (MODULENAME, severity_level::error, "Unable to open LED controller board " + DESC + ": " + ec.message());
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::imaging_led_initerror, 
					errorInstance_, 
					instrument_error::severity_level::error));
				onCompleteCurrentState (InitStates::Error);
				return;
			}
			onCompleteCurrentState (InitStates::Reset);
			return;
		}

		case InitStates::Reset:
		{
			std::string cmd(OmicronLedControllerBoardCommand::ResetControllerCmd);
			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd, onCompleteCurrentState](bool status, std::string sResponse) -> void {
				if (!status) {
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_commserror, 
						errorInstance_, 
						instrument_error::severity_level::error));
					onCompleteCurrentState (InitStates::Error);
				}
				onCompleteCurrentState (InitStates::GetFW);
			}));
			return;
		}

		case InitStates::GetFW:
		{
			// Get LED controller firmware info and set the response delimiter to '|'.
			std::string cmd(OmicronLedControllerBoardCommand::GetFirmwareCmd);
			cmd += "|";	// Command response delimiter.
			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd, onCompleteCurrentState](bool status, std::string sResponse) -> void {
				if (!status) {
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_commserror, 
						errorInstance_, 
						instrument_error::severity_level::error));
					onCompleteCurrentState(InitStates::Error);
				}
				onCompleteCurrentState(InitStates::GetOpMode);
			}));
			return;
		}

		case InitStates::GetOpMode:
		{
			std::string cmd(OmicronLedControllerBoardCommand::GetOperatingModeCmd);
			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd, onCompleteCurrentState](bool status, std::string sResponse) -> void {
				if (!status) {
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_commserror, 
						errorInstance_, 
						instrument_error::severity_level::error));
					onCompleteCurrentState(InitStates::Error);
				}

				dumpOperatingMode (sResponse);

				onCompleteCurrentState (InitStates::SetOpMode);
			}));
			return;
		}

		case InitStates::SetOpMode:
		{
			// The following bits are turned on: AutoPowerup, AutoStartup, Analog Input Impedance,
			// Digital Input Impedance, Bit 10 Reserved, Bit 6 Reserved, Digital Input Release,
			// Operating Level Release, Bias Level Release.
			// The *reserved* bits are left at their default state.
			uint16_t operatingMode = 0xDC78;
			dumpOperatingMode (operatingMode);

			std::stringstream ss;
			ss << std::hex << std::setfill('0') << std::setw(4) << operatingMode;
			std::string cmd(OmicronLedControllerBoardCommand::SetOperatingModeCmd);
			cmd += ss.str();

			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd, onCompleteCurrentState](bool status, std::string sResponse) -> void {
				if (!status) {
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_commserror, 
						errorInstance_, 
						instrument_error::severity_level::error));
					onCompleteCurrentState (InitStates::Error);
				}

				if (sResponse == ">") {
					onCompleteCurrentState (InitStates::SetBoostCurrentMonitoring);
				} else {
					onCompleteCurrentState (InitStates::Error);
				}
			}));
			return;
		}

		case InitStates::SetBoostCurrentMonitoring:
		{
			std::string cmd(OmicronLedControllerBoardCommand::SetBoostCurrentMonitoringCmd);
			std::string turnOn = "1";
			cmd += turnOn;

			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd, onCompleteCurrentState](bool status, std::string sResponse) -> void {
				if (!status) {
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_commserror, 
						errorInstance_, 
						instrument_error::severity_level::error));
					onCompleteCurrentState(InitStates::Error);
				}
				onCompleteCurrentState(InitStates::SetInitialPower);
			}));
			return;
		}

		case InitStates::SetInitialPower:
		{
			float power = pLedConfig_->percentPower;
			setPower(power, [this, power, onCompleteCurrentState](bool setPowerOk)
			{
				if (!setPowerOk) {
					Logger::L().Log (MODULENAME, severity_level::error, "setLedPowerAsync : Failed to set led power : " + std::to_string(power));
				} else
				{
					Logger::L().Log (MODULENAME, severity_level::normal, "setLedPowerAsync:: new power setting: " + std::to_string(power));
				}
				onCompleteCurrentState(InitStates::Complete);
			});
			return;
		}

		case InitStates::Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "initializeLed: <exit with success>");
			pCBOService_->enqueueInternal(callback, true);
			return;
		}

		case InitStates::Error:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "initializeLed: <exit, initialization failed>");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_hardwareerror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}
	}

	// unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}

//*****************************************************************************
void OmicronLed::setPowerAsync (float percentPower, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	pCBOService_->enqueueInternal ([this, callback, percentPower] {
		// Set the Omicron LED percentage power level to default value.
		std::string cmd(OmicronLedControllerBoardCommand::TemporaryPowerCmd);
		std::string value = boost::str(boost::format("%5.1f") % percentPower);
		boost::algorithm::trim(value);
		cmd += value;

		sendCommand (cmd, [this, callback, cmd, percentPower](bool status, std::string sResponse) -> void
		{
			if (!status)
			{
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::imaging_led_commserror, 
					errorInstance_, 
					instrument_error::severity_level::error));
				pCBOService_->enqueueInternal (callback, status);
				return;
			}

			verifyCurrentLedPowerAsync (percentPower, callback);
		});
	});
}

//*****************************************************************************
// Note: LedType is not used here.
//*****************************************************************************
void OmicronLed::getPowerAsync (std::function<void(boost::optional<float>)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::string cmd(OmicronLedControllerBoardCommand::TemporaryPowerCmd);
	pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd](bool status, std::string sResponse) -> void {
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal(std::bind(callback, boost::none));
			return;
		}

		pCBOService_->enqueueInternal (std::bind(callback, std::stof(sResponse)));
	}));
}

//*****************************************************************************
void OmicronLed::verifyCurrentLedPowerAsync (float percentPower, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	Logger::L().Log (MODULENAME, severity_level::debug1, "verifyCurrentLedPowerAsync: <enter>");

	getPower([this, callback, percentPower](boost::optional<float> cbPower)
	{
		if (!cbPower)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "verifyCurrentLedPowerAsync: Failed to read Brightfield LED power from controller board!");
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		if (!LedBase::isPowerSame(percentPower, cbPower.get()))
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("verifyCurrentLedPowerAsync: Retrieved led power (%5.1f) is not same as input led power (%5.1f)")
					% cbPower.get()
					% percentPower));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		pCBOService_->enqueueInternal(callback, true);
	});
	
}

//*****************************************************************************
void printInfo (const std::string& function, const std::string& cmd, const std::string& response)
{
	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("%s: <cmd: %s, response: %s")
			% function
			% cmd
			% response));
}

//*****************************************************************************
void OmicronLed::getInfo (std::string cmd, std::function<void(bool)> callback) {

	HAWKEYE_ASSERT (MODULENAME, callback);

	pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd](bool status, std::string sResponse) -> void {
		printInfo ("getInfo", cmd, sResponse);
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		pCBOService_->enqueueInternal(callback, true);
	}));
}

//*****************************************************************************
void OmicronLed::getAllInfo (LedInfo infoToGet, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onCompleteCurrentState = [this, callback](LedInfo nextState)
	{
		pCBOService_->enqueueInternal([=]() { getAllInfo(nextState, callback); });
	};

	std::string cmd;
	LedInfo nextInfoToGet;

	switch (infoToGet) {
		case LedInfo::SerialNumber:
		{
			cmd = OmicronLedControllerBoardCommand::GetSerialNumberCmd;
			nextInfoToGet = LedInfo::SerialNumberHead;
		}
		break;

		case LedInfo::SerialNumberHead:
		{
			cmd = OmicronLedControllerBoardCommand::GetSerialNumberHeadCmd;
			nextInfoToGet = LedInfo::SpecInfo;
		}
		break;

		case LedInfo::SpecInfo:
		{
			cmd = OmicronLedControllerBoardCommand::GetSpecInfoCmd;
			nextInfoToGet = LedInfo::MaxPower;
		}
		break;

		case LedInfo::MaxPower:
		{
			cmd = OmicronLedControllerBoardCommand::GetMaxPowerCmd;
			nextInfoToGet = LedInfo::WorkingHours;
		}
		break;

		case LedInfo::WorkingHours:
		{
			cmd = OmicronLedControllerBoardCommand::GetWorkingHoursCmd;
			nextInfoToGet = LedInfo::MeasureTempDiode;
		}
		break;

		case LedInfo::MeasureTempDiode:
		{
			cmd = OmicronLedControllerBoardCommand::GetMeasureTempDiodeCmd;
			nextInfoToGet = LedInfo::MeasureTempAmbient;
		}
		break;

		case LedInfo::MeasureTempAmbient:
		{
			cmd = OmicronLedControllerBoardCommand::GetMeasureTempAmbientCmd;
			nextInfoToGet = LedInfo::LevelPower;
		}
		break;

		case LedInfo::LevelPower:
		{
			cmd = OmicronLedControllerBoardCommand::GetLevelPowerCmd;
			nextInfoToGet = LedInfo::PercentPower;
		}
		break;

		case LedInfo::PercentPower:
		{
			cmd = OmicronLedControllerBoardCommand::GetPercentPowerCmd;
			nextInfoToGet = LedInfo::TemporaryPower;
		}
		break;

		case LedInfo::TemporaryPower:
		{
			cmd = OmicronLedControllerBoardCommand::TemporaryPowerCmd;
			nextInfoToGet = LedInfo::ICGFreq;
		}
		break;

		case LedInfo::ICGFreq:
		{
			cmd = OmicronLedControllerBoardCommand::GetICGFreqCmd;
			nextInfoToGet = LedInfo::ICGDutyCycle;
		}
		break;

		case LedInfo::ICGDutyCycle:
		{
			cmd = OmicronLedControllerBoardCommand::GetICGDutyCycleCmd;
			nextInfoToGet = LedInfo::Complete;
		}
		break;

		case LedInfo::Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "getLedControllerBoardStatus: <exit, success>");
			pCBOService_->enqueueInternal(callback, true);
			return;
		}

		case LedInfo::Error:
		default:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "getLedControllerBoardStatus: <exit, failed>");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_hardwareerror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}
	}

	pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd, nextInfoToGet, onCompleteCurrentState](bool status, std::string sResponse) -> void {
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			onCompleteCurrentState (LedInfo::Error);
		}
		onCompleteCurrentState (nextInfoToGet);
	}));
}

const uint16_t SoftInterlockBit = 0x0001;
const uint16_t ExternalInterlockBit = 0x0200;
//*****************************************************************************
void OmicronLed::sendGetLatchedFailureByte (std::function<void(bool)> callback)
{
	std::string cmd = OmicronLedControllerBoardCommand::GetLatchedFailureCmd;
 	pLCBC_->sendAsync (cmd,
		[this, cmd, callback](std::shared_ptr<std::vector<std::string>> responseList)
	{
		std::vector<std::string> strVec;
		bool responseOk = ParseResponse (cmd, responseList->at(0), strVec);
		if (!responseOk) {
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("sendGetLatchedFailureByte: failed to parse response: Command <%d> Response <%s>") % cmd % responseList->at(0)));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal (callback, false);
			return;
		}

		// Now look at the details of the response from the GetLatchedFailureByte command.
		size_t pos = 0;
		uint16_t errorStatus = (uint16_t)std::stoul (&responseList->at(1)[3], &pos, 16);
		dumpErrorStatus (errorStatus);

		// If either of the interlock bits are set, reset the Omicron LED.
		if (errorStatus & SoftInterlockBit || errorStatus & ExternalInterlockBit) {
			std::string cmd(OmicronLedControllerBoardCommand::ResetControllerCmd);
			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd](bool status, std::string sResponse) -> void {
				if (!status) {
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::imaging_led_commserror, 
						errorInstance_, 
						instrument_error::severity_level::error));
					pCBOService_->enqueueInternal (callback, false);
					return;
				}

				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::imaging_led_reset,
					errorInstance_, 
					instrument_error::severity_level::warning));
				pCBOService_->enqueueInternal (callback, status);
			}));
		}

		pCBOService_->enqueueInternal (callback, responseOk);
	});
}

//*****************************************************************************
void OmicronLed::sendGetFailureByte (std::function<void(bool)> callback)
{
	std::string cmd = OmicronLedControllerBoardCommand::GetFailureByteCmd;
	pLCBC_->sendAsync (cmd,
		[this, cmd, callback](std::shared_ptr<std::vector<std::string>> responseList)
	{
		std::vector<std::string> strVec;
		bool responseOk = ParseResponse (cmd, responseList->at(0), strVec);
		if (!responseOk) {
			Logger::L().Log (MODULENAME, severity_level::error, boost::str(boost::format("sendGetFailureByte: failed to parse response: Command <%d> Response <%s>") % cmd % responseList->at(0)));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal (callback, false);
			return;
		}

		// Now look at the details of the response from the GetFailureByte command.
		size_t pos = 0;
		uint16_t errorStatus = (uint16_t)std::stoul (&responseList->at(1)[3], &pos, 16);

//errorStatus = 0x02C3; // testing...

		// Check if any bit other than bit 0 is set.
		// If so, then read the latched failure byte.
		if (errorStatus & 0x0001) {
			dumpErrorStatus (errorStatus);
			pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendGetLatchedFailureByte, this, [this, callback](bool status) -> void {
				//NOTE: any errors that are found are reported by *dumpErrorStatus* in *sendGetLatchedFailureByte*.
				pCBOService_->enqueueInternal (callback, status);
			}));
			return;
		}

		pCBOService_->enqueueInternal (callback, true);
	});
}

const uint16_t OmicronErrorBit = 0x0001;

void OmicronLed::sendCommandCallback(std::string cmd, std::function<void(bool, std::string)> callback, bool doretry, std::shared_ptr<std::vector<std::string>> responseList)
{
	/*
	*   Assumes that we have one command sent explicitly ('cmd') and one command sent implicitly ("GAS") as a follow-up.
	*   Both commands need to be checked; failure to parse EITHER command leads to one retry attempt of the initial command.
	*
	*   Only exception is the RESET command which will NOT have any status code follow-up.
	*/

	std::string sResponse{};

	// Do basic checks on the response.
	{
		std::vector<std::string> parsedResponse;
		bool responseOk = ParseResponse(cmd, responseList->at(0), parsedResponse);

		if (!responseOk || parsedResponse.size() < 1)
		{
			Logger::L().Log(MODULENAME, severity_level::error, boost::str(boost::format("sendCommand: failed to parse response: Command <%s> Response <%s>") % cmd % responseList->at(0)));

			if (doretry)
			{
				Logger::L().Log(MODULENAME, severity_level::error, boost::str(boost::format("sendCommand: Retrying: Command <%s>") % cmd));
				pLCBC_->sendAsync(cmd, std::bind(&OmicronLed::sendCommandCallback, this, cmd, callback, false, std::placeholders::_1));
			}
			else
			{
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::imaging_led_commserror,
					errorInstance_,
					instrument_error::severity_level::error));
				pCBOService_->enqueueInternal(callback, false, std::string("0"));
			}
			return;
		}
		sResponse = parsedResponse[0];

		if (cmd == OmicronLedControllerBoardCommand::ResetControllerCmd) {
			// Do not need to check any other status codes.
			pCBOService_->enqueueInternal(callback, true, std::string("0"));
			return;
		}
	}
	// Now look at the details of the response from the GetActualStatus command which is sent along with each 
	// command in the behind-the-scenes.
	// The process of reading the actual status, failure byte and latched failure byte is 
	// described in section 5.1.2 Error Handling section of the Omicron programming guide.
	{
		std::vector<std::string> parsedResponse;
		bool responseOk = ParseResponse(OmicronLedControllerBoardCommand::GetActualStatusCmd, responseList->at(1), parsedResponse);

		if (!responseOk || parsedResponse.size() < 1)
		{
			Logger::L().Log(MODULENAME, severity_level::error, boost::str(boost::format("sendCommand: failed to parse response: Command <%s> Response <%s>")
				% OmicronLedControllerBoardCommand::GetActualStatusCmd % responseList->at(1)));

			if (doretry)
			{
				Logger::L().Log(MODULENAME, severity_level::error, boost::str(boost::format("sendCommand: Retrying: Command <%s>") % cmd));
				pLCBC_->sendAsync(cmd, std::bind(&OmicronLed::sendCommandCallback, this, cmd, callback, false, std::placeholders::_1));
			}
			else
			{
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::imaging_led_commserror,
					errorInstance_,
					instrument_error::severity_level::error));
				pCBOService_->enqueueInternal(callback, false, std::string("0"));
			}
			return;
		}

		size_t pos = 0;
		uint16_t actualStatus = (uint16_t)std::stoul(parsedResponse[0], &pos, 16);

		dumpActualStatus(actualStatus);

		//NOTE: leave this in to be able to exercise the error path.
		//actualStatus = 0x02C3; // testing...

		if (actualStatus & OmicronErrorBit) {
			pCBOService_->enqueueInternal(std::bind(&OmicronLed::sendGetFailureByte, this, [this, callback](bool status) -> void {
				//NOTE: any errors that are found are reported by *dumpErrorStatus* in *sendGetLatchedFailureByte*.
				pCBOService_->enqueueInternal(callback, status, std::string("0"));
				}));
			return;
		}
	}

	// Return the response to the original message
	pCBOService_->enqueueInternal(callback, true, sResponse);
}


//*****************************************************************************
void OmicronLed::sendCommand (std::string cmd, std::function<void(bool, std::string)> callback)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "sendCommand: cmd: " + cmd);

	pLCBC_->sendAsync(cmd, std::bind(&OmicronLed::sendCommandCallback, this, cmd, callback, true, std::placeholders::_1));
}

//*****************************************************************************
// triggerMode: sets whether or not the camera is in data acquisition mode
// or is in service mode (continuously on).
//  true: data acquisition mode (digital trigger).
// false: service mode (continuously on).
//*****************************************************************************
void OmicronLed::getTriggerMode (std::function<void(bool, TriggerMode)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	std::string cmd(OmicronLedControllerBoardCommand::GetOperatingModeCmd);
	pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd](bool status, std::string sResponse) -> void {
		if (!status) {
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
		}

		uint16_t response = static_cast<uint16_t>(std::stoul(sResponse, nullptr, 16));
		dumpOperatingMode (response);

		TriggerMode triggerMode = TriggerMode::ContinousMode;
		uint16_t temp = response & OmicronLedControllerBoardCommand::LedTriggerMode;
		if (temp) {
			triggerMode = TriggerMode::DigitalMode;
		}

		pCBOService_->enqueueInternal (callback, status, triggerMode);
	}));
}

//*****************************************************************************
// triggerState: sets whether or not the camera is in data acquisition mode
// or is in service mode (continuously on).
//  true: data acquisition mode (digital trigger).
// false: service mode (continuously on).
//*****************************************************************************
void OmicronLed::setTriggerMode (TriggerMode triggerMode, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// The following bits are turned on: AutoPowerup, AutoStartup, Analog Input Impedance,
	// Digital Input Impedance, Bit 10 Reserved, Bit 6 Reserved, Operating Level Release, Bias Level Release.
	// Digital Input Release (digital trigger mode) is off.
	// The *reserved* bits are left on as that was their state out of the box.
	uint16_t operatingMode = 0xDC78;

	if (triggerMode == LedBase::ContinousMode)
	{
		// Set Omicron to DigitalMode.
		operatingMode &= ~OmicronLedControllerBoardCommand::LedTriggerMode;
	}

	dumpOperatingMode (operatingMode);

	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(4) << operatingMode;
	std::string cmd(OmicronLedControllerBoardCommand::SetOperatingModeCmd);
	cmd += ss.str();

	pCBOService_->enqueueInternal (std::bind (&OmicronLed::sendCommand, this, cmd, [this, callback, cmd](bool status, std::string sResponse) -> void {
		if (!status)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_commserror, 
				errorInstance_, 
				instrument_error::severity_level::error));
		}
		pCBOService_->enqueueInternal (callback, status);
	}));
}

//*************************************************************************
// This API only does cursory checks on the response to the command.
// All command responses begin with the three character command name.
//*************************************************************************
bool OmicronLed::ParseResponse (std::string sCmd, std::string sResponse, std::vector<std::string>& parsedResult)
{
	parsedResult.clear();

	// If the response is too short there is an error.
	if (sResponse.size() < (OmicronLedControllerBoardCommand::OmicronCommandLength + 1))
	{
		if (sCmd == OmicronLedControllerBoardCommand::ResetControllerCmd) {
			// Reset command response does not include an error code.
			// Append a fake error code to mirror the other commands.
			sResponse.append ("0000");

		} else {
			Logger::L().Log (MODULENAME, severity_level::error,
							boost::str (boost::format ("Response (%s) to command (%s) is too short")
										% sResponse
										% sCmd));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_omicron_response_too_short, 
				errorInstance_, 
				instrument_error::severity_level::notification));
			return false;
		}
	}

	std::string sTemp = sResponse.substr(OmicronLedControllerBoardCommand::OmicronCommandLength);	// String off the cmd from the response.
	CommandParser::parse ("|", sTemp, parsedResult);

	if (!Logger::L().IsOfInterest(severity_level::debug2))
	{
		std::string str = "ParseResponse: ";
		for (const auto r : parsedResult)
		{
			str.append (r + " ");
		}
		Logger::L().Log (MODULENAME, severity_level::debug2, str);
	}

	// Check for *set* error condition in the response.
	if (parsedResult.size() != 0 && parsedResult[0].compare("x") == 0)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ParseResponse: Received failed response (x) : " + parsedResult[0]);
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::imaging_led_commserror,
			errorInstance_, 
			instrument_error::severity_level::error));
		return false;
	}

	return true;
}

//****************************************************************************
void OmicronLed::dumpOperatingMode (std::string sOperatingMode) const
{
	size_t pos = 0;
	uint16_t operatingMode = (uint16_t)std::stoul (sOperatingMode, &pos, 16);
	dumpOperatingMode (operatingMode);
}

//****************************************************************************
void OmicronLed::dumpOperatingMode (uint16_t operatingMode)
{
	if (!Logger::L().IsOfInterest(severity_level::debug1))
	{
		return;
	}

	std::string str = { };
	str.append("\n**************************************************************************\n");
	str.append(boost::str (boost::format ("Operating Mode: 0x%04X\n") % operatingMode));
	
	if (Logger::L().IsOfInterest(severity_level::debug2))
	{
		str.append("    Auto Power-up            : " + std::string((operatingMode & 0x8000) ? "true" : "false") + "\n");
		str.append("    Auto Start-up            : " + std::string((operatingMode & 0x4000) ? "true" : "false") + "\n");
		str.append("    USB Ad-hoc Mode          : " + std::string((operatingMode & 0x2000) ? "true" : "false") + "\n");
		str.append("    Analog Input Impedance   : " + std::string((operatingMode & 0x1000) ? "true" : "false") + "\n");
		str.append("    Digital Input Impedance  : " + std::string((operatingMode & 0x0800) ? "true" : "false") + "\n");
		str.append("    Bit 10 Reserved          : " + std::string((operatingMode & 0x0400) ? "true" : "false") + "\n");
		str.append("    Bit 9 Reserved           : " + std::string((operatingMode & 0x0200) ? "true" : "false") + "\n");
		str.append("    APC Mode                 : " + std::string((operatingMode & 0x0100) ? "true" : "false") + "\n");
		str.append("    Analog Input Release     : " + std::string((operatingMode & 0x0080) ? "true" : "false") + "\n");
		str.append("    Bit 6 Reserved           : " + std::string((operatingMode & 0x0040) ? "true" : "false") + "\n");
		str.append("    Digital Input Release    : " + std::string((operatingMode & 0x0020) ? "true" : "false") + "\n");
		str.append("    Operating Level Release  : " + std::string((operatingMode & 0x0010) ? "true" : "false") + "\n");
		str.append("    Bias Level Release       : " + std::string((operatingMode & 0x0008) ? "true" : "false") + "\n");
		str.append("    Internal Clock Generator : " + std::string((operatingMode & 0x0004) ? "true" : "false") + "\n");
		str.append("    Bit 1 Reserved           : " + std::string((operatingMode & 0x0002) ? "true" : "false") + "\n");
		str.append("    Bit 0 Reserved           : " + std::string((operatingMode & 0x0001) ? "true" : "false") + "\n");
	}
	str.append("********************************************************************************");

	Logger::L().Log (MODULENAME, severity_level::debug2, str);
}

//****************************************************************************
void OmicronLed::dumpActualStatus (std::string sActualStatus) const
{
	size_t pos = 0;
	uint16_t actualStatus = (uint16_t)std::stoul (sActualStatus, &pos, 16);
	dumpActualStatus (actualStatus);
}

//****************************************************************************
void OmicronLed::dumpActualStatus (uint16_t actualStatus)
{
	if (!Logger::L().IsOfInterest(severity_level::debug1))
	{
		return;
	}

	std::string str = { };
	str.append("\n********************************************************************************\n");
	str.append(boost::str (boost::format ("Actual Status: 0x%04X\n") % actualStatus));

	if (Logger::L().IsOfInterest(severity_level::debug2))
	{
		str.append("    Bit 15 Reserved : " + std::string((actualStatus & 0x8000) ? "true" : "false") + "\n");
		str.append("    Bit 14 Reserved : " + std::string((actualStatus & 0x4000) ? "true" : "false") + "\n");
		str.append("    Bit 13 Reserved : " + std::string((actualStatus & 0x2000) ? "true" : "false") + "\n");
		str.append("    Bit 12 Reserved : " + std::string((actualStatus & 0x1000) ? "true" : "false") + "\n");
		str.append("    Bit 11 Reserved : " + std::string((actualStatus & 0x0800) ? "true" : "false") + "\n");
		str.append("    Bit 10 Reserved : " + std::string((actualStatus & 0x0400) ? "true" : "false") + "\n");
		str.append("    System Power    : " + std::string((actualStatus & 0x0200) ? "true" : "false") + "\n");
		str.append("    Toggle Key      : " + std::string((actualStatus & 0x0100) ? "true" : "false") + "\n");
		str.append("    Key Switch      : " + std::string((actualStatus & 0x0080) ? "true" : "false") + "\n");
		str.append("    LED Enabled     : " + std::string((actualStatus & 0x0040) ? "true" : "false") + "\n");
		str.append("    Bit 5 Reserved  : " + std::string((actualStatus & 0x0020) ? "true" : "false") + "\n");
		str.append("    Bit 4 Reserved  : " + std::string((actualStatus & 0x0010) ? "true" : "false") + "\n");
		str.append("    Bit 3 Reserved  : " + std::string((actualStatus & 0x0008) ? "true" : "false") + "\n");
		str.append("    Preheating      : " + std::string((actualStatus & 0x0004) ? "true" : "false") + "\n");
		str.append("    LED On          : " + std::string((actualStatus & 0x0002) ? "true" : "false") + "\n");
		str.append("    Interlock       : " + std::string((actualStatus & 0x0001) ? "true" : "false") + "\n");
	}
	str.append("********************************************************************************");

	Logger::L().Log (MODULENAME, severity_level::debug2, str);
}

//****************************************************************************
std::string reportError (uint16_t errorStatus, uint16_t errorBitMask, std::string prefix, uint32_t errorCode)
{
	std::string str;

	str.append (prefix);
	if (errorStatus & errorBitMask) {
		str.append ("true\n");
		ReportSystemError::Instance().ReportError(errorCode);
	} else {
		str.append ("false\n");
	}

	return str;
}

//****************************************************************************
void OmicronLed::dumpErrorStatus (uint16_t errorStatus) const
{
	std::string str = { };
	str.append("\n*******************************************************************************\n");
	str.append(boost::str (boost::format ("Error Status: 0x%04X\n") % errorStatus));

	// These are the only errors applicable to the Omicron LEDMOD device.
	uint32_t errorInstance;
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_diode_power, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x8000, "    Diode Power          : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_internal_error, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x4000, "    Internal Error       : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_diode_temp, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x1000, "    Diode Temperature    : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_ambient_temp, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x0800, "    Ambient Temperature  : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_diode_current, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x0400, "    Diode Current        : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_external_interlock, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x0200, "    External Interlock   : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_underover_voltage, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x0100, "    Under/Over Voltage   : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_cdrh_error, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError(errorStatus, 0x0010, "    CDRH Error           : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_response_too_short, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError(errorStatus, 0x0020, "    Internal Comm. Error : ", errorInstance));
	errorInstance = BuildErrorInstance (instrument_error::imaging_omicron_interlock, errorInstance_, instrument_error::severity_level::error);
	str.append (reportError (errorStatus, 0x0001, "    Interlock            : ", errorInstance));
	str.append("********************************************************************************");

	Logger::L().Log (MODULENAME, severity_level::normal, str);
}

//*****************************************************************************
float OmicronLed::getBrightfieldTolerance()
{
	return HawkeyeConfig::Instance().get().leds.omicronTolerance;
}

bool OmicronLed::IsPresent()
{
	//Omicro is considered present if we can open the led board
	if (pLCBC_->getLedCBI().IsSerialOpen())
	{
		//It's already open. Return that it is present.
		return true;
	}
	else
	{
		//It's not open yet. Open the serial port and close it.
		auto error = openLedControllerBoard();
		closeLedControllerBoard();
		if (error)
		{
			return false;
		}
		return true;
	}
}
