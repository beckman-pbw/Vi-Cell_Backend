#pragma once

#include <streambuf>

struct MemoryStreamBuf : std::streambuf {
	MemoryStreamBuf (char* base, std::size_t size) {
		this->setp(base, base + size);
		this->setg(base, base, base + size);
	}
	std::size_t written() const {
		return this->pptr() - this->pbase();
	}
	std::size_t read() const {
		return this->gptr() - this->eback();
	}
	
	//NOTE: save for future use...
	//int getline (std::string& str, const char delimiter)
	//{
	//	char ch;
	//	int charsRead = 0;
	//	do
	//	{
	//		if (this->snextc() == EOF)
	//		{
	//			return EOF;
	//		}
	//		ch = this->sgetc();
	//		str.append (&ch);
	//		charsRead++;
	//	} while (ch != delimiter);

	//	return charsRead;
	//}
};
