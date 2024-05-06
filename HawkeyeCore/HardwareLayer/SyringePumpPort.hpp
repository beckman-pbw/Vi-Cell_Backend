#pragma once

#include <stdint.h>
#include <string>

typedef enum PhysicalPort : uint8_t
{
	PortA = 1,
	PortB = 2,
	PortC = 3,
	PortD = 4,
	PortE = 5,
	PortF = 6,
	PortG = 7,
	PortH = 8,
} PhysicalPort_t;

#define CHM_PORT_OFFSET 10
class SyringePumpPort
{
public:
	enum Port : uint8_t
	{
		InvalidPort = 0,
		Waste       = 1,
		Sample      = 2,	// Vi-Cell
		CHM_ACup    = 2 + CHM_PORT_OFFSET,	// CHM A-Cup
		FlowCell    = 3,
		Reagent_1   = 4,	// Trypan Blue
		Cleaner_1   = 5,	// Cleaning Agent
		Cleaner_2   = 6,	// Conditioning Solution
		Cleaner_3   = 7,	// Buffer
		ViCell_ACup = 8,	// Vi-Cell A-Cup
		Diluent     = 8 + CHM_PORT_OFFSET,	// CHM DI Water
	};

	SyringePumpPort();
	SyringePumpPort(Port port);
	Port Get();
	void Set (Port port);
	virtual ~SyringePumpPort() {};
	std::string ToString(); 
	static SyringePumpPort ParamToPort(uint32_t param);
	static PhysicalPort_t ToPhysicalPort (SyringePumpPort port);
	static SyringePumpPort FromPhysicalPort (PhysicalPort_t port);

private:
	Port _port;
};

class SyringePumpDirection {
public:
	enum Direction {
		Clockwise = 0,
		CounterClockwise = 1,
		DirectionError = 2,
	};

	SyringePumpDirection() { _direction = DirectionError; }
	SyringePumpDirection (Direction direction) { _direction = direction; }
	std::string ToString();
	Direction Get() const { return _direction; }

private:
	Direction _direction;
};
