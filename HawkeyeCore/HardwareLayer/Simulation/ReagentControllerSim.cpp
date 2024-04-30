#include "stdafx.h"
#include "CellHealthReagents.hpp"
#include "HawkeyeConfig.hpp"
#include "ReagentControllerSim.hpp"

static const char MODULENAME[] = "ReagentControllerSim";

const int kMOVE_TIME = 750;
const int kUNLATCH_TIME = 125;

/**
 ***************************************************************************
 * \brief Block for a minimum amount of time and optionally add a random amount
 * \param min_ms - minimum number of milli-seconds to block for
 * \param rand_ms - a uniformly random amount of time up to this value is added to the delay
 */
static void Delay(uint32_t min_ms, uint32_t rand_ms)
{
	const int kSLEEP_TIME = 10;

	auto delayT = min_ms;
	if (delayT < kSLEEP_TIME)
		delayT = kSLEEP_TIME;
	if (rand_ms > 0)
		delayT += (rand() % rand_ms);

	// We want this to be a busy wait
	// this code in intentionally ineffecient 
	while (delayT > 0)
	{
		Sleep(kSLEEP_TIME);
		if (delayT <= kSLEEP_TIME)
			delayT = 0;
		else
			delayT -= kSLEEP_TIME;
	}
}


//*****************************************************************************
ReagentControllerSim::ReagentControllerSim (std::shared_ptr<CBOService> pCBOService)
	: ReagentControllerBase(pCBOService)
{
	armHomePos_ = 0;
	armHomeOffset_ = 0;
	armPurgePosition_ = ReagentArmPurgePosition;
	armPosition_ = armPurgePosition_ / 3; // Not in the Up nor the Down positions
	isDoorClosed_ = false;
	isPackInstalled_ = false;

	pRfid_ = std::make_shared<RfidSim>(pCBOService);

	srand(static_cast<unsigned int>(time(NULL)));
}

//*****************************************************************************
ReagentControllerSim::~ReagentControllerSim()
{
	Reset();
}

//*****************************************************************************
void ReagentControllerSim::Initialize (std::function<void(bool)> callback, std::string cfgFile, bool apply)
{
	reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfPoweredOn, -1, ePositionDescription::Unknown);
		
	armPosition_ = ReagentArmMaxTravel / 3; // Not in the Up nor the Down positions
	isDoorClosed_ = true;
	isPackInstalled_ = true;	

	reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfConfigured, -1, ePositionDescription::Unknown);
	
	pCBOService_->enqueueInternal (callback, true);
}

//*****************************************************************************
void ReagentControllerSim::Reset()
{
	reagentMotorStatus_.reset();
}

void ReagentControllerSim::ClearErrors(std::function<void(bool)> callback)
{
	pCBOService_->enqueueInternal(callback, true);
}

bool ReagentControllerSim::ControllerOk()
{
	return true;
}

void ReagentControllerSim::Stop()
{
	reagentMotorStatus_.UpdateMotorHealth(eMotorFlags::mfPositionKnown, armPosition_, ePositionDescription::Current);
}

bool ReagentControllerSim::IsDoorClosed()
{
	return isDoorClosed_;
}

bool ReagentControllerSim::IsPackInstalled()
{
	return isPackInstalled_;
}

bool ReagentControllerSim::IsHome()
{
	return armPosition_ == armHomePos_;
}

bool ReagentControllerSim::IsUp()
{
	return IsHome();
}

bool ReagentControllerSim::IsDown()
{
	return armPosition_ == ReagentArmMaxTravel;
}

void ReagentControllerSim::GetMotorStatus(MotorStatus & reagentMotorStatus)
{
	reagentMotorStatus_.ToCStyle(reagentMotorStatus);
}

void ReagentControllerSim::ArmPurge (std::function<void(bool)> callback)
{
	bool moveOk = false;
	if (IsDoorClosed())
	{
		bool downOk = IsDown();
		if (!downOk)
		{
			MoveToLimit(true);
			downOk = IsDown();
		}

		if (downOk)
		{
			int32_t startPos = armPosition_;
			int32_t tgtPos = startPos + armPurgePosition_;
			int32_t endPos = MoveToPosition(armPosition_ + armPurgePosition_);	//StepArmPosition(armPurgePosition);
			moveOk = tgtPos == endPos;
		}
	}
	
	pCBOService_->enqueueExternal ([=]() -> void
	{
		if (callback)
		{
			callback(moveOk);
		}
	});
}

void ReagentControllerSim::ArmHome (std::function<void(bool)> callback)
{
	ArmUp(callback);
}

