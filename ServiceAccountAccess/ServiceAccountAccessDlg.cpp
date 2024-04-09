
// ServiceAccountAccessDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ServiceAccountAccess.h"
#include "ServiceAccountAccessDlg.h"
#include "afxdialogex.h"

#include "SecurityHelpers.hpp"
#include <chrono>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CServiceAccountAccessDlg dialog



CServiceAccountAccessDlg::CServiceAccountAccessDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SERVICEACCOUNTACCESS_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServiceAccountAccessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DATEPICKER2, DatePickerCtrl);
	DDX_Control(pDX, IDC_STATIC_PASSWORD, PasswordTextCtrl);
	DDX_Control(pDX, IDC_SPIN_TZOFFSET, TimeZoneSpinner);
	DDX_Control(pDX, IDC_EDIT_TZOFFSET, TimeZoneEdit);
}

BEGIN_MESSAGE_MAP(CServiceAccountAccessDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_GENERATE, &CServiceAccountAccessDlg::OnBnClickedBtnGenerate)
END_MESSAGE_MAP()


// CServiceAccountAccessDlg message handlers

BOOL CServiceAccountAccessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	PasswordTextCtrl.SetWindowTextW(L"");
	DatePickerCtrl.SetFormat(L"yyyy/MMM/dd HH:mm");
	TimeZoneSpinner.SetRange(-12, +14);
	
	

	// Set the date-time picker to display the local time with a UTC offset.
	// This will let the user override with the time and offset as given on the system
	//  and then we can convert that over appropriately so we generate the right math.
	std::chrono::system_clock::time_point ctNow(std::chrono::system_clock::now());
	CTime tNow(std::chrono::system_clock::to_time_t(ctNow));
	
	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation(&tzi);

	// Bias: UTC = Local + Bias
	// Therefore, when displaying a local time: Local = UTC - bias
	int16_t hoursbias = (int16_t)(tzi.Bias / 60);
	hoursbias *= -1;

	// This will be the LOCAL time as initially shown
	DatePickerCtrl.SetTime(&tNow);
	TimeZoneSpinner.SetPos(hoursbias);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServiceAccountAccessDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServiceAccountAccessDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CServiceAccountAccessDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#include <string>
void CServiceAccountAccessDlg::OnBnClickedBtnGenerate()
{
	CTime ctTime;
	SYSTEMTIME sysTime;
	DatePickerCtrl.GetTime(ctTime);
	DatePickerCtrl.GetTime(&sysTime);// This gets a "raw" time with no inherent TZ info embedded.  Add the tzOffset to get to UTC.

	// Move "raw" time to UTC directly
	tm tmSys;
	tmSys.tm_year = sysTime.wYear - 1900;
	tmSys.tm_mon = sysTime.wMonth - 1;
	tmSys.tm_mday = sysTime.wDay;

	tmSys.tm_hour = sysTime.wHour;
	tmSys.tm_min = sysTime.wMinute;
	tmSys.tm_sec = sysTime.wSecond;

	time_t ttGMT = _mkgmtime(&tmSys);

	int16_t tzOffset = (int16_t)TimeZoneSpinner.GetPos();
	std::chrono::system_clock::time_point tpGMT(std::chrono::system_clock::from_time_t(ttGMT));
	tpGMT -= std::chrono::hours(tzOffset);

	std::chrono::system_clock::time_point tpNow(std::chrono::system_clock::now());

	// Debugging cross-check - if you adjust the displayed time / UTC offset in the dialog to variants on the current time
	// (that is: set it "correctly" for a different timezone) then we should be able to sync up correctly with "now"
	std::chrono::time_point<std::chrono::system_clock, std::chrono::hours> hGMT, hNOW;
	hGMT = std::chrono::time_point_cast<std::chrono::hours>(tpGMT);
	hNOW = std::chrono::time_point_cast<std::chrono::hours>(tpNow);



	std::string passcode = SecurityHelpers::GenerateHMACPasscode(tpGMT, SecurityHelpers::HMAC_YEARS);
	PasswordTextCtrl.SetWindowTextW(CA2W(passcode.c_str()));
}
