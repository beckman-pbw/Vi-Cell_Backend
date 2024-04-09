#pragma once

#include "boost/filesystem.hpp"
#include "boost/format.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/opencv.hpp"
#include <trap.h>


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the HAWKEYEDATAACCESS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// HAWKEYEDATAACCESS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef HAWKEYEDATAACCESS_EXPORTS
#define HAWKEYEDATAACCESS_API __declspec(dllexport)
#else
#define HAWKEYEDATAACCESS_API __declspec(dllimport)
#endif


#define GCM_TAG_SIZE 12 /*96 bits*/


const boost::property_tree::xml_writer_settings<std::string> xmlFileSettings('\t', 1);


/// Converts a string to a vector of unsigned char
/// text: string containing the text to convert
/// Return value: vector of unsigned char representing the string value passed in
std::vector<unsigned char> ConvertStringToVector(const std::string& text);

/// Determines if the path for the supplied file is valid. If it doesn't exist, it is created
/// filename: string value containing the file path to validate
/// Return value: boolean value indicating whether the path is valid
bool ValidateFilePath(const std::string& filename);

/// Determines if the supplied filename is valid
/// filename: string value containing the filename to validate
/// Return value: boolean value indicating whether the filename is valid
bool ValidateFilename(const std::string& filename);

///// Encrypts the contents of a file to a new file
///// inputfilename: pointer to char containing name of the file to encrypt
///// encryptedfilename: pointer to char containing the name of the encrypted file to create
///// Return value: boolean value indicating whether the function was successful
//bool EncryptDataFile (const char* inputFilename, const char* encryptedFilename);
//
///// Decrypts the contents of a file to a new file
///// encryptedfilename: pointer to char containing the name of the file to decrypt
///// outputfilename: pointer to char containing the name of the decrypted file to create
///// Return value: boolean value indicating whether the function was successful
//bool DecryptDataFile(const char* encryptedFilename, const char* outputFilename);

/// <summary>
/// Encrypts the given string and returns a string with only "0-9 and A-F"
/// </summary>
/// <param name="plaintext">The test to encrypt</param>
/// <param name="encryptedtext">the encrypted string</param>
/// <returns>True if the encryption succeeds, False if not</returns>
bool EncryptString(const std::string& plaintext, char*& encryptedtext);

/// <summary>
/// Decrypts the given string and returns the plain text
/// </summary>
/// <param name="encryptedtext">The text to decrypt</param>
/// <param name="plaintext">the decrypted text</param>
/// <returns>True if the decryption succeeds, False if not</returns>
bool DecryptString(const std::string& encryptedtext, char*& plaintext);

//bool DecryptDataFile (const char* encryptedFilename, std::vector<byte>& decryptedData);

