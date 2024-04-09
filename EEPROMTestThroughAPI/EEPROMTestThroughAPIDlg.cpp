#include "stdafx.h"

#include <string>  
#include <thread>

#include "afxdialogex.h"

#include "EEPROMTestThroughAPI.h"
#include "EEPROMTestThroughAPIDlg.h"

#include "HawkeyeLogic.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEEPROMTestThroughAPIDlg dialog
void hkInitialize(bool hw)
{
	Initialize(hw);
}


CEEPROMTestThroughAPIDlg::CEEPROMTestThroughAPIDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_EEPROMTESTTHROUGHAPI_DIALOG, pParent)
	, chk_Hardware(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEEPROMTestThroughAPIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_USER, edit_UserName);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, edit_Password);
	DDX_Control(pDX, IDC_EDIT_SN, edit_SerialNumber);
	DDX_Control(pDX, IDC_STATIC_SERIALNUMBER, static_SerialNumber);
	DDX_Control(pDX, IDC_STATIC_MESSAGETEXT, static_MessageText);
	DDX_Check(pDX, IDC_CHECK_HARDWARE, chk_Hardware);
}

BEGIN_MESSAGE_MAP(CEEPROMTestThroughAPIDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_INITIALIZE, &CEEPROMTestThroughAPIDlg::OnBnClickedBtnInitialize)
	ON_BN_CLICKED(IDC_BTN_LOGIN, &CEEPROMTestThroughAPIDlg::OnBnClickedBtnLogin)
	ON_BN_CLICKED(IDC_BTN_RETRIEVESN, &CEEPROMTestThroughAPIDlg::OnBnClickedBtnRetrievesn)
	ON_BN_CLICKED(IDC_BTN_SETSN, &CEEPROMTestThroughAPIDlg::OnBnClickedBtnSetsn)
	ON_BN_CLICKED(IDC_BUTTON_SHUTDOWN, &CEEPROMTestThroughAPIDlg::OnBnClickedButtonShutdown)
END_MESSAGE_MAP()


// CEEPROMTestThroughAPIDlg message handlers

BOOL CEEPROMTestThroughAPIDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here


	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEEPROMTestThroughAPIDlg::OnPaint()
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
HCURSOR CEEPROMTestThroughAPIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CEEPROMTestThroughAPIDlg::OnBnClickedBtnInitialize()
{
	UpdateData();
	hkInitialize(chk_Hardware);

	InitializationState is;
	do
	{
		is = IsInitializationComplete();
		std::string state;
		switch (is)
		{
			case InitializationState::eInitializationInProgress:
				static_MessageText.SetWindowTextW(L"In Progress");
				break;
			case InitializationState::eInitializationFailed:
				static_MessageText.SetWindowTextW(L"Failed");
				break;
			case InitializationState::eFirmwareUpdateInProgress:
				static_MessageText.SetWindowTextW(L"Firwmare Update In Progress");
				break;
			case InitializationState::eInitializationComplete:
				static_MessageText.SetWindowTextW(L"Completed");
				break;
			case InitializationState::eFirmwareUpdateFailed:
				static_MessageText.SetWindowTextW(L"Firmware Update Failed");
				break;
			case InitializationState::eInitializationStopped_CarosuelTubeDetected:
				static_MessageText.SetWindowTextW(L"TubeDetected");
				break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	} while ((is == InitializationState::eInitializationInProgress) ||
	         (is == InitializationState::eFirmwareUpdateInProgress));

}


void CEEPROMTestThroughAPIDlg::OnBnClickedBtnLogin()
{
	UpdateData();

	CString un, pw;
	edit_UserName.GetWindowText(un);
	edit_Password.GetWindowText(pw);
	HawkeyeError HE = LoginUser(CT2A(un), CT2A(pw));
	static_MessageText.SetWindowTextW((HE == HawkeyeError::eSuccess) ? L"Logged in" : L"Login Failed");
}


void CEEPROMTestThroughAPIDlg::OnBnClickedBtnRetrievesn()
{
	UpdateData();

	char* sn;
	HawkeyeError HE = GetSystemSerialNumber(sn);

	if (HE == HawkeyeError::eSuccess)
	{
		CStringA SN(sn);

		static_SerialNumber.SetWindowTextW(CStringW(SN));
		static_MessageText.SetWindowTextW(L"Serial number retrieved");
	}
	else if (HE == HawkeyeError::eNoneFound)
	{
		static_SerialNumber.SetWindowTextW(L"");
		static_MessageText.SetWindowTextW(L"Serial number not set");
	}
	else if (HE == HawkeyeError::eEntryInvalid)
	{
		CStringA SN(sn);

		static_SerialNumber.SetWindowTextW(CStringW(SN));
		static_MessageText.SetWindowTextW(L"Serial number invalid");
	}
	else
	{
		static_MessageText.SetWindowTextW(L"Other error retrieving SN.");
	}
	if (sn)
		FreeCharBuffer(sn);
}



void CEEPROMTestThroughAPIDlg::OnBnClickedBtnSetsn()
{
	UpdateData();

	CString sn;
	edit_SerialNumber.GetWindowTextW(sn);

	CString pw;
	edit_Password.GetWindowTextW(pw);

	HawkeyeError HE = svc_SetSystemSerialNumber(CT2A(sn), CT2A(pw));

	if (HE != HawkeyeError::eSuccess)
		static_MessageText.SetWindowTextW(L"Set serial number failed");
	else
		static_MessageText.SetWindowTextW(L"Set serial number Succeeded");
}


void CEEPROMTestThroughAPIDlg::OnBnClickedButtonShutdown()
{
	UpdateData();
	Shutdown();

	while (!IsShutdownComplete())
	{
		static_MessageText.SetWindowTextW(L"Shutting down...");
	
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	static_MessageText.SetWindowTextW(L"Shut down");

}
