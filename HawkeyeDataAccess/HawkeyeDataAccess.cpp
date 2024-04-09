#include "stdafx.h"

#include <string>

#include <boost/algorithm/string/replace.hpp>
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "boost/uuid/random_generator.hpp"
#include <boost/property_tree/info_parser.hpp>

#include "HawkeyeDataAccess.h"
#include "MemoryStreamBuf.hpp"

// Crypto files.
#include "aes.h"
#include "files.h"
#include "gcm.h"
#include "hex.h"

#define ENCRYPTION_PASSCODE "11DA8861BFDB43BCBCEDEBB8B536EB3A"
#define ENCRYPTION_IV "7206C77A5C2F44F0FB7F1C4FD81865F8"

#define FILE_EXTENSION_PNG        ".png"
#define FILE_EXTENSION_XML        ".xml"
#define FILE_EXTENSION_INFO       ".info"
#define FILE_EXTENSION_TEXT       ".txt"
#define FILE_ENCRYPTED_BIN_EXTN   ".ebin"
#define FILE_ENCRYPTED_KEYWORD    ".e"

static std::vector<byte> encryption_iv = ConvertStringToVector(ENCRYPTION_IV);
static std::vector<byte> encryption_passcode = ConvertStringToVector(ENCRYPTION_PASSCODE);

//*****************************************************************************
std::vector<unsigned char> ConvertStringToVector (const std::string & text)
{
	std::vector<unsigned char> decoded;
	CryptoPP::HexDecoder decoder;

	decoder.Put((byte*)text.data(), text.size());
	decoder.MessageEnd();

	std::size_t size = decoder.MaxRetrievable();
	if (size && size <= SIZE_MAX)
	{
		decoded.resize(size);
		decoder.Get((byte*)decoded.data(), decoded.size());
	}

	return decoded;
}

