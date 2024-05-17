
// ResetPasswordGeneratorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ResetPasswordGenerator.h"
#include "ResetPasswordGeneratorDlg.h"
#include "afxdialogex.h"

#include <chrono>
#include <string>



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


// CResetPasswordGeneratorDlg dialog

CResetPasswordGeneratorDlg::CResetPasswordGeneratorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_RESET_PWD_GEN_DIALOG, pParent)
	, cstrSN(_T(""))
	, snStr("")
	, cstrUname( _T( "" ) )
	, unameStr( "" )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CResetPasswordGeneratorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange( pDX );
	DDX_Control( pDX, IDC_DATEPICKER, DatePickerCtrl );
	DDX_Control( pDX, IDC_EDIT_INSTRUMENT_SN, SNEdit );
	DDX_Text( pDX, IDC_EDIT_INSTRUMENT_SN, cstrSN );
	DDX_Control( pDX, IDC_EDIT_USERNAME, UnameEdit );
	DDX_Text( pDX, IDC_EDIT_USERNAME, cstrUname );
	DDX_Control( pDX, IDC_EDIT_PASSWORD, PasswordTextDisplay );
	DDX_Control( pDX, IDC_BTN_GENERATE, GenerateBtn );
}

BEGIN_MESSAGE_MAP(CResetPasswordGeneratorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDC_BTN_GENERATE, &CResetPasswordGeneratorDlg::OnBnClickedBtnGenerate)
	ON_EN_CHANGE( IDC_EDIT_INSTRUMENT_SN, &CResetPasswordGeneratorDlg::OnEnChangeSNEdit )
	ON_EN_CHANGE( IDC_EDIT_USERNAME, &CResetPasswordGeneratorDlg::OnEnChangeUnameEdit )
END_MESSAGE_MAP()


// CResetPasswordGeneratorDlg message handlers

BOOL CResetPasswordGeneratorDlg::OnInitDialog()
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
	DatePickerCtrl.SetFormat(L"yyyy/MMM/dd HH:mm");
	SNEdit.Clear();
	SNEdit.SetReadOnly( false );
	UnameEdit.Clear();
	UnameEdit.SetReadOnly( false );
	PasswordTextDisplay.Clear();
	PasswordTextDisplay.SetReadOnly( true );
	GenerateBtn.EnableWindow( FALSE );

#ifdef _DEBUG
	cstrSN = "INSTRUMENTDEFAULT";
	SNEdit.SetWindowTextW( cstrSN );
	cstrUname= "factory_admin";
	UnameEdit.SetWindowTextW( cstrUname );
	GenerateBtn.EnableWindow( TRUE );
#endif

	// Set the date-time picker to display the current local time.
	// The user can override with the desired date for the password to be valid
	// and then we can convert that appropriately so we generate the right math.
	std::chrono::system_clock::time_point ctNow(std::chrono::system_clock::now());
	CTime tNow(std::chrono::system_clock::to_time_t(ctNow));

	// This will be the LOCAL time as initially shown
	DatePickerCtrl.SetTime( &tNow );

	UpdateData( FALSE );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CResetPasswordGeneratorDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CResetPasswordGeneratorDlg::OnPaint()
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
HCURSOR CResetPasswordGeneratorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#ifdef _DEBUG
#include <cstdarg>
#include <vector>

// requires at least C++11
const std::string vformat( const char* const zcFormat, ... )
{
	// initialize use of the variable argument array
	va_list vaArgs;
	va_start( vaArgs, zcFormat );

	// reliably acquire the size
	// from a copy of the variable argument array
	// and a functionally reliable call to mock the formatting
	va_list vaArgsCopy;
	va_copy( vaArgsCopy, vaArgs );
	const int iLen = std::vsnprintf( NULL, 0, zcFormat, vaArgsCopy );
	va_end( vaArgsCopy );

	// return a formatted string without risking memory mismanagement
	// and without assuming any compiler or platform specific behavior
	std::vector<char> zc( iLen + 1 );
	std::vsnprintf( zc.data(), zc.size(), zcFormat, vaArgs );
	va_end( vaArgs );
	return std::string( zc.data(), iLen );
}

uint64_t DayUnits ( std::chrono::system_clock::time_point timepoint, SecurityHelpers::timepoint_to_epoch_converter_t scale)
{
	uint64_t days = scale( timepoint );
	return days;
}
#endif

