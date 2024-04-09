
// MotorSysTest.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "MotorSysTestDlg.h"


// MotorSysTestApp:
// See MotorSysTest.cpp for the implementation of this class
//

class MotorSysTestApp : public CWinApp
{
public:
	MotorSysTestApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern MotorSysTestApp theApp;

class AppCmdLineInfo : public CCommandLineInfo
{
public:
    AppCmdLineInfo();
    ~AppCmdLineInfo();

private:
    void CheckEditFlag( CString & paramStr, BOOL bFlag );

public:
    // plain char* version on UNICODE for source-code backwards compatibility
    virtual void ParseParam( const TCHAR* pszParam, BOOL bFlag, BOOL bLast );
#ifdef _UNICODE
    virtual void ParseParam( const char* pszParam, BOOL bFlag, BOOL bLast );
#endif

    BOOL m_bAllowEdit;
    BOOL m_bSimMode;
    MotorSysTestDlg::InstrumentTypes m_nInstType;
};
