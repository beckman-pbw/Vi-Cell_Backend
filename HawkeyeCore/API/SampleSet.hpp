#pragma once

#include <DBif_Api.h>

#include "SampleDefinition.hpp"
#include "StageDefines.hpp"

enum eFilterItem : uint16_t
{
	eSampleSet = 0,
	eSample,
};


struct SampleSet
{
	uuid__t uuid;
	uint16_t index;		// Data structure ositional info for UI to be able to update the correct sample.
	uint64_t timestamp;
	uint16_t numSamples;
	eCarrierType carrier;
	SampleDefinition* samples;
	char* name;
	char* username;
	DBApi::eSampleSetStatus status;
};
