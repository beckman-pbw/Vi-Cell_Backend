#pragma once

#include "DBif_Structs.hpp"
#include "HawkeyeError.hpp"

class CellHealthReagents
{
public:
	
	enum class FluidType : int16_t
	{
		Unknown = 0,
		TrypanBlue = 1,
		Cleaner = 2,
		ConditioningSolution = 3,
		Buffer = 4,
		Diluent = 5,
	};
	
	// Used by Automation interface.
	static HawkeyeError GetVolume (FluidType type, int& volume_ul);
	static HawkeyeError SetVolume (FluidType type, int volume_ul);
	static HawkeyeError AddVolume (FluidType type, int volume_ul);

	// Internal use only.  Used in the SyringePump code to track CellHealth module reagent usage.
	static HawkeyeError AspirateVolume (CellHealthReagents::FluidType type, int volume_ul);
	static uint16_t GetRemainingReagentUses();
	
	static bool Initialize();
	static bool Log();
};
