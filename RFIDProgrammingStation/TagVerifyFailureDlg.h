#pragma once


// TagVerifyFailureDlg dialog
#define UWM_VERIFYERR_ERROR_SINCE (WM_USER + 7)
#define UWM_VERIFYERR_ERROR_AT (WM_USER + 10)

class TagVerifyFailureDlg : public CDialogEx
{
	DECLARE_DYNAMIC(TagVerifyFailureDlg)

public:
	TagVerifyFailureDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~TagVerifyFailureDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TAGVERIFYFAILURE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CBrush m_brush;

	//{{AFX_MSG(TagWriteFailureDlg)
	afx_msg LRESULT OnTagsSinceError(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTagsAtError(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	afx_msg void OnPaint();

public:
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);

	virtual void PostNcDestroy() override;
	virtual BOOL OnInitDialog() override;
	
	CString SLBL_ErrorAtTag_Value;
	CString SLBL_TagsSinceError_Value;

	afx_msg void OnBnClickedVerifyError();
};
