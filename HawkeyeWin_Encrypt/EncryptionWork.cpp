// EncryptionWork.cpp : implementation file
//

#include "stdafx.h"
#include "HawkeyeWin_Encrypt.h"
#include "EncryptionWork.h"
#include "SecurityHelpers.hpp"
#include "FirmwareDownload.hpp"

#include <boost/thread.hpp>
#include <boost/thread/scoped_thread.hpp>

// EncryptionWork

IMPLEMENT_DYNAMIC(EncryptionWork, CWnd)
UINT MyThreadProc(LPVOID pParam);

EncryptionWork::EncryptionWork() 
{
#ifndef _WIN32_WCE
	EnableActiveAccessibility();
#endif

}

EncryptionWork::~EncryptionWork()
{
}

void EncryptionWork::SetInfile(CString in)
{
	std::string std_str(CW2A(in.GetString(), CP_UTF8));
	_infile = std_str;
}

void EncryptionWork::SetOutFile(CString out)
{
	std::string std_str(CW2A(out.GetString(), CP_UTF8));
	_outfile = std_str;
}

CString EncryptionWork::GetHashKey() const
{
	return CString(_hashkey.c_str());
}

bool EncryptionWork::DoEncryption()
{
	return do_work();
}

bool EncryptionWork::open_files()
{
	_in_filestream.open(_infile, std::ios_base::in);
	if (!_in_filestream.is_open())
		return false;
	_out_filestream.open(_outfile, std::ios_base::out | std::ios_base::ate | std::ios_base::binary);
	if (!_out_filestream.is_open())
		return false;
	return true;
}

void EncryptionWork::close_files()
{
	_in_filestream.close();
	_out_filestream.close();
}


bool EncryptionWork::do_work()
{
	if (!open_files())return false;
	if (!SecurityHelpers::SecurityEncryptFile(_in_filestream, _out_filestream,
											  FIRMWARE_ENCRYPTION_VI,
											  FIRMWARE_ENCRYPTION_SALT, _hashkey))
	{
		CString message = _T("Error occurred.");
		return false;
	}
	close_files();
	return true;
	
}


BEGIN_MESSAGE_MAP(EncryptionWork, CWnd)
END_MESSAGE_MAP()



// EncryptionWork message handlers


