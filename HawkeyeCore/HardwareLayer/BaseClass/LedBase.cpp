#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"
#include "LedBase.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "LedBase";

//*****************************************************************************
LedBase::LedBase (std::shared_ptr<CBOService> pCBOService)
	: pCBOService_(std::move(pCBOService))
{
	//*****************************************************************************
	// triggerMode: sets whether or not the camera is in data acquisition mode
	// or is in service mode (continuously on).
	//  true: data acquisition mode (digital trigger).
	// false: service mode (continuously on).
	//*****************************************************************************
	simAndBCITriggerMode = DigitalMode;
}

//*****************************************************************************
LedBase::~LedBase()
{ }

//*****************************************************************************
void LedBase::initialize (HawkeyeConfig::LedType ledType, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrapCallback = [this, callback](bool status)
	{
		pCBOService_->enqueueExternal (callback, status);
	};

	// Get the appropriate LedConfig from HawkeyeConfig.
	// The LED fields in HawkeyeConfig are not used after this.
	pLedConfig_ = std::make_shared<HawkeyeConfig::LedConfig_t>(HawkeyeConfig::Instance().get().leds.ledmap[ledType]);

	errorInstance_ = getLedErrorInstance(ledType);
	registerId_ = ledTypeToRegisterId(ledType);

	HAWKEYE_ASSERT (MODULENAME, callback);

	pCBOService_->enqueueInternal (std::bind(&LedBase::initializeAsync, this, wrapCallback));
}

//*****************************************************************************
void LedBase::initializeAsync (std::function<void(bool)> callback)
{
	pCBOService_->enqueueInternal (std::bind(callback, true));
}

//*****************************************************************************
void LedBase::getPower (std::function<void(boost::optional<float>)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto wrapCallback = [this, callback](boost::optional<float> percentPower)
	{
		if (percentPower) {
			pLedConfig_->percentPower = percentPower.get();
		}
		pCBOService_->enqueueExternal (callback, percentPower);
	};

	pCBOService_->enqueueInternal (std::bind(&LedBase::getPowerAsync, this, wrapCallback));
}

//*****************************************************************************
void LedBase::getPowerAsync (std::function<void(boost::optional<float>)> callback)
{
	pCBOService_->enqueueInternal(std::bind(callback, boost::optional<float>(pLedConfig_->percentPower)));
}

//*****************************************************************************
std::string LedBase::getTriggerModeAsStr (TriggerMode triggerMode)
{
	if (triggerMode == ContinousMode) {
		return "Continous";
	} else {
		return "Digital";
	}
}

//*****************************************************************************
void LedBase::setPower (float percentPower, std::function<void(bool)> callback, TriggerMode triggerMode)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, 
		boost::str (boost::format ("setPower:: led: %s, triggerMode: %s, power: %f")
			% pLedConfig_->name
			% getTriggerModeAsStr(triggerMode)
			% percentPower));

	HAWKEYE_ASSERT (MODULENAME, callback);

	auto setTriggerMode_cb = [this, callback, percentPower, triggerMode](bool status) -> void {
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "setPower: <exit, setTriggerMode failed>");
			pCBOService_->enqueueExternal (callback, status);

			return;
		}

		auto wrapCallback = [this, callback, percentPower](bool status) {
			if (status)
			{
				setPowerInConfig (percentPower);
			}
			pCBOService_->enqueueExternal (callback, status);
		};

		pCBOService_->enqueueInternal (std::bind(&LedBase::setPowerAsync, this, percentPower, wrapCallback));
	};

	setTriggerMode (triggerMode, setTriggerMode_cb);
}

//*****************************************************************************
void LedBase::setPowerAsync (float percentPower, std::function<void(bool)> callback)
{
	pCBOService_->enqueueInternal (std::bind(callback, true));
}

//*****************************************************************************
// triggerMode: sets whether or not the camera is in data acquisition mode
// or is in service mode (continuously on).
//  true: data acquisition mode (digital trigger).
// false: service mode (continuously on).
//*****************************************************************************
void LedBase::getTriggerMode (std::function<void(bool, TriggerMode)> callback) {

	pCBOService_->enqueueInternal (callback, true, simAndBCITriggerMode);
}

//*****************************************************************************
void LedBase::setTriggerMode (TriggerMode triggerMode, std::function<void(bool)> callback) {

	simAndBCITriggerMode = triggerMode;
	pCBOService_->enqueueInternal (callback, true);
}

