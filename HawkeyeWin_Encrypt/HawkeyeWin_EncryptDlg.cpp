
// HawkeyeWin_EncryptDlg.cpp : implementation file
//

#include "stdafx.h"
#include "HawkeyeWin_Encrypt.h"
#include "HawkeyeWin_EncryptDlg.h"
#include "DlgProxy.h"
#include "afxdialogex.h"

#include "FirmwareDownload.hpp"

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


// CHawkeyeWin_EncryptDlg dialog


IMPLEMENT_DYNAMIC(CHawkeyeWin_EncryptDlg, CDialogEx);

CHawkeyeWin_EncryptDlg::CHawkeyeWin_EncryptDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_HAWKEYEWIN_ENCRYPT_DIALOG, pParent)
	, m_inFilename(_T(""))
	, m_stringHashKey(_T(""))
	, m_outFilename(_T(""))
{
	AfxInitRichEdit();
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = NULL;
}

CHawkeyeWin_EncryptDlg::~CHawkeyeWin_EncryptDlg()
{
	// If there is an automation proxy for this dialog, set
	//  its back pointer to this dialog to NULL, so it knows
	//  the dialog has been deleted.
	if (m_pAutoProxy != NULL)
		m_pAutoProxy->m_pDialog = NULL;
}

void CHawkeyeWin_EncryptDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_INFILE, m_inFilename);
	DDV_MaxChars(pDX, m_inFilename, 256);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_INFILE, m_browseInFile);
	DDX_Control(pDX, IDC_MFCEDITBROWSE_OUTFILE, m_browseOutFile);
	DDX_Control(pDX, IDC_BUTTON_ENCRYPT, m_buttonStartEncrypt);
	DDX_Text(pDX, IDC_EDIT_HASHKEY, m_stringHashKey);
	DDX_Text(pDX, IDC_MFCEDITBROWSE_OUTFILE, m_outFilename);
	DDX_Control(pDX, IDC_EDIT_HASHKEY, m_editHashKey);
}

BEGIN_MESSAGE_MAP(CHawkeyeWin_EncryptDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_MFCEDITBROWSE_INFILE, &CHawkeyeWin_EncryptDlg::OnEnChangeMfceditbrowseInfile)
	ON_BN_CLICKED(IDC_BUTTON_ENCRYPT, &CHawkeyeWin_EncryptDlg::OnBnClickedButtonEncrypt)
	ON_EN_CHANGE(IDC_MFCEDITBROWSE_OUTFILE, &CHawkeyeWin_EncryptDlg::OnEnChangeMfceditbrowseOutfile)
	ON_BN_CLICKED(IDC_BUTTON_COPY2CLIP, &CHawkeyeWin_EncryptDlg::OnBnClickedButtonCopy2clip)
END_MESSAGE_MAP()


// CHawkeyeWin_EncryptDlg message handlers

BOOL CHawkeyeWin_EncryptDlg::OnInitDialog()
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

	ShowWindow(SW_NORMAL);

	// TODO: Add extra initialization here
	m_browseInFile.EnableFileBrowseButton(_T("SREC"), _T("S-Record files|*.srec||"));
	m_browseOutFile.EnableFileBrowseButton(_T("BIN"), _T("Binary files|*.bin||"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHawkeyeWin_EncryptDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CHawkeyeWin_EncryptDlg::OnPaint()
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
HCURSOR CHawkeyeWin_EncryptDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CHawkeyeWin_EncryptDlg::OnClose()
{
	if (CanExit())
		CDialogEx::OnClose();
}

void CHawkeyeWin_EncryptDlg::OnOK()
{
	if (CanExit())
		CDialogEx::OnOK();
}

void CHawkeyeWin_EncryptDlg::OnCancel()
{
	if (CanExit())
		CDialogEx::OnCancel();
}

BOOL CHawkeyeWin_EncryptDlg::CanExit()
{
	// If the proxy object is still around, then the automation
	//  controller is still holding on to this application.  Leave
	//  the dialog around, but hide its UI.
	if (m_pAutoProxy != NULL)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}



void CHawkeyeWin_EncryptDlg::OnEnChangeMfceditbrowseInfile()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	this->UpdateData();
	m_outFilename = m_inFilename;
	m_outFilename.Replace(_T(".srec"), _T(".bin"));
	m_browseOutFile.SetWindowTextW(m_outFilename);
	m_encryption_work_.SetInfile(m_inFilename);
}


void CHawkeyeWin_EncryptDlg::OnBnClickedButtonEncrypt()
{
	// TODO: Add your control notification handler code here
	if(!m_encryption_work_.DoEncryption())
		AfxMessageBox(_T("Unable to encrypt s-record file!"), MB_OK);
	m_stringHashKey = m_encryption_work_.GetHashKey();
	m_editHashKey.SetWindowTextW(m_stringHashKey);
	this->UpdateData();
}


void CHawkeyeWin_EncryptDlg::OnEnChangeMfceditbrowseOutfile()
{
	m_encryption_work_.SetOutFile(m_outFilename);
}


void CHawkeyeWin_EncryptDlg::OnBnClickedButtonCopy2clip()
{
	if (OpenClipboard())
	{
		HGLOBAL hgClipBuffer = nullptr;
		std::size_t sizeInWords = m_stringHashKey.GetLength() + 1;
		std::size_t sizeInBytes = sizeInWords * sizeof(wchar_t);
		hgClipBuffer = GlobalAlloc(GHND | GMEM_SHARE, sizeInBytes);
		if (!hgClipBuffer)
		{
			CloseClipboard();
			return;
		}
		wchar_t *wgClipBoardBuffer = static_cast<wchar_t*>(GlobalLock(hgClipBuffer));
		wcscpy_s(wgClipBoardBuffer, sizeInWords, m_stringHashKey.GetBuffer());
		GlobalUnlock(hgClipBuffer);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hgClipBuffer);
		CloseClipboard();
	}
	else
	{
		AfxMessageBox(_T("Unable get ownership of the system clipboard!"), MB_OK);
	}
}
