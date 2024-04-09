// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"


#define DLL_CLASS           __declspec(dllexport)

#include "HwConditions.hpp"   // bring in the hardware/firmware conditionals

//#define ASYNC_MOTOR_WAIT

#ifdef ASYNC_MOTOR_WAIT

#define STANDALONE_MOTOR_WAIT

#endif // ASYNC_MOTOR_WAIT



// Windows Header Files:
#include <windows.h>

