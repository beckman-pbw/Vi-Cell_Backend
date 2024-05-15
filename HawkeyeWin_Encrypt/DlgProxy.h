
// DlgProxy.h: header file
//

#pragma once

class CHawkeyeWin_EncryptDlg;


// CHawkeyeWin_EncryptDlgAutoProxy command target

class CHawkeyeWin_EncryptDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CHawkeyeWin_EncryptDlgAutoProxy)

	CHawkeyeWin_EncryptDlgAutoProxy();           // protected constructor used by dynamic creation

// Attributes
public:
	CHawkeyeWin_EncryptDlg* m_pDialog;

// Operations
public:

// Overrides
	public:
	virtual void OnFinalRelease();

// Implementation
protected:
	virtual ~CHawkeyeWin_EncryptDlgAutoProxy();

	// Generated message map functions

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CHawkeyeWin_EncryptDlgAutoProxy)

	// Generated OLE dispatch map functions

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

