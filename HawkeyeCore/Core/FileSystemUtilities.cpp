#include "stdafx.h"

#include <algorithm>
#include <cstdio>
#include <errhandlingapi.h>
#include <fstream>
#include <iostream>
#include <shellapi.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "DataConversion.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeUUID.hpp"
#include "Logger.hpp"
#include "SystemErrors.hpp"

#define POW_2_10 1024.0
// (Total Disk Capacity * 0.05) = XXXXX is the Buffer disk space 
// (500 Giga Bytes * 0.05) = 25 GB is the Buffer disk space
#define BUFFER_DISK_SPACE_PERCENTAGE 0.05 // 5% 

using boost::filesystem::exists;

static const char MODULENAME[] = "FileSystemUtilities";
namespace FileSystemUtilities
{
	//*****************************************************************************
	bool VerifyUserAccessiblePath(const std::string& path)
	{
		if (path.empty())
			return false;

		boost::filesystem::path givenPath(path);
		boost::filesystem::path accessiblePath(HawkeyeDirectory::Instance().getExportDir(true));

		boost::filesystem::path parentpath = {};

		if (boost::filesystem::is_directory(givenPath))
		{
			if (!HawkeyeConfig::Instance().get().hardwareConfig.stageController)
			{
				return true;
			}
			parentpath = givenPath;
		}
		else
		{
			parentpath = givenPath.parent_path();
		}

		auto baseDirectory = HawkeyeDirectory::Instance().getDriveId(); 

		if ( !givenPath.has_root_name() )
		{
			if (( givenPath.root_path() == givenPath.root_directory() ) &&
				( givenPath.root_directory() == "\\" ))
			{
				std::string newPath = HawkeyeDirectory::Instance().getDriveId();
				newPath.append( givenPath.string() );
				givenPath = newPath;
			}
		}
		if ( givenPath.is_relative() )
			return false;

		auto baseDir = givenPath.root_name().string();
		boost::to_upper(baseDirectory);
		boost::to_upper(baseDir);
		if (baseDir == baseDirectory)
		{
			if (exists(accessiblePath) && exists(parentpath))
			{
				std::string accesiblePathStr = boost::filesystem::canonical(accessiblePath).string();
				boost::to_lower (accesiblePathStr);
				std::string userGivenPathStr = boost::filesystem::canonical(parentpath).string();
				boost::to_lower (userGivenPathStr);
				
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

	//*****************************************************************************
	bool ValidateFileNameAndPath(const std::string& filepath)
	{
		if (filepath.empty())
			return false;

		boost::filesystem::path p(filepath);
		auto filename = p.filename().string();
		
		if (!p.has_filename() || !boost::filesystem::windows_name(filename))
			return false;

		// Check if the file path can be written to.
		std::ofstream handler(filepath);
		if (!handler.is_open())
			return false;

		handler.close();
		std::remove(filepath.c_str());

		return true;
	}

	//*****************************************************************************
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

	//*****************************************************************************
	bool CreateDirectories(const std::string& dirPath, bool ensure_empty)
	{
		if (dirPath.empty())
		{
			return false;
		}
		namespace fs = boost::filesystem;
		fs::path p(dirPath);
		boost::system::error_code ec;

		if (fs::exists(p) && fs::is_directory(p))
		{
			//Logger::L().Log (MODULENAME, severity_level::debug1, "Data directory already exists: " + p.generic_string());
			if (ensure_empty)
			{
				try
				{

					for (fs::directory_iterator end_iter, content_iter(p); content_iter != end_iter; ++content_iter)
					{
						fs::remove_all(content_iter->path());
					}
					Logger::L().Log (MODULENAME, severity_level::debug2, "emptied existing data directory: " + p.generic_string());
				}
				catch (...)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "failed to empty existing data directory: " + p.generic_string());
					ReportSystemError::Instance().ReportError (BuildErrorInstance(
						instrument_error::published_errors::instrument_storage_writeerror, 
						instrument_error::instrument_storage_instance::directory, 
						instrument_error::severity_level::warning));
					return false;
				}
			}

			return true;
		}

		fs::create_directories(p, ec);

		if (ec != boost::system::errc::success)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "failed to create data directory: " + p.generic_string() + " : " + ec.message());
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::published_errors::instrument_storage_writeerror, 
				instrument_error::instrument_storage_instance::directory, 
				instrument_error::severity_level::warning));
			return false;
		}

		return true;
	}

	//*****************************************************************************
	bool FileExists(const std::string & filepath)
	{
		boost::filesystem::path fp(filepath);
		return boost::filesystem::exists(fp);
	}

	//*****************************************************************************
	bool IsDirectory(const std::string & dir)
	{
		boost::filesystem::path p(dir);
		return boost::filesystem::is_directory(p);
	}

	//*****************************************************************************
	boost::filesystem::space_info GetSpaceInfoOfDisk(const std::string & path)
	{
		boost::filesystem::path p(path);
		boost::system::error_code ec;
		auto sp_info = boost::filesystem::space(p, ec);

		if (ec != boost::system::errc::success)
		{
			return {};
		}

		return sp_info;
	}

	//*****************************************************************************
	double getDiskFreeSpace(const std::string& path, eDiskInfoFormat formatType)
	{
		boost::filesystem::space_info sp_info = GetSpaceInfoOfDisk(path);
		double powerFactor = static_cast<double>(DataConversion::enumToRawVal(formatType));
		return sp_info.free / std::pow(POW_2_10, powerFactor);
	}

	//*****************************************************************************
	double getDiskCapacity(const std::string& path, eDiskInfoFormat formatType)
	{
		boost::filesystem::space_info sp_info = GetSpaceInfoOfDisk(path);
		double powerFactor = static_cast<double>(DataConversion::enumToRawVal(formatType));
		return sp_info.capacity / std::pow(POW_2_10, powerFactor);
	}

	//*****************************************************************************
	bool IsRequiredFreeDiskSpaceAvailable(const uint64_t & req_free_disk_size, eDiskInfoFormat formatType)
	{
		std::string system_disk_path = HawkeyeDirectory::Instance().getDriveId();
		double free_space_in_disk = getDiskFreeSpace(system_disk_path, formatType);
		double buffer_size = getDiskCapacity(system_disk_path, formatType) * BUFFER_DISK_SPACE_PERCENTAGE;
		return  free_space_in_disk > (req_free_disk_size + buffer_size);
	}

	//*****************************************************************************
	bool RemoveAll(const std::string& path)
	{
		boost::filesystem::path p(path);
		try
		{
			if (!(FileExists(path)))
			{
				Logger::L().Log (MODULENAME, severity_level::warning, "RemoveAll: <exit, " + path + " does not exist>");
				return true;
			}

			boost::system::error_code ec;
			auto count = boost::filesystem::remove_all(p, ec);

			Logger::L().Log (MODULENAME, severity_level::debug2, "RemoveAll: <deleted " + std::to_string(count) + " files>");
			if (ec != boost::system::errc::success)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "RemoveALL: <exit, failed to remove the path: " + path + " >");
				return false;
			}
		}
		catch (...)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "RemoveAll: <exit, exception occurred>");
			return false;
		}

		return true;
	}

	//*****************************************************************************
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

	//*****************************************************************************
	bool CopyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination)
	{
		namespace fs = boost::filesystem;

		// Check whether the function call is valid
		if (!fs::exists(source) || !fs::is_directory(source))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Source directory | " + source.string() + " | does not exist or is not a directory.");
			return false;
		}

		if (fs::exists(destination))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Destination directory | " + destination.string() + " | already exists.");
			return false;
		}
		// Create the destination directory
		if(!CreateDirectories(destination.string()))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to create directory | " + destination.string() + " |");
			return false;
		}


		// Iterate through the source directory
		for (fs::directory_iterator file(source); file != fs::directory_iterator(); ++file)
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (!CopyDir(current, destination / current.filename()))
				{
					return false;
				}
			}
			else
			{
				// Found file: Copy
				boost::system::error_code ec;
				boost::filesystem::path destination_path(destination / current.filename());
				fs::copy_file(current, destination_path, ec);

				if (ec)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "Failed to copy file | " + destination_path.generic_string());
					return false;
				}
			}
		}
		return true;
	}

	//*****************************************************************************
	bool Copy_File(const std::string& from, const std::string& to)
	{
		if (!boost::filesystem::exists(from) ||
			!boost::filesystem::is_regular_file(from))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "File does not exist |" + from + " |");
			return false;
		}

		boost::filesystem::path to_path(to);

		if (to.empty() || !to_path.has_filename())
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Destination file path is empty");
			return false;
		}


		if (!CreateDirectories(to_path.parent_path().string()))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to create the directory | " + to_path.parent_path().generic_string() + " |");
			return false;
		}

		boost::system::error_code ec;
		boost::filesystem::copy_file(from, to, ec);

		if (ec)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "Failed to copy the file From : |" + from + "| to |" + to + "| " + ec.message());
			return false;
		}

		return true;
	}

	//Return Codes From 7-Zip 
	//	0 No error
	//	1 Warning (Non fatal error(s)).For example, one or more files were locked by some other application, so they were not compressed.
	//	2 Fatal error
	//	7 Command line error
	//	8 Not enough memory for operation
	//	255 User stopped the process
	eArchiveErrorCode ArchiveData(const std::string& archive_file, const std::vector<std::string>& files_to_archive, bool use_absolute_path)
	{
		std::string archive_file_table_of_contents = boost::str(boost::format("Export_%s.txt")% HawkeyeUUID::Generate().to_string());
		std::ofstream output_file(archive_file_table_of_contents, std::ofstream::out);

		if (!output_file.is_open() || files_to_archive.empty())
		{
			return eArchiveErrorCode::eInvalidInputs;
		}

		std::ostream_iterator<std::string> output_iterator(output_file, "\n");
		std::copy(files_to_archive.begin(), files_to_archive.end(), output_iterator);
		output_file.close();

		auto ret_code = ArchiveData(archive_file, archive_file_table_of_contents, use_absolute_path);

		RemoveAll(archive_file_table_of_contents);

		return ret_code;
	}

	//*****************************************************************************
	// Run a windows command and return it's string output
	std::string execute(const std::string& command, uint32_t &retCode) {
		retCode = system((command + " > temp.txt").c_str());

		std::ifstream ifs("temp.txt");
		std::string ret{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };
		ifs.close(); // must close the inout stream so the file can be cleaned up
		if (std::remove("temp.txt") != 0) {
			perror("Error deleting temporary file");
		}
		return ret;
	}

	//*****************************************************************************
	eArchiveErrorCode ArchiveData(const std::string& archive_file, const std::string& export_files_input_list_file, bool use_absolute_path)
	{
		if (archive_file.empty() || export_files_input_list_file.empty())
			return eArchiveErrorCode::eInvalidInputs;

		std::string use_absolute_path_switch = use_absolute_path ? "-spf2" : "";

		std::string cmd = boost::str(boost::format("\"%s\"") % zip_utility);		
		std::string parameters = boost::str(boost::format(" a \"%s\" %s @%s\"")
			% archive_file
			% use_absolute_path_switch
			% export_files_input_list_file);

		// Convert std::string to LPCWSTR.
		std::wstring t_wstring = std::wstring(cmd.begin(), cmd.end());
		LPCWSTR cmdStr = t_wstring.c_str();
		std::wstring t_wstring2 = std::wstring(parameters.begin(), parameters.end());
		LPCWSTR paramStr = t_wstring2.c_str();

		SHELLEXECUTEINFO ShExecInfo = { 0 };
		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShExecInfo.hwnd = nullptr;
		ShExecInfo.lpVerb = nullptr;
		ShExecInfo.lpFile = cmdStr;
		ShExecInfo.lpParameters = paramStr;
		ShExecInfo.lpDirectory = nullptr;
		ShExecInfo.nShow = SW_HIDE;
		ShExecInfo.hInstApp = nullptr;
		ShellExecuteEx (&ShExecInfo);
		WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

		DWORD errorCode = GetLastError();		
		switch (errorCode)
		{
			case ERROR_SUCCESS:
				Logger::L().Log(MODULENAME, severity_level::debug1, "archiveData: zip successful");
				return eArchiveErrorCode::eComplete;

			case ERROR_NOT_ENOUGH_MEMORY:
				Logger::L().Log(MODULENAME, severity_level::error, "archiveData: zip failed, low memory");
				return eArchiveErrorCode::eLowMemory;

			case ERROR_NO_ASSOCIATION:
				Logger::L().Log(MODULENAME, severity_level::error, "archiveData: 7-zip not found");
				return eArchiveErrorCode::eFatalError;

			default:
				Logger::L().Log(MODULENAME, severity_level::error,
					boost::str(boost::format("archiveData:  zip failed, unknown error (%Xx)") % errorCode));
				return eArchiveErrorCode::eUnknown;
		}
	}
	
	//*****************************************************************************
	// fileExtension must in include the dot (".") along with the extension itself.
	//*****************************************************************************
	void GetFileListByExtension (std::string rootDir, std::string fileExtension, std::vector<std::string>& completeFileList)
	{
		for (boost::filesystem::recursive_directory_iterator end, file(rootDir); file != end; ++file) {
			if (file->path().extension().string() == fileExtension) {
				completeFileList.push_back (file->path().string());
			}
		}

	}

	//*****************************************************************************
	uint64_t DiskFreeSpaceInBytes (const std::string& path)
	{
		boost::filesystem::space_info sp_info = GetSpaceInfoOfDisk(path);
		return sp_info.free;
	}

	//*****************************************************************************
	uint64_t DiskCapacityInBytes (const std::string& path)
	{
		boost::filesystem::space_info sp_info = GetSpaceInfoOfDisk(path);
		return sp_info.capacity;
	}

	//*****************************************************************************
	double DiskCapacityInMegaBytes (const std::string& path)
	{
		uint64_t capacity_in_bytes = DiskCapacityInBytes(path);
		return capacity_in_bytes / (POW_2_10 * POW_2_10);
	}

	//*****************************************************************************
	double DiskFreeSpaceInMegaBytes (const std::string& path)
	{
		uint64_t  free_space_in_bytes = DiskFreeSpaceInBytes(path);
		return free_space_in_bytes / (POW_2_10 * POW_2_10);
	}

	//*****************************************************************************
	bool IsRequiredFreeDiskSpaceInMBAvailable (const uint64_t& req_free_disk_size)
	{
		std::string system_disk_path = HawkeyeDirectory::Instance().getDriveId();
		double free_space_in_disk = DiskFreeSpaceInMegaBytes(system_disk_path);
		double buffer_size = DiskCapacityInMegaBytes(system_disk_path) * BUFFER_DISK_SPACE_PERCENTAGE;
		return  free_space_in_disk > (req_free_disk_size + buffer_size);
	}
}
