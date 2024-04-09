#include "stdafx.h"

#include <stdint.h>
#include <sstream>

#include "SignalStatus.hpp"



SignalStatus::SignalStatus() noexcept
	: status_(0)
{
}

SignalStatus::SignalStatus (uint32_t status)
	: status_(status)
{
}


uint32_t SignalStatus::get()
{
    return status_;
}

bool SignalStatus::isSet( SignalStatusBits bitPosition )
{
    return ( ( status_ & ( 1 << bitPosition ) ) != 0 );
}

void SignalStatus::clearBit( SignalStatusBits bitPosition )
{
    status_ &= ~( 1 << bitPosition );
}

std::string SignalStatus::getAsString()
{
    std::stringstream ss;

    if ( isSet( CarouselTube ) )
    {
        ss << "CarouselTube ";
    }

    if ( isSet( CarouselPresent ) )
    {
        ss << "CarouselPresent ";
    }

    if ( isSet( ReagentDoorClosed ) )
    {
        ss << "ReagentDoorClosed ";
    }

    if ( isSet( ReagentPackInstalled ) )
    {
        ss << "ReagentPackInstalled ";
    }
	
    if ( isSet( RadiusMotorHome ) )
    {
        ss << "RadiusMotorHome ";
    }

    if ( isSet( RadiusMotorLimit ) )
    {
        ss << "RadiusMotorAtLimit ";
    }

    if ( isSet( ThetaMotorHome ) )
    {
        ss << "ThetaMotorHome ";
    }

    if ( isSet( ThetaMotorLimit ) )
    {
        ss << "ThetaMotorAtLimit ";
    }

    if ( isSet( ProbeMotorHome ) )
    {
        ss << "ProbeMotorHome ";
    }

    if ( isSet( ProbeMotorLimit ) )
    {
        ss << "ProbeMotorAtLimit ";
    }

    if ( isSet( ReagentMotorHome ) )
    {
        ss << "ReagentMotorHome ";
    }

    if ( isSet( ReagentMotorLimit ) )
    {
        ss << "ReagentMotorAtLimit ";
    }

    if ( isSet( FocusMotorHome ) )
    {
        ss << "FocusMotorHome ";
    }

    if ( isSet( FocusMotorLimit ) )
    {
        ss << "FocusMotorAtLimit ";
    }

    if ( isSet( LEDRackMotorHome ) )
    {
        ss << "LEDRackMotorHome ";
    }

    if ( isSet( LEDRackMotorLimit ) )
    {
        ss << "LEDRackMotorAtLimit ";
    }

    if ( isSet( Rack1MotorHome ) )
    {
        ss << "Rack1MotorHome ";
    }

    if ( isSet( Rack1MotorLimit ) )
    {
        ss << "Rack1MotorAtLimit ";
    }

    if ( isSet( Rack2MotorHome ) )
    {
        ss << "Rack2MotorHome ";
    }

    if ( isSet( Rack2MotorLimit ) )
    {
        ss << "Rack2MotorAtLimit ";
    }

    if ( isSet( ObjectiveMotorHome ) )
    {
        ss << "ObjectiveMotorHome ";
    }

    if ( isSet( ObjectiveMotorLimit ) )
    {
        ss << "ObjectiveMotorAtLimit ";
    }

    return ss.str();
}

