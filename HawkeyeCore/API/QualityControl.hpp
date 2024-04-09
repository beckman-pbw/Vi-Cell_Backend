#pragma once

#include "QualityControlCommon.hpp"

typedef struct QualityControl_t
{
	char* qc_name;
	
	uint32_t cell_type_index;

	assay_parameter assay_type;

	char* lot_information;
	
	double assay_value;
	double plusminus_percentage;

	uint64_t expiration_date; // days since 1/1/1970 local time

	char* comment_text;
} QualityControl_t;
