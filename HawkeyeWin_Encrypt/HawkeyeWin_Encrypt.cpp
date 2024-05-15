
// HawkeyeWin_Encrypt.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "HawkeyeWin_Encrypt.h"
#include "HawkeyeWin_EncryptDlg.h"
#include "CommandLineParse.h"
#include <boost/filesystem.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHawkeyeWin_EncryptApp

BEGIN_MESSAGE_MAP(CHawkeyeWin_EncryptApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CHawkeyeWin_EncryptApp construction

CHawkeyeWin_EncryptApp::CHawkeyeWin_EncryptApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CHawkeyeWin_EncryptApp object

CHawkeyeWin_EncryptApp theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0xD8CE0FCC, 0xCAC2, 0x4E78, { 0x8F, 0xAC, 0x7D, 0x1, 0x3C, 0x3, 0x68, 0x63 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


// CHawkeyeWin_EncryptApp initialization

BOOL CHawkeyeWin_EncryptApp::InitInstance()
{
//TODO: call AfxInitRichEdit2() to initialize richedit2 library.
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	if (process_cmdline() || m_cmdline.m_paramsHandled)
		return FALSE; //no need to run the user interface if we process cmd line parameters.

	CHawkeyeWin_EncryptDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

#ifndef _AFXDLL
	ControlBarCleanUp();
#endif

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CHawkeyeWin_EncryptApp::ExitInstance()
{
	AfxOleTerm(FALSE);

	return CWinApp::ExitInstance();
}

bool CHawkeyeWin_EncryptApp::find_infilename()
{
	//make sure we have the file to encrypt. 
	if (!boost::filesystem::exists(m_cmdline.m_paramInfile.GetString()))
	{
		m_cmdline.HelpMsgToConsole(_T("Input File Not Found!\r\n"));
		return false;
	}
	return true;
}

void CHawkeyeWin_EncryptApp::fix_outfilename()
{
	if (!m_cmdline.m_switchOutfile)
	{
		m_cmdline.m_paramOutfile = m_cmdline.m_paramInfile;
	}
	if (m_cmdline.m_paramOutfile.Find(_T(".srec")) >= 0)
		m_cmdline.m_paramOutfile.Replace(_T(".srec"), _T(".bin"));
	else
		m_cmdline.m_paramOutfile.Append(_T(".bin"));
}

void CHawkeyeWin_EncryptApp::do_encryption_work(EncryptionWork &encryption_work)
{
	encryption_work.SetInfile(m_cmdline.m_paramInfile);
	encryption_work.SetOutFile(m_cmdline.m_paramOutfile);
	encryption_work.DoEncryption();
}

bool CHawkeyeWin_EncryptApp::writeout_hashfile(CString haskkey)
{
	const std::string str_hash(CW2A(haskkey.GetString(), CP_UTF8));
	const std::string str_hashfile(CW2A(m_cmdline.m_paramHashfile.GetString(), CP_UTF8));
	std::ofstream out_stream(str_hashfile, std::ios_base::out);
	if (!out_stream.is_open())
	{
		return false;
	}
	out_stream << str_hash.c_str() <<std::endl;
	out_stream.close();
	return true;
}

BOOL CHawkeyeWin_EncryptApp::cmdline_encryption_work()
{
	DWORD nw;
	if (!find_infilename()) return FALSE;
	fix_outfilename();

	EncryptionWork encryption_work;
	do_encryption_work(encryption_work);

	CString haskkey = encryption_work.GetHashKey();
	if(m_cmdline.m_switchHashFile)
	{
		return writeout_hashfile(haskkey);
	}
	else
	{
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), haskkey, haskkey.GetLength(), &nw, NULL);
	}
	return TRUE;
}

BOOL CHawkeyeWin_EncryptApp::process_cmdline()
{

	// Parse command line 
	ParseCommandLine(m_cmdline);

	if(m_cmdline.m_paramsHandled)
	{
		if (!AttachConsole(ATTACH_PARENT_PROCESS))   // try to hijack existing console of command line
			AllocConsole();                           // or create your own.

		if (m_cmdline.m_switchHelp)
		{
			m_cmdline.HelpMsgToConsole();
			return TRUE;
		}

		return cmdline_encryption_work();
	}

	// App was launched with /Embedding or /Automation switch.
	// Run app as automation server.
	else if (m_cmdline.m_bRunEmbedded || m_cmdline.m_bRunAutomated)
	{
		// Register class factories via CoRegisterClassObject().
		COleTemplateServer::RegisterAll();
	}
	// App was launched with /Unregserver or /Unregister switch.  Remove
	// entries from the registry.
	else if (m_cmdline.m_nShellCommand == CCommandLineInfo::AppUnregister)
	{
		COleObjectFactory::UpdateRegistryAll(FALSE);
		AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor);
		return TRUE;
	}
	// App was launched standalone or with other switches (e.g. /Register
	// or /Regserver).  Update registry entries, including typelibrary.
	else
	{
		COleObjectFactory::UpdateRegistryAll();
		AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid);
		if (m_cmdline.m_nShellCommand == CCommandLineInfo::AppRegister)
			return TRUE;
	}

	return FALSE;
}

