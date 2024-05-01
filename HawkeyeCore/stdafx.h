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

#include "..\VariantToBuild.hpp"

#ifdef HAWKEYECORE_EXPORTS
#define DLL_CLASS           __declspec(dllexport)
#else
#define DLL_CLASS           __declspec(dllimport)
#endif

#include "HwConditions.hpp"   // bring in the hardware/firmware conditionals

// Windows Header Files:
#include <windows.h>
