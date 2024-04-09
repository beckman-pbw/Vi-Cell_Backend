#pragma once

#include <cstdint>
#include <string>

#include "SetString.hpp"
#include "SamplePosition.hpp"

enum eSamplePostWash : uint16_t
{
	eNormalWash = 0,
	eFastWash,
};

struct SampleParameters
{
	SampleParameters& operator= (const SampleParameters& other)
	{
		if (this != &other)
		{
			SetString (&label, other.label);
			SetString (&tag, other.tag);
			SetString (&bp_qc_name, other.bp_qc_name);
			analysisIndex     = other.analysisIndex;
			celltypeIndex     = other.celltypeIndex;
			dilutionFactor    = other.dilutionFactor;
			postWash          = other.postWash;
			saveEveryNthImage = other.saveEveryNthImage;
		}

		return *this;
	}

	void clear()
	{
		if (label) delete[] label;
		if (tag) delete[] tag;
		if (bp_qc_name) delete[] bp_qc_name;
	}

	char*       label;
	char*       tag;
	char*       bp_qc_name;
	uint16_t    analysisIndex;
	uint32_t    celltypeIndex;
	uint32_t    dilutionFactor;
	eSamplePostWash postWash;
	uint32_t    saveEveryNthImage;
};
