#include <stdafx.h>
#include "Utilities.hpp"
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include "SecurityHelpers.hpp"
#include <fstream>

std::string CStrToStdStr(const CString& c_string)
{

	CT2CA pszConvertedString(c_string);

	return std::string(pszConvertedString);
}

CString StdStrToCStr(const std::string& std_string)
{
	return CString(std_string.c_str());
}

COleDateTime StringToOleDateTime(const std::string& YYMMDD)
{
	COleDateTime codt;
	codt.SetStatus(ATL::COleDateTime::DateTimeStatus::null);

	// 6 numeric values.

	if (YYMMDD.length() != 6)
	{
		return codt;
	}
	for (auto c : YYMMDD)
	{
		if (c < '0' || c > '9')
			return codt;
	}
	
	uint32_t year = 2000 + atoi(YYMMDD.substr(0, 2).c_str());
	uint32_t month = atoi(YYMMDD.substr(2, 2).c_str());
	uint32_t day = atoi(YYMMDD.substr(4, 2).c_str());

	switch (month)
	{
		case 9: // 30 days has September, 
		case 4: // April
		case 6: // June
		case 11:// and November
		{
			if (day == 0 || day > 30)
				return codt;
			break;
		}
		case 1: // All the rest have 31
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
		{
			if (day == 0 || day > 31)
				return codt;
			break;
		}
		case 2: // Except February which is complicated
			if (year % 4 == 0 &&
				(year % 100 == 0 &&
				 year % 400 != 0))
			{
				if (day == 0 || day > 29)
					return codt;
			}
			else
				if (day == 0 || day > 28)
					return codt;
			break;
		default:
			return codt;
	}

	codt.SetDate(year, month, day);
	return codt;
}

// Return as days since 1/1/1970
uint32_t OleDateTimeToEpochRef(const COleDateTime& odt)
{
	COleDateTime epoch;
	epoch.SetDate(1970, 1, 1);

	// COleDateTime actually represents as float value (as days since 12/30/1899) so some simple subtraction works.
	return uint32_t(static_cast<float>(odt) - float(epoch));
}

bool ParseBarcodeString(const std::string& barcode, std::string& GTIN, std::string& MFG_date, std::string& EXP_date, std::string& LOT_number, std::string& FAILURE)
{
	/*  0    0    1    1    2    2    3    3    4    4
	    0    5    0    5    0    5    0    5    0    5
	    01123234345456561116111817101118107654321...
		--              --      --      --
		| --------------| ------| ------| -------...
		| |             | |     | |     | |___Lot Number (alphanum, varible length) (34,0)
		| |             | |     | |     |___Lot Number Identifier ("10") (32,2)
		| |             | |     | |___Expiration Date (YYMMDD) (26,6)
		| |             | |     |___Expiration Date Identifier ("17") (24,2)
		| |             | |___Manufacture Date (YYMMDD) (18,6)
		| |             |___Manufacture Date Identifier ("11") (16,2)
		| |___GTIN code (2,14)
		|___Packaging Level Identifier ("01") (0,2)
	*/

	GTIN.clear();
	MFG_date.clear();
	EXP_date.clear();
	LOT_number.clear();
	FAILURE.clear();

	if (barcode.length() < 35)
	{
		FAILURE = "Barcode too short (expected minimum 35 characters)\n\t";
		FAILURE += barcode;
		return false;
	}
	if (barcode.substr(0, 2) != "01")
	{
		FAILURE = "Invalid packaging level identifier (expected \"01\")\n\t";
		FAILURE += barcode.substr(0, 2);
		return false;
	}
	if (barcode.substr(16, 2) != "11")
	{
		FAILURE = "Invalid manufacture date identifier (expected \"11\")\n\t";
		FAILURE += barcode.substr(16, 2);
		return false;
	}
	if (barcode.substr(24, 2) != "17")
	{
		FAILURE = "Invalid expriation date identifier (expected \"17\")\n\t";
		FAILURE += barcode.substr(24, 2);
		return false;
	}
	if (barcode.substr(32, 2) != "10")
	{
		FAILURE = "Invalid lot number identifier (expected \"10\")\n\t";
		FAILURE += barcode.substr(32, 2);
		return false;
	}

	GTIN = barcode.substr(2, 14);
	MFG_date = barcode.substr(18, 6);
	EXP_date = barcode.substr(26, 6);
	LOT_number = barcode.substr(34, barcode.length());
	return true;
}

