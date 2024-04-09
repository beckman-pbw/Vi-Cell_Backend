#include <stdint.h>
#include <string>

#include "HawkeyeError.hpp"

//*****************************************************************************
const char* HawkeyeErrorAsString(HawkeyeError he)
{

	switch ( he )
	{
		case HawkeyeError::eSuccess:
			return "Success";
		case HawkeyeError::eInvalidArgs:
			return "Invalid Arguments";
		case HawkeyeError::eNotPermittedByUser:
			return "NotPermittedByUser";
		case HawkeyeError::eNotPermittedAtThisTime:
			return "NotPermittedAtThisTime";
		case HawkeyeError::eAlreadyExists:
			return "AlreadyExists";
		case HawkeyeError::eIdle:
			return "Idle";
		case HawkeyeError::eBusy:
			return "Busy";
		case HawkeyeError::eHardwareFault:
			return "HardwareFault";
		case HawkeyeError::eSoftwareFault:
			return "SoftwareFault";
		case HawkeyeError::eValidationFailed:
			return "ValidationFailed";
		case HawkeyeError::eEntryNotFound:
			return "EntryNotFound";
		case HawkeyeError::eNotSupported:
			return "NotSupported";
		case HawkeyeError::eNoneFound:
			return "NoneFound";
		case HawkeyeError::eEntryInvalid:
			return "EntryInvalid";
		case HawkeyeError::eStorageFault:
			return "Storage fault";
		case HawkeyeError::eStageNotRegistered:
			return "Sample carrier registration not performed";
		case HawkeyeError::ePlateNotFound:
			return "Plate not found";
		case HawkeyeError::eCarouselNotFound:
			return "Carousel not found";
		case HawkeyeError::eLowDiskSpace:
			return "Disk Storage Space Running Low";
		case HawkeyeError::eReagentError:
			return "Reagent error";
		case HawkeyeError::eSpentTubeTray:
			return "Waste tube tray full";
		default:
			return "Unknown Error";
	}
}
