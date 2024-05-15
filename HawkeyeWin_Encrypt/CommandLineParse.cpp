#include "stdafx.h"
#include "CommandLineParse.h"


CCommandLineParse::CCommandLineParse() :
	m_paramsHandled(0),
	m_switchHelp(0), 
	m_switchInfile(0),
	m_switchOutfile(0)
{
}

CCommandLineParse::~CCommandLineParse()
{
}


void CCommandLineParse::remove_path(CString& module_name)
{
	int last_index=0;
	int index = 0;
	//look for last instance of the backslash to extract the module (program) name. 
	do
	{
		index = module_name.Find('\\', index+1);// we need to look 1 character ahead, so we would not be suck in a loop.
		if (index >= 0)
			last_index = index;
	} while (index >= 0);
	last_index = module_name.GetLength() - last_index;
	module_name = module_name.Right(last_index-1);
}

void CCommandLineParse::HelpMsgToConsole(CString topmessage)
{
	DWORD nw;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), _T("\n\r")+topmessage, topmessage.GetLength(), &nw, NULL);

	TCHAR module_buffer[MAX_PATH] = { 0 };
	DWORD buf_size = sizeof(module_buffer) / sizeof(*module_buffer);
	
	GetModuleFileName(NULL, module_buffer, buf_size);
	CString module_name(module_buffer);
	remove_path(module_name);
	
	CString output = _T("\n\r") + module_name + _T(" [/h] [/infile] <file to Encrypt> [/outfile <output filename of Encrypted File>] [/hashfile <filename to store hash key>]\n\r");
	output += _T("\t /h : \t\t This help page. (/help /H /Help -h -Help) \n\r");
	output += _T("\t /infile: \t The name of the file that will be encrypted. The switch is not required when there is no output file name. (/i) \n\r");
	output += _T("\t /outfile: \t The name of the encrypted file. The input file name will be used when output name is not given. (/o) \n\r");
	output += _T("\t /hashfile: \t The filename where to store the generated hash key. \n\r");
	
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), output, output.GetLength(), &nw, NULL);
}

void CCommandLineParse::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	static CString lastparam;
	const CString strParam(pszParam);
	const TCHAR* lpszParse = pszParam;
	if (bFlag)
	{
		switch (*lpszParse)
		{
			case _T('h'):
				if(strParam.Find(_T("hashfile")) >= 0)
				{
					m_switchHashFile = true;
					m_paramsHandled = true;
					break;
				}
			case _T('H'):
			case _T('?'):
				m_switchHelp = true;
				m_paramsHandled = true;
			break;
			case _T('i'):
				m_switchInfile = true;
				m_paramsHandled = true;
			break;
			case _T('o'):
				m_switchOutfile = true;
				m_paramsHandled = true;
			break;
			default: 
				m_switchHelp = true; //unknown switch. output help page
				m_paramsHandled = true;
		}
		lastparam = CString(lpszParse);
	}
	else if(m_switchInfile && lastparam[0]=='i')
	{
		m_paramInfile = CString(lpszParse);
	}
	else if (m_switchOutfile && lastparam[0] == 'o')
	{
		m_paramOutfile = CString(lpszParse);
	}
	else if (m_switchHashFile && lastparam[0] == 'h')
	{
		m_paramHashfile = CString(lpszParse);
	}
	// If the last parameter has no flag, it is treated as the file name to be
	//  opened and the string is stored in the m_paramInfile member.
	if(bLast && !m_paramsHandled)
	{
		m_paramInfile = CString(lpszParse);
		m_switchInfile = true;
		m_paramsHandled = true;
	}

	//this will get default MFC parameters
	if (!m_paramsHandled)
		CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
	
}
