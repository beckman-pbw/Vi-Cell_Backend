#pragma once

#include <cstdint>
#include <string>

//*****************************************************************************
void static SetString (char** dest, char* src) {

	if (src == nullptr)
	{
		*dest = new char[1];
		**dest = 0;
	}
	else
	{
		size_t len = strlen (src) + 1;
		*dest = new char[len];
		strcpy_s (*dest, len, src);
	}
}
