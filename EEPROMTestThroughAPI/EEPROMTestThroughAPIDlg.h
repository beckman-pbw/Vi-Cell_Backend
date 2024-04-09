
// EEPROMTestThroughAPIDlg.h : header file
//
#pragma once
#include "afxwin.h"

// CEEPROMTestThroughAPIDlg dialog
class CEEPROMTestThroughAPIDlg : public CDialogEx
{
// Construction
public:
	CEEPROMTestThroughAPIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EEPROMTESTTHROUGHAPI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnInitialize();
	afx_msg void OnBnClickedBtnLogin();
	CEdit edit_UserName;
	CEdit edit_Password;
	CEdit edit_SerialNumber;
	CStatic static_SerialNumber;
	CStatic static_MessageText;
	afx_msg void OnBnClickedBtnRetrievesn();
	afx_msg void OnBnClickedBtnSetsn();
	BOOL chk_Hardware;
	afx_msg void OnBnClickedButtonShutdown();
};
