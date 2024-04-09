#pragma once

#include <stdint.h>
#include "CBOService.hpp"

class VoltageMeasurementBase
{
public:
	typedef struct
	{
		float v_5;
		float v_12;
		float v_24;
		float P5Vsen;
		float neg_v_3;
		float v_33;
		float extTemp;
		float boardTemp;
		float cpuTemp;
		float b1Out;	// Not used
		float b2Out;	// Not used
		float LED_SW;
		float LED_ISENSE;
		float LED_DR;
		float pd1Out;
		float pd2Out;
	} VoltageMeasurements_t;

	VoltageMeasurementBase (std::shared_ptr<CBOService> pCBOService);
	virtual ~VoltageMeasurementBase();

	virtual bool Initialize (void);

	virtual void ReadVoltages (std::function<void (VoltageMeasurements_t vgs)> cb) = 0;

protected:
	std::shared_ptr<CBOService> pCBOService_;
};

class VoltageMonitorOperation : public ControllerBoardOperation::Operation
{
public:
	VoltageMonitorOperation ()
	{
		Operation::Initialize (&regs_);
		regAddr_ = RegisterIds::MonitorVoltageBitMap;
	}

protected:
	uint32_t regs_[(VOLT_MON_REG_SIZE + VOLTMON_BITMAP_SIZE) / sizeof(uint32_t)];
};

class VoltageReadOperation : public VoltageMonitorOperation
{
public:
	VoltageReadOperation ()
	{
		mode_ = ReadMode;
		lengthInBytes_ = VOLT_MON_REG_SIZE + VOLTMON_BITMAP_SIZE;
	}
};


