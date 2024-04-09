// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"

#define DLL_CLASS           __declspec(dllimport)
//#define DLL_CLASS

#include "HwConditions.hpp"     // bring in the hardware/firmware conditionals

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#define	NO_ERROR_REPORT

#include <stdio.h>
#include <tchar.h>

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


