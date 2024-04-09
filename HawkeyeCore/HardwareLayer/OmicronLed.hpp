#pragma once

#include "LedBase.hpp"
#include "OmicronLedControllerBoardCommand.hpp"

class OmicronLed : public LedBase
{
public:
	OmicronLed (std::shared_ptr<CBOService> pCBOService);
	virtual ~OmicronLed();

	virtual void initializeAsync (std::function<void(bool)> callback) override;
	virtual void setPowerAsync (float percentPower, std::function<void(bool)> callback) override;
	virtual void getPowerAsync (std::function<void(boost::optional<float>)> callback) override;
	virtual void getTriggerMode (std::function<void(bool, TriggerMode)> callback) override;
	virtual void setTriggerMode (TriggerMode triggerMode, std::function<void(bool)> callback) override;
	virtual float getBrightfieldTolerance() override;
	virtual bool IsPresent() override;

private:
	enum class InitStates : uint8_t
	{
		EntryPoint = 0,
		GetFW,
		GetOpMode,
		ActualStatus,
		Reset,
		SetOpMode,
		SetBoostCurrentMonitoring,
		SetInitialPower,
		Complete,
		Error,
	};

	enum class LedInfo : uint8_t
	{
		SerialNumber = 0,
		SerialNumberHead,
		SpecInfo,
		MaxPower,
		WorkingHours,
		MeasureTempDiode,
		MeasureTempAmbient,
		LevelPower,
		PercentPower,
		TemporaryPower,
		ICGFreq,
		ICGDutyCycle,
		Complete,
		Error,
	};

	const std::string DESC = std::string("LEDMOD");

	//enum class LedStatusStates : uint8_t
	//{
	//	GetMeasureTempDiode = 0,
	//	GetMeasureTempAmbient,
	//	Complete,
	//	Error,
	//};

	void initialize (InitStates currentState, std::function<void(bool)> callback);
	void getInfo (std::string cmd, std::function<void(bool)> callback);
	void getAllInfo (LedInfo infoToGet, std::function<void(bool)> callback);
	void sendCommand (std::string cmd, std::function<void(bool, std::string)> callback);
	void sendCommandCallback(std::string cmd, std::function<void(bool, std::string)> callback, bool doretry, std::shared_ptr<std::vector<std::string>> responseList);
	void sendGetFailureByte (std::function<void(bool)> callback);
	void sendGetLatchedFailureByte (std::function<void(bool)> callback);
	void dumpOperatingMode (std::string sOperatingMode) const;
	static void dumpOperatingMode (uint16_t operatingMode);
	void dumpActualStatus (std::string sActualStatus) const;
	static void dumpActualStatus (uint16_t actualStatus);
	void dumpErrorStatus (std::string sErrorStatus); 
	void dumpErrorStatus (uint16_t errorStatus) const; 
	bool ParseResponse (std::string sCmd, std::string sResponse, std::vector<std::string>& parsedResult);
	void verifyCurrentLedPowerAsync (float percentPower, std::function<void(bool)> callback);
	boost::system::error_code openLedControllerBoard();
	void closeLedControllerBoard();

	std::shared_ptr<OmicronLedControllerBoardCommand> pLCBC_;
};
