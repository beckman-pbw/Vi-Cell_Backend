#pragma once

#include "DataConversion.hpp"
#include "HawkeyeConfig.hpp"
#include "SyringePumpPort.hpp"
#include "TxRxBuffer.hpp"
#include "CBOService.hpp"

enum class SyringePumpCommand {
	Initialize = 1,
	SetPosition = 2,
	SetValve = 3,
	SendDirect = 4,
};

class SyringePumpBase
{
public:

	// *** Instrument Port # to ControllerBoard Port # Conversions ***
	//                  Instrument  ControllerBoard  Syringe Pump
	//   Description       Port          Port            Port
	//      Waste            1             6               F
	//      A-Cup            2             7               G
	//     FlowCell          3             8               H
	//    TrypanBlue         4             1               A
	//     Cleaner_1         5             2               B
	//     Cleaner_2         6             3               C
	//     Cleaner_3         7             4               D
	//      Diluent          8             5               E
	//
	// The Instrument Port is from the user perspective.  These were arbitrarily 
	// assigned during the feasibility study using LabView.
	// ControllerBoard Port maps the Syringe Pump ports (A, B, C...) to 1, 2, 3...

	SyringePumpBase (std::shared_ptr<CBOService> pCBOService);
	virtual ~SyringePumpBase() = default;

	virtual void initialize(std::function<void(bool)> callback);
	virtual bool getPosition (uint32_t& pos);
	virtual void setPosition(std::function<void(bool)> callback, uint32_t volume, uint32_t speed) = 0;
	virtual SyringePumpPort getValve();
	virtual bool getValve (SyringePumpPort& port);
	virtual void setValve (std::function<void(bool)> callback, SyringePumpPort port, SyringePumpDirection direction) = 0;
	virtual std::string getVersion();
	virtual void rotateValve (std::function<void(bool)> callback, uint32_t angle, SyringePumpDirection direction);
	virtual void sendValveCommand(std::function<void(bool)> callback, std::string valveCommand);

	virtual bool getPhysicalValve (PhysicalPort_t& valveNum);
	virtual void setPhysicalValve (std::function<void(bool)> callback, char port, SyringePumpDirection direction);
	virtual bool isAspirating(uint32_t target_volume_uL);

	static SyringePumpPort paramToPort (uint32_t param);
	static SyringePumpDirection paramToDirection (uint32_t param);

	static char MinPhysicalValvePosition;
	static char MaxPhysicalValvePosition;
	static bool isPhysicalPortValid (char PhysicalPort);
	static bool isPhysicalPortValid (PhysicalPort_t physicalPort);
	static bool isAspirationAllowed (PhysicalPort_t physicalPort);
	static bool isDisPenseAllowed (PhysicalPort_t physicalPort);
	static bool FromPhysicalPortToChar (PhysicalPort_t physicalPort, char& physicalPortChar);
	static bool ToPhysicalPortFromChar (char physicalPortChar, PhysicalPort_t& physicalPort);
	static bool UpdateReagentVolume (PhysicalPort_t physicalPort, uint32_t volume);

protected:	
	std::shared_ptr<CBOService> pCBOService_;
	const HawkeyeConfig::HawkeyeConfig_t* config_;
	uint32_t cur_volume_uL_;
	PhysicalPort_t curPhysicalPort_;
	std::string version_;
	SyringeRegisters registerCache_;
};
