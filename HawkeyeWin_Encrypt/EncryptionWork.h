#pragma once
#include <string>
#include <fstream>
#include <boost/thread/thread.hpp>


// EncryptionWork

class EncryptionWork : public CWnd
{
	DECLARE_DYNAMIC(EncryptionWork)

public:
	EncryptionWork();
	virtual ~EncryptionWork();

	void SetInfile(CString in);
	void SetOutFile(CString out);
	CString GetHashKey() const;

	bool DoEncryption() ;

	bool do_work();
protected:
	DECLARE_MESSAGE_MAP()

private:
	std::string _infile;
	std::string _outfile;
	std::string _hashkey;

	std::ifstream _in_filestream;
	std::ofstream _out_filestream;

	bool open_files();
	void close_files();
};


