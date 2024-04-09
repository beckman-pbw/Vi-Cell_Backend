#include "stdafx.h"

#include "Logger.hpp"
#include "Registers.hpp"
#include "SyringePumpSim.hpp"

static const char MODULENAME[] = "SyringePumpSim";

const int kVALVE_POS_MOVE_TIME = 50;
const int kRAND_TIME = 25;

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

	// Fine tune the delay value by adjusting it by a percentage
#ifdef _DEBUG
	delayT /= 4;
#else
	delayT *= 7;
	delayT /= 10;
#endif

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
SyringePumpSim::SyringePumpSim (std::shared_ptr<CBOService> pCBOService)
	: SyringePumpBase (pCBOService)
{
	srand(static_cast<unsigned int>(time(NULL)));
}

//*****************************************************************************
SyringePumpSim::~SyringePumpSim()
{
}

//*****************************************************************************
void SyringePumpSim::setPosition(std::function<void(bool)> callback, uint32_t target_volume_uL, uint32_t speed)
{
	HAWKEYE_ASSERT(MODULENAME, callback);

	// Compute the timeout for the syringe move based on the volume of fluid being moved in or out.
	uint32_t volume_being_moved;
	if (target_volume_uL > cur_volume_uL_) {
		// drawing fluid in.
		volume_being_moved = target_volume_uL - cur_volume_uL_;

	}
	else {
		// pushing fluid out.
		volume_being_moved = cur_volume_uL_ - target_volume_uL;
	}

	Logger::L().Log(MODULENAME, severity_level::debug1,
		boost::str(boost::format("setPosition: moving to %dul at %dul/sec, current volume: %d, volume being moved: %d")
			% target_volume_uL
			% speed
			% cur_volume_uL_
			% volume_being_moved
		));

	uint32_t delayT = 10;
	if ((speed > 0) && (volume_being_moved > 0))
	{
		delayT = ((1000 * volume_being_moved) / speed);
	}
	Delay(delayT, kRAND_TIME);

	cur_volume_uL_ = target_volume_uL;

	UpdateReagentVolume (curPhysicalPort_, volume_being_moved);

	pCBOService_->enqueueExternal(callback, true);
}

//*****************************************************************************
void SyringePumpSim::setValve(std::function<void(bool)> callback, SyringePumpPort port, SyringePumpDirection direction)
{
	HAWKEYE_ASSERT(MODULENAME, callback);

	Logger::L().Log(MODULENAME, severity_level::debug1, "setValve: set " + port.getAsString() + " valve " + direction.getAsString());

	int v1 = curPhysicalPort_;
	int v2 = SyringePumpPort::ToPhysicalPort(port.get());
	uint32_t delta = abs(v1 - v2);

	uint32_t delayT = (delta * kVALVE_POS_MOVE_TIME);
	Delay(delayT, kRAND_TIME);

	curPhysicalPort_ = SyringePumpPort::ToPhysicalPort(port.get());

	pCBOService_->enqueueInternal(callback, true);
}