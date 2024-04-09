#include "stdafx.h"

#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "FileSystemUtilities.hpp"
//#include "Logger.hpp"
//#include "SystemErrors.hpp"

#define POW_2_10 1024.0
// (Total Disk Capacity * 0.05) = XXXXX is the Buffer disk space 
// (500 Giga Bytes * 0.05) = 25 GB is the Buffer disk space
#define BUFFER_DISK_SPACE_PERCENTAGE 0.05 // 5% 

using boost::filesystem::exists;

static const char MODULENAME[] = "FileSystemUtilities";
namespace FileSystemUtilities
{
	bool VerifyUserAccessiblePath(const std::string& path)
	{
		if (path.empty())
			return false;

		boost::filesystem::path givenPath(path);
		boost::filesystem::path accessiblePath(HawkeyeDirectory::Instance().getExportDir());

		auto parentpath = givenPath.parent_path();
		auto baseDirectory = HawkeyeDirectory::Instance().getDriveId();

		if (givenPath.is_relative())
			return false;

		auto baseDir = givenPath.root_name().string();
		boost::to_upper(baseDir);
		if (baseDir == baseDirectory)
		{
			if (exists(accessiblePath) && exists(parentpath))
			{
				std::string accesiblePathStr = boost::filesystem::canonical(accessiblePath).string();
				std::string userGivenPathStr = boost::filesystem::canonical(parentpath).string();
				size_t contains = userGivenPathStr.find(accesiblePathStr);

				if (contains != std::string::npos)
					return exists(parentpath);
			}
		}
		else
		{
			if (!parentpath.empty())
				return exists(parentpath);
		}

		return false;
	}

	bool ValidateFileNameAndPath(const std::string& filepath)
	{
		if (filepath.empty())
			return false;

		boost::filesystem::path p(filepath);
		if (!VerifyUserAccessiblePath(filepath) || !p.has_filename())
			return false;

		auto filename = p.filename().string();
		if (filename.empty() || !boost::filesystem::windows_name(filename))
			return false;

		// Check if the file path can be written to.
		std::ofstream handler(filepath);
		if (!handler.is_open())
			return false;

		handler.close();
		std::remove(filepath.c_str());

		return true;
	}

	bool CheckFileExtension(const std::string& filepath, const std::string& fileextn)
	{
		if (filepath.empty() || fileextn.empty())
			return false;

		boost::filesystem::path p(filepath);

		if (!p.has_filename() || !p.has_extension())
			return false;

		std::string extn = p.filename().extension().string();
		if (extn != fileextn)
			return false;

		return true;
	}

	bool CreateDirectories(std::string dirPath)
	{
		if (dirPath.empty())
		{
			return false;
		}

		boost::filesystem::path p(dirPath);
		boost::system::error_code ec;
		boost::filesystem::create_directories(p, ec);

		if (ec != boost::system::errc::success)
		{
			//Logger::L().Log (MODULENAME, severity_level::error, "failed to create data directory: " + p.generic_string() + " : " + ec.message());
			return false;
		}

		return true;
	}

	bool FileExists(const std::string & filepath)
	{
		boost::filesystem::path fp(filepath);
		return boost::filesystem::exists(fp);
	}

	bool IsDirectory(const std::string & dir)
	{
		boost::filesystem::path p(dir);
		return boost::filesystem::is_directory(p);
	}

	boost::filesystem::space_info GetSpaceInfoOfDisk(const std::string & path)
	{
		boost::filesystem::path p(path);
		boost::system::error_code ec;
		auto sp_info = boost::filesystem::space(p, ec);

		if (ec != boost::system::errc::success)
		{
			return{};
		}

		return sp_info;
	}

	uint64_t DiskFreeSpaceInBytes(const std::string& path)
	{
		boost::filesystem::space_info sp_info = GetSpaceInfoOfDisk(path);
		return sp_info.free;
	}

	double DiskFreeSpaceInKiloBytes(const std::string& path)
	{
		uint64_t  free_space_in_bytes = DiskFreeSpaceInBytes(path);
		return free_space_in_bytes / POW_2_10;
	}

	double DiskFreeSpaceInMegaBytes(const std::string& path)
	{
		uint64_t  free_space_in_bytes = DiskFreeSpaceInBytes(path);
		return free_space_in_bytes / (POW_2_10 * POW_2_10);
	}

	double DiskFreeSpaceInGigaBytes(const std::string& path)
	{
		uint64_t  free_space_in_bytes = DiskFreeSpaceInBytes(path);
		return free_space_in_bytes / (POW_2_10 * POW_2_10 * POW_2_10);
	}

	uint64_t DiskCapacityInBytes(const std::string& path)
	{
		boost::filesystem::space_info sp_info = GetSpaceInfoOfDisk(path);
		return sp_info.capacity;
	}

	double DiskCapacityInKiloBytes(const std::string& path)
	{
		uint64_t capacity_in_bytes = DiskCapacityInBytes(path);
		return capacity_in_bytes / POW_2_10;
	}

	double DiskCapacityInMegaBytes(const std::string& path)
	{
		uint64_t capacity_in_bytes = DiskCapacityInBytes(path);
		return capacity_in_bytes / (POW_2_10 * POW_2_10);
	}