extern "C"
{
	/// Saves the specified Mat image to the specified file. The extension of the filename determines the format for the file created
	/// filename: string value containing the name and location of the image file
	/// matimage: Mat object containing the image to save
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool HDA_WriteImageToFile(const std::string& filename, const cv::Mat matImage);

	/// Saves the specified Mat image to the specified file. The extension of the filename determines the format for the file created
	/// The file will be encrypted and saved with a .bin extension
	/// filename: string value containing the name and location of the image file
	/// matimage: Mat object containing the image to save
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool HDA_WriteEncryptedImageFile (const cv::Mat matImage, const std::string& filename);

	/// Reads a Mat image from the specified file.
	/// filename: string value containing the name and location of the image file
	/// matimage: Mat object to hold the returned image
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool ReadImageFromFile(const std::string& filename, cv::Mat& matImage);
	/// Reads a Gray scale Mat image from the specified file.
	/// filename: string value containing the name and location of the image file
	/// matimage: Mat object to hold the returned image
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool ReadGrayScaleImageFromFile(const std::string& filename, cv::Mat& matImage);
	
	/// Reads a Mat image from the specified file. The function will look for a .bin file with the same filename to decrypt prior to reading the contents.
	/// filename: string value containing the name and location of the image file
	/// matimage: Mat object to hold the returned image
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedImageFile (const std::string& filename, cv::Mat& matImage);

	///// Reads a Gray scale Mat image from the specified file. The function will look for a .bin file with the same filename to decrypt prior to reading the contents.
	///// filename: string value containing the name and location of the image file
	///// matimage: Mat object to hold the returned image
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool ReadGrayScaleImageFromEncryptedFile(const std::string& filename, cv::Mat& matImage);

	HAWKEYEDATAACCESS_API void HDA_PrintPtree(boost::property_tree::ptree& tree, int16_t level);

	/// Saves the specified ptree data to the specified file. The file extension determines the format for the data. 
	/// This function supports .xml and .info file types. The encrypted file will be saved with a .bin extension.
	/// filename: string value containing the name and location of the file
	/// tree: ptree object containing the data to save
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool HDA_WriteEncryptedPtreeFile (const boost::property_tree::ptree& tree, const std::string& filename);

	/// Reads ptree data from the specified file. The file extension determines the format of the data in the file. 
	/// This function supports .xml and .info file types. This function will look for a .bin file with the same filename to decrypt prior to reading the contents.
	/// filename: string value containing the name and location of the file
	/// tree: ptree object to hold the returned data
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedPtreeFile (const std::string& filename, boost::property_tree::ptree& tree);
	
	HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedTextFile (const std::string& filename, std::stringstream& text);

	HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedBinFile (const std::string& filename, std::vector<byte>& decryptedData);

	/// Saves the specified string data to the specified file. The encrypted file will be saved with a .bin extension.
	/// filename: string value containing the name and location of the file
	/// stringlist: vector of strings object containing the data to save
	/// Return value: boolean value indicating whether the function was successful
	//NOTE: not currently used...  save for future use...
	//HAWKEYEDATAACCESS_API bool HDA_WriteEncryptedStringListFile(const std::string& filename, const std::vector<std::string>& stringList);
	
	///// Reads string data from the specified file.
	///// filename: string value containing the name and location of the file
	///// stringlist: vector of strings object to hold the returned data
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool ReadStringListDataFromFile(const std::string& filename, std::vector<std::string>& stringList);
	
	/// Reads string data from the specified file. This function will look for a .bin file with the same filename to decrypt prior to reading the contents.
	/// filename: string value containing the name and location of the file
	/// stringlist: vector of strings object to hold the returned data
	/// Return value: boolean value indicating whether the function was successful
	//NOTE: not currently used...  save for future use...
	//HAWKEYEDATAACCESS_API bool HDA_ReadEncryptedStringListFile (const std::string& filename, std::vector<std::string>& stringList);

	/// Determines if the specified encrypted file exists. This function will search for a .bin file with the same filename
	/// filename: string value containing the name and location of the file
	/// Return value: boolean value indicating whether the function was successful
	HAWKEYEDATAACCESS_API bool HDA_EncryptedFileExists (const std::string& filename);

	HAWKEYEDATAACCESS_API bool HDA_GetEncryptedFileName(const std::string& originalFilename, std::string& encryptedFilename);

	///// Copies the specified file. Validation is performed on the fromfile prior to the copy operation and then on the tofile after the copy is created.
	///// fromfile: string value containing the name and location of the file to copy
	///// tofile: string value containing the name and location of the copied file
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool CopyAndValidateFile(const std::string& fromFile, const std::string& toFile);
	///// Makes a backup copy of an encrypted file to the same folder with a .bak extension.
	///// This function will look for a .bin file with the same filename to use for the copy
	///// filename: string value containing the name and location of the file
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool SaveEncryptedFileToBackup(const std::string& filename);
	///// Makes a backup copy of an encrypted file to the specified backup location.
	///// This function will look for a .bin file with the same filename to use for the copy
	///// filename: string value containing the name and location of the file
	///// backuppath: string value containing the name of the folder where the backup file will be saved.
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool SaveEncryptedFileToBackupLocation(const std::string& filename, const std::string& backupPath);
	///// Restores an encrypted file from its local backup copy in the same folder.
	///// This function will look for a .bak file with the same filename and restore it to a .bin file.
	///// filename: string value containing the name and location of the file
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool RestoreEncryptedFileFromBackup(const std::string& filename);
	///// Restores an encrypted file from its backup location.
	///// This function will look for a .bin file with the same filename in the backup location to restore.
	///// filename: string value containing the name and location of the file
	///// backuppath: string value containing the name of the folder where the backup file will be saved.
	///// Return value: boolean value indicating whether the function was successful
	//HAWKEYEDATAACCESS_API bool RestoreEncryptedFileFromBackupLocation(const std::string& filename, const std::string& backupPath);
	
	/// Encrypts the contents of a file to a new file
	/// filename: pointer to char containing name of the file to encrypt
	/// encryptedfilename: pointer to char containing the name of the encrypted file to create
	/// Return value: boolean value indicating whether the function was successful, 
	///               returns false if the "filename" or "encryptedfilename" is nullptr
	HAWKEYEDATAACCESS_API bool HDA_FileEncrypt (const char* filename, const char* encryptedfilename);
	HAWKEYEDATAACCESS_API bool HDA_EncryptFile (const unsigned char* data, size_t dataSize, const std::string& encryptedFilename);

	/// Decrypts the contents of a file to a new file
	/// encryptedfilename: pointer to char containing the name of the file to decrypt
	/// decryptedfilename: pointer to char containing the name of the decrypted file to create
	/// Return value: boolean value indicating whether the function was successful
	///               returns false if the "filename" or "encryptedfilename" is nullptr
	HAWKEYEDATAACCESS_API bool HDA_FileDecrypt (const char* encryptedfilename, const char* decryptedfilename);
	HAWKEYEDATAACCESS_API bool HDA_DecryptFile (const std::string& encryptedFilename, std::vector<byte>& decryptedData  /*uint8_t*& decryptedData, size_t& decryptedDataSize*/);

	HAWKEYEDATAACCESS_API void HDA_FreeCharBufferData (char* data);

	HAWKEYEDATAACCESS_API bool HDA_EncryptString (const std::string& plaintext, char*& encryptedtext);
	HAWKEYEDATAACCESS_API bool HDA_DecryptString (const std::string& encryptedtext, char*& plaintext);

}
