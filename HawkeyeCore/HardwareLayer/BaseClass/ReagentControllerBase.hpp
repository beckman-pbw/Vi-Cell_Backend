#pragma once

#include <stdint.h>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "ChronoUtilities.hpp"
#include "ControllerBoardOperation.hpp"
#include "CBOService.hpp"
#include "MotorBase.hpp"
#include "MotorStatusDLL.hpp"
#include "Rfid.hpp"
#include "RfidSim.hpp"
#include "SyringePumpPort.hpp"
#include "SystemErrors.hpp"

const int32_t DefaultReagentArmPositionTolerance = UnitsChange_0_1_mm;
const int32_t DoorLatchTimeout = 2000;

class DLL_CLASS ReagentControllerBase
{

public:

	enum class DoorCmds
	{
		LatchDoor = 0,
		UnlatchDoor = 1,
	};

	ReagentControllerBase (std::shared_ptr<CBOService> pCBOService);
	virtual ~ReagentControllerBase();

	virtual void        Initialize (std::function<void(bool)> callback, std::string cfgFile, bool apply = false) = 0;
	virtual void        Reset(void) = 0;
	virtual bool        ControllerOk(void) = 0;
	virtual void        Stop(void) = 0;
	virtual bool        IsDoorClosed(void) = 0;
	virtual bool        IsPackInstalled(void) = 0;
	virtual bool        IsHome(void) = 0;
	virtual bool        IsUp(void) = 0;
	virtual bool        IsDown(void) = 0;
	virtual void        ClearErrors(std::function<void(bool)> callback) = 0;
	virtual void		GetMotorStatus (MotorStatus& reagentMotorStatus) = 0;
	virtual void        ArmPurge(std::function<void(bool)> callback) = 0;
	virtual void        ArmHome(std::function<void(bool)> callback) = 0;
	virtual void        ArmUp(std::function<void(bool)> callback) = 0;
	virtual void        ArmDown(std::function<void(bool)> callback) = 0;
	virtual void        UnlatchDoor (std::function<void(bool)> callback) = 0;

	enum class ReagentPackFluids :uint16_t
	{
		eCleaner1 = 0,
		eCleaner2,
		eCleaner3,
		eReagent1,
		eNone
	};

	enum class ReagentPosition : uint8_t
	{
		eMainBay = 0, // "0x00 - Square bottle - Main bay" - Multi Fluid/Reagent 1
		eDoorRightBay,// "0x01 - Round bottle - Door Right bay" - Reagent 2
		eDoorLeftBay, // "0x02 - Square bottle - Door left bay" - Reagent 1
		eInvalid

		// Information from the RFID Layout Proposal.xlsx
		/*Container Types
		0x00: Multi - fluid
		0x01 : Single fluid Round
		0x02 : Single - fluid square
		0x03..0x0F: undefined*/
	};

	static ReagentPackFluids convertToReagentFluid(const SyringePumpPort::Port& port);
	static ReagentPackFluids convertToReagentFluid(const PhysicalPort& port);
	static SyringePumpPort::Port convertToSyringePort(const ReagentPackFluids& reagentPackFluid);
	static uint16_t ConvertFluidVolumetoUsageCount(SyringePumpPort::Port port, uint32_t volume, bool calculateremaininguses = false);
	
	static ReagentPackFluids GetReagentPosition(const RfidTag_t& rfidTag, uint8_t index, bool checkClearner);
	static ReagentPosition GetReagentContainerPosition(const RfidTag_t& rfidTag);
	static instrument_error::reagent_pack_instance getReagentPackInstance(const RfidTag_t& rfidTag);

	void isFluidAvailable (std::function<void(bool)> callback, const SyringePumpPort::Port& port, uint32_t volume);

	virtual bool isRfidSim();
	virtual void scanRfidTags (std::function<void(bool)> callback);
	virtual void readRfidTags (std::function<void(std::shared_ptr<std::vector<RfidTag_t>>)> callback);
	virtual void setValveMap (std::function<void(bool)> callback, ReagentValveMap_t valveMap);
	
	static bool IsReagent(const ReagentControllerBase::ReagentPackFluids& fluid) { return ReagentControllerBase::ReagentPackFluids::eReagent1 == fluid;	}

protected:

	virtual bool isFluidAvailableInternal (std::shared_ptr<std::vector<RfidTag_t>> rfidTags, const SyringePumpPort::Port& port, uint32_t volume);

	std::shared_ptr<CBOService> pCBOService_;
	std::shared_ptr<RfidBase> pRfid_;
	MotorStatusDLL reagentMotorStatus_;
};
