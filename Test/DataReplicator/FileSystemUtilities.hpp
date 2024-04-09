#pragma once
#include <string>

#include "HawkeyeDirectory.hpp"
#include <boost/filesystem.hpp>
namespace FileSystemUtilities
{
	bool VerifyUserAccessiblePath(const std::string& path);
	bool ValidateFileNameAndPath(const std::string& filepath);
	bool CheckFileExtension(const std::string & filepath, const std::string& fileextn);
	bool CreateDirectories(std::string dirPath);
	bool FileExists(const std::string& filepath);
	bool IsDirectory(const std::string& dir);

    // Disk Space information
	uint64_t DiskFreeSpaceInBytes(const std::string& path);
	double DiskFreeSpaceInKiloBytes(const std::string& path);
	double DiskFreeSpaceInMegaBytes(const std::string& path);
	double DiskFreeSpaceInGigaBytes(const std::string& path);

	uint64_t DiskCapacityInBytes(const std::string& path);
	double DiskCapacityInKiloBytes(const std::string& path);
	double DiskCapacityInMegaBytes(const std::string& path);
	double DiskCapacityInGigaBytes(const std::string& path);
	// Device drive is considered for calculation (C:// drive)
	// Checks for (required memory + BUFFER size)
	bool IsRequiredFreeDiskSpaceInMBAvailable(const uint64_t& req_free_disk_size);

	bool RestoreFromBackup (HawkeyeDirectory::FileType fileType);
	bool SaveToBackup (HawkeyeDirectory::FileType fileType);

	bool RemoveAll(const std::string& path);

	uint64_t GetSizeoFTheFileorDirectory(const boost::filesystem::path& path);
	boost::filesystem::space_info GetSpaceInfoOfDisk(const std::string & path);
}
