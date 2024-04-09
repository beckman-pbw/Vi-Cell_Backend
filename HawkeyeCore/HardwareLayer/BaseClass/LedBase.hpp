#pragma once

#include <array>

#include "CBOService.hpp"
#include "HawkeyeConfig.hpp"

class LedBase
{
public:
	enum TriggerMode {
		ContinousMode = 1,
		DigitalMode,
	};

	LedBase (std::shared_ptr<CBOService> pCBOService);
	virtual ~LedBase();

	void initialize (HawkeyeConfig::LedType ledType, std::function<void(bool)> callback);
	void getPower (std::function<void(boost::optional<float>)> callback);
	void setPower (float percentPower, std::function<void(bool)> callback, TriggerMode triggerMode = DigitalMode);
	bool setConfig (float percentPower, uint32_t simmerCurrentVoltage, uint32_t ltcd, uint32_t ctld, uint32_t feedbackPhotodiode);
	bool isPowerSame(float srcPower, float dstPower);
	//virtual bool setOnTime (Type ledType, uint32_t onTime_usecs);
	//virtual bool set (HawkeyeConfig::LedType ledType, float percentPower, uint32_t simmerCurrentVoltage, uint32_t ltcd, uint32_t ctld, uint32_t feedbackPhotodiode);
	//virtual bool setSimmerCurrent (Type ledType, uint32_t simmerCurrent);
	//virtual bool setLEDToCETDelay (Type ledType, uint32_t ledToCETDelay);
	//virtual bool setCETToLEDDelay (Type ledType, uint32_t cetToLEDDelay);
	//virtual bool setPhotoDiode (Type ledType, uint8_t photoDiodeNum);	
	virtual void getTriggerMode (std::function<void(bool, TriggerMode)> callback);
	virtual void setTriggerMode (TriggerMode triggerMode, std::function<void(bool)> callback);
	static instrument_error::imaging_led_instances getLedErrorInstance(HawkeyeConfig::LedType ledType);
	void setPowerInConfig (float percentPower);
	static RegisterIds ledTypeToRegisterId (HawkeyeConfig::LedType ledType);
	std::shared_ptr<HawkeyeConfig::LedConfig_t> getConfig() { return pLedConfig_; }
	virtual bool IsPresent() { return true; }

protected:
	virtual void initializeAsync (std::function<void(bool)> callback);
	virtual void getPowerAsync (std::function<void(boost::optional<float>)> callback);
	virtual void setPowerAsync (float percentPower, std::function<void(bool)> callback);
	virtual float getBrightfieldTolerance();
	std::string getTriggerModeAsStr (TriggerMode triggerMode);

	// Keepi track of the state of the BCI LED and simulation.
	TriggerMode simAndBCITriggerMode;

	instrument_error::imaging_led_instances errorInstance_;
	RegisterIds registerId_;

	std::shared_ptr < HawkeyeConfig::LedConfig_t>  pLedConfig_;
	std::shared_ptr<CBOService> pCBOService_;
};
