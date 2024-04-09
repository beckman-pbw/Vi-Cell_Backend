#pragma once

#include <string>
#include <afx.h>
#include <ATLComTime.h>
#include <vector>

std::string CStrToStdStr(const CString& c_string);
CString StdStrToCStr(const std::string& std_string);

COleDateTime StringToOleDateTime(const std::string& YYMMDD);
uint32_t OleDateTimeToEpochRef(const COleDateTime& odt);

bool ParseBarcodeString(const std::string& barcode, std::string& GTIN, std::string& MFG_date, std::string& EXP_date, std::string& LOT_number, std::string& FAILURE);

// Lot information is intended to be a string representation of a decimal number.
// Lot information MAY be padded with leading '0' characters - this will lead to incorrect
//  reading of the lot ID as an OCTAL number and incorrectly encoding it on the tag.
// This function explicitly assumes decimal formatting in the input string
uint32_t LotNumberToUINT32(const std::string& fromstring);

class FirmwareBinaryEntry
{
private:
	std::string GTIN;
	std::string Description;
	std::string BINFilename;
	std::string BINFileHash;
	std::vector<unsigned char> RFIDByteData;

public:
	FirmwareBinaryEntry() {};
	FirmwareBinaryEntry(const std::string& fromstring);

	bool SetFromString(const std::string& fromstring);

	bool isLoaded() { return !RFIDByteData.empty(); }
	std::string GetGTIN() { return isLoaded() ? GTIN : ""; }
	std::string GetDescription() { return isLoaded() ? Description : Description + " INVALID!!"; }
	std::string GetBINFilename() { return BINFilename; }
	std::string GetGBinFileHash() { return BINFileHash; }
	std::vector<unsigned char> GetRFIDContents(uint32_t expiration_date, uint32_t lot_number);
};


std::vector<std::string> ReadAndVerifyPayloadFileContents(const std::string& filename);

std::vector<FirmwareBinaryEntry> LoadFirmwarePayloads(const std::string& filename);
std::vector<FirmwareBinaryEntry> LoadFirmwarePayloads_f(const std::string& foldername);

#define EXPIRATION_FIELD_START 7
#define EXPIRATION_FIELD_LEN 4
#define LOTNUMBER_FIELD_START 17
#define LOTNUMBER_FIELD_LEN 4