#include "stdafx.h"

#include "Logger.hpp"
#include "ReagentCommon.hpp"
#include "ReagentControllerBase.hpp"

static const char MODULENAME[] = "ReagentControllerBase";

// TODO Can this be moved to Configuration file?
#define VOLUME_FOR_ONE_USAGE_OF_REAGENT_IN_UL 150.0
#define VOLUME_FOR_ONE_USAGE_OF_CLEANSER_IN_UL 600.0

ReagentControllerBase::ReagentControllerBase (std::shared_ptr<CBOService> pCBOService)
	: pCBOService_(std::move(pCBOService))
{
}

ReagentControllerBase::~ReagentControllerBase() {
	pRfid_.reset();
}

//*****************************************************************************
// "calculateremaininguses" will tell you how many complete uses can be done with this volume (floor)
// Otherwise it'll tell you how many complete uses this volume will TAKE (ceiling)
 uint16_t ReagentControllerBase::ConvertFluidVolumetoUsageCount (SyringePumpPort::Port port, uint32_t volume, bool calculateremaininguses)
{
	double usage_count;

	// Reagents count: 150 ul ="1" count
	if (port == SyringePumpPort::Reagent_1)
	{
		usage_count = volume / VOLUME_FOR_ONE_USAGE_OF_REAGENT_IN_UL;
	}
	else // Cleanser count : 600 ul = "1" count
	{
		usage_count = volume / VOLUME_FOR_ONE_USAGE_OF_CLEANSER_IN_UL;
	}

	// If the usage is less than one Usage Count/More than one usage count( fraction values ), then Round up to nearest higher value.
	// Example 1.25 usage count ----> will be 2 usage count.
	// NOTE: This case is considered if any fluid is not being used in multiples of "ONE_USAGE"

	// If we're using this function to calculate remaining uses, partial uses don't count, so we round DOWN.
	// If we're using this function to calcualte how many uses a volume will consume, we round UP (partial uses count as a whole)
	return calculateremaininguses ? (uint16_t)std::floor(usage_count) : (uint32_t)std::ceil(usage_count); 
}


//*****************************************************************************
ReagentControllerBase::ReagentPackFluids ReagentControllerBase::convertToReagentFluid (const SyringePumpPort::Port& port)
{
	switch (port)
	{
		case SyringePumpPort::Port::Cleaner_1: // Cleaning Agent
			return ReagentPackFluids::eCleaner1;
		case SyringePumpPort::Port::Cleaner_2: // Cleaner_2
			return ReagentPackFluids::eCleaner2;
		case SyringePumpPort::Port::Cleaner_3: // Cleaner_3
			return ReagentPackFluids::eCleaner3;
		case SyringePumpPort::Port::Reagent_1: // reagent 1
			return ReagentPackFluids::eReagent1;
		default:
			return ReagentPackFluids::eNone;
	}
}

//*****************************************************************************
ReagentControllerBase::ReagentPackFluids ReagentControllerBase::convertToReagentFluid(const PhysicalPort& port)
{
	return convertToReagentFluid(SyringePumpPort::FromPhysicalPort(port).get());
}

//*****************************************************************************
SyringePumpPort::Port ReagentControllerBase::convertToSyringePort(const ReagentPackFluids& reagentPackFluid)
{
	switch (reagentPackFluid)
	{
		case ReagentControllerBase::ReagentPackFluids::eCleaner1:	// Cleaning Agent
			return SyringePumpPort::Port::Cleaner_1;
		case ReagentControllerBase::ReagentPackFluids::eCleaner2:	// Cleaner_2
			return SyringePumpPort::Port::Cleaner_2;
		case ReagentControllerBase::ReagentPackFluids::eCleaner3:	// Cleaner_3
			return SyringePumpPort::Port::Cleaner_3;
		case ReagentControllerBase::ReagentPackFluids::eReagent1:	// Reagent 1
			return SyringePumpPort::Port::Reagent_1;
		default:
			return SyringePumpPort::Port::InvalidPort;
	}
}

