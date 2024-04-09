#pragma once

#include <cstdint>

//NOTE: these are duplicates of what is in *Registers.hpp* MotorIds.
// This duplication is needed to prevent the inclusion of *Registers.hpp* here.
enum class eMotorType
{
	ProbeMotorId = 1,
	RadiusMotorId,        // sample slide
	ThetaMotorId,         // sample rotate
	FocusMotorId,
	ReagentMotorId,       // reagent pierce
	Rack1MotorId,
	Rack2MotorId,
	ObjectiveMotorId,
};

// Bitmask values for interpreting the Motor Status "flags" register
enum eMotorFlags : uint16_t
{
	mfUnknown = 0x0000,
	mfConfigured = 0x0001,	// if !(flags & mfConfigured) - motor information not important
	mfHomed = 0x0002,   // has motor been homed / zeroed
	mfInMotion = 0x0004,
	mfAtPosition = 0x0008,
	mfErrorState = 0x0010,   // if (flags & mfErrorState) - motor is in a faulted condition
	mfPoweredOn = 0x0020,   // if (flags & mfPoweredOn) - drive/holding power is applied to motor.
	mfPositionKnown = 0x0040,
};

struct MotorStatus
{
	uint16_t flags;			// Combination of multiple flags of type "eMotorFlags"
	int32_t position;
	char position_description[16]; // up, down, "A5", in, out...
};
