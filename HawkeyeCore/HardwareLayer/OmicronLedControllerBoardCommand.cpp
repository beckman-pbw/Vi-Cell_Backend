#include "stdafx.h"

#include "Logger.hpp"
#include "OmicronLedControllerBoardCommand.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "OmicronLedControllerBoardCommand";

const std::string OmicronLedControllerBoardCommand::GetFirmwareCmd = "GFw";
const std::string OmicronLedControllerBoardCommand::GetSerialNumberCmd = "GSN";
const std::string OmicronLedControllerBoardCommand::GetSerialNumberHeadCmd = "GSH";
const std::string OmicronLedControllerBoardCommand::GetSpecInfoCmd = "GSI";
const std::string OmicronLedControllerBoardCommand::GetMaxPowerCmd = "GMP";
const std::string OmicronLedControllerBoardCommand::GetWorkingHoursCmd = "GWH";
const std::string OmicronLedControllerBoardCommand::GetMeasureTempDiodeCmd = "MTD";
const std::string OmicronLedControllerBoardCommand::GetMeasureTempAmbientCmd = "MTA";
const std::string OmicronLedControllerBoardCommand::GetActualStatusCmd = "GAS";
const std::string OmicronLedControllerBoardCommand::GetFailureByteCmd = "GFB";
const std::string OmicronLedControllerBoardCommand::GetLatchedFailureCmd = "GLF";
const std::string OmicronLedControllerBoardCommand::GetLevelPowerCmd = "GLP";          // Not used.
const std::string OmicronLedControllerBoardCommand::SetLevelPowerCmd = "SLP";          // Not used.
const std::string OmicronLedControllerBoardCommand::GetPercentPowerCmd = "GPP";        // Not used.
const std::string OmicronLedControllerBoardCommand::SetPercentPowerCmd = "SPP";        // Not used.
const std::string OmicronLedControllerBoardCommand::TemporaryPowerCmd = "TPP";
const std::string OmicronLedControllerBoardCommand::GetOperatingModeCmd = "GOM";
const std::string OmicronLedControllerBoardCommand::SetOperatingModeCmd = "SOM";
const std::string OmicronLedControllerBoardCommand::SetPowerOnCmd = "POn";             // Not used.
const std::string OmicronLedControllerBoardCommand::SetPowerOffCmd = "POf";            // Not used.
const std::string OmicronLedControllerBoardCommand::SetLEDOnCmd = "LOn";               // Not used.
const std::string OmicronLedControllerBoardCommand::SetLEDOffCmd = "LOf";              // Not used.
const std::string OmicronLedControllerBoardCommand::ResetControllerCmd = "RsC";
const std::string OmicronLedControllerBoardCommand::GetICGFreqCmd = "GPF";             // Not used.
const std::string OmicronLedControllerBoardCommand::SetICGFreqCmd = "SPF";             // Not used.
const std::string OmicronLedControllerBoardCommand::GetICGDutyCycleCmd = "GDC";        // Not used.
const std::string OmicronLedControllerBoardCommand::SetICGDutyCycleCmd = "SDC";        // Not used.
const std::string OmicronLedControllerBoardCommand::SetBoostCurrentMonitoringCmd = "BCM";
const std::size_t OmicronLedControllerBoardCommand::OmicronCommandLength = 3;

//****************************************************************************
OmicronLedControllerBoardCommand::OmicronLedControllerBoardCommand(
	std::shared_ptr<boost::asio::io_context> pUpstream,
	std::shared_ptr<boost::asio::io_context> pInternalIos)
	: pUpstream_(pUpstream)
	, pInternalIos_(pInternalIos)
{
	pLCBI_ = std::make_unique<OmicronLedControllerBoardInterface>(pInternalIos_, "LEDMOD");
}

//*************************************************************************
OmicronLedControllerBoardCommand::~OmicronLedControllerBoardCommand()
{
	pLCBI_.reset();
}

