#pragma once

#include <stdint.h>
#include <string>

//*****************************************************************************
// Use 4 spaces for indentation instead of tab
static std::string SetIndentation (uint8_t tabCnt, bool newLine = true)
{
	std::string indentation = "";
	if (newLine)
	{
		indentation += "\n";
	}
	for (uint8_t index = 0; index < tabCnt; index++)
	{
		indentation += "    ";
	}
	return indentation;
}