//*****************************************************************************
bool LedBase::setConfig (float percentPower, uint32_t simmerCurrentVoltage, 
						uint32_t ltcd, uint32_t ctld, uint32_t feedbackPhotodiode) {

	//	leds[ledtype].wavelength
	pLedConfig_->percentPower = percentPower;
	pLedConfig_->simmerCurrentVoltage = simmerCurrentVoltage;
	pLedConfig_->ltcd = ltcd;
	pLedConfig_->ctld = ctld;
	pLedConfig_->feedbackPhotodiode = feedbackPhotodiode;

	return true;
}

//*****************************************************************************
void LedBase::setPowerInConfig (float percentPower) {

	if (percentPower < 0.0f) {
		percentPower = 0.0f;
	}
	if (percentPower > 100.0) {
		percentPower = 100.0f;
	}

	pLedConfig_->percentPower = percentPower;

	Logger::L().Log (MODULENAME, severity_level::debug1, 
		boost::str (boost::format ("Setting LED %s power to %d percent") 
			% pLedConfig_->name
			% percentPower));
}

//TODO: Deprecated for now...
////*****************************************************************************
//bool LedBase::setSimmerCurrent (Type ledType, uint32_t simmerCurrent) {
//	
//    if (!isLedTypeValid(ledType)) {
//        return false;
//    }
//
//    leds[ledType].simmerCurrentVoltage = simmerCurrent;
//
//	return true;
//}
//
////*****************************************************************************
//bool LedBase::setLEDToCETDelay (Type ledType, uint32_t ledToCETDelay) {
//
//    if (!isLedTypeValid(ledType)) {
//        return false;
//    }
//
//    leds[ledType].ltcd = ledToCETDelay;
//
//	return true;
//}
//
////*****************************************************************************
//bool LedBase::setCETToLEDDelay (Type ledType, uint32_t cetToLEDDelay) {
//
//    if (!isLedTypeValid(ledType)) {
//        return false;
//    }
//
//    leds[ledType].ctld = cetToLEDDelay;
//
//	return true;
//}
//
////*****************************************************************************
//bool LedBase::setPhotoDiode (Type ledType, uint8_t photoDiodeNum) {
//
//    if (!isLedTypeValid(ledType)) {
//        return false;
//    }
//
//    leds[ledType].feedbackPhotodiode = photoDiodeNum;
//
//	return true;
//}

//////*****************************************************************************ge
//////TODO: not currently supported in ControllerBoard.
//////*****************************************************************************
//bool LedBase::setOnTime (Type ledType, uint32_t onTime_usecs) {
//
//    if (!isLedTypeValid(ledType)) {
//        return false;
//    }
//
//	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format("Setting LED ON time to %d usecs") % onTime_usecs));
//
//	return true;
//}

//*****************************************************************************
RegisterIds LedBase::ledTypeToRegisterId (HawkeyeConfig::LedType ledType) {

	switch (ledType)
	{
		case HawkeyeConfig::LedType::LED_BrightField:
			return RegisterIds::LED1Regs;
		case HawkeyeConfig::LedType::LED_TOP1:
			return RegisterIds::LED2Regs;
		case HawkeyeConfig::LedType::LED_BOTTOM1:
			return RegisterIds::LED3Regs;
		case HawkeyeConfig::LedType::LED_TOP2:
			return RegisterIds::LED4Regs;
		case HawkeyeConfig::LedType::LED_BOTTOM2:
			return RegisterIds::LED5Regs;
		default:
			Logger::L().Log (MODULENAME, severity_level::error, "Unknown LED type: " + std::to_string(static_cast<uint8_t>(ledType)));
			HAWKEYE_ASSERT (MODULENAME, false);
	}
}

//*****************************************************************************
// This is a default value for simulation mode.
//*****************************************************************************
float LedBase::getBrightfieldTolerance()
{
	return 0.1f;
}

//*****************************************************************************
bool LedBase::isPowerSame(float srcPower, float dstPower)
{
	return std::abs(srcPower - dstPower) <= getBrightfieldTolerance();
}

//*****************************************************************************
instrument_error::imaging_led_instances LedBase::getLedErrorInstance(HawkeyeConfig::LedType ledType)
{
	switch (ledType)
	{
		case HawkeyeConfig::LedType::LED_BrightField:
			return instrument_error::imaging_led_instances::brightfield;
		case HawkeyeConfig::LedType::LED_TOP1:
			return instrument_error::imaging_led_instances::top_1;
		case HawkeyeConfig::LedType::LED_BOTTOM1:
			return instrument_error::imaging_led_instances::bottom_1;
		case HawkeyeConfig::LedType::LED_TOP2:
			return instrument_error::imaging_led_instances::top_2;
		case HawkeyeConfig::LedType::LED_BOTTOM2:
			return instrument_error::imaging_led_instances::bottom_2;
		default:
			break;
	}

	HAWKEYE_ASSERT (MODULENAME, false);
}