//*************************************************************************
void OmicronLedControllerBoardCommand::sendAsync (std::string cmd, std::function<void(std::shared_ptr<std::vector<std::string>>)> responseCallback)
{
	HAWKEYE_ASSERT (MODULENAME, responseCallback);

	std::vector<std::string> cmds;
	cmds.push_back (cmd);

	// Do not send GetActualStatus command after the Reset command, the response is junk.
	if (cmd != ResetControllerCmd) {
		cmds.push_back (GetActualStatusCmd);
	}
	auto responseList = std::make_shared<std::vector<std::string>>();
	responseList->reserve(cmds.size());
	const size_t startIndex = 0;

	sendMultipleInternal ([this, responseCallback, cmds, responseList](bool status)
	{
		for (auto i = 0; i<cmds.size(); i++) {
			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format ("sendAsync: <cmd: %s, response: %s>")
					% cmds[i]
					% responseList->at(i)));
		}

		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "sendAsync: operation failed!");
		}
		// this workflow does not require the user id, so do not use the transient user technique
		pUpstream_->post (std::bind(responseCallback, responseList));

	}, cmds, startIndex, responseList);
}

//*************************************************************************
void OmicronLedControllerBoardCommand::sendMultipleAsync(
	std::vector<std::string> cmdList,
	std::function<void(std::vector<std::string>)> responseCallback)
{
	HAWKEYE_ASSERT (MODULENAME, responseCallback);

	auto responseList = std::make_shared<std::vector<std::string>>();
	responseList->reserve(cmdList.size());
	const size_t startIndex = 0;
	
	sendMultipleInternal([this, responseCallback, responseList](bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "sendMultipleAsync: operation failed!");
		}
		// this workflow does not require the user id, so do not use the transient user technique
		pUpstream_->post(std::bind(responseCallback, *responseList));

	}, cmdList, startIndex, responseList);
}

//****************************************************************************
void OmicronLedControllerBoardCommand::sendMultipleInternal(
	std::function<void(bool)> callback,
	std::vector<std::string> cmdList,
	size_t currentIndex,
	std::shared_ptr<std::vector<std::string>> responseList)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (!responseList)
	{
		responseList.reset(new std::vector<std::string>());
		responseList->reserve(cmdList.size());
	}

	if (currentIndex >= cmdList.size())
	{
		bool status = cmdList.size() == responseList->size();
		pInternalIos_->post(std::bind(callback, status));
		return;
	}

	auto onSingleCmdCompletion = [this, callback, cmdList, currentIndex, responseList](std::string response)
	{
		size_t nextIndex = currentIndex + 1;
		responseList->emplace_back(response);
		sendMultipleInternal(callback, cmdList, nextIndex, responseList);
	};

	const std::string cmd = cmdList[currentIndex];
	Logger::L().Log (MODULENAME, severity_level::debug1, "sendMultipleInternal: cmd: " + cmd);
	if (cmd.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "sendMultipleInternal: exit, empty command");
		pInternalIos_->post(std::bind(onSingleCmdCompletion, std::string()));
		return;
	}

	auto txb = std::make_shared<OmicronLedControllerBoardInterface::t_txrxbuf>();
	txb->resize(cmd.size());
	char* p = (char*)txb->data();
	memcpy((void*)p, (void*)cmd.data(), cmd.size());

	pLCBI_->Write(txb, 
	              std::bind(&OmicronLedControllerBoardCommand::onCompletion, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, onSingleCmdCompletion), 
	              pInternalIos_);
}

//****************************************************************************
//NOTE: this is the callback for the completion of the entire message
//      transaction (write command and read response).
//****************************************************************************
void OmicronLedControllerBoardCommand::onCompletion(
	boost::system::error_code ec,
	OmicronLedControllerBoardInterface::t_ptxrxbuf tx,
	OmicronLedControllerBoardInterface::t_ptxrxbuf rx,
	std::function<void(std::string)> callback) const
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	if (ec)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onCompletion callback: " + ec.message());
		// this workflow does not require the user id, so do not use the transient user technique
		pInternalIos_->post(std::bind(callback, std::string()));
		return;
	}

	if (!rx)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "onCompletion callback received no buffer: " + ec.message());
		// this workflow does not require the user id, so do not use the transient user technique
		pInternalIos_->post(std::bind(callback, std::string()));
		return;
	}

	// Copy data for returning.
	std::string retVal;
	retVal.resize (rx->size());
	auto b = rx->begin();
	auto e = b + rx->size();
	std::copy (b, e, retVal.begin());

	// this workflow does not require the user id, so do not use the transient user technique
	pInternalIos_->post(std::bind(callback, retVal));
}
