#pragma once
#include "afxwin.h"


class CCommandLineParse :
	public CCommandLineInfo
{
public:
	CCommandLineParse();
	virtual ~CCommandLineParse();

	BOOL m_paramsHandled;
	BOOL m_switchHelp;
	BOOL m_switchInfile;
	BOOL m_switchOutfile;
	BOOL m_switchHashFile;

	CString m_paramInfile;
	CString m_paramOutfile;
	CString m_paramHashfile;

	void HelpMsgToConsole(CString topmessage = _T(""));

private:
	void ParseParam(const TCHAR *pszParam, BOOL bFlag, BOOL bLast) override;
	void remove_path(CString& module_name);
};

