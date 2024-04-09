#pragma once

#include "OmicronLedControllerBoardInterface.hpp"

class OmicronLedControllerBoardCommand
{
public:
	static const std::string GetFirmwareCmd;
	static const std::string GetSerialNumberCmd;
	static const std::string GetSerialNumberHeadCmd;
	static const std::string GetSpecInfoCmd;
	static const std::string GetMaxPowerCmd;
	static const std::string GetWorkingHoursCmd;
	static const std::string GetMeasureTempDiodeCmd;
	static const std::string GetMeasureTempAmbientCmd;
	static const std::string GetActualStatusCmd;
	static const std::string GetFailureByteCmd;
	static const std::string GetLatchedFailureCmd;
	static const std::string GetLevelPowerCmd;
	static const std::string SetLevelPowerCmd;
	static const std::string GetPercentPowerCmd;
	static const std::string SetPercentPowerCmd;
	static const std::string TemporaryPowerCmd;
	static const std::string GetOperatingModeCmd;
	static const std::string SetOperatingModeCmd;
	static const std::string SetPowerOnCmd;
	static const std::string SetPowerOffCmd;
	static const std::string SetLEDOnCmd;
	static const std::string SetLEDOffCmd;
	static const std::string ResetControllerCmd;
	static const std::string GetICGFreqCmd;
	static const std::string SetICGFreqCmd;
	static const std::string GetICGDutyCycleCmd;
	static const std::string SetICGDutyCycleCmd;
	static const std::string SetBoostCurrentMonitoringCmd;
	static const std::size_t OmicronCommandLength;

	// Bit to set when turning on *digital trigger*.
	static const uint16_t LedTriggerMode = 0x0020;

	OmicronLedControllerBoardCommand(
		std::shared_ptr<boost::asio::io_context> pUpstream,
		std::shared_ptr<boost::asio::io_context> pInternalIos);
	virtual ~OmicronLedControllerBoardCommand();
	void sendAsync (std::string cmd, std::function<void(std::shared_ptr<std::vector<std::string>>)> responseCallback);
	void sendMultipleAsync(std::vector<std::string> cmdList, std::function<void(std::vector<std::string>)> responseCalback);

	OmicronLedControllerBoardInterface& getLedCBI() const
	{
		assert(pLCBI_);
		return *pLCBI_;
	}

private:
	void sendMultipleInternal(std::function<void(bool)> callback,
		std::vector<std::string> cmdList, size_t currentIndex, std::shared_ptr<std::vector<std::string>> responseList);

	void onCompletion(
		boost::system::error_code ec,
		OmicronLedControllerBoardInterface::t_ptxrxbuf tx,
		OmicronLedControllerBoardInterface::t_ptxrxbuf rx,
		std::function<void(std::string)> callback) const;

	std::unique_ptr<OmicronLedControllerBoardInterface> pLCBI_;
	std::shared_ptr<boost::asio::io_context> pUpstream_;
	std::shared_ptr<boost::asio::io_context> pInternalIos_;
	bool isError_;
};
