#pragma once

#include <cstdint>

#include "SampleDefinition.hpp"
#include "SampleSetDLL.hpp"
#include "uuid__t.hpp"

//TODO: create API to free this data...

struct Worklist
{	
	uuid__t uuid;
	char* username;
	char* runUsername;
	char* label;
	uint64_t timestamp;
	eCarrierType carrier;
	ePrecession precession; // Only applies to plate processing...
	SampleParameters defaultParameterSettings;

	uint16_t numSampleSets;
	SampleSet* sampleSets;

	bool        useSequencing;
	bool        sequencingTextFirst;
	char*       sequencingBaseLabel;
	uint16_t    sequencingStartingDigit;
	uint16_t    sequencingNumberOfDigits;
};
