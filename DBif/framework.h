#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the HAWKEYDBIF_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// HAWKEYEDBIF_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#define DLL_CLASS

#ifdef HAWKEYEDBIF_EXPORTS
#define HAWKEYEDBIF_API	__declspec(dllexport)
#define HAWKEYECORE_API	__declspec(dllexport)
#else
#define HAWKEYEDBIF_API	__declspec(dllimport)
#define HAWKEYECORE_API	__declspec(dllimport)
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT
