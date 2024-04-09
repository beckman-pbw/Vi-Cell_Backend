#include "stdafx.h"

#include <codecvt>
#include <locale>
#include <regex>

#include <boost/program_options.hpp>

#include "DataImporter.hpp"

#include "AuditLog.hpp"
#include "DataIntegrityChecks.hpp"
#include "DLLVersion.hpp"
#include "EnumConversion.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeResultsDataManager.hpp"
#include "InstrumentConfig.hpp"
#include "LoggerDB.hpp"
#include "WorkListDLL.hpp"

const std::string DefaultVersionToBeImported = "v1.0";

enum class CommandReturnCode
{
	Success = 0,
	ConflictingOptions,
	UnknownSoftwareVersion,
	IncorrectSoftwareVersion,
	DatabaseConnectFailed,
	ConfigurationImportFailed,
	DataImportFailed,
	FailedToClearDB,
	FailedToDeleteResultBinaries,
	FailedToDeleteConfigInstalledDirectory,
	FailedToDeleteConfigBackupDirectory,
	FailedToDeleteLogsDirectory,
	InstrumentDirNotFound,
	FailedToReadMetadata,
};

//*****************************************************************************
static int SetReturnCode (CommandReturnCode errorCode)
{
	return static_cast<int>(errorCode);
}

//*****************************************************************************
void setToNotReadOnly (std::string pathToChange)
{
	std::wstring wFilename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(pathToChange);
	bool bStatus = SetFileAttributes (wFilename.c_str(), FILE_ATTRIBUTE_NORMAL);
}

//*****************************************************************************
bool GetViCellVersion (const std::string& driveLetter, uint16_t& major, uint16_t& minor, uint16_t& build, uint16_t& privatePart)
{
	DWORD  verHandle = 0;
	UINT   size = 0;
	LPBYTE lpBuffer = NULL;
	bool retval = false;

	major = 0;
	minor = 0;
	build = 0;
	privatePart = 0;

	std::string filename = driveLetter + ":\\Instrument\\Software\\ViCellBLU_UI.exe";

	std::wstring wFilename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(filename);
	LPCWSTR szVersionFile = wFilename.c_str();

	DWORD verSize = GetFileVersionInfoSize (szVersionFile, &verHandle);
	if (verSize != NULL)
	{
		LPSTR verData = new char[verSize];

		if (GetFileVersionInfo(szVersionFile, verHandle, verSize, verData))
		{
			if (VerQueryValue (verData, TEXT("\\"), (VOID FAR * FAR*) & lpBuffer, &size))
			{
				if (size)
				{
					VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
					if (verInfo->dwSignature == 0xfeef04bd)
					{
						major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
						minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
						build = (verInfo->dwFileVersionLS >> 16) & 0xffff;
						privatePart = (verInfo->dwFileVersionLS >> 0) & 0xffff;

						retval = true;
					}
				}
			}
		}
		delete[] verData;
	}

	return retval;
}

//*****************************************************************************
static bool VerifyViCellBLUVersion (std::string driveLetter, std::string& version)
{
	uint16_t major;
	uint16_t minor;
	uint16_t build;
	uint16_t privatePart;
	if (!GetViCellVersion (driveLetter, major, minor, build, privatePart))
	{ // Couldn't find the ViCellBLU application.
		DataImporter::Log ("Unknown ViCellBLU version");
		return false;
	}

	version = boost::str (boost::format ("%d.%d") % major % minor);

	return true;
}

//*****************************************************************************
static bool FindVersionFile (const std::string& dirPath, std::string& versionStr)
{
	const std::string filePath = dirPath +"\\v*";
	
	WIN32_FIND_DATAA fileData;

	const HANDLE hFind = FindFirstFileA (filePath.c_str(), &fileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		versionStr = std::string(fileData.cFileName).substr(1);
		FindClose (hFind);

		try
		{
			double value = std::stod (versionStr);
		}
		catch (std::exception e)
		{
			// If the conversion is a invalid assume the default import version (v1.2).
			return false;
		}

		return true;
	}

	return false;
}

//*****************************************************************************
int main (int argc, char* argv[])
{
	/*
	 * Supported Functionality
	 *	Import CellHealth offline export v1.0 and later for Offline Analysis.
	 *
	 *
	 * Modes of Operation
	 * 	 *    
	 * Importing exported v1.0 data (zipfile) to v1.0 offline application.
	 *    uses "import_data.bat".
	 *    unzipped data is written to "% USERPROFILE% \AppData\Local\Temp\".
	 */

	// The default drive letter is "C".  Use -d to select a different drive.
	// The existence of [driver letter]:\Instrument\Software\ViCellBLU_UI.exe is verified.
	// The -o option is used when importing v1.3 data into v1.3 offline analysis s/w using the import_data.bat script.
	// The -i option is used to specify the instrument directory for testing.
	//   When the -o option is used, the -i option specifies the Instrument directory in "%USERPROFILE%/AppData/Local/Temp".
	//	   ex: when "-i Instrument_v13" is specified in the Debug properties the s/w will look for the v1.3 data to
	//     import in "%USERPROFILE%/AppData/Local/Temp/Instrument_v13" for offline analysis.
	//   When the -i option is used w/o the -o option the s/w will look for the v1.2 data to import in C:.
	//     ex: when "-i Instrument_v12" is specified in the Debug properties the s/w will look for the v1.2 data to 
	//     import in "C:/Instrument_v12".

	bool importForOffline = false;

	boost::program_options::options_description desc{ "Options" };
	desc.add_options()
		("help,h", "DataImporter: -d[drive letter] -o | -i[Instrument directory name]")
		("drive,d", boost::program_options::value<std::string>(), "Installation drive letter")
		("offline,o", boost::program_options::bool_switch(&importForOffline), "Import data for offline analysis")
		("serialnumber,s", boost::program_options::value<std::string>(), "Serial # from zip filename")
		("instrumentdir,i", boost::program_options::value<std::string>(), "Instrument directory name (only used for testing)");

	boost::program_options::variables_map varMap;
	boost::program_options::store (boost::program_options::parse_command_line (argc, argv, desc), varMap);
	boost::program_options::notify (varMap);

	std::string serialNumber;
	if (varMap.count("serialnumber") == 1)
	{
		serialNumber = varMap["serialnumber"].as<std::string>();
	}

	std::string driveLetter = "C";
	if (varMap.count("drive") == 1)
	{
		driveLetter = varMap["drive"].as<std::string>();
	}

	boost::system::error_code ec;
	Logger::L().Initialize (ec, driveLetter + ":\\Instrument\\Config\\DataImporter.info", "logger");
	Logger::L().SetAlwaysFlush (true);

	std::string dllVersion = GetDLLVersion();
	DataImporter::Log ("DataImporter v" + dllVersion);

	std::string curSwVersion;
	// Check that there is a ViCell BLU exe at the specified location.
	if (!VerifyViCellBLUVersion (driveLetter, curSwVersion))
	{
		return SetReturnCode (CommandReturnCode::UnknownSoftwareVersion);
	}

	// Set the default path.
	std::string instrumentDir = "Instrument";

	// Default version of the data to be imported.
	// See comment in the "#if 0" block further down in the code.
	std::string versionToImport = DefaultVersionToBeImported;

	std::string importPath;

	// Determine where the data to be imported exists.
	if (importForOffline)
	{
		// Import of data for offline analysis (v1.2 or v1.3 and later).
		// Get the directory where the unzipped files are located.
		// Get this path "%USERPROFILE%/AppData/Local/Temp/".
		TCHAR lpBuffer[MAX_PATH];
		DWORD status = GetTempPath (MAX_PATH, lpBuffer);
		std::wstring wstr(lpBuffer);
#pragma warning(push) // Save warning settings
// Disable warning: conversion from 'wchar_t' to 'const _Elem', possible loss of data.
// This does not affect the resultant string.
#pragma warning(disable : 4244)
		std::string str(wstr.begin(), wstr.end());
#pragma warning(pop)
		// Remove the drive letter, colon and slashes.  These are specified separately.
		importPath = str.substr (3);
	}

	if (varMap.count("instrumentdir") == 1)
	{ // Specify an alternate Instrument directory to use as source for testing 
	  // upgrading v1.2 data to v1.3.
		instrumentDir = varMap["instrumentdir"].as<std::string>();
	}

	instrumentDir = importPath + instrumentDir;

	DataImporter::Log ("Starting DataImporter...");

	// Now set the full Instrument path.
	HawkeyeDirectory::Instance().setDriveId (driveLetter);
	HawkeyeDirectory::Instance().setImportInstrumentDir (instrumentDir);
	HawkeyeDirectory::Instance().useImportInstrumentDir();
	
	// Find the version of the export for Offline Analysis to be imported, default is v1.2.
	// This is only applicable for import for Offline Analysis.
	if (importForOffline)
	{
#if 0
		// In ViCellBLU this code was used to differentiate between different versions of the software
		// to allow fixed to data caused by erroneous behavior of a particular version of the software.
		// As this is the first version of the CellHealth software this code is not needed and is
		// being kept for possible (probable?) future use.
		std::string versionStr;

		// Check for "vX.Y" file in the current Instrument directory.
		// This is only found in v1.3 and later exports for Offline Analysis.
		if (FindVersionFile ("\\" + instrumentDir, versionStr))
		{
			versionToImport = versionStr;
		}
		else
		{
			std::string encryptedFileName;
			bool isValidFilename = HDA_GetEncryptedFileName (HawkeyeDirectory::Instance().getFilePath(HawkeyeDirectory::FileType::DataImporter), encryptedFileName);
			if (isValidFilename && FileSystemUtilities::FileExists(encryptedFileName))
			{
				// The /Instrument/Config/DataImporter.einfo file exists...  this must be a v1.3 or later system.
				// If NOT in offline mode, then no import is required...  it will be added later as needed or will be done by updating the DB tables.
				
				versionToImport = curSwVersion;
			}
			else
			{
				// what to assume here???
				versionToImport = DefaultVersionToBeImported;
			}
		}
#endif

		if (varMap.count("instrumentdir") == 1)
		{
			std::string str = boost::str (boost::format ("TESTING: Using %s:\\%s as the %s data source for importing to %s...")
				% driveLetter
				% instrumentDir
				% versionToImport
				% dllVersion);
			DataImporter::Log (str);
		}
	}

	time_t sTime;
	time (&sTime);

	if (!InstrumentConfig::Instance().Initialize())
	{
		Logger::L().Log(MODULENAME, severity_level::critical, "Failed to connect to database");
		return SetReturnCode(CommandReturnCode::DatabaseConnectFailed);
	}
		
	if (!FileSystemUtilities::IsDirectory(HawkeyeDirectory::Instance().getInstrumentDir()))
	{
		DataImporter::Log (HawkeyeDirectory::Instance().getInstrumentDir() + " does not exist...", severity_level::error);
		return SetReturnCode (CommandReturnCode::InstrumentDirNotFound);
	}

	// Begin the import process...
	DataImporter dataImporter = {};

	// This only imports CellHealth v1.0 and later data exported for v1.0 and later data offline analysis.
	if (!importForOffline)
	{
		//NOTE: keep for possible future use: PerformDataIntegrityChecks();

		DataImporter::Log("Import - Done!");
		return SetReturnCode(CommandReturnCode::Success);
	}
		
	// This only needs to be done for importing data for offline analysis.
	// This data came from a zip file and brought into the offline analysis software by
	// by the *import_data.bat* script.
	// This operation will fail if pgadmin is connected to the database <or>
	// there are pgadmin users still connected to the database.
	DataImporter::Log ("Clearing database...");
	if (!DBApi::DbClearDb (HawkeyeConfig::Instance().get().database.dbName))
	{
		DataImporter::Log ("Unable to clear database...", severity_level::error);
		return SetReturnCode (CommandReturnCode::FailedToClearDB);
	}

	std::string str = boost::str (
		boost::format ("Importing %s data from s/n %s at %s into v%s database...")
			% versionToImport
			% serialNumber
			% HawkeyeDirectory::Instance().getInstrumentDir()
			% curSwVersion);
	DataImporter::Log (str);

	// Delete any files in the "Instrument/ResultsData/Images" directory.
	// This is probably only of interest in testing.
	HawkeyeDirectory::Instance().useInstrumentDir();
	FileSystemUtilities::RemoveAll (HawkeyeDirectory::Instance().getImagesBaseDir());
	HawkeyeDirectory::Instance().useImportInstrumentDir();

	if (!dataImporter.ReadMetaData (HawkeyeDirectory::Instance().getMetaDataFile()))
	{
		return SetReturnCode (CommandReturnCode::FailedToReadMetadata);
	}

	if (!dataImporter.ImportConfigurationForOfflineAnalysis())
	{
		DataImporter::Log ("Failed to import configuration for offline analysis...");
		return SetReturnCode (CommandReturnCode::DataImportFailed);
	}

	if (!dataImporter.ImportDataForOfflineAnalysis(serialNumber))
	{
		DataImporter::Log ("Failed to import data for offline analysis...");
		return SetReturnCode (CommandReturnCode::DataImportFailed);
	}
		

	time_t eTime;
	time (&eTime);
	auto duration = ChronoUtilities::ConvertToTimePoint< std::chrono::seconds> (eTime - sTime);
	std::string msg = "Data import successful... elapsed time " + ChronoUtilities::ConvertToString (duration, "%H:%M:%S");
	DataImporter::Log (msg);

	return SetReturnCode (CommandReturnCode::Success);
}

//*****************************************************************************
void DataImporter::Log (std::string str, severity_level severityLevel)
{
	Logger::L().Log (MODULENAME, severityLevel, str);
	std::cout << str << std::endl;
}
