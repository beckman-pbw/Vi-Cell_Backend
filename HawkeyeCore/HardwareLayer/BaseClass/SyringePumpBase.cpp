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
	: pCBOService_(std::move(pCBOService))
	, version_("unknown")
	, cur_volume_uL_(0)
{
	portMap_[SyringePumpPort::Waste]     = PhysicalPort::PortF;
	portMap_[SyringePumpPort::FlowCell]  = PhysicalPort::PortH;
	portMap_[SyringePumpPort::Reagent_1] = PhysicalPort::PortA;
	portMap_[SyringePumpPort::Cleaner_1] = PhysicalPort::PortB;
	portMap_[SyringePumpPort::Cleaner_2] = PhysicalPort::PortC;
	portMap_[SyringePumpPort::Cleaner_3] = PhysicalPort::PortD;

#ifdef CELLHEALTH_MODULE
	portMap_[SyringePumpPort::ACup]      = PhysicalPort::PortG;
	portMap_[SyringePumpPort::Diluent]   = PhysicalPort::PortE;
#endif
#ifdef VICELL_BLU
	portMap_[SyringePumpPort::Sample]    = PhysicalPort::PortG;
	portMap_[SyringePumpPort::Sample_2]  = PhysicalPort::PortE;	// Vi-Cell_BLU ACup
#endif
//TODO: is something miss from the above ???

	// Set default syringe pump valve position.
	curPhysicalPort_ = SyringePumpPort::ToPhysicalPort (SyringePumpPort::Waste);
}

//*****************************************************************************
void SyringePumpBase::initialize (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	pCBOService_->enqueueInternal (callback, true);
}

//*****************************************************************************
bool SyringePumpBase::getPosition (uint32_t& position)
{
	position = cur_volume_uL_;
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
	return SyringePumpPort::FromPhysicalPort (curPhysicalPort_);
}

//*****************************************************************************
bool SyringePumpBase::getPhysicalValve (PhysicalPort_t& valveNum)
{
	valveNum = curPhysicalPort_;
	return true;
}

//*****************************************************************************
void SyringePumpBase::setPhysicalValve (std::function<void(bool)> callback, char port, SyringePumpDirection direction)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	// Convert the given the port character to Physical valve number.
	curPhysicalPort_ = (PhysicalPort_t)(port - MinPhysicalValvePosition + 1);

	pCBOService_->enqueueInternal (callback, true);
}

//*****************************************************************************
void SyringePumpBase::rotateValve (std::function<void(bool)> callback, uint32_t angle, SyringePumpDirection direction)
{
	pCBOService_->enqueueInternal (callback, true);
}

//*****************************************************************************
void SyringePumpBase::sendValveCommand(std::function<void(bool)> callback, std::string valveCommand)
{
	pCBOService_->enqueueInternal(callback, true);
}

//*****************************************************************************
bool SyringePumpBase::isAspirating(uint32_t target_volume_uL)
{
	return target_volume_uL > cur_volume_uL_;
}

//*****************************************************************************
SyringePumpPort SyringePumpBase::paramToPort (uint32_t param) {

	switch (param) {
		case 1:
			return SyringePumpPort(SyringePumpPort::Waste);
		case 2:
#ifdef CELLHEALTH_MODULE
			return SyringePumpPort(SyringePumpPort::ACup);
#endif
#ifdef VICELL_BLU
			return SyringePumpPort(SyringePumpPort::Sample);
#endif
		case 3:
			return SyringePumpPort(SyringePumpPort::FlowCell);
		case 4:
			return SyringePumpPort(SyringePumpPort::Reagent_1);
		case 5:
			return SyringePumpPort(SyringePumpPort::Cleaner_1);
		case 6:
			return SyringePumpPort(SyringePumpPort::Cleaner_2);
		case 7:
			return SyringePumpPort(SyringePumpPort::Cleaner_3);
		case 8:
#ifdef CELLHEALTH_MODULE
			return SyringePumpPort(SyringePumpPort::Diluent);
#endif
#ifdef VICELL_BLU
			return SyringePumpPort(SyringePumpPort::Sample_2);
#endif
		default:
			Logger::L().Log (MODULENAME, severity_level::critical, "paramToPort: invalid port: " + std::to_string(param));
			return SyringePumpPort(SyringePumpPort::InvalidPort);
	}
}

//*****************************************************************************
SyringePumpDirection SyringePumpBase::paramToDirection (uint32_t param) {

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
std::string SyringePumpPort::getAsString() {
	switch (port_) {
		case Waste:
			return "Waste";
#ifdef CELLHEALTH_MODULE
		case ACup:
			return "ACup";
#endif
#ifdef VICELL_BLU
		case Sample:
			return "Sample";
#endif
		case FlowCell:
			return "FlowCell";
		case Reagent_1:
			return "Reagent_1";
		case Cleaner_1:
			return "Cleaner_1";
		case Cleaner_2:
			return "Cleaner_2";
		case Cleaner_3:
			return "Cleaner_3";
#ifdef CELLHEALTH_MODULE
		case Diluent:
			return "Diluent";
#endif
#ifdef VICELL_BLU
		case Sample_2:
			return "Sample_2";	//TODO: should this also return A-Cup ?
#endif
		default:
			return "InvalidPort";
	}
}

std::string SyringePumpDirection::getAsString() {
	switch (direction_) {
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

	return portMap_[port.get()];
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