void CResetPasswordGeneratorDlg::OnBnClickedBtnGenerate()
{
	SNEdit.GetWindowText( cstrSN );
	UnameEdit.GetWindowText( cstrUname );

	if ( cstrSN.IsEmpty() || cstrUname.IsEmpty() )
	{
		MessageBox( L"User name and instrument serial number must be supplied.",
					L"Password generation error",
					MB_ICONERROR | MB_TASKMODAL | MB_OK );
		return;
	}

	cstrSN.MakeUpper();
	snStr = CT2A( cstrSN.GetString() );
	unameStr = CT2A( cstrUname.GetString() );

	std::string keyStr = unameStr;
	keyStr.append( snStr );

	CTime ctTime = {};
	SYSTEMTIME sysTime = {};

	DatePickerCtrl.GetTime( ctTime );
	DatePickerCtrl.GetTime( &sysTime );						// This gets a "raw" time with no inherent TZ info embedded, but is referenced to the local time of this application.  Add the tz Offset value to get to UTC.

	// Move "raw" time to UTC directly
	tm tmSys;
	tmSys.tm_year = sysTime.wYear - 1900;
	tmSys.tm_mon = sysTime.wMonth - 1;
	tmSys.tm_mday = sysTime.wDay;

	tmSys.tm_hour = sysTime.wHour;
	tmSys.tm_min = sysTime.wMinute;
	tmSys.tm_sec = sysTime.wSecond;

	// NOTE: The timebase for generation should ALWAYS the local day epoch.
	// Timepoint maniipulations must generate an epoch aligned with the local
	// 00:00 (midnight or 12:00 AM) to 23:59 (11:59 PM) day epoch.

	time_t ttTgt = _mkgmtime( &tmSys );
	std::chrono::system_clock::time_point tpTgt( std::chrono::system_clock::from_time_t( ttTgt ) );		// timepoint from entered date and time
#ifdef _DEBUG
	// For debugging, the value from the date-picker should closely match the 'now' time if
	// the date-picker is unmodified.  Copy to compare the original unadjusted value from
	// the picker, the adjusted value from the picker, and the 'now' value and the passwords
	// generated from those values.
	std::chrono::system_clock::time_point tpBase = tpTgt;												// copy base timepoint from entered date and time
	std::chrono::system_clock::time_point tpNow( std::chrono::system_clock::now() );					// local time
#endif

	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation( &tzi );

	int tzBias = -( static_cast<int>( tzi.Bias ) );
	tpTgt -= std::chrono::minutes( tzBias );

#ifdef _DEBUG
	// For debugging, generate the days since epoch-start for each of the potential time
	// input values.  For a selected calendar date, all the days value generated from the
	// original and adjusted date-picker and the 'now' date value should match, regardless
	// of the time value entered into the picker within the selected date (i.e. 12:01 AM
	// or 23:59 PM on 2023-06-01)
	uint64_t daysBase = DayUnits( tpBase, SecurityHelpers::HMAC_DAYS );
	uint64_t daysTgt = DayUnits( tpTgt, SecurityHelpers::HMAC_DAYS );
	uint64_t daysNow = DayUnits( tpNow, SecurityHelpers::HMAC_DAYS );

	// For a selected calendar date, all the passwords generated from all the input date
	// values should match, regardless of the time value entered into the picker within
	// the selected date (i.e. 12:01 AM or 23:59 PM on 2023-06-01)
	std::string passcodeBase = SecurityHelpers::GenerateHMACPasscode( tpBase, SecurityHelpers::HMAC_DAYS, keyStr );
	std::string passcodeTgt = SecurityHelpers::GenerateHMACPasscode( tpTgt, SecurityHelpers::HMAC_DAYS, keyStr );
	std::string passcodeNow = SecurityHelpers::GenerateHMACPasscode( tpNow, SecurityHelpers::HMAC_DAYS, keyStr );
	std::string passcodeStr2 = vformat( "Base: days - %d  passcode - %s   Tgt: days - %d  passcode - %s   Now: days - %d  passcode - %s", daysBase, passcodeBase.c_str(), daysTgt, passcodeTgt.c_str(), daysNow, passcodeNow.c_str() );
//	std::string passcodeStr2 = "Base: " + passcodeBase + "  Tgt: " + passcodeTgt + "  Now: " + passcodeNow;
#endif

	std::string passcodeStr = SecurityHelpers::GenerateHMACPasscode( tpTgt, SecurityHelpers::HMAC_DAYS, keyStr );
	PasswordTextDisplay.SetWindowTextW( CA2W( passcodeStr.c_str() ) );
}

void CResetPasswordGeneratorDlg::OnEnChangeSNEdit()
{
	static bool inSNEdit = false;

	// this may generate another change event...
	if (!inSNEdit)				// not already processing a change...
	{
		inSNEdit = true;		// prevent reentry due to this edit manipulation
		UpdateData( TRUE );
		SNEdit.GetWindowText( cstrSN );
		snStr = CT2A( cstrSN.GetString() );
		if ( cstrSN.IsEmpty() || cstrUname.IsEmpty() )
		{
			GenerateBtn.EnableWindow( FALSE );
		}
		else
		{
			GenerateBtn.EnableWindow( TRUE );
		}
		inSNEdit = false;
	}
}

void CResetPasswordGeneratorDlg::OnEnChangeUnameEdit()
{
	static bool inUNEdit = false;

	// this may generate another change event...
	if ( !inUNEdit )			// not already processing a change...
	{
		inUNEdit = true;		// prevent reentry due to this edit manipulation
		UpdateData( TRUE );
		UnameEdit.GetWindowText( cstrUname );
		unameStr = CT2A( cstrUname.GetString() );
		if ( cstrSN.IsEmpty() || cstrUname.IsEmpty() )
		{
			GenerateBtn.EnableWindow( FALSE );
		}
		else
		{
			GenerateBtn.EnableWindow( TRUE );
		}
		inUNEdit = false;
	}
}

