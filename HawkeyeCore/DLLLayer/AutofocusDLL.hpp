#pragma once

#include "Autofocus.hpp"
#include "ImageWrapperDLL.hpp"

typedef struct AutofocusResultsDLL
{
	AutofocusResultsDLL() {	}

	AutofocusResults ToCStyle()
	{
		AutofocusResults result = {};

		result.focus_successful = focus_successful;

		DataConversion::create_Clist_from_vector(
			dataset, result.dataset, result.nFocusDatapoints);

		result.bestfocus_af_position = bestfocus_af_position;
		result.offset_from_bestfocus_um = offset_from_bestfocus_um;
		result.final_af_position = final_af_position;
		result.bestfocus_af_image = bestfocus_af_image.ToCStyle();

		return result;
	}

	bool focus_successful;

	std::vector<AutofocusDatapoint> dataset;

	int32_t bestfocus_af_position;      // Position identified as that of sharpest focus (motor position)
	int32_t offset_from_bestfocus_um;   // Configured offset (in microns) from sharpest focus
	int32_t final_af_position;          // Final position of autofocus after offset (motor position);

	ImageWrapperDLL bestfocus_af_image; // Final image at the "bestfocus_af_position" position (motor position);
} AutofocusResultsDLL;