	double DiskCapacityInGigaBytes(const std::string& path)
	{
		uint64_t capacity_in_bytes = DiskCapacityInBytes(path);
		return capacity_in_bytes / (POW_2_10 * POW_2_10 * POW_2_10);
	}

	bool IsRequiredFreeDiskSpaceInMBAvailable(const uint64_t & req_free_disk_size)
	{
		std::string system_disk_path = HawkeyeDirectory::Instance().getDriveId();
		double free_space_in_disk = DiskFreeSpaceInMegaBytes(system_disk_path);
		double buffer_size = DiskCapacityInMegaBytes(system_disk_path) * BUFFER_DISK_SPACE_PERCENTAGE;
		return  free_space_in_disk > (req_free_disk_size + buffer_size);
	}

	bool RestoreFromBackup(HawkeyeDirectory::FileType fileType)
	{
//Logger::L().Log (MODULENAME, severity_level::warning, "RestoreFromBackup: <restoring backup " + HawkeyeDirectory::Instance().getFilename(fileType) + ">");

		std::string fromFile = HawkeyeDirectory::Instance().getBackupFile(fileType);
		std::string   toFile = HawkeyeDirectory::Instance().getFilePath (fileType);

		if (!boost::filesystem::exists(fromFile) || !boost::filesystem::is_regular_file(fromFile) || boost::filesystem::file_size(fromFile) == 0)
		{
//Logger::L().Log (MODULENAME, severity_level::warning, "RestoreFromBackup: <restoring installed " + HawkeyeDirectory::Instance().getFilename(fileType) + ">");
			fromFile = HawkeyeDirectory::Instance().getInstalledFile(fileType);
		}

		if (!boost::filesystem::exists(fromFile))
		{
//ReportSystemError::Instance().ReportError (BuildErrorInstance(
//	instrument_error::instrument_configuration_restorefailed, 
//	0, 
//	instrument_error::severity_level::error));
			return false;
		}

		boost::filesystem::copy_file(boost::filesystem::path(fromFile), boost::filesystem::path(toFile), boost::filesystem::copy_option::overwrite_if_exists);

		if (!boost::filesystem::exists(toFile) || !boost::filesystem::is_regular_file(toFile) || boost::filesystem::file_size(toFile) == 0)
		{
//ReportSystemError::Instance().ReportError (BuildErrorInstance(
//	instrument_error::instrument_configuration_restorefailed, 
//	0, 
//	instrument_error::severity_level::error));
			return false;
		}

		return true;
	}

	bool SaveToBackup(HawkeyeDirectory::FileType fileType)
	{
		std::string fromFile = HawkeyeDirectory::Instance().getFilePath(fileType);
		std::string   toFile = HawkeyeDirectory::Instance().getBackupFile(fileType);

		boost::filesystem::copy_file(boost::filesystem::path(fromFile), boost::filesystem::path(toFile), boost::filesystem::copy_option::overwrite_if_exists);

		if (!boost::filesystem::exists(toFile) || !boost::filesystem::is_regular_file(toFile) || boost::filesystem::file_size(toFile) == 0)
		{
//ReportSystemError::Instance().ReportError (BuildErrorInstance(
//	instrument_error::instrument_configuration_savefailed, 
//	0, 
//	instrument_error::severity_level::error));
			return false;
		}

		return true;
	}

	bool RemoveAll(const std::string& path)
	{
		boost::filesystem::path p(path);
		try
		{
			if (!(FileExists(path)))
			{
				//Logger::L().Log (MODULENAME, severity_level::warning, "RemoveAll: <Exit Path does not exixts: " + path + ">");
				return false;
			}

			boost::system::error_code ec;
			auto count = boost::filesystem::remove_all(p, ec);

			//Logger::L().Log (MODULENAME, severity_level::debug1, "RemoveAll: <Number of files deleted # " + std::to_string(count) + " >");
			if (ec != boost::system::errc::success)
			{
				//Logger::L().Log (MODULENAME, severity_level::error, "RemoveALL: <Exit Failed to remove the path : " + path + " >");
				return false;
			}
		}
		catch (...)
		{
			///Logger::L().Log (MODULENAME, severity_level::error, "RemoveAll: An exception occurred >");
			return false;
		}

		return true;
	}


	uint64_t GetSizeoFTheFileorDirectory(const boost::filesystem::path& path)
	{
		uint64_t total_size = 0;

		if (!boost::filesystem::exists(path))
		{
			std::cerr << "File not found: " << path << std::endl;
			return 0;
		}

		//Returns the size of the file
		if (boost::filesystem::is_regular_file(path))
		{
			total_size += boost::filesystem::file_size(path);
			return total_size;
		}

		//Returns the size of the directory (Iterates through all the sub directories till it finds no more directories)
		if (boost::filesystem::is_directory(path))
		{
			// Create an iterator 
			boost::filesystem::directory_iterator it(path);
			for (it; it != boost::filesystem::directory_iterator(); it++)
			{
				total_size += GetSizeoFTheFileorDirectory(*it);
			}
		}

		return total_size;
	}

}