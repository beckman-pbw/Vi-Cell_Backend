#pragma once

#include <cstdint>

// These are *general* errors that the HawkeyeLogic APIs return.
// When the UI gets anything other than *eSuccess*, the UI must not allow use of the system to continue.
// ReportSystemError class is used to report a more detailed error to the UI.

/* General Intent of each HawkeyeError value
   Individual APIs may add a different "meaning" to particular error values because
    * Similar but distinct failure modes exist on a call
	* Reuse of an existing value for a very unique failure circumstance to avoid adding a new value

| HawkeyeError              | Meaning 
|---------------------------|---------------------------
| `eSuccess`                | Action / Request succeeded 
| `eInvalidArgs`            | Provided argument(s) are invalid 
| `eNotPermittedByUser`     | Current user lacks correct permissions / no user logged in 
| `eNotPermittedAtThisTime` | Call is correct, but system is not currently able to honor the request(try again later) 
| `eAlreadyExists`          | Request would have created a duplicate entry 
| `eIdle`                   | System is not presently executing an activity 
| `eBusy`                   | Request cannot be honored because system is currently executing a related task / already running that sequence 
| `eTimedout`               | Unable to execute request in the allowed time 
| `eHardwareFault`          | Request cannot be honored because the system has a hardware fault / experienced a hardware fault while executing the request 
| `eSoftwareFault`          | Request cannot be honored because the system has a software fault / experienced a software fault while executing the request 
| `eValidationFailed`       | An item / object / resource required for the request failed validity checks 
| `eEntryNotFound`          | A specific resource referenced by the request was not found(failure: resource was expected to exist) 
| `eNotSupported`           | Feature not yet implemented / Feature unsupported on this hardware 
| `eNoneFound`              | The requested search returned no matching items(NOT a failure condition) 
| `eEntryInvalid`           | Resource / item failed an integrity check(similar to `eValidationFailed` 
| `eStorageFault            | Software experienced a disk fault.
| `eStageNotRegistered`     | Request cannot be honored because plate,carousel is not registered, demands motor registration.
| `ePlateNotFound`          | Request cannot be honored because plate not found.
| `eCarouselNotFound`       | Request cannot be honored because carousel not found.
| `eLowDiskSpace`           | Request cannot be honored because system running out of memory.
| `eReagentError`           | Request cannot be honored because reagent pack is expired/invalid/empty.
| `eSpentTubeTray`          | Request cannot be honored because spent tube tray is full.
*/

enum class HawkeyeError
{
	eSuccess = 0,
	eInvalidArgs,
	eNotPermittedByUser,
	eNotPermittedAtThisTime,
	eAlreadyExists,
	eIdle,
	eBusy,
	eTimedout,
	eHardwareFault,
	eSoftwareFault,
	eValidationFailed,
	eEntryNotFound,
	eNotSupported,
	eNoneFound,
	eEntryInvalid,
	eStorageFault,
	eStageNotRegistered,
	ePlateNotFound,
	eCarouselNotFound,
	eLowDiskSpace,
	eReagentError,
	eSpentTubeTray,
	eDatabaseError,
	eDeprecated,
};

const char* HawkeyeErrorAsString (HawkeyeError he);
