#pragma once
#include <string>

#include "HawkeyeDirectory.hpp"

namespace FileSystemUtilities
{
	bool VerifyUserAccessiblePath(const std::string& path);
	bool ValidateFileNameAndPath(const std::string& filepath);
	bool CheckFileExtension(const std::string & filepath, const std::string& fileextn);
	bool CreateDirectories(const std::string& dirPath, bool ensure_empty = false);
	bool FileExists(const std::string& filepath);
	bool IsDirectory(const std::string& dir);
	boost::filesystem::space_info GetSpaceInfoOfDisk(const std::string& path);
	bool IsRequiredFreeDiskSpaceInMBAvailable (const uint64_t& req_free_disk_size);

	enum class eDiskInfoFormat : uint8_t
	{
		eBytes = 0,      //1024^0
		eKiloBytes = 1,  //1024^1
		eMegaBytes = 2,  //1024^2
		eGigaBytes = 3,  //1024^3
	};

	enum class eArchiveErrorCode : int16_t
	{
		eComplete      =  0,
		eFatalError    = -1,
		eLowMemory     = -2,
		eInvalidInputs = -3,
		eUnknown       = -4,
	};

	// Disk Space information
	double getDiskFreeSpace(const std::string& path, eDiskInfoFormat formatType);
	double getDiskCapacity(const std::string& path, eDiskInfoFormat formatType);
	// Device drive is considered for calculation (C:// drive)
	// Checks for (required memory + BUFFER size)
	bool IsRequiredFreeDiskSpaceAvailable(const uint64_t& req_free_disk_size, eDiskInfoFormat formatType);
	bool RemoveAll(const std::string& path);
	uint64_t GetSizeoFTheFileorDirectory(const boost::filesystem::path& path);
	bool CopyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination);
	bool Copy_File(const std::string& from, const std::string& to);

	//Return Codes From 7-Zip 
	//	0 No error
	//	1 Warning (Non fatal error(s)).For example, one or more files were locked by some other application, so they were not compressed.
	//	2 Fatal error
	//	7 Command line error
	//	8 Not enough memory for operation
	//	255 User stopped the process
	eArchiveErrorCode ArchiveData(const std::string& archive_file, const std::vector<std::string>& files_to_archive, bool use_absolute_path = true);
	eArchiveErrorCode ArchiveData(const std::string& archive_file, const std::string& export_files_input_list_file, bool use_absolute_path = true);

	void GetFileListByExtension (std::string rootDir, std::string fileExtension, std::vector<std::string>& completeFileList);
}
