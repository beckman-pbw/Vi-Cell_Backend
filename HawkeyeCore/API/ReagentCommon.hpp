#pragma once

#include <cstdint>

/*
* ReagentContainerStatus
*
* Describes the state of a reagent in the system.
* "eOK" indicates that the reagent is
*  - Loaded
*  - Validated
*  - Not expired
*  - Has at least 1 remaining event
*/
enum ReagentContainerStatus : uint32_t
{
	eOK = 0,          // Reagent pack present, loaded, functional.
	eEmpty,           // Reagent Pack present but with no remaining events

//TODO: moved to ReagentLoadStatus, delete this one when the other is working...
	eNotDetected,     // Pack not detected
	eInvalid,         // Pack not recognized / failed validation
	eExpired,         // Pack expired (too long in-service or past shelf-life date)
	eFaulted,         // Pack is in a faulted state (fluidic / mechanical / other)
	eUnloading,       // Pack is being unloaded
	eUnloaded,        // Pack is in an unloaded state (piercing mechanism retracted & idle) - may not be present.
	eLoading,         // Pack is being loaded (piercing mechanism activated / pack recognition and validation in progress)	
}; 

enum ReagentContainerPosition : uint16_t
{
	eMainBay_1 = 0,
	eDoorLeft_2,
	eDoorRight_3,
	eUnknown,
};

enum ReagentUnloadOption : uint16_t
{
	eULNone = 0,              // No specific action
	eULDrainToWaste,          // Drain container contents to Waste 
	eULPurgeLinesToContainer, // Flush reagent lines back into container
};

enum ReagentUnloadSequence : uint16_t
{
	eULIdle = 0,
	eULDraining1,
	eULPurging1,
	eULDraining2,
	eULPurging2,
	eULDraining3,
	eULDraining4,
	eULDraining5,
	eULPurging3,
	eULPurgingReagent1,
	eULPurgingReagent2,
	eULRetractingProbes,
	eULUnlatchingDoor,    // If probes are retracted, jump ahead to this state.

	// Successful termination state
	eULComplete,

	// Failure termination state
	eULFailed_DrainPurge,		// Unable to continue.
	eULFailed_ProbeRetract,
	eULFailed_DoorUnlatch,
	eULFailure_StateMachineTimeout,
};

enum ReagentLoadSequence : uint16_t
{
	eLIdle = 0,
	eLWaitingForDoorLatch,
	eLWaitingForReagentSensor,
	eLIdentifyingReagentContainers,
	eLWaitingOnContainerLocation,   // If one or more single-fluid containers are discovered...
	eLInsertingProbes,
	eLSynchronizingReagentData,
	eLPrimingFluidLines,

	// Successful termination state
	eLComplete,

	// Failure termination state
	eLFailure_DoorLatchTimeout,
	eLFailure_ReagentSensorDetect,
	eLFailure_NoReagentsDetected,
	eLFailure_NoWasteDetected,
	eLFailure_ReagentInvalid,            // Failed validation
	eLFailure_ReagentEmpty,              // No remaining quantity
	eLFailure_ReagentExpired,            // Expiration date passed
	eLFailure_InvalidContainerLocations, // multi-fluid must be in 1, 
	eLFailure_ProbeInsert,
	eLFailure_Fluidics,
	eLFailure_StateMachineTimeout
};

struct ReagentContainerLocation
{
	uint8_t identifier[8];
	ReagentContainerPosition position;
};
