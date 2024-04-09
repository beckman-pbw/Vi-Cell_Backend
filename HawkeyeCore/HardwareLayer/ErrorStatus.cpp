#include "stdafx.h"

#include <stdint.h>
#include <sstream>

#include "ErrorStatus.hpp"

//*****************************************************************************
ErrorStatus::ErrorStatus () noexcept 
	: status_(0)
{
}

ErrorStatus::ErrorStatus (uint64_t status)
	: status_(status)
{
}

void ErrorStatus::set (uint64_t error_status)
{
	status_ = error_status;
}

uint64_t ErrorStatus::get()
{
	return status_;
}

bool ErrorStatus::isSet (StatusBit bitPosition)
{
    if ( bitPosition == DoNotCheckAnyBit )
    {
        return false;
    }
    return ((status_ & ((uint64_t)1 << bitPosition)) != 0);
}

std::string ErrorStatus::getAsString()
{
	std::stringstream ss;

	// These are from the ErrorStatus1 register.
	if (isSet(MonitorVoltageBitmap))
    {
		ss << "MonitorVoltageBitmap ";
	}
	if (isSet(PowerSupplyFault))
    {
		ss << "PowerSupplyFault";
	}
	if (isSet(RadiusMotor))
    {
		ss << "RadiusMotor ";
	}
	if (isSet(ThetaMotor))
    {
		ss << "ThetaMotor ";
	}
	if (isSet(ProbeMotor))
    {
		ss << "ProbeMotor ";
	}
	if (isSet(ReagentMotor))
    {
		ss << "ReagentMotor ";
	}
    if ( isSet( FocusMotor ) )
    {
        ss << "FocusMotor ";
    }
    if ( isSet( Rack1Motor ) )
    {
        ss << "Rack1Motor ";
    }
    if ( isSet( Rack2Motor ) )
    {
        ss << "Rack2Motor ";
    }
    if ( isSet( ObjectiveMotor ) )
    {
        ss << "ObjectiveMotor ";
    }
    if ( isSet( Fan1 ) )
    {
        ss << "Fan1 ";
    }
    if ( isSet( Fan2 ) )
    {
        ss << "Fan2 ";
    }
    if ( isSet( LED1 ) )
    {
        ss << "LED1 ";
    }
    if ( isSet( LED2 ) )
    {
        ss << "LED2 ";
    }
    if ( isSet( LED3 ) )
    {
        ss << "LED3 ";
    }
    if ( isSet( LED4 ) )
    {
        ss << "LED4 ";
    }
    if ( isSet( LED5 ) )
    {
        ss << "LED5 ";
    }
    if ( isSet( LED6 ) )
    {
        ss << "LED6 ";
    }
    if ( isSet( PhotoDiode1 ) )
    {
        ss << "PhotoDiode1 ";
    }
    if ( isSet( PhotoDiode2 ) )
    {
        ss << "PhotoDiode2 ";
    }
    if ( isSet( PhotoDiode3 ) )
    {
        ss << "PhotoDiode3 ";
    }
    if ( isSet( PhotoDiode4 ) )
    {
        ss << "PhotoDiode4 ";
    }
    if ( isSet( PhotoDiode5 ) )
    {
        ss << "PhotoDiode5 ";
    }
    if ( isSet( PhotoDiode6 ) )
    {
        ss << "PhotoDiode6 ";
    }
    if ( isSet( DoorLatch ) )
    {
        ss << "DoorLatch ";
    }
    if ( isSet( Toggle2 ) )
    {
        ss << "Toggle2 ";
    }
    if ( isSet( Toggle3 ) )
    {
        ss << "Toggle3 ";
    }
    if ( isSet( Toggle4 ) )
    {
        ss << "Toggle4 ";
    }
    if ( isSet( RS232_0 ) )
    {
        ss << "RS232_0 ";
    }
    if ( isSet( RS232_3 ) )
    {
        ss << "RS232_3 ";
    }
    if ( isSet( RS232_5 ) )
    {
        ss << "RS232_5 ";
    }
    // These are from the ErrorStatus2 register.
    if ( isSet( Syringe ) )
    {
        ss << "Syringe ";
    }
    if ( isSet( Camera1 ) )
    {
        ss << "Camera1 ";
    }
    if ( isSet( Camera2 ) )
    {
        ss << "Camera2 ";
    }
    if ( isSet( Camera3 ) )
    {
        ss << "Camera3 ";
    }
    if ( isSet( HostComm ) )
    {
        ss << "HostComm ";
    }
    if ( isSet( Reagent ) )
    {
        ss << "Reagent ";
    }
    if ( isSet( PersistentMemory ) )
    {
        ss << "PersistentMemory ";
    }
	if ( isSet( FwUpdate ) )
	{
		ss << "FwUpdate ";
	}

	return ss.str();
}
