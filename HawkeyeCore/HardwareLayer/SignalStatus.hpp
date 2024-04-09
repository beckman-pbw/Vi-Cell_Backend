#pragma once

#include <stdint.h>
#include <sstream>

//*****************************************************************************
class DLL_CLASS SignalStatus
{
public:
    SignalStatus() noexcept;
    SignalStatus( uint32_t status );
	SignalStatus& operator= (const SignalStatus& other) = default;
    ~SignalStatus() = default;


    enum SignalStatusBits : int32_t
    {
        DoNotCheckAnyBit = -1,
        CarouselTube = 0,
        CarouselPresent = 1,
        ReagentDoorClosed = 2,
        ReagentPackInstalled = 3,
        Motor1Home = 4,
        Motor1Limit = 5,
        Motor2Home = 6,
        Motor2Limit = 7,
        Motor3Home = 8,
        Motor3Limit = 9,
        Motor4Home = 10,
        Motor4Limit = 11,
        Motor5Home = 12,
        Motor5Limit = 13,
        Motor6Home = 14,
        Motor6Limit = 15,
        Motor7Home = 16,
        Motor7Limit = 17,
        Motor8Home = 18,
        Motor8Limit = 19,
        UnusedBit20 = 20,
        UnusedBit21 = 21,
        UnusedBit22 = 22,
        UnusedBit23 = 23,
        UnusedBit24 = 24,
        UnusedBit25 = 25,
        UnusedBit26 = 26,
        UnusedBit27 = 27,
        UnusedBit28 = 28,
        UnusedBit29 = 29,
        UnusedBit30 = 30,
        UnusedBit31 = 31,
        UnusedBit = UnusedBit31,
        ProbeMotorHome = Motor1Home,
        ProbeMotorLimit = Motor1Limit,
        RadiusMotorHome = Motor2Home,
        RadiusMotorLimit = Motor2Limit,
        ThetaMotorHome = Motor3Home,
        ThetaMotorLimit = Motor3Limit,
        FocusMotorHome = Motor4Home,
        FocusMotorLimit = Motor4Limit,
        ReagentMotorHome = Motor5Home,
        ReagentMotorLimit = Motor5Limit,
        Rack1MotorHome = Motor6Home,
        Rack1MotorLimit = Motor6Limit,
        Rack2MotorHome = Motor7Home,
        Rack2MotorLimit = Motor7Limit,
        ObjectiveMotorHome = Motor8Home,
        ObjectiveMotorLimit = Motor8Limit,
        LEDRackMotorHome = UnusedBit,
        LEDRackMotorLimit = UnusedBit,
    };

    uint32_t get();
    bool isSet( SignalStatusBits bitPosition );
    void clearBit( SignalStatusBits bitPosition );
    std::string getAsString();

private:
    uint32_t status_;
};
