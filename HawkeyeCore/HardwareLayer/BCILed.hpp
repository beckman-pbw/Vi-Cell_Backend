#pragma once

#include "LedBase.hpp"

// This class supports the custom BCI LEDs (brightfield and fluorescent).
// These LEDs are controlled from the ControllerBoard unlike the Omicron LED which is
// which is controlled by commands to its USB port.

class BCILed : public LedBase
{
public:
	BCILed (std::shared_ptr<CBOService> pCBOService);
	virtual ~BCILed();

	virtual void initializeAsync (std::function<void(bool)> callback) override;
	virtual void setPowerAsync (float percentPower, std::function<void(bool)> callback) override;
	virtual void setTriggerMode (TriggerMode triggerMode, std::function<void(bool)> callback) override;
	virtual float getBrightfieldTolerance() override;
	virtual bool IsPresent() override;

private:
	uint32_t convertLedPowerToVoltage (float percentPower);

	enum BCILEDCommand {
		BCILed_On = 1,
		BCILed_Off,
	};

	class BCILedWriteOperation : public ControllerBoardOperation::Operation
	{
	protected:
		BCILedWriteOperation()
			: ledCmd_({})
		{
			Operation::Initialize(&ledCmd_);
			mode_ = WriteMode;
		}

		LEDRegisters ledCmd_;
	};

	class BCILedInitializeOperation : public BCILedWriteOperation
	{
	public:
		BCILedInitializeOperation(RegisterIds regId, LEDRegisters ledReg)
			: BCILedWriteOperation()
		{
			ledCmd_.Command = ledReg.Command;
			ledCmd_.ErrorCode = ledReg.ErrorCode;
			ledCmd_.Power = ledReg.Power;
			ledCmd_.SimmerCurrent = ledReg.SimmerCurrent;
			ledCmd_.MAXON = ledReg.MAXON;
			ledCmd_.LTCD = ledReg.LTCD;
			ledCmd_.CTLD = ledReg.CTLD;
			ledCmd_.FeedBackPhotodiode = ledReg.FeedBackPhotodiode;

			regAddr_ = regId;
			lengthInBytes_ = sizeof(LEDRegisters);
		}
	};

	class BCISetPowerOperation : public ControllerBoardOperation::Operation
	{
	public:
		BCISetPowerOperation()
		{
			Operation::Initialize (&regData_);
		}

	protected:
		uint32_t regData_;
	};

	class BCILedSetPowerOperation : public BCISetPowerOperation
	{
	public:
		BCILedSetPowerOperation(RegisterIds regId, uint32_t power) 
			: BCISetPowerOperation()
		{
			mode_ = WriteMode;
			regData_ = power;
			regAddr_ = regId + offsetof(LEDRegisters, LEDRegisters::Power);
			lengthInBytes_ = sizeof(uint32_t);
		}
	};

	class BCILedSetPowerStateOperation : public BCILedWriteOperation
	{
	public:
		BCILedSetPowerStateOperation (RegisterIds regId, TriggerMode triggerMode)
			: BCILedWriteOperation()
		{
			if (triggerMode == TriggerMode::ContinousMode)
			{
				ledCmd_.Command = (uint32_t)BCILed_On;
			} else
			{
				ledCmd_.Command = (uint32_t)BCILed_Off;
			}

			regAddr_ = regId;
			lengthInBytes_ = sizeof(uint32_t);
		}
	};
};