uint32_t LotNumberToUINT32(const std::string& fromstring)
{
	// Input: assume a decimal-format string.  Leading '0' characters will be ignored.
	//        alpha characters will be ignored (for now)

	auto temp = boost::algorithm::trim_copy(fromstring);

	boost::remove_erase_if(temp, boost::algorithm::is_alpha());

	return std::stoul(temp, 0, 10);
}

FirmwareBinaryEntry::FirmwareBinaryEntry(const std::string& fromstring)
{
	SetFromString(fromstring);
}

bool FirmwareBinaryEntry::SetFromString(const std::string& fromstring)
{
	/*
	Format:
	[GTIN][DESCRIPTION][BIN FILENAME][BIN HASH]
	*/

	GTIN.clear();
	Description.clear();
	BINFilename.clear();
	BINFileHash.clear();
	RFIDByteData.clear();

	if ((std::count(fromstring.begin(), fromstring.end(), '[') != 4) ||
		(std::count(fromstring.begin(), fromstring.end(), ']') != 4))
		return false;

	std::string t_gtin, t_desc, t_filen, t_fileh;

	if (fromstring[0] != '[')
		return false;

	std::size_t openpos = 0;
	std::size_t closepos = fromstring.find(']', openpos);
	t_gtin = fromstring.substr(openpos + 1, closepos - (openpos + 1));


	openpos = fromstring.find('[', closepos);
	if (openpos != closepos + 1)
		return false;
	closepos = fromstring.find(']', openpos);
	t_desc = fromstring.substr(openpos + 1, closepos - (openpos + 1));

	openpos = fromstring.find('[', closepos);
	if (openpos != closepos + 1)
		return false;
	closepos = fromstring.find(']', openpos);
	t_filen = fromstring.substr(openpos + 1, closepos - (openpos + 1));

	openpos = fromstring.find('[', closepos);
	if (openpos != closepos + 1)
		return false;
	closepos = fromstring.find(']', openpos);
	t_fileh = fromstring.substr(openpos + 1, closepos - (openpos + 1));


	// Verify that file exists.
	if (!boost::filesystem::exists(t_filen) || !boost::filesystem::is_regular_file(t_filen))
		return false;

	// Check hash of file contents.
	std::ifstream binfile;
	binfile.open(t_filen, std::ifstream::in | std::ifstream::binary | std::ifstream::ate); // ::ios::ate to support initial tellg...

	if (!binfile.good())
	{
		binfile.close();
		return false;
	}

	std::size_t fsz = binfile.tellg();
	if (fsz > 256)
	{
		binfile.close();
		return false;
	}
	std::vector<unsigned char> t_vec;
	t_vec.resize(fsz, 0);
	binfile.seekg(0, binfile.beg);
	
	binfile.read((char*)(&t_vec[0]), fsz);
	binfile.close();

	std::string s = "BeckmanCoulterReagent";
	std::vector<unsigned char> salt;
	for (auto c : s)
		salt.push_back(c);
	
	std::string calc_hash = SecurityHelpers::CalculateHash(t_vec, salt, 1978);

	if (calc_hash != t_fileh)
		return false;


	this->GTIN = t_gtin;
	this->Description = t_desc;
	this->BINFilename = t_filen;
	this->BINFileHash = t_fileh;
	this->RFIDByteData = t_vec;

	return true;
}

