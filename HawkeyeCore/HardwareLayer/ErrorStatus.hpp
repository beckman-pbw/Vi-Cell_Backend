#pragma once

#include <cstdint>
#include <sstream>

//*****************************************************************************
class DLL_CLASS ErrorStatus {
public:
	enum StatusBit {
		DoNotCheckAnyBit = -1,
		MonitorVoltageBitmap = 0,
        PowerSupplyFault,
        Reserved1,
        Motor1,
        Motor2,
        Motor3,
        Motor4,
        Motor5,
        Motor6,
        Motor7,
        Motor8,
        ProbeMotor = Motor1,
        RadiusMotor = Motor2,
        ThetaMotor = Motor3,
        FocusMotor = Motor4,
        ReagentMotor = Motor5,
        Rack1Motor = Motor6,
        Rack2Motor = Motor7,
        ObjectiveMotor = Motor8,
        MotorLast = Motor8,
		Fan1 = MotorLast + 1,
		Fan2,
		LED1,
		LED2,
		LED3,
		LED4,
		LED5,
		LED6,
		PhotoDiode1,
		PhotoDiode2,
		PhotoDiode3,
		PhotoDiode4,
		PhotoDiode5,
		PhotoDiode6,
		DoorLatch,
		Toggle2,
		Toggle3,
		Toggle4,
		RS232_0,
		RS232_3,
		RS232_5,

		Reserved2,
		Syringe,
		Reserved3,	// was Syringe2,
		Camera1,
		Camera2,
		Camera3,
		HostComm,
		Reagent,
		PersistentMemory,
		FwUpdate,
	};

	ErrorStatus() noexcept;
	ErrorStatus (uint64_t status);
	~ErrorStatus() = default;

	uint64_t get();
	void set (uint64_t error_status);
	bool isSet (StatusBit bitPosition);
	std::string getAsString();

private:
	uint64_t status_;
};
