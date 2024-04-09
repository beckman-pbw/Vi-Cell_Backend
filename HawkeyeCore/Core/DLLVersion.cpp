#include "stdafx.h"

#include "../../target/properties/HawkeyeCore/dllversion.hpp"

#include <string>

#include "CommandParser.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "GetDLLVersion";

//*****************************************************************************
// Returns the Major.Minor version of the DLL.
//*****************************************************************************
std::string GetDLLVersion (bool getFullVersion)
{
	const auto delimiter = ".";
	
	std::vector<std::string> versions{};

	if (!CommandParser::parse (delimiter, DLL_Version, versions))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Failed to parse DLL_Version: " + std::string(DLL_Version));
	}
	
	std::string ver = {};
	
	if (versions.size() > 0)
	{
		ver += versions[0];
		if (versions.size() > 1)
		{
			ver += delimiter + versions[1];

			if (getFullVersion)
			{
				if (versions.size() > 2)
				{
					ver += delimiter + versions[2];

					if (versions.size() > 3)
						ver += delimiter + versions[3];
				}
			}
		}
	}

	return ver;
}