//*****************************************************************************
bool ValidateFilePath(const std::string & filename)
{
	try
	{
		if (filename.empty())
			return false;

		char drive[_MAX_DRIVE];
		char path[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath_s(filename.c_str(), drive, path, fname, ext);

		std::string filePath = std::string(drive) + std::string(path);

		// See if the file path exists
		boost::filesystem::path fp(filePath);
		if (!boost::filesystem::is_directory(fp))
		{
			// It doesn't exist, create it
			boost::system::error_code ec;
			boost::filesystem::create_directory(fp, ec);

			if (ec != boost::system::errc::success)
				return false;
		}
	}
	catch (const std::exception ex)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
bool ValidateFilename(const std::string & filename)
{
	try
	{
		boost::filesystem::path p(filename);
		if (!ValidateFilePath(filename) || !p.has_filename())
			return false;

		auto name = p.filename().string();
		if (name.empty() || !boost::filesystem::windows_name(name))
			return false;
	}
	catch (const std::exception ex)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
static std::string GetEncryptedFileExtension (const std::string&originalFileExtn)
{
	if (originalFileExtn.empty())
		return FILE_ENCRYPTED_BIN_EXTN;

	auto pos = originalFileExtn.find(".", 0);
	if (pos == std::string::npos)
		return FILE_ENCRYPTED_KEYWORD + originalFileExtn;

	if (pos == 0)
	{
		auto tmpStr = originalFileExtn;
		return tmpStr.replace(pos, 1, FILE_ENCRYPTED_KEYWORD);
	}

	return FILE_ENCRYPTED_BIN_EXTN;
}

//*****************************************************************************
static std::string GetDecryptedFileExtension (const std::string& encryptedFileExtn)
{
	if (encryptedFileExtn.empty())
		return FILE_EXTENSION_TEXT;

	auto pos = encryptedFileExtn.find(FILE_ENCRYPTED_KEYWORD);
	if (pos == std::string::npos)
		return "." + encryptedFileExtn;

	if (pos == 0)
	{
		auto tmpStr = encryptedFileExtn;
		return tmpStr.replace(pos, 2, ".");
	}

	return  FILE_EXTENSION_TEXT;
}

//*****************************************************************************
static bool EncryptFile (const std::string& inputFilename, const std::string& encryptedFilename)
{
	if (!inputFilename.length() || !encryptedFilename.length())
		return false;

	try
	{
		CryptoPP::GCM< CryptoPP::AES >::Encryption encryptor;
		encryptor.SetKeyWithIV(encryption_passcode.data(), encryption_passcode.size(), encryption_iv.data(), encryption_iv.size());

		CryptoPP::FileSource fs (inputFilename.c_str(), true, new CryptoPP::AuthenticatedEncryptionFilter(encryptor, new CryptoPP::FileSink(encryptedFilename.c_str()), false, GCM_TAG_SIZE));
	}
	catch (const std::exception ex)
	{
		if (boost::filesystem::exists(encryptedFilename))
			boost::filesystem::remove(encryptedFilename, boost::system::error_code());

		return false;
	}

	return true;
}

//*****************************************************************************
static bool DecryptFile (const char* encryptedFilename, const char* outputFilename)
{
	if (encryptedFilename == nullptr || outputFilename == nullptr)
		return false;

	try
	{
		CryptoPP::GCM<CryptoPP::AES>::Decryption decryptor;
		decryptor.SetKeyWithIV (encryption_passcode.data(), encryption_passcode.size(), encryption_iv.data(), encryption_iv.size());

		CryptoPP::FileSource fs(encryptedFilename, true,
			new CryptoPP::AuthenticatedDecryptionFilter(decryptor, 
				new CryptoPP::FileSink(outputFilename), CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS, GCM_TAG_SIZE));
	}
	catch (const std::exception ex)
	{
		if (boost::filesystem::exists(outputFilename))
			boost::filesystem::remove(outputFilename, boost::system::error_code());

		return false;
	}

	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_FileEncrypt (const char* filename, const char* encryptedfilename)
{
	return EncryptFile (filename, encryptedfilename);
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_EncryptFile (const unsigned char* data, size_t dataSize, const std::string& encryptedFilename)
{
	if (data == nullptr || dataSize == 0 || encryptedFilename.empty())
		return false;

	try
	{
		CryptoPP::GCM< CryptoPP::AES >::Encryption encryptor;
		encryptor.SetKeyWithIV(encryption_passcode.data(), encryption_passcode.size(), encryption_iv.data(), encryption_iv.size());

		std::vector<byte> outData;
		outData.resize (dataSize + GCM_TAG_SIZE);
		CryptoPP::ArraySink sink (outData.data(), outData.size());

		CryptoPP::ArraySource (data, dataSize, true,
			new CryptoPP::AuthenticatedEncryptionFilter(encryptor, 
				new CryptoPP::Redirector(sink), false, GCM_TAG_SIZE));

		std::ofstream fout (encryptedFilename, std::ios::trunc | std::ios::out | std::ios::binary);
		fout.write ((char*)outData.data(), outData.size() * sizeof (char));
		fout.close ();
	}
	catch (const std::exception ex)
	{
		if (boost::filesystem::exists(encryptedFilename))
			boost::filesystem::remove(encryptedFilename, boost::system::error_code());

		return false;
	}

	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_FileDecrypt (const char* encryptedfilename, const char* decryptedfilename)
{
	return DecryptFile (encryptedfilename, decryptedfilename);
}

//*****************************************************************************
bool HAWKEYEDATAACCESS_API HDA_DecryptFile (const std::string& encryptedFilename, std::vector<byte>& decryptedData)
{
	bool retStat = false;

	if (encryptedFilename.empty()) {
		return retStat;
	}

	try
	{
		std::ifstream input;
		input.open (encryptedFilename, std::ifstream::binary);
		std::vector<byte> source = std::vector<byte>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());		
		input.close();

		CryptoPP::GCM<CryptoPP::AES>::Decryption decryptor;
		decryptor.SetKeyWithIV (encryption_passcode.data(), encryption_passcode.size(), encryption_iv.data(), encryption_iv.size());

		//std::vector<byte> outData;
		decryptedData.clear();
		decryptedData.resize (source.size());
		CryptoPP::ArraySink sink (decryptedData.data(), decryptedData.size());

		CryptoPP::ArraySource (source.data(), source.size(), true,
			new CryptoPP::AuthenticatedDecryptionFilter (decryptor, 
				new CryptoPP::Redirector(sink), CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS, GCM_TAG_SIZE));

		retStat = true;
	}
	catch (const std::exception ex)
	{
	}

	return retStat;
}

//*****************************************************************************
bool DecryptDataFile(const char* encryptedFilename, const char* outputFilename)
{
	if (encryptedFilename == nullptr || outputFilename == nullptr)
		return false;

	std::vector<byte> iv = ConvertStringToVector(ENCRYPTION_IV);
	std::vector<byte> key = ConvertStringToVector(ENCRYPTION_PASSCODE);

	try
	{
		CryptoPP::GCM< CryptoPP::AES >::Decryption decryptor;
		decryptor.SetKeyWithIV(key.data(), key.size(), iv.data(), iv.size());

		CryptoPP::FileSource fs(encryptedFilename, true, new CryptoPP::AuthenticatedDecryptionFilter(decryptor, new CryptoPP::FileSink(outputFilename), CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS, GCM_TAG_SIZE));
	}
	catch (const std::exception ex)
	{
		if (boost::filesystem::exists(outputFilename))
			boost::filesystem::remove(outputFilename, boost::system::error_code());

		return false;
	}

	return true;
}


/**************************************** Image File Handling ****************************************/
#pragma region Image File Handling

HAWKEYEDATAACCESS_API bool HDA_WriteImageToFile(const std::string & filename, const cv::Mat matImage)
{
	if (filename.empty() || matImage.empty())
	{
		return false;
	}

	try
	{
		// Must use the C language API as the C++ API causes the application to crash
		// when building for debug.
		// Refer to: http://answers.opencv.org/question/93302/save-png-with-imwrite-read-access-violation/

		// This C++ API crashes in debug mode.
		//	imwrite ("testimage.png", image);
		cvSaveImage (filename.c_str(), &IplImage(matImage));
	}
	catch (const std::exception ex)
	{
		return false;
	}

	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_WriteEncryptedImageFile (const cv::Mat matImage, const std::string& filename)
{
	std::string encryptedFilename;
	if (filename.empty() || !HDA_GetEncryptedFileName(filename, encryptedFilename))
		return false;

	try
	{
		// Convert image Mat to in-memory buffer.
		std::vector<unsigned char> imageBuf;
		bool bStatus = cv::imencode (".png", matImage, imageBuf, std::vector<int>());

		CryptoPP::GCM<CryptoPP::AES>::Encryption encryptor;
		encryptor.SetKeyWithIV (encryption_passcode.data(), encryption_passcode.size(), encryption_iv.data(), encryption_iv.size());

		std::vector<byte> outData;
		outData.resize (imageBuf.size() + GCM_TAG_SIZE);
		CryptoPP::ArraySink sink (outData.data(), outData.size());

		CryptoPP::ArraySource (imageBuf.data(), imageBuf.size(), true,
			new CryptoPP::AuthenticatedEncryptionFilter(encryptor, 
				new CryptoPP::Redirector(sink), false, GCM_TAG_SIZE));

		std::ofstream fout (encryptedFilename, std::ios::out | std::ios::binary);
		fout.write ((char*)outData.data(), outData.size() * sizeof (char));
		fout.close();
	}
	catch (const std::exception ex)
	{
		if (boost::filesystem::exists (encryptedFilename))
			boost::filesystem::remove (encryptedFilename, boost::system::error_code ());

		return false;
	}

	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool ReadImageFromFile (const std::string& filename, cv::Mat& matImage)
{
	if (filename.empty())
	{
		matImage = cv::Mat();
		return false;
	}

	matImage = cv::imread (filename, CV_LOAD_IMAGE_UNCHANGED);
	return matImage.empty() ? false : true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool ReadGrayScaleImageFromFile (const std::string & filename, cv::Mat & matImage)
{
	if (filename.empty())
	{
		matImage = cv::Mat();
		return false;
	}

	matImage = cv::imread (filename, CV_LOAD_IMAGE_GRAYSCALE);
	return !matImage.empty();
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedImageFile (const std::string& filename, cv::Mat& matImage)
{
	std::string encryptedFilename;
	if (filename.empty() || !HDA_GetEncryptedFileName(filename, encryptedFilename))
		return false;

	boost::filesystem::path encryptedFilePath (encryptedFilename);
	if (!boost::filesystem::exists(encryptedFilePath))
		return false;

	bool retStat = false;

	try
	{
		std::ifstream input;
		input.open (encryptedFilename, std::ifstream::binary);
		std::vector<byte> source = std::vector<byte>((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		input.close();

		CryptoPP::GCM<CryptoPP::AES>::Decryption decryptor;
		decryptor.SetKeyWithIV (encryption_passcode.data(), encryption_passcode.size(), encryption_iv.data(), encryption_iv.size());

		std::vector<byte> outData;
		outData.resize (source.size());
		CryptoPP::ArraySink sink (outData.data(), outData.size());

		CryptoPP::ArraySource (source.data(), source.size(), true,
			new CryptoPP::AuthenticatedDecryptionFilter (decryptor,
				new CryptoPP::Redirector(sink), CryptoPP::AuthenticatedDecryptionFilter::DEFAULT_FLAGS, GCM_TAG_SIZE));

		// Convert in-memory buffer to image Mat.
		matImage = cv::imdecode (outData, cv::IMREAD_UNCHANGED);

		retStat = true;
	}
	catch (const std::exception ex)
	{
	}

	return retStat;
}

#pragma endregion

/**************************************** Ptree File Handling ****************************************/
#pragma region Ptree File Handling

//*****************************************************************************
static bool ConvertToPtree (const std::string filetype, std::istream& dataStream, boost::property_tree::ptree& ptree)
{
	if (filetype == ".xml")
		boost::property_tree::read_xml (dataStream, ptree, boost::property_tree::xml_parser::trim_whitespace);
	else if (filetype == ".info" || filetype == ".cfg")
		boost::property_tree::read_info (dataStream, ptree);
	else
		return false;

	return true;
}

//*****************************************************************************
static bool ConvertFromPtree (const std::string filetype, const boost::property_tree::ptree& ptree, std::stringstream& dataStream)
{
	if (filetype == ".xml")
	{
		boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
		boost::property_tree::write_xml (dataStream, ptree, settings);
	}
	else if (filetype == ".info" || filetype == ".cfg")
	{
		boost::property_tree::info_writer_settings<char> settings('\t', 1);
		boost::property_tree::write_info (dataStream, ptree, settings);
	}
	else
		return false;

	// When writing to a stream (not a file stream) CR is not written (only LF is written).
	// To make editing in the "DataEncryptDecrypt" application easier add the CR back in.
	// This is not needed when writing to an output stream.
	std::string newStr = dataStream.str();
	boost::replace_all (newStr, "\n", "\r\n");
	dataStream.str (std::string());
	dataStream << newStr;

	return true;
}

//*****************************************************************************
static std::string indent (int level) {
	std::string s;
	for (int i = 0; i < level; i++) s += "   ";
	return s;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API void HDA_PrintPtree (boost::property_tree::ptree& tree, int16_t level) {


	if (tree.empty()) {
		std::cout << tree.data();

	} else {
		if (level >= 0) {
			std::cout << std::endl;
			std::cout << indent(level) << "{" << std::endl;
		}

		for (boost::property_tree::ptree::iterator pos = tree.begin(); pos != tree.end();) {
			std::cout << indent(level + 1) << pos->first << " ";
			HDA_PrintPtree(pos->second, level + 1);
			++pos;
			std::cout << std::endl;
		}
		if (level != -1) {
			std::cout << indent(level) << "}";
		}
	}
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_WriteEncryptedPtreeFile (const boost::property_tree::ptree& ptree, const std::string& filename)
{
	std::string encryptedFilename;
	if (filename.empty() || !HDA_GetEncryptedFileName(filename, encryptedFilename))
		return false;

	boost::filesystem::path fp(filename);
	std::string ext = boost::filesystem::extension(fp);

	std::stringstream ss;
	ConvertFromPtree (ext, ptree, ss);

	return HDA_EncryptFile ((unsigned char*)ss.str().c_str(), ss.str().length(), encryptedFilename);
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedPtreeFile (const std::string& filename, boost::property_tree::ptree& ptree)
{
	std::string encryptedFilename;
	if (filename.empty() || !HDA_GetEncryptedFileName(filename, encryptedFilename))
		return false;

	boost::filesystem::path encryptedFilePath (encryptedFilename);
	if (!boost::filesystem::exists(encryptedFilePath))
		return false;

	std::vector<byte> decryptedData;
	if (!HDA_DecryptFile (encryptedFilename, decryptedData))
		return false;

	MemoryStreamBuf dataBuf (reinterpret_cast<char*>(decryptedData.data()), decryptedData.size());
	std::istream dataStream(&dataBuf);

	boost::filesystem::path fp(filename);
	std::string ext = boost::filesystem::extension(fp);

	return ConvertToPtree (ext, dataStream, ptree);
}

#pragma endregion

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedTextFile (const std::string& filename, std::stringstream& textStream)
{
	std::string encryptedFilename;
	if (filename.empty() || !HDA_GetEncryptedFileName(filename, encryptedFilename))
		return false;

	boost::filesystem::path encryptedFilePath (encryptedFilename);
	if (!boost::filesystem::exists(encryptedFilePath))
		return false;

	std::vector<byte> decryptedData;
	if (!HDA_DecryptFile (encryptedFilename, decryptedData))
		return false;

	MemoryStreamBuf dataBuf (reinterpret_cast<char*>(decryptedData.data()), decryptedData.size());
	std::istream dataStream(&dataBuf);

	textStream << dataStream.rdbuf();

	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedBinFile (const std::string& filename, std::vector<byte>& decryptedData)
{
	std::string encryptedFilename;
	if (filename.empty() || !HDA_GetEncryptedFileName(filename, encryptedFilename))
		return false;

	boost::filesystem::path encryptedFilePath (encryptedFilename);
	if (!boost::filesystem::exists(encryptedFilePath))
		return false;

	return HDA_DecryptFile (encryptedFilename, decryptedData);
}

/**************************************** String Vector File Handling ****************************************/
#pragma region String Vector File Handling

//NOTE: not currently used...  save for future use...
////*****************************************************************************
//static bool WriteStringListDataToFile (const std::string& filename, const std::vector<std::string>& stringList)
//{
//	// Make sure the input parameters are valid
//	if (!ValidateFilename(filename))
//		return false;
//
//	if (stringList.empty())
//		return false;
//
//	// Write the data to the file
//	std::ofstream outStream(filename);
//	if (!outStream.is_open())
//		return false;
//
//	for (auto line : stringList)
//	{
//		outStream << line;
//	}
//
//	outStream.close();
//
//	return true;
//}

//NOTE: not currently used...  save for future use...
////*****************************************************************************
//HAWKEYEDATAACCESS_API bool HDA_WriteEncryptedStringListFile (const std::string& filename, const std::vector<std::string>& stringList)
//{
//	// Build the new file names
//	std::string tempFilename;
//	std::string encryptedFilename;
//	if (!GetNewFilenames(filename, tempFilename, encryptedFilename))
//		return false;
//
//	// Save the file first
//	if (!WriteStringListDataToFile(tempFilename, stringList))
//		return false;
//
//	// Encrypt the file
//	if (!EncryptFile(&tempFilename[0], &encryptedFilename[0]))
//		return false;
//
//	// Remove the temporary file leaving only the encrypted file
//	boost::filesystem::path tempFilePath(tempFilename);
//	if (boost::filesystem::exists(tempFilePath))
//		boost::filesystem::remove(tempFilePath);
//
//	return true;
//}

//NOTE: not currently used...  save for future use...
////*****************************************************************************
//static bool ReadStringListDataFromFile (const std::string& filename, std::vector<std::string>& stringList)
//{
//	// Validate the input file
//	boost::filesystem::path fp(filename);
//	if (!boost::filesystem::exists(fp))
//		return false;
//
//	// Open the input file
//	std::ifstream fileStream(filename);
//	if (!fileStream.is_open())
//		return false;
//
//	// Read the data into the string list
//	stringList.clear();
//	std::string readLine;
//	while (std::getline(fileStream, readLine))
//	{
//		stringList.push_back(readLine);
//	}
//
//	fileStream.close();
//
//	return true;
//}

//NOTE: not currently used...  save for future use...
////*****************************************************************************
//HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedStringListFile (const std::string& filename, std::vector<std::string>& stringList)
//{
//	// Build the file names
//	std::string tempFilename;
//	std::string encryptedFilename;
//	if (!GetNewFilenames(filename, tempFilename, encryptedFilename))
//		return false;
//
//	boost::filesystem::path encryptedFilePath(encryptedFilename);
//	if (!boost::filesystem::exists(encryptedFilePath))
//		return false;
//
//	// Decrypt the file
//	if (!DecryptFile(&encryptedFilename[0], &tempFilename[0]))
//		return false;
//
//	// Read the string list data from the decrypted file
//	bool success = ReadStringListDataFromFile(tempFilename, stringList);
//
//	// Remove the temporary file
//	boost::filesystem::path fp(tempFilename);
//	if (boost::filesystem::exists(fp))
//		boost::filesystem::remove(fp);
//
//	return success;
//}

#pragma endregion 


//*****************************************************************************
static bool FileExists (const std::string& filename)
{
	boost::filesystem::path fp(filename);
	return boost::filesystem::exists(fp) && boost::filesystem::is_regular(filename);
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_EncryptedFileExists (const std::string & filename)
{
	if (filename.empty())
		return false;

	std::string encryptedFilename;
	auto status = HDA_GetEncryptedFileName(filename, encryptedFilename);

	return status && FileExists(encryptedFilename);
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_GetEncryptedFileName (const std::string& originalFilename, std::string& encryptedFilename)
{
	if (originalFilename.empty())
		return false;

	std::string originalFilePath = originalFilename;
	boost::filesystem::path p(originalFilePath);

	if (!p.has_extension())
		return false;

	if (p.is_relative())
	{
		originalFilePath.clear();
		originalFilePath = boost::str(boost::format("%s\\%s") % boost::filesystem::current_path().string() % originalFilename);
	}

	char drive[_MAX_DRIVE];
	char path[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	_splitpath_s (originalFilePath.c_str(), drive, path, fname, ext);

	encryptedFilename = std::string(drive) + std::string(path) + std::string(fname) + GetEncryptedFileExtension(std::string(ext));

	if (!ValidateFilename(encryptedFilename))
		return false;

	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API void HDA_FreeCharBufferData (char* data)
{
	if (data != nullptr)
	{
		delete[] data;
		data = nullptr;
	}
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_EncryptString (const std::string& plaintext, char*& encryptedtext)
{
	if (plaintext.size() < 1)
		return false;

	try
	{
		CryptoPP::AES::Encryption aesEncryption(encryption_passcode.data(), encryption_passcode.size());
		CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, encryption_iv.data(), static_cast<int>(encryption_iv.size()));

		std::string strhash = "";
		CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(strhash));
		stfEncryptor.Put(reinterpret_cast<const unsigned char*>(plaintext.c_str()), plaintext.size());
		stfEncryptor.MessageEnd();

		// Now convert each byte into a two char pair representing the hex value
		// Exanple: ASCII 'N' is 0x4E so 'N' in the input string is converted to "4E" in the output
		size_t len = strhash.length();
		size_t biglen = (len * 2) + 1;
		encryptedtext = new char[biglen];
		memset(encryptedtext, 0, biglen);
		for (size_t j = 0; j < len; j++)
		{
			// Tricky code here - the 'size' of the buffer used for sprintf_s 
			// is the size from the current position to the end of the buffer 
			// not the full size of the buffer 
			size_t curridx = j * 2;
			size_t sizetoend = (biglen - curridx);
			sprintf_s(&encryptedtext[curridx], sizetoend, "%02X", static_cast<byte>(strhash[j]));
		}

	}
	catch (const std::exception ex)
	{
		return false;
	}
	return true;
}

//*****************************************************************************
HAWKEYEDATAACCESS_API bool HDA_DecryptString (const std::string& encryptedtext, char*& plaintext)
{
	if (encryptedtext.size() < 1)
		return false;

	try
	{
		// Convert the input string of 2 byte hex values into a single byte
		// Example: "4E" in the input string is converted into a single byte containing 0x4E or 'N'
		size_t len = encryptedtext.length();
		std::string hexstr;
		size_t cnt = (len & ~1); // Gotta be an even number since we are using two chars at once
		for (size_t j = 0; j < cnt; j += 2)
		{
			std::string byte = encryptedtext.substr(j, 2);
			char chr = (char)(int)strtol(byte.c_str(), NULL, 16);
			hexstr.push_back(chr);
		}

		CryptoPP::AES::Decryption aesDecryption(encryption_passcode.data(), encryption_passcode.size());
		CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, encryption_iv.data(), static_cast<int>(encryption_iv.size()));

		std::string strplain = "";
		CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(strplain));
		stfDecryptor.Put(reinterpret_cast<const unsigned char*>(hexstr.c_str()), hexstr.length());
		stfDecryptor.MessageEnd();

		len = strplain.length();
		plaintext = new char[len + 1];
		memset(plaintext, 0, (len + 1));
		strncpy_s(plaintext, len + 1, strplain.c_str(), _TRUNCATE);
	}
	catch (const std::exception ex)
	{
		return false;
	}

	return true;
}
