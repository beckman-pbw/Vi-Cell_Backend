#pragma once

#include <cstdint>

typedef struct Bioprocess_t
{
	char* bioprocess_name;
	
	char* reactor_name;
	
	uint32_t cell_type_index;

	uint64_t start_time; // minutes since 1/1/1970 UDT

	bool is_active;

	char* comment_text;
} Bioprocess_t;


