
// DlgProxy.cpp : implementation file
//

#include "stdafx.h"
#include "HawkeyeWin_Encrypt.h"
#include "DlgProxy.h"
#include "HawkeyeWin_EncryptDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CHawkeyeWin_EncryptDlgAutoProxy

IMPLEMENT_DYNCREATE(CHawkeyeWin_EncryptDlgAutoProxy, CCmdTarget)

CHawkeyeWin_EncryptDlgAutoProxy::CHawkeyeWin_EncryptDlgAutoProxy()
{
	EnableAutomation();
	
	// To keep the application running as long as an automation 
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();

	// Get access to the dialog through the application's
	//  main window pointer.  Set the proxy's internal pointer
	//  to point to the dialog, and set the dialog's back pointer to
	//  this proxy.
	ASSERT_VALID(AfxGetApp()->m_pMainWnd);
	if (AfxGetApp()->m_pMainWnd)
	{
		ASSERT_KINDOF(CHawkeyeWin_EncryptDlg, AfxGetApp()->m_pMainWnd);
		if (AfxGetApp()->m_pMainWnd->IsKindOf(RUNTIME_CLASS(CHawkeyeWin_EncryptDlg)))
		{
			m_pDialog = reinterpret_cast<CHawkeyeWin_EncryptDlg*>(AfxGetApp()->m_pMainWnd);
			m_pDialog->m_pAutoProxy = this;
		}
	}
}

CHawkeyeWin_EncryptDlgAutoProxy::~CHawkeyeWin_EncryptDlgAutoProxy()
{
	// To terminate the application when all objects created with
	// 	with automation, the destructor calls AfxOleUnlockApp.
	//  Among other things, this will destroy the main dialog
	if (m_pDialog != NULL)
		m_pDialog->m_pAutoProxy = NULL;
	AfxOleUnlockApp();
}

void CHawkeyeWin_EncryptDlgAutoProxy::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CHawkeyeWin_EncryptDlgAutoProxy, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CHawkeyeWin_EncryptDlgAutoProxy, CCmdTarget)
END_DISPATCH_MAP()

// Note: we add support for IID_IHawkeyeWin_Encrypt to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .IDL file.

// {4F769907-1E5F-4872-8504-E9BA36528E02}
static const IID IID_IHawkeyeWin_Encrypt =
{ 0x4F769907, 0x1E5F, 0x4872, { 0x85, 0x4, 0xE9, 0xBA, 0x36, 0x52, 0x8E, 0x2 } };

BEGIN_INTERFACE_MAP(CHawkeyeWin_EncryptDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CHawkeyeWin_EncryptDlgAutoProxy, IID_IHawkeyeWin_Encrypt, Dispatch)
END_INTERFACE_MAP()

// The IMPLEMENT_OLECREATE2 macro is defined in StdAfx.h of this project
// {726B3ABB-8C65-4EA1-8609-CCE83267E005}
IMPLEMENT_OLECREATE2(CHawkeyeWin_EncryptDlgAutoProxy, "HawkeyeWin_Encrypt.Application", 0x726b3abb, 0x8c65, 0x4ea1, 0x86, 0x9, 0xcc, 0xe8, 0x32, 0x67, 0xe0, 0x5)


// CHawkeyeWin_EncryptDlgAutoProxy message handlers
