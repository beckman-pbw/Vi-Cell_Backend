
// HawkeyeWin_EncryptDlg.h : header file
//

#pragma once
#include "afxeditbrowsectrl.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "EncryptionWork.h"

class CHawkeyeWin_EncryptDlgAutoProxy;


// CHawkeyeWin_EncryptDlg dialog
class CHawkeyeWin_EncryptDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHawkeyeWin_EncryptDlg);
	friend class CHawkeyeWin_EncryptDlgAutoProxy;

// Construction
public:
	CHawkeyeWin_EncryptDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CHawkeyeWin_EncryptDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HAWKEYEWIN_ENCRYPT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	CHawkeyeWin_EncryptDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
public:
	EncryptionWork m_encryption_work_;
	CString m_inFilename;
	afx_msg void OnEnChangeMfceditbrowseInfile();
	CMFCEditBrowseCtrl m_browseInFile;
	CMFCEditBrowseCtrl m_browseOutFile;
	CButton m_buttonStartEncrypt;
	CRichEditCtrl m_richeditHashKey;
	CString m_stringHashKey;
	afx_msg void OnBnClickedButtonEncrypt();
	CString m_outFilename;
	afx_msg void OnEnChangeMfceditbrowseOutfile();
	CEdit m_editHashKey;
	afx_msg void OnBnClickedButtonCopy2clip();
};
