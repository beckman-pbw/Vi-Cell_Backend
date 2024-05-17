#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

#ifdef AUTOMATION_USER_EXPORTS
#define DLL_CLASS           __declspec(dllexport)
#else
#define DLL_CLASS           __declspec(dllimport)
#endif

// Windows Header Files:
#include <windows.h>
