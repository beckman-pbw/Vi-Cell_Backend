
// ResetPasswordGeneratorDlg.h : header file
//

#pragma once
#include "afxdtctl.h"
#include "afxwin.h"
#include "afxcmn.h"
#include <string>

#ifdef _DEBUG
#include "ChronoUtilities.hpp"
#endif
#include "SecurityHelpers.hpp"

#ifdef _DEBUG
uint64_t DayUnits( std::chrono::system_clock::time_point timepoint, SecurityHelpers::timepoint_to_epoch_converter_t scale );
#endif

// CResetPasswordGeneratorDlg dialog
class CResetPasswordGeneratorDlg : public CDialogEx
{

// Construction
public:
	CResetPasswordGeneratorDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RESET_PWD_GEN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;
	std::string snStr;
	std::string unameStr;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnEnChangeSNEdit();
	afx_msg void OnEnChangeUnameEdit();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBtnGenerate();

	CDateTimeCtrl DatePickerCtrl;

	CEdit SNEdit;
	CString cstrSN;
	CEdit UnameEdit;
	CString cstrUname;
	CEdit PasswordTextDisplay;
	CButton GenerateBtn;
};