//*****************************************************************************
ReagentControllerBase::ReagentPackFluids ReagentControllerBase::GetReagentPosition(
	const RfidTag_t& rfidTag, uint8_t index, bool checkClearner)
{
	// container_cleaners
	if (checkClearner)
	{
		switch (index)
		{
			case 0:
				return ReagentPackFluids::eCleaner1;
			case 1:
				return ReagentPackFluids::eCleaner2;
			case 2:
				return ReagentPackFluids::eCleaner3;
			default:
				return ReagentPackFluids::eNone;
		}
	}
	else
	{
//TODO: note, this code is duplicated in the method below (GetReagentContainerPosition).
		uint8_t reagent_location = rfidTag.numCleaners >> 4;
		switch (reagent_location)
		{
			case ReagentContainerPosition::eMainBay_1:
			case ReagentContainerPosition::eDoorLeft_2:
				return ReagentPackFluids::eReagent1;
			case ReagentContainerPosition::eDoorRight_3:
			default:
				return ReagentPackFluids::eNone;
		}
	}
	return ReagentPackFluids::eNone;
}

//*****************************************************************************
ReagentControllerBase::ReagentPosition ReagentControllerBase::GetReagentContainerPosition(const RfidTag_t& rfidTag)
{
	// Higher nibble gives the reagent Container format 
	uint8_t reagent_location = rfidTag.numCleaners >> 4;
	// Information from the RFID Layout Proposal.xlsx
	/*Container Types
	0x00: Multi - fluid - eMainBay_1
	0x01 : Single fluid Round - eDoorRight_3
	0x02 : Single - fluid square --eDoorLeft_2
	0x03..0x0F: undefined*/
	switch (reagent_location)
	{
		case ReagentContainerPosition::eMainBay_1:
			return ReagentControllerBase::ReagentPosition::eMainBay;
		case ReagentContainerPosition::eDoorRight_3:
			return ReagentControllerBase::ReagentPosition::eDoorRightBay;
		case ReagentContainerPosition::eDoorLeft_2:
			return ReagentControllerBase::ReagentPosition::eDoorLeftBay;
		default:
			return ReagentControllerBase::ReagentPosition::eInvalid;
	}
}

//*****************************************************************************
instrument_error::reagent_pack_instance ReagentControllerBase::getReagentPackInstance(const RfidTag_t& rfidTag)
{
	auto position = ReagentControllerBase::GetReagentContainerPosition(rfidTag);
	switch (position)
	{
		case ReagentControllerBase::ReagentPosition::eMainBay:
			return instrument_error::reagent_pack_instance::main_bay;
		case ReagentControllerBase::ReagentPosition::eDoorRightBay:
			return instrument_error::reagent_pack_instance::door_right;
		case ReagentControllerBase::ReagentPosition::eDoorLeftBay:
			return instrument_error::reagent_pack_instance::door_left;
		default:
			return instrument_error::reagent_pack_instance::general;
	}
}

//*****************************************************************************
bool ReagentControllerBase::isRfidSim()
{
	return pRfid_->IsSim();
}

//*****************************************************************************
// TODO: Rewrite this function, to calculate the fluid remaining based on the Fluid Index or Fluid label.
//*****************************************************************************
void ReagentControllerBase::isFluidAvailable (std::function<void(bool)> callback, const SyringePumpPort::Port& port, uint32_t volume)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	readRfidTags ([this, callback, port, volume](std::shared_ptr<std::vector<RfidTag_t>> rfidTags)
	{
		pCBOService_->enqueueExternal (callback, isFluidAvailableInternal(rfidTags, port, volume));
	});
}

//*****************************************************************************
void ReagentControllerBase::scanRfidTags (std::function<void(bool)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	pCBOService_->enqueueInternal ([this, callback]() -> void {
		pRfid_->scan (callback);
	});
}

//*****************************************************************************
void ReagentControllerBase::readRfidTags (std::function<void(std::shared_ptr<std::vector<RfidTag_t>>)> callback)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	auto onReadTagsComplete = [this, callback](bool status, std::shared_ptr<std::vector<RfidTag_t>> rfidTags) -> void
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "readRFIDTags: <exit, failed to read tags>");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::reagent_rfid_rfiderror, 
				instrument_error::severity_level::error));
		}
		pCBOService_->enqueueExternal (callback, rfidTags);
	};

	pCBOService_->enqueueInternal([this, onReadTagsComplete]() -> void {
		pRfid_->read (onReadTagsComplete);
	});
}

