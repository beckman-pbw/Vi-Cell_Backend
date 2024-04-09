#pragma once


// TagWriteFailureDlg dialog
#define UWM_WRITEERR_ERROR_SINCE (WM_USER + 6)
#define UWM_WRITEERR_ERROR_AT (WM_USER + 11)

class TagWriteFailureDlg : public CDialogEx
{
	DECLARE_DYNAMIC(TagWriteFailureDlg)

public:
	TagWriteFailureDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~TagWriteFailureDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TAGWRITEFAILURE };
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

	afx_msg void OnBnClickedWriteError();
};
