#pragma once

#include "MotorStatus.hpp"

// Enum for Position Description
typedef enum ePositionDescription : uint8_t
{
	Unknown = 0,
	AtPosition = 1,
	Home = 2,
	Current = 3,
	Target = 4,
}ePositionDescription;

typedef class MotorStatusDLL
{

public:
	MotorStatusDLL();
	MotorStatusDLL& operator|=(const uint16_t& flags);

	void UpdateMotorHealth(eMotorFlags flag, int32_t position, ePositionDescription pd);
	void UpdateMotorHealth(uint16_t flags, int32_t position, ePositionDescription pd);	
	void clearStatusBit(uint16_t flags);
	void reset();
	void ToCStyle (MotorStatus& motorStatus);		
	std::string getMotorStatusAsString() const;

	bool isInMotion() const
	{
		return (eMotorFlags::mfInMotion & this->motorFlags) != 0;
	}	

	// Static methods
	static std::string getMotorStatusAsString(const MotorStatus& motorStatus);
	static std::string getMotorStatusFlagsAsStr(const eMotorFlags& flags);
	static std::string getPositionDescription(const ePositionDescription& pd);
	static std::string getMotorHealthAsStr(const uint16_t& motorStatusFlags);

private:

	uint16_t motorFlags;
	int32_t position;
	ePositionDescription pd;

}MotorStatusDLL;