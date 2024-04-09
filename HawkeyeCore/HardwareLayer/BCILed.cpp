#include "stdafx.h"

#include "BCILed.hpp"
#include "ErrorCode.hpp"

static const char MODULENAME[] = "BCILed";

//*****************************************************************************
BCILed::BCILed (std::shared_ptr<CBOService> pCBOService)
	: LedBase (pCBOService)
{
}

//*****************************************************************************
BCILed::~BCILed()
{
}

bool BCILed::IsPresent()
{
	//There's no good way to detect a BCI LED, so just return true. Client code should check if an Omicron is
	//present first before testing BCI due to this.
	return true;
}


//*****************************************************************************
uint32_t BCILed::convertLedPowerToVoltage (float percentPower)
{
	uint32_t value = (uint32_t)(static_cast<float>(HawkeyeConfig::Instance().get().leds.maxBCILedVoltage_uv) * (percentPower / 100.0f));
	return value;
}

//*****************************************************************************
void BCILed::initializeAsync (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onComplete = [this, callback](ControllerBoardOperation::CallbackData_t cbData)
	{
		if (cbData.status != ControllerBoardOperation::eStatus::Success)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_hardwareerror, 
				errorInstance_,
				instrument_error::severity_level::error));
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("initializeAsync: failed, ledType: %s, status: 0x%08X, %s") 
					% pLedConfig_->name
					% cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		pCBOService_->enqueueInternal(callback, true);
	};

	// Setup BCI LED using the ControllerBoard.
	LEDRegisters ledCmd = { };
	ledCmd.Command = (uint32_t)BCILed_Off;
	ledCmd.ErrorCode = 0;
	ledCmd.Power = convertLedPowerToVoltage(pLedConfig_->percentPower);
	ledCmd.SimmerCurrent = convertLedPowerToVoltage(static_cast<float>(pLedConfig_->simmerCurrentVoltage));
	ledCmd.MAXON = 40000000;
	ledCmd.LTCD = pLedConfig_->ltcd;
	ledCmd.CTLD = pLedConfig_->ctld;
	ledCmd.FeedBackPhotodiode = pLedConfig_->feedbackPhotodiode;

	const uint32_t timeout_millisec = 1000;
	auto tid = pCBOService_->CBO()->Execute (BCILedInitializeOperation(registerId_, ledCmd), timeout_millisec, onComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("initializeAsync, task %d") % (*tid)));
	} else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "initializeAsync::CBO Execute failed");
		pCBOService_->enqueueInternal(callback, false);
	}
}

//*****************************************************************************
void BCILed::setPowerAsync (float percentPower, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onComplete = [this, percentPower, callback](ControllerBoardOperation::CallbackData_t cbData)
	{
		if (cbData.status != ControllerBoardOperation::eStatus::Success) {
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("setPowerAsync: 0x%08X, %s") % cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_hardwareerror,
				errorInstance_, 
				instrument_error::severity_level::error));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		pCBOService_->enqueueInternal(callback, true);
	};

	const uint32_t timeout_millisec = 1000;
	const uint32_t ledPower = convertLedPowerToVoltage (percentPower);
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("setPowerAsync: percentPower: %d, ledPower: %d") % percentPower % ledPower));

	auto tid = pCBOService_->CBO()->Execute (BCILedSetPowerOperation(registerId_, ledPower), timeout_millisec, onComplete);
	if (tid) {
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("setPowerAsync, task %d") % (*tid)));

	} else {
		Logger::L().Log (MODULENAME, severity_level::error, "setPowerAsync::CBO Execute failed");
		pCBOService_->enqueueInternal(callback, false);
	}
}

//*****************************************************************************
// triggerMode: sets whether or not the camera is in data acquisition mode
// or is in service mode (continuously on).
//  true: data acquisition mode (digital trigger).
// false: service mode (continuously on).
//*****************************************************************************
void BCILed::setTriggerMode (TriggerMode triggerMode, std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	simAndBCITriggerMode = triggerMode;

	auto onComplete = [this, callback, triggerMode](ControllerBoardOperation::CallbackData_t cbData)
	{
		if (cbData.status != ControllerBoardOperation::eStatus::Success)
		{
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::imaging_led_hardwareerror, 
				errorInstance_, 
				instrument_error::severity_level::error));
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("setTriggerMode: failed, ledType: %s, status: 0x%08X, %s")
					% pLedConfig_->name
					% cbData.errorCode % ErrorCode(cbData.errorCode).getAsString()));
			pCBOService_->enqueueInternal(callback, false);
			return;
		}

		simAndBCITriggerMode = triggerMode;

		pCBOService_->enqueueInternal(callback, true);
	};

	const uint32_t timeout_millisec = 1000;
	auto tid = pCBOService_->CBO()->Execute (BCILedSetPowerStateOperation(registerId_, triggerMode), timeout_millisec, onComplete);
	if (tid)
	{
		Logger::L().Log (MODULENAME, severity_level::debug2, boost::str(boost::format("initializeAsync, task %d") % (*tid)));
	} else
	{
		Logger::L().Log (MODULENAME, severity_level::error, "initializeAsync::CBO Execute failed");
		pCBOService_->enqueueInternal(callback, false);
	}
}

//*****************************************************************************
float BCILed::getBrightfieldTolerance()
{
	return HawkeyeConfig::Instance().get().leds.bciTolerance;
}
