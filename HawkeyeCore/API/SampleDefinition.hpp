#pragma once

#include <cstdint>

#include "SampleParameters.hpp"
#include "uuid__t.hpp"

/// This structure represents a single "sample definition" in the system.
/// All members with the name "____Index" are indices into the appropriate list of items
///   (ex: "celltypeIndex" is an index identifying a particular item in the system-wide list of 
///    cell types.)
/// A sample may select 1 or 2 reagents which will be mixed at a ratio of 1:1 (or (0.5 + 0.5):1 if two reagents are selected).
/// A sample may select 0 or more fluorescent illumination sources (Hunter).  The Brightfield source will always be used.
/// Within a Worklist, the following must be true at the time it is submitted for processing:
///   - All Indices must be valid (Valid: within range of the system-wide list AND 
///      where applicable, valid for the currently-logged-in user
///      (ex: If WQI.celltypeIndex is '4', then '4' must be a valid index into the system-wide celltype list
///       AND the logged-in-user must have '4' in there list of allowed celltypes).

enum ePrecession : uint16_t
{
	eRowMajor = 0,
	eColumnMajor,
};

enum eSampleStatus : uint16_t
{
	eNotProcessed = 0,
	eInProcess_Aspirating,	// several states of "in process"
	eInProcess_Mixing,
	eInProcess_ImageAcquisition,
	eInProcess_Cleaning,
	eAcquisition_Complete,
	eCompleted,
	eSkip_Manual,			// For samples not processed when the carousel is stopped or tube not found in defined location
	eSkip_Error,			// For samples not processed when there was a processing error
	eSkip_NotProcessed,		// For samples that have not been processed when a SampleSet is cancelled.
};

struct SampleDefinition
{
	uuid__t  sampleSetUuid;
	uuid__t  sampleDefUuid;
	uuid__t  sampleDataUuid;
	uint16_t index;				// Data structure positional info for UI to be able to update the correct sample.
	uint16_t sampleSetIndex;	// Data structure positional info for UI to be able to update the correct sample.
	char*    username;
	uint64_t timestamp;
	SamplePosition   position;
	eSampleStatus    status;
	SampleParameters parameters;

	void clear()
	{
		if (username) delete[] username;
		parameters.clear();
	}
};
