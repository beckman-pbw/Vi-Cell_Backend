// TagWriteFailureDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RFIDProgrammingStation.h"
#include "TagWriteFailureDlg.h"
#include "afxdialogex.h"
#include "RFIDProgrammingStationDlg.h"
#include <playsoundapi.h>


// TagWriteFailureDlg dialog

IMPLEMENT_DYNAMIC(TagWriteFailureDlg, CDialogEx)

BEGIN_MESSAGE_MAP(TagWriteFailureDlg, CDialogEx)
	//{{AFX_MSG_MAP(TagWriteFailureDlg)
	ON_MESSAGE(UWM_WRITEERR_ERROR_SINCE, &TagWriteFailureDlg::OnTagsSinceError)
	ON_MESSAGE(UWM_WRITEERR_ERROR_AT, &TagWriteFailureDlg::OnTagsAtError)
	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDSTOP_WRITE_ERROR, &TagWriteFailureDlg::OnBnClickedWriteError)
END_MESSAGE_MAP()

TagWriteFailureDlg::TagWriteFailureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TAGWRITEFAILURE, pParent)
	, SLBL_ErrorAtTag_Value(_T(""))
	, SLBL_TagsSinceError_Value(_T(""))
{
#ifndef _WIN32_WCE
	EnableActiveAccessibility();
#endif
	CDialogEx::Create(IDD_TAGWRITEFAILURE, pParent);
}

TagWriteFailureDlg::~TagWriteFailureDlg()
{
}

void TagWriteFailureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, LABL_ERROR_AT_TAG, SLBL_ErrorAtTag_Value);
	DDX_Text(pDX, LABL_TAGS_SINCE_ERROR, SLBL_TagsSinceError_Value);
}

void TagWriteFailureDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BOOL TagWriteFailureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CFont font1;
	font1.CreateFont(45, 0, 0, 0, FW_BOLD, 1, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
	GetDlgItem(LABL_WRITEERROR_TITLE)->SetFont(&font1);

	CFont font2;
	font2.CreateFont(15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, 0, 0, 0, 0, 0, _T("Arial"));
	GetDlgItem(LABL_WRITEERROR_MESSAGE)->SetFont(&font2);

	PlaySound(_T("sounds\\Industrial Alarm.wav"), NULL, SND_APPLICATION | SND_ASYNC);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void TagWriteFailureDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	RECT rect;

	CRect rectButton;
	this->GetWindowRect(&rectButton);

	COLORREF cr = RGB(60, 180, 80);
	dc.FillSolidRect(&rect, cr); // Background color
}

void TagWriteFailureDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

LRESULT TagWriteFailureDlg::OnTagsSinceError(WPARAM wParam, LPARAM lParam)
{
	uint32_t tags_since_error = static_cast<uint32_t>(lParam);
	SLBL_TagsSinceError_Value.Format(_T("%d"), tags_since_error);

	//- Transfer data from variables to controls
	UpdateData(FALSE);
	return LRESULT(0);
}

LRESULT TagWriteFailureDlg::OnTagsAtError(WPARAM wParam, LPARAM lParam)
{
	uint32_t error_at_tag = static_cast<uint32_t>(wParam);
	if (SLBL_ErrorAtTag_Value != "")
		SLBL_ErrorAtTag_Value.Append(_T(", "));
	SLBL_ErrorAtTag_Value.AppendFormat(_T("%d"), error_at_tag);

	//- Transfer data from variables to controls
	UpdateData(FALSE);
	return LRESULT(0);
}

void TagWriteFailureDlg::OnBnClickedWriteError()
{
	GetParent()->SendMessage(UWM_DISTROY_WRITEERR_DLG, static_cast<WPARAM>(true));
}
