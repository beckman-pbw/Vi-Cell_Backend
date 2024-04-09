#include "stdafx.h"

#include <stdint.h>
#include <sstream>

#include "BoardStatus.hpp"


//*****************************************************************************
//*****************************************************************************
BoardStatus::BoardStatus() noexcept
	: status_(0)
{
}

//*****************************************************************************
BoardStatus::BoardStatus (uint16_t status)
	: status_(status)
{
}

//*****************************************************************************
uint16_t BoardStatus::get()
{
	return status_;
}

//*****************************************************************************
bool BoardStatus::isSet (StatusBit bitPosition)
{
    if ( bitPosition == DoNotCheckAnyBit )
    {
        return false;
    }
    return ((status_ & (1 << bitPosition)) != 0);
}

//*****************************************************************************
void BoardStatus::clearBit (StatusBit bitPosition)
{
    if ( bitPosition != DoNotCheckAnyBit )
    {
        status_ &= ~( 1 << bitPosition );
    }
}

//*****************************************************************************
void BoardStatus::setBit (StatusBit bitPosition) {
	if (bitPosition != DoNotCheckAnyBit) {
		status_ |= (1 << bitPosition);
	}
}

//*****************************************************************************
std::string BoardStatus::getAsString() {
	std::stringstream ss;

	if (isSet(Error)) {
		ss << "ErrorBit ";
	}
	if (isSet(HostCommError)) {
		ss << "HostCommError ";
	}
	if (isSet(UnusedBit2)) {
		ss << "UnusedBit2 ";
	}
	if (isSet(ProbeMotorBusy)) {
		ss << "ProbeMotorBusy ";
	}
	if (isSet(RadiusMotorBusy)) {
		ss << "RadiusMotorBusy ";
	}
	if (isSet(ThetaMotorBusy)) {
		ss << "ThetaMotorBusy ";
	}
	if (isSet(FocusMotorBusy)) {
		ss << "FocusMotorBusy ";
	}
	if (isSet(ReagentMotorBusy)) {
		ss << "ReagentMotorBusy ";
	}
	if (isSet(Rack1MotorBusy)) {
		ss << "Rack1MotorBusy ";
	}
    if (isSet(Rack2MotorBusy)) {
        ss << "Rack2MotorBusy ";
    }
    if (isSet(ObjectiveMotorBusy)) {
        ss << "ObjectiveMotorBusy ";
    }
    if (isSet(SyringeBusy)) {
		ss << "SyringeBusy ";
	}
	if (isSet(EEPROMBusy)) {
		ss << "EEPROMBusy ";
	}
	if (isSet(ExposureTimerBusy)) {
		ss << "ExposureTimerBusy ";
	}
	if (isSet(ReagentBusy)) {
		ss << "ReagentBusy ";
	}
	if (isSet(FwUpdateBusy)) {
		ss << "FwUpdateBusy ";
	}

	return ss.str();
}

