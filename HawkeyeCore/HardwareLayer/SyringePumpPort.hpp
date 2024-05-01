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

class SyringePumpPort
{
public:
#ifdef CELLHEALTH_MODULE
	enum Port : uint8_t
	{
		InvalidPort = 0,
		Waste       = 1,
		ACup        = 2,
		FlowCell    = 3,
		Reagent_1   = 4,	// Trypan Blue
		Cleaner_1   = 5,	// Cleaning Agent
		Cleaner_2   = 6,	// Conditioning Solution
		Cleaner_3   = 7,	// Buffer
		Diluent     = 8,	// DI Water
	};
#endif

#ifdef VICELL_BLU
	enum Port : uint8_t
	{
		InvalidPort = 0,
		Waste       = 1,
		Sample      = 2,
		FlowCell    = 3,
		Reagent_1   = 4,	// Trypan Blue
		Cleaner_1   = 5,	// Cleaning Agent
		Cleaner_2   = 6,	// Conditioning Solution
		Cleaner_3   = 7,	// Buffer
		Sample_2    = 8,	// ACup
	};
#endif

	SyringePumpPort() { port_ = InvalidPort; }
	SyringePumpPort(Port port) { port_ = port; }
	virtual ~SyringePumpPort() {};
	std::string getAsString();
	Port get() const { return port_; }

	static PhysicalPort_t ToPhysicalPort (SyringePumpPort port);
	static SyringePumpPort FromPhysicalPort (PhysicalPort_t port);

private:
	Port port_;
};

class SyringePumpDirection {
public:
	enum Direction {
		Clockwise = 0,
		CounterClockwise = 1,
		DirectionError = 2,
	};

	SyringePumpDirection() { direction_ = DirectionError; }
	SyringePumpDirection (Direction direction) { direction_ = direction; }
	std::string getAsString();
	Direction get() const { return direction_; }

private:
	Direction direction_;
};