std::vector<unsigned char> FirmwareBinaryEntry::GetRFIDContents(uint32_t expiration_date, uint32_t lot_number)
{
	std::vector<unsigned char> vec;
	if (!isLoaded())
		return vec;

	vec.resize(RFIDByteData.size());
	std::copy(RFIDByteData.begin(), RFIDByteData.end(), vec.begin());
	
	for (unsigned int i = 0; i < EXPIRATION_FIELD_LEN; i++)
	{
		unsigned char b = (expiration_date >> (i * 8)) & 0xFF;
		vec[EXPIRATION_FIELD_START + i] = b;
	}
	for (unsigned int i = 0; i < LOTNUMBER_FIELD_LEN; i++)
	{
		unsigned char b = (lot_number >> (i * 8)) & 0xFF;
		vec[LOTNUMBER_FIELD_START + i] = b;
	}

	return vec;
}

std::vector<std::string> ReadAndVerifyPayloadFileContents(const std::string& filename)
{
	/*
		File contents:
			HASH OF ALL LINES FOLLOWING NOT COUNTING NEW LINES\n
			[GTIN][DESCRIPTION][BIN FILENAME][BIN HASH]\n
			[GTIN][DESCRIPTION][BIN FILENAME][BIN HASH]\n
			[...]\n
	*/

	std::vector<std::string> contents;

	if (!boost::filesystem::exists(filename) || !boost::filesystem::is_regular_file(filename))
		return contents;

	std::ifstream file;
	file.open(filename, std::ifstream::in);
	if (!file.good())
		return contents;

	std::string hashval;
	std::getline(file, hashval);

	while (!file.eof())
	{
		std::string t;
		std::getline(file, t);
		if (!t.empty())
			contents.push_back(t);
	}
	file.close();

	std::string slt("BeckmanCoulterReagent");
	std::vector<unsigned char> salt(slt.begin(), slt.end());
	SecurityHelpers::RunningSHADigest sha(salt, 1978);

	for (auto s : contents)
	{
		sha.UpdateDigest(s);
	}

	std::string calc_hash = sha.FinalizeDigestToHash();

	if (calc_hash != hashval)
		contents.clear();

	return contents;
}

std::vector<FirmwareBinaryEntry> LoadFirmwarePayloads(const std::string& filename)
{
	std::vector<FirmwareBinaryEntry> vfbe;

	std::vector<std::string> vs = ReadAndVerifyPayloadFileContents(filename);

	// Invalid / empty?
	if (vs.empty())
		return vfbe;

	for (auto s : vs)
	{
		FirmwareBinaryEntry fbe(s);

		if (fbe.isLoaded())
			vfbe.push_back(fbe);
	}

	return vfbe;
}

std::vector<FirmwareBinaryEntry> LoadFirmwarePayloads_f(const std::string& foldername)
{
	std::vector<FirmwareBinaryEntry> vfbe;
	std::vector<std::string> text_entries;

	boost::filesystem::path p(foldername);

	if (!boost::filesystem::exists(p) || !boost::filesystem::is_directory(p))
		return vfbe;

	boost::filesystem::directory_iterator it(p);
	for (/**/; it != boost::filesystem::directory_iterator{}; it++)
	{
		boost::filesystem::path p2{ *it };
		std::string extension{ ".payload" };
		std::string f_ext = p2.extension().string();
		if (!boost::filesystem::is_regular_file(p2) || !p2.has_extension() || (p2.extension() != extension))
			continue;

		std::vector<std::string> vs = ReadAndVerifyPayloadFileContents(p2.string());
		for (auto s : vs)
		{
			text_entries.push_back(s);
		}
	}
	

	// FirmwareBinaryEntry assumes LOCAL files.  Need to temporarily change the working directory in case we're not there.
	boost::filesystem::path original_wd = boost::filesystem::current_path();
	boost::filesystem::current_path(p);

	for (auto s : text_entries)
	{
		FirmwareBinaryEntry fbe(s);
		if (fbe.isLoaded())
			vfbe.push_back(fbe);
	}
	//... and restore the previous directory.
	boost::filesystem::current_path(original_wd);
	
	return vfbe;
}