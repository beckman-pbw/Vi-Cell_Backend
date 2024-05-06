#include "stdafx.h"

#include "CellHealthReagents.hpp"
#include "Logger.hpp"
#include "Registers.hpp"
#include "SyringePumpBase.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "SyringePumpBase";

static std::map<SyringePumpPort::Port, PhysicalPort_t> portMap_;
char SyringePumpBase::MinPhysicalValvePosition = 'A';
char SyringePumpBase::MaxPhysicalValvePosition = 'H';

//*****************************************************************************
SyringePumpBase::SyringePumpBase (std::shared_ptr<CBOService> pCBOService)
	: _pCBOService(std::move(pCBOService))
	, _version("unknown")
	, _cur_volume_uL(0)
{
	portMap_[SyringePumpPort::Waste]       = PhysicalPort::PortF;
	portMap_[SyringePumpPort::FlowCell]    = PhysicalPort::PortH;
	portMap_[SyringePumpPort::Reagent_1]   = PhysicalPort::PortA;
	portMap_[SyringePumpPort::Cleaner_1]   = PhysicalPort::PortB;
	portMap_[SyringePumpPort::Cleaner_2]   = PhysicalPort::PortC;
	portMap_[SyringePumpPort::Cleaner_3]   = PhysicalPort::PortD;
	portMap_[SyringePumpPort::Sample]      = PhysicalPort::PortG;
	portMap_[SyringePumpPort::CHM_ACup]    = PhysicalPort::PortG;
	portMap_[SyringePumpPort::ViCell_ACup] = PhysicalPort::PortE;
	portMap_[SyringePumpPort::Diluent]     = PhysicalPort::PortE;

	// Set default syringe pump valve position.
	_curPhysicalPort = SyringePumpPort::ToPhysicalPort (SyringePumpPort::Waste);
}

//*****************************************************************************
void SyringePumpBase::initialize (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	_pCBOService->enqueueInternal (callback, true);
}

//*****************************************************************************
bool SyringePumpBase::getPosition (uint32_t& position)
{
	position = _cur_volume_uL;
	return true;
}

//*****************************************************************************
bool SyringePumpBase::getValve (SyringePumpPort& port)
{
	port = getValve();
	return true;
}

//*****************************************************************************
SyringePumpPort SyringePumpBase::getValve()
{
	return SyringePumpPort::FromPhysicalPort (_curPhysicalPort);
}

//*****************************************************************************
bool SyringePumpBase::getPhysicalValve (PhysicalPort_t& valveNum)
{
	valveNum = _curPhysicalPort;
	return true;
}

//*****************************************************************************
void SyringePumpBase::setPhysicalValve (std::function<void(bool)> callback, char port, SyringePumpDirection direction)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// Convert the given the port character to Physical valve number.
	_curPhysicalPort = (PhysicalPort_t)(port - MinPhysicalValvePosition + 1);

	_pCBOService->enqueueInternal (callback, true);
}

//*****************************************************************************
void SyringePumpBase::rotateValve (std::function<void(bool)> callback, uint32_t angle, SyringePumpDirection direction)
{
	_pCBOService->enqueueInternal (callback, true);
}

//*****************************************************************************
void SyringePumpBase::sendValveCommand(std::function<void(bool)> callback, std::string valveCommand)
{
	_pCBOService->enqueueInternal(callback, true);
}

//*****************************************************************************
bool SyringePumpBase::isAspirating(uint32_t target_volume_uL)
{
	return target_volume_uL > _cur_volume_uL;
}

//*****************************************************************************
SyringePumpDirection SyringePumpBase::ParamToDirection (uint32_t param) {

	switch (param) {
		case 0:
			return SyringePumpDirection::Clockwise;
		case 1:
			return SyringePumpDirection::CounterClockwise;
		default:
			return SyringePumpDirection::DirectionError;
	}
}

//*****************************************************************************
std::string SyringePumpBase::getVersion()
{
	return "000";
}