void ReagentControllerSim::ArmUp (std::function<void(bool)> callback)
{
	bool upOk = false;
	if (IsDoorClosed())
	{
		upOk = IsUp();
		if (!upOk)
		{
			MoveToLimit(false);
			upOk = IsUp();
		}
	}

	pCBOService_->enqueueExternal ([=]() -> void
	{
		if (callback)
		{
			callback(upOk);
		}
	});
}

void ReagentControllerSim::ArmDown (std::function<void(bool)> callback)
{
	bool downOk = false;
	if (IsDoorClosed())
	{
		downOk = IsDown();
		if (!downOk)
		{
			MoveToLimit(true);
			downOk = IsDown();
		}
	}
	
	pCBOService_->enqueueExternal ([=]() -> void
	{
		if (callback)
		{
			callback(downOk);
		}
	});
}

void ReagentControllerSim::UnlatchDoor (std::function<void(bool)> callback)
{
	bool upOk = IsUp();
	if (upOk)
	{
		// Unlatch Door
		Delay(kUNLATCH_TIME, kUNLATCH_TIME);
	}
	
	pCBOService_->enqueueExternal ([=]() -> void
	{
		if (callback)
		{
			callback(upOk);
		}
	});
}

int32_t ReagentControllerSim::MoveToPosition (int32_t target)
{
	Delay(kMOVE_TIME, kMOVE_TIME);
	reagentMotorStatus_.UpdateMotorHealth (eMotorFlags::mfAtPosition, target, ePositionDescription::AtPosition);
	armPosition_ = target;
	return armPosition_;
}

bool ReagentControllerSim::MoveToLimit (bool downLimit)
{
	if (downLimit)
	{
		int32_t downPos = ReagentArmMaxTravel;
		MoveToPosition(downPos);
	}
	else
	{
		// StepArmPosition(armHomeOffset);                   // must use StepArmPosition directly to avoid home check logic
		int32_t homePos = armHomePos_ + armHomeOffset_;
		MoveToPosition(homePos);
	}
	return true;
}

CellHealthReagents::FluidType ConvertToCHReagentsFluid(ReagentControllerBase::ReagentPackFluids fluid)
{
	switch (fluid)
	{
		case ReagentControllerBase::ReagentPackFluids::eCleaner1: return CellHealthReagents::FluidType::Cleaner;
		case ReagentControllerBase::ReagentPackFluids::eCleaner2: return CellHealthReagents::FluidType::ConditioningSolution;
		case ReagentControllerBase::ReagentPackFluids::eCleaner3: return CellHealthReagents::FluidType::Buffer;
		case ReagentControllerBase::ReagentPackFluids::eReagent1: return CellHealthReagents::FluidType::TrypanBlue;
		case ReagentControllerBase::ReagentPackFluids::eNone:
		default:
			return CellHealthReagents::FluidType::Unknown;
	}
}

//*****************************************************************************
bool ReagentControllerSim::isFluidAvailableInternal(std::shared_ptr<std::vector<RfidTag_t>> rfidTags, const SyringePumpPort::Port& port, uint32_t volume)
{
	if (HawkeyeConfig::Instance().get().instrumentType != HawkeyeConfig::CellHealth_ScienceModule)
	{
		return ReagentControllerBase::isFluidAvailableInternal(rfidTags, port, volume);
	}

	// The Cell Health Module wants to think in pure volume, not in activiites.  The reagent Controller is simulated in this device
	// and we need to avoid the unit conversion to "activities" otherwise we run into trouble at low volumes
	
	Logger::L().Log(MODULENAME, severity_level::debug1, "isFluidAvailableInternal:: port: " +
		std::to_string(port) + ", volume: " + std::to_string(volume) + ">");

	ReagentPackFluids fluid = convertToReagentFluid(port);

	if (fluid == ReagentPackFluids::eNone || volume == 0)
	{
		// If Fluid availability is checked for "Sample", "Waste" or "FlowCell" return true, to avoid throwing an system error because of non availibilty of the fluids, which are not supposed to be checked.
		Logger::L().Log(MODULENAME, severity_level::debug1, "isFluidAvailableInternal: <exit, no volume tracked>");
		return true;
	}

	auto fl2 = ConvertToCHReagentsFluid(fluid);

	int remainingVolume = 0;	// This "may" be < 0.
	if (CellHealthReagents::GetVolume(fl2, remainingVolume) != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "isFluidAvailableInternal- <Exit - Fluid is not available - Unable to retrieve volume> ");
		return false;
	}

	std::string outs = boost::str(boost::format("Port %d / Fluid %d: %d uL required; %d uL remaining") % port % (uint16_t) fluid % volume % remainingVolume);
	Logger::L().Log(MODULENAME, severity_level::debug1, outs);

	return remainingVolume >= static_cast<uint32_t>(volume);
}
