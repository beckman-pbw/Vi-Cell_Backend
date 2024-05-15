
// HawkeyeWin_Encrypt.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "CommandLineParse.h"
#include "EncryptionWork.h"


// CHawkeyeWin_EncryptApp:
// See HawkeyeWin_Encrypt.cpp for the implementation of this class
//

class CHawkeyeWin_EncryptApp : public CWinApp
{
public:
	CHawkeyeWin_EncryptApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

private:
	CCommandLineParse m_cmdline;
	BOOL process_cmdline();
	void fix_outfilename();
	void do_encryption_work(EncryptionWork& encryption_work);
	bool writeout_hashfile(CString haskkey);
	bool find_infilename();
	BOOL cmdline_encryption_work();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CHawkeyeWin_EncryptApp theApp;