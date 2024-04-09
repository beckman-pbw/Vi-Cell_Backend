#pragma once

#include "afxwin.h"

#include "FirmwareDownload.hpp"

//FirmwareUpdateProgressDlg dialog
#define UWM_UPDATE_FW_PROGRESS (WM_USER + 4)

class FirmwareUpdateProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(FirmwareUpdateProgressDlg)

public:
	FirmwareUpdateProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~FirmwareUpdateProgressDlg();

	
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FWUPDATE_PROG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:
	virtual void PostNcDestroy() override;
	virtual void OnCancel() override;
	virtual void OnOK() override;
	virtual BOOL OnInitDialog() override;
	CProgressCtrl PRGS_FirmwareUpdateControl;

	afx_msg LRESULT OnUpdateFWProgress(WPARAM wparam, LPARAM lparam);
};