//*****************************************************************************
void ReagentControllerBase::setValveMap (std::function<void(bool)> callback, ReagentValveMap_t valveMap)
{
	HAWKEYE_ASSERT (MODULENAME, callback);

	pCBOService_->getInternalIosRef().post ([this, callback, valveMap]() -> void {
		pRfid_->setValveMap (callback, valveMap);
	});
}

//*****************************************************************************
bool ReagentControllerBase::isFluidAvailableInternal (std::shared_ptr<std::vector<RfidTag_t>> rfidTags, const SyringePumpPort::Port & port, uint32_t volume)
{
	// Convert the Volume into Count
	uint32_t quantity = ConvertFluidVolumetoUsageCount (port, volume);

	Logger::L().Log (MODULENAME, severity_level::normal, "isFluidAvailableInternal:: port: " +
		std::to_string(port) + ", quantity: " + std::to_string(quantity) + ">");

	ReagentPackFluids fluid = convertToReagentFluid(port);
	if (fluid == ReagentPackFluids::eNone || volume == 0)
	{
		// If Fluid availability is checked for "Sample", "Waste" or "FlowCell" return true, to avoid throwing an system error because of non availibilty of the fluids, which are not supposed to be checked.
		Logger::L().Log (MODULENAME, severity_level::debug1, "isFluidAvailableInternal: <exit, no volume tracked>");
		return true;
	}

	bool isReagent = IsReagent(fluid);

	for (auto rfidTag : *rfidTags)
	{
		{
			std::string outs = "RFID Tag Uses:\n";
			if (rfidTag.hasReagent) outs += boost::str(boost::format("\tReagent: %d uses\n") % rfidTag.reagent.remainingUses);

			for (uint8_t i = 0; i < (rfidTag.numCleaners & 0x0F); i++)
			{
				outs += boost::str(boost::format("\tCleaner %d: %d uses\n") % (i+1) %rfidTag.cleaners[i].remainingUses);
			}
			Logger::L().Log(MODULENAME, severity_level::debug1, outs);
		}
		if (isReagent)
		{ // Check for reagent.

			uint8_t reagent_location = rfidTag.numCleaners >> 4;     // Higher nibble gives the reagent container format  .

			if (rfidTag.hasReagent)
			{
				if ((fluid == ReagentPackFluids::eReagent1 &&
					(reagent_location == (uint8_t)ReagentPosition::eMainBay || reagent_location == (uint8_t)ReagentPosition::eDoorLeftBay)))
				{
					if (rfidTag.reagent.remainingUses >= quantity)
					{
						Logger::L().Log (MODULENAME, severity_level::debug1, "isFluidAvailableInternal: <exit, available reagent uses: "
										+ std::to_string(rfidTag.reagent.remainingUses)
										+ " location : " + std::to_string(reagent_location) + ">");

						return true;
					}

					Logger::L().Log (MODULENAME, severity_level::error, "isFluidAvailableInternal: <exit, reagent is not available>");
					return false;
				}
			}

		}
		else
		{ // Check for cleaner.

			uint8_t cleanser_count = rfidTag.numCleaners & 0xF;  // lower nibble gives the cleaners information

			if ((uint8_t)fluid < cleanser_count)
			{
				if (rfidTag.cleaners[(uint8_t)fluid].remainingUses >= quantity)
				{
					Logger::L().Log (MODULENAME, severity_level::debug1, "isFluidAvailableInternal- <Exit - Available cleaner uses> : "
									+ std::to_string(rfidTag.cleaners[(uint8_t)fluid].remainingUses));

					return true;
				}

				Logger::L().Log (MODULENAME, severity_level::debug1, "isFluidAvailableInternal- <Exit - Cleaner not available> :");
				return false;
			}
		} // End "if (isReagent)"

	} // End "for (const auto& rf : rfidTags)"

	Logger::L().Log (MODULENAME, severity_level::error, "isFluidAvailableInternal- <Exit - Fluid is not available> ");

	return false;
}
