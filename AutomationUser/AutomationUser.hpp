#pragma once

#include "stdafx.h"

extern "C"
{
	//****************************************************************************
	// Automation client password . generator;  provides the time-based password expected by the Vi-CELL
	DLL_CLASS const char* AutomationPassword();
	// Automation client username generator;  provides the commmon username expected by the instrument
	DLL_CLASS const char* AutomationUsername();
}