//*****************************************************************************
std::string SyringePumpDirection::ToString() {
	switch (_direction) {
		case Clockwise:
			return "Clockwise";
		case CounterClockwise:
			return "CounterClockwise";
		default:
			return "DirectionError";
	}
}

//*****************************************************************************
PhysicalPort_t SyringePumpPort::ToPhysicalPort (SyringePumpPort port) {

	return portMap_[port.Get()];
}

//*****************************************************************************
SyringePumpPort SyringePumpPort::FromPhysicalPort (PhysicalPort_t port) {

	for (auto v : portMap_) {
		if (v.second == port) {
			return v.first;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::critical, "fromPhysicalPort: invalid Physical port: " + std::to_string(port));
	assert (false);

	return SyringePumpPort::Waste;	// This is here to satisfy the compiler.
}

//*****************************************************************************
bool SyringePumpBase::FromPhysicalPortToChar (PhysicalPort_t physicalPort, char& physicalPortChar) {

	if (!isPhysicalPortValid(physicalPort))
		return false;

	physicalPortChar = physicalPort + 'A' - 1;
	return true;
}

//*****************************************************************************
bool SyringePumpBase::ToPhysicalPortFromChar (char physicalPortChar, PhysicalPort_t& physicalPort) {

	if (!isPhysicalPortValid(physicalPortChar))
		return false;

	physicalPort = static_cast<PhysicalPort_t>(physicalPortChar - 'A' + 1);
	return true;
}

//*****************************************************************************
bool SyringePumpBase::isPhysicalPortValid (char physicalPortChar)
{
	if (!(physicalPortChar >= SyringePumpBase::MinPhysicalValvePosition && physicalPortChar <= SyringePumpBase::MaxPhysicalValvePosition))
		return false;

	return true;
}

//*****************************************************************************
bool SyringePumpBase::isPhysicalPortValid (PhysicalPort_t physicalPort)
{
	if (!(physicalPort >= PhysicalPort::PortA && physicalPort <= PhysicalPort::PortH))
		return false;

	return true;
}

//*****************************************************************************
bool SyringePumpBase::isAspirationAllowed (PhysicalPort_t physicalPort)
{
	// Block the Aspiration if the valve selected is "Waste".
	if (physicalPort == PhysicalPort::PortF)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
bool SyringePumpBase::isDisPenseAllowed (PhysicalPort_t physicalPort)
{

	switch (physicalPort)
	{
#ifdef CELLHEALTH_MODULE
		case PhysicalPort::PortF:	// Waste
		case PhysicalPort::PortG:	// ACup
		case PhysicalPort::PortH:	// FlowCell
#endif
#ifdef VICELL_BLU
		case PhysicalPort::PortE:	//TODO: label these ...
		case PhysicalPort::PortF:
		case PhysicalPort::PortG:
		case PhysicalPort::PortH:
#endif
			return true;
		default:
			return false;
	}
}

//*****************************************************************************
bool SyringePumpBase::UpdateReagentVolume (PhysicalPort_t physicalPort, uint32_t volume)
{
	CellHealthReagents::FluidType fluidType;
	
	switch (physicalPort)
	{
		case PhysicalPort::PortA:	// Reagent_1
			fluidType = CellHealthReagents::FluidType::TrypanBlue;
			break;		
		case PhysicalPort::PortB:	// Cleaner_1
			fluidType = CellHealthReagents::FluidType::Cleaner;
			break;
		case PhysicalPort::PortC:	// Cleaner_2
			fluidType = CellHealthReagents::FluidType::ConditioningSolution;
			break;
		case PhysicalPort::PortD:	// Cleaner_3
			fluidType = CellHealthReagents::FluidType::Buffer;
			break;
		case PhysicalPort::PortE:	// Diluent
			fluidType = CellHealthReagents::FluidType::Diluent;
			break;
		default:
			return false;
	}

	CellHealthReagents::AspirateVolume (fluidType, volume);

	return true;
}
