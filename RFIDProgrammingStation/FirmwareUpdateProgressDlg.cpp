// FirmwareUpdateProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FirmwareUpdateProgressDlg.h"
#include "afxdialogex.h"
#include "resource.h"


// FirmwareUpdateProgressDlg dialog

IMPLEMENT_DYNAMIC(FirmwareUpdateProgressDlg, CDialogEx)


FirmwareUpdateProgressDlg::FirmwareUpdateProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_FWUPDATE_PROG, pParent)
{
	CDialogEx::Create(IDD_FWUPDATE_PROG, pParent);
}

FirmwareUpdateProgressDlg::~FirmwareUpdateProgressDlg()
{
}

void FirmwareUpdateProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS_FW_UPDATE, PRGS_FirmwareUpdateControl);
}


BEGIN_MESSAGE_MAP(FirmwareUpdateProgressDlg, CDialogEx)
	ON_MESSAGE(UWM_UPDATE_FW_PROGRESS, OnUpdateFWProgress)
END_MESSAGE_MAP()


// FirmwareUpdateProgressDlg message handlers

void FirmwareUpdateProgressDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	GetParent()->PostMessage(WM_CLOSE, 0, 0);
	delete this;
}

void FirmwareUpdateProgressDlg::OnCancel()
{
	DestroyWindow();
}

void FirmwareUpdateProgressDlg::OnOK()
{
	if (UpdateData(true))
	{
		DestroyWindow();
	}

}

BOOL FirmwareUpdateProgressDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	PRGS_FirmwareUpdateControl.SetRange32(0,100);

	CFont m_font;
	m_font.CreateFont(10, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
	GetDlgItem(LABL_FW_TITLE_MESSAGE)->SetFont(&m_font);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT FirmwareUpdateProgressDlg::OnUpdateFWProgress(WPARAM wparam, LPARAM lparam)
{
	auto progress = static_cast<uint32_t>(wparam);
	if(PRGS_FirmwareUpdateControl.GetPos() != progress)
		PRGS_FirmwareUpdateControl.SetPos(progress);
	return true;
}
