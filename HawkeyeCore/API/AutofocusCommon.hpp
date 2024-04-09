#pragma once
#include <cstdint>


enum eAutofocusState : uint32_t
{
	af_WaitingForSample = 0,		// Moving to the supplied sample location
	af_PreparingSample,				// Adding reagents, mixing, measuring
	af_SampleToFlowCell,			// Dispensing sample into flow cell
	af_SampleSettlingDelay,			// Delay to allow sample to settle / stabilize in flow cell
	af_AcquiringFocusData,			// Moving motor through focal range, acquiring and analyzing image
	af_WaitingForFocusAcceptance,	// Focus results have been provided to the host; waiting for instructions
									//  Host can : Accept (will record new focus, move to af_FlushingSample
									//  Cancel(will retain previous focus, move to af_FlushingSample
									//  RETRY operations are not supported by UI
									//    Retry on current cells (will revert to af_AcquiringFocusData
									//    Retry on new cells (will revert to af_SampleToFlowCell)
	af_FlushingSample,				// Cleaning up (not dependent on success or failure)
	af_Cancelling,					// Operation is being cancelled either by user or unable to perform
	af_Idle,						// Completed successfully or cancelled
	af_Failed,
	af_Exiting,						// exit process started; to prevent duplicated process flow
	af_FocusAcceptance,				// Focus results have been accepted or rejected by the host; cleanup and exit workflow
	af_FindingTube,
	af_TfComplete,
	af_TfCancelled,
};

enum eAutofocusCompletion : uint16_t
{
	afc_Accept = 0,
	afc_Cancel,
	afc_RetryOnCurrentCells,
	/*afc_RetryOnNewCells,*/
};

typedef struct AutofocusDatapoint
{
	int32_t position;
	uint32_t focalvalue;
} AutofocusDatapoint;
