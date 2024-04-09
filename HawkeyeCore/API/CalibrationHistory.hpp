#pragma once

#include <cstdint>

#include "CalibrationCommon.hpp"
#include "uuid__t.hpp"

typedef struct calibration_consumable {
	char* label;
	char* lot_id;
	uint32_t expiration_date; // days since 1/1/1970
	double assay_value;
} calibration_consumable;

typedef struct calibration_history_entry {
	uint64_t		timestamp;
	char*			username;
	calibration_type cal_type;
	uint16_t		num_consumables;
	calibration_consumable* consumable_list;
	double			slope;
	double			intercept;
	uint32_t		image_count;
	uuid__t			uuid;
} calibration_history_entry;
