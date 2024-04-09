
// ServiceAccountAccessDlg.h : header file
//

#pragma once
#include "afxdtctl.h"
#include "afxwin.h"
#include "afxcmn.h"


// CServiceAccountAccessDlg dialog
class CServiceAccountAccessDlg : public CDialogEx
{
// Construction
public:
	CServiceAccountAccessDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVICEACCOUNTACCESS_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CDateTimeCtrl DatePickerCtrl;
	CStatic PasswordTextCtrl;
	afx_msg void OnBnClickedBtnGenerate();
	CSpinButtonCtrl TimeZoneSpinner;
	CEdit TimeZoneEdit;
};
