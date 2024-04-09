// TagVerifyFailureDlg.cpp : implementation file

#include "stdafx.h"
#include "RFIDProgrammingStation.h"
#include "TagVerifyFailureDlg.h"
#include "afxdialogex.h"
#include "RFIDProgrammingStationDlg.h"
#include <playsoundapi.h>


// TagVerifyFailureDlg dialog

IMPLEMENT_DYNAMIC(TagVerifyFailureDlg, CDialogEx)

BEGIN_MESSAGE_MAP(TagVerifyFailureDlg, CDialogEx)
	//{{AFX_MSG_MAP(TagWriteFailureDlg)
	ON_MESSAGE(UWM_VERIFYERR_ERROR_SINCE, &TagVerifyFailureDlg::OnTagsSinceError)
	ON_MESSAGE(UWM_VERIFYERR_ERROR_AT, &TagVerifyFailureDlg::OnTagsAtError)
	//}}AFX_MSG_MAP

	ON_BN_CLICKED(IDSTOP_VERIFY_ERROR, &TagVerifyFailureDlg::OnBnClickedVerifyError)
END_MESSAGE_MAP()

TagVerifyFailureDlg::TagVerifyFailureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TAGVERIFYFAILURE, pParent)
	, SLBL_ErrorAtTag_Value(_T(""))
	, SLBL_TagsSinceError_Value(_T(""))
{
#ifndef _WIN32_WCE
	EnableActiveAccessibility();
#endif
	CDialogEx::Create(IDD_TAGVERIFYFAILURE, pParent);
}

TagVerifyFailureDlg::~TagVerifyFailureDlg()
{
}

void TagVerifyFailureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, LABL_ERROR_AT_TAG_VERIFY, SLBL_ErrorAtTag_Value);
	DDX_Text(pDX, LABL_TAGS_SINCE_ERROR, SLBL_TagsSinceError_Value);
}

void TagVerifyFailureDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BOOL TagVerifyFailureDlg::OnInitDialog()
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

void TagVerifyFailureDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	RECT rect;

	CRect rectButton;
	this->GetWindowRect(&rectButton);

	COLORREF cr = RGB(60, 180, 80);
	dc.FillSolidRect(&rect, cr); // Background color
}

void TagVerifyFailureDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

LRESULT TagVerifyFailureDlg::OnTagsSinceError(WPARAM wParam, LPARAM lParam)
{
	uint32_t tags_since_error = static_cast<uint32_t>(wParam);
	SLBL_TagsSinceError_Value.Format(_T("%d"), tags_since_error);

	//- Transfer data from variables to controls
	UpdateData(FALSE);
	return LRESULT(0);
}

LRESULT TagVerifyFailureDlg::OnTagsAtError(WPARAM wParam, LPARAM lParam)
{
	uint32_t error_at_tag = static_cast<uint32_t>(wParam);
	if (SLBL_ErrorAtTag_Value != "")
		SLBL_ErrorAtTag_Value.Append(_T(", "));
	SLBL_ErrorAtTag_Value.AppendFormat(_T("%d"), error_at_tag);
	
	//- Transfer data from variables to controls
	UpdateData(FALSE);
	return LRESULT(0);
}

void TagVerifyFailureDlg::OnBnClickedVerifyError()
{
	GetParent()->SendMessage(UWM_DISTROY_VERIFYERR_DLG, static_cast<WPARAM>(true));
}
