#pragma once

#include "AnalysisDefinitionCommon.hpp"
#include "ErrorCodes.h"

typedef struct measurement_t
{
	Hawkeye::Characteristic_t characteristic;
	float value;
} measurement_t;

typedef struct large_cluster_data_t
{
	uint16_t num_cells_in_cluster;
	uint16_t top_left_x;
	uint16_t top_left_y;
	uint16_t bottom_right_x;
	uint16_t bottom_right_y;
} large_cluster_data_t;

typedef struct BasicResultAnswers
{
	//Basic answers (pop count, pop of interest count, concentrations, some averages...)

	E_ERRORCODE eProcessedStatus;
	uint32_t nTotalCumulative_Imgs;

	/// PRIMARY
	uint32_t count_pop_general;
	uint32_t count_pop_ofinterest;
	float concentration_general;    // x10^6
	float concentration_ofinterest; // x10^6
	float percent_pop_ofinterest;

	/// SECONDARY
	float avg_diameter_pop;
	float avg_diameter_ofinterest;
	float avg_circularity_pop;
	float avg_circularity_ofinterest;

	/// TERTIARY
	float coefficient_variance;
	uint16_t average_cells_per_image;
	uint16_t average_brightfield_bg_intensity;
	uint16_t bubble_count;
	uint16_t large_cluster_count;
} BasicResultAnswers;

typedef struct histogrambin_t
{
	float bin_nominal_value;
	uint32_t count;
} histogrambin_t;

enum eExportImages : uint16_t
{
	eFirstAndLastOnly = 0,
	eAll,
	eExportNthImage
};
