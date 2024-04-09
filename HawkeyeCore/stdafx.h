// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

// Inhibit the use of the ancient min/max macros.
// Doing this allow std::min/std::max to compile correctly.
#define NOMINMAX

#include "targetver.h"


#ifdef HAWKEYECORE_EXPORTS
#define DLL_CLASS           __declspec(dllexport)
#else
#define DLL_CLASS           __declspec(dllimport)
#endif

#include "HwConditions.hpp"   // bring in the hardware/firmware conditionals

//#define ASYNC_MOTOR_WAIT

#ifdef ASYNC_MOTOR_WAIT

#define STANDALONE_MOTOR_WAIT

#endif // ASYNC_MOTOR_WAIT



// Windows Header Files:
#include <windows.h>

