#pragma once

#include "AutofocusCommon.hpp"
#include "ImageWrapperCommon.hpp"

typedef struct AutofocusResults
{
	bool focus_successful;

	uint32_t nFocusDatapoints;
	AutofocusDatapoint* dataset;

	int32_t bestfocus_af_position;      // Position identified as that of sharpest focus (motor position)
	int32_t offset_from_bestfocus_um;   // Configured offset (in microns) from sharpest focus
	int32_t final_af_position;          // Final position of autofocus after offset (motor position);

	ImageWrapper_t bestfocus_af_image;  // Final image at the "bestfocus_af_position" position (motor position);
} AutofocusResults;