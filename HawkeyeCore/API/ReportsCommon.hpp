#pragma once

#include <cstdint>

// Used to set the final sample status in the Sample.log file. 
enum sample_completion_status : uint16_t
{
	sample_completed = 0,
	sample_skipped,
	sample_errored,
	sample_not_run,
};
