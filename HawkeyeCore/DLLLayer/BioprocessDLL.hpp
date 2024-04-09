#pragma once

#include <string>
#include "Bioprocess.hpp"
#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"

typedef struct bioprocess_tDLL
{
	bioprocess_tDLL() {}
	bioprocess_tDLL(Bioprocess_t bp)
	{
		DataConversion::convertToStandardString(bioprocess_name, bp.bioprocess_name);
		DataConversion::convertToStandardString(reactor_name, bp.reactor_name);
		cell_type_index = bp.cell_type_index;
		start_time = ChronoUtilities::ConvertToTimePoint<std::chrono::minutes>(bp.start_time);
		is_active = bp.is_active;
		DataConversion::convertToStandardString(comment_text, bp.comment_text);
	}

	Bioprocess_t ToCStyle()
	{
		Bioprocess_t bp = {};

		DataConversion::convertToCharPointer(bp.bioprocess_name, bioprocess_name);
		DataConversion::convertToCharPointer(bp.reactor_name, reactor_name);
		bp.cell_type_index = cell_type_index;
		bp.start_time = ChronoUtilities::ConvertFromTimePoint<std::chrono::minutes>(start_time);
		bp.is_active = is_active;
		DataConversion::convertToCharPointer(bp.comment_text, comment_text);

		return bp;
	}

	std::string bioprocess_name;

	std::string reactor_name;

	uint32_t cell_type_index;

	system_TP start_time; // minutes since 1/1/1970 UDT

	bool is_active;

	std::string comment_text;
} bioprocess_tDLL;
