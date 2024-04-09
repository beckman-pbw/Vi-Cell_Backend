#include "stdafx.h"

#include "boost/format.hpp"
#include "MotorStatusDLL.hpp"
#include "DataConversion.hpp"

MotorStatusDLL::MotorStatusDLL()
{
	this->motorFlags = eMotorFlags::mfUnknown;
	reset();
}

MotorStatusDLL& MotorStatusDLL::operator|=(const uint16_t& flags)
{
	// If input flag is unknown then reset to default
	if (flags == static_cast<uint16_t>(eMotorFlags::mfUnknown))
	{
		this->motorFlags = eMotorFlags::mfUnknown;
		return *this;
	}
	
	// In motor flags, eMotorFlags::mfPoweredOn,  eMotorFlags::mfConfigured,  eMotorFlags::mfHomed
	// flags are initially set and should not re-modified unless the motor reset.
	// So this will keep these flags untouched if already set
	// Rest other flags can be modified with each update.
	
	clearStatusBit(eMotorFlags::mfInMotion | eMotorFlags::mfAtPosition | eMotorFlags::mfPositionKnown | eMotorFlags::mfErrorState);
	this->motorFlags = this->motorFlags | flags;
	return *this;
}

void MotorStatusDLL::UpdateMotorHealth(eMotorFlags flag, int32_t position, ePositionDescription pd)
{
	UpdateMotorHealth(static_cast<uint16_t>(flag), position, pd);
}

void MotorStatusDLL::UpdateMotorHealth(uint16_t flags, int32_t position, ePositionDescription pd)
{
	this->position = position;
	this->pd = pd;

	// Don't assign the input flags to motor flags, since we need to carry the already success states
	// Bitwise OR operator is overloaded to handle this scenario
	*this |= flags;
}

void MotorStatusDLL::clearStatusBit(uint16_t flags)
{
	// Clear particular bit(s) from motor flags
	this->motorFlags &= ~flags;
}

void MotorStatusDLL::reset()
{	
	// Reset to unknown state
	UpdateMotorHealth(eMotorFlags::mfUnknown, -1, ePositionDescription::Unknown);
}

void MotorStatusDLL::ToCStyle (MotorStatus& motorStatus)
{
	motorStatus = { };

	motorStatus.flags = this->motorFlags;
	motorStatus.position = this->position;
	DataConversion::convertToCharArray(
		motorStatus.position_description, 
		sizeof(motorStatus.position_description), 
		getPositionDescription(this->pd));
}

std::string MotorStatusDLL::getMotorStatusAsString(const MotorStatus& motorStatus)
{
	std::string statusStr = boost::str(boost::format ("Flags: %s (0x%04X), Position: %d, Desc: %s")
									   % getMotorHealthAsStr(motorStatus.flags)
									   % motorStatus.flags
									   % motorStatus.position
									   % motorStatus.position_description);
	return statusStr;
}

std::string MotorStatusDLL::getMotorStatusAsString() const
{
	std::string statusStr = boost::str(boost::format ("Flags: %s (0x%04X), Position: %d, Desc: %s")
									   % getMotorHealthAsStr(this->motorFlags)
									   % this->motorFlags
									   % this->position
									   % getPositionDescription(this->pd));
	return statusStr;
}

std::string MotorStatusDLL::getMotorStatusFlagsAsStr(const eMotorFlags& flags)
{
	// Difference between "eMotorFlags::AtPosition" and "eMotorFlags::PositionKnown"

	/*"AtPosition" indicates that (from the perspective of the motor driver) it is at the 
	requested position and stable there. "PositionKnown" indicates that (from the perspective of the object/entity that's USING the motor driver)
	the absolute position of the motor within the system is known.

	At power up, the position within the system is unknown because the motor hasn't been homed/zeroed. 
	However, the motor knows that it is at position '0'. If you issue a "move relative 500 steps", 
	the motor can complete that move and tell you that it's at the requested position - but that does not mean you know where it is within the system.
	Only after homing and moving to any zero-offset does the SYSTEM "know" where the motor is.*/

	switch (flags)
	{
		case mfConfigured:
			return "Configured";
		case mfHomed:
			return "Homed";
		case mfInMotion:
			return "InMotion";
		case mfAtPosition:
			return "AtPosition";
		default:
		case mfErrorState:
			return "Error State";
		case mfPoweredOn:
			return "Powered On";

		case mfPositionKnown:
			return "Position Known";
	}
}

std::string MotorStatusDLL::getMotorHealthAsStr(const uint16_t& motorStatusFlags)
{
	if (motorStatusFlags & ~(eMotorFlags::mfUnknown))
	{
		return getMotorStatusFlagsAsStr(eMotorFlags::mfUnknown);
	}

	std::string healthStr = std::string();

	if (eMotorFlags::mfPoweredOn & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfPoweredOn));
		healthStr.append(" | ");
	}

	if (eMotorFlags::mfConfigured & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfConfigured));
		healthStr.append(" | ");
	}

	if (eMotorFlags::mfHomed & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfHomed));
		healthStr.append(" | ");
	}

	if (eMotorFlags::mfInMotion & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfInMotion));
		healthStr.append(" | ");
	}

	if (eMotorFlags::mfAtPosition & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfAtPosition));
		healthStr.append(" | ");
	}

	if (eMotorFlags::mfPositionKnown & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfPositionKnown));
		healthStr.append(" | ");
	}

	if (eMotorFlags::mfErrorState & motorStatusFlags)
	{
		healthStr.append(getMotorStatusFlagsAsStr(eMotorFlags::mfErrorState));		
	}

	return healthStr;
}

std::string MotorStatusDLL::getPositionDescription(const ePositionDescription& pd)
{
	switch (pd)
	{
		case ePositionDescription::Unknown:
			return "Unknown Position";
		case ePositionDescription::AtPosition:
			return "At Position";
		case ePositionDescription::Home:
			return "Home Position";
		case ePositionDescription::Current:
			return "Current Position";
		case ePositionDescription::Target:
			return "Target Position";
		default:
			return "Invalid Input Position Description";
	}

	return std::string();
}