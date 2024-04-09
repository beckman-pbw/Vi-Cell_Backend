#pragma once

#include <cstdint>
#include <sstream>

//*****************************************************************************
class DLL_CLASS BoardStatus
{
public:
	enum StatusBit : int32_t // required for access to all 16 bits...
    {
		DoNotCheckAnyBit = -1,
		Error = 0,
		HostCommError = 1,
		UnusedBit2 = 2,
		Motor1Busy = 3,
		Motor2Busy = 4,
		Motor3Busy = 5,
		Motor4Busy = 6,
		Motor5Busy = 7,
		Motor6Busy = 8,
        Motor7Busy = 9,
        Motor8Busy = 10,
        ProbeMotorBusy = Motor1Busy,
        RadiusMotorBusy = Motor2Busy,
        ThetaMotorBusy = Motor3Busy,
        FocusMotorBusy = Motor4Busy,
        ReagentMotorBusy = Motor5Busy,
        Rack1MotorBusy = Motor6Busy,
        Rack2MotorBusy = Motor7Busy,
        ObjectiveMotorBusy = Motor8Busy,
		SyringeBusy = 11,
		EEPROMBusy = 12,
		ExposureTimerBusy = 13,
        ReagentBusy = 14,
		FwUpdateBusy = 15,
	};

	BoardStatus() noexcept;
	BoardStatus (uint16_t status);
	~BoardStatus() = default;

	uint16_t get();
	bool isSet (StatusBit bitPosition);
	void clearBit (StatusBit bitPosition);
	void setBit (StatusBit bitPosition);
	std::string getAsString();

private:
	uint16_t status_;
};

