#include "stdafx.h"

#include <map>

#include "ChronoUtilities.hpp"
#include "CommandParser.hpp"
#include "DLLVersion.hpp"
#include "EnumConversion.hpp"
#include "ExportData.hpp"
#include "ExportUtilities.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "ExportData";

static std::atomic<bool> exportInProgress_ = false;

const std::map<ExportData::ExportSequence, std::string>
EnumConversion<ExportData::ExportSequence>::enumStrings<ExportData::ExportSequence>::data =
{
	{ ExportData::ExportSequence::eCreateFileNamesAndStagingDir, std::string("eCreateFileNamesAndStagingDir") },
	{ ExportData::ExportSequence::eExportMetadata, std::string("eExportMetadata") },
	{ ExportData::ExportSequence::eCheckForDiskSpace, std::string("eCheckForDiskSpace") },
	{ ExportData::ExportSequence::eArchiveData, std::string("eArchiveData") },
	{ ExportData::ExportSequence::eClearOnSuccess, std::string("eClearOnSuccess") },
	{ ExportData::ExportSequence::eClearOnCancelOrFailure, std::string("eClearOnCancelOrFailure") },
	{ ExportData::ExportSequence::eExportComplete, std::string("eExportComplete") },
};

//*****************************************************************************
//TODO: ...  need to update to read records from DB???
//*****************************************************************************
bool ExportData::checkAvailableStorageSpace (const std::string& export_path, std::vector<std::string>& data_files_path_list)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "CheckAvailableMemoryToExportData: <enter>");

	uint64_t required_size_in_bytes = {};

	for (const auto& path : data_files_path_list)
	{
		required_size_in_bytes += FileSystemUtilities::GetSizeoFTheFileorDirectory(path);
	}

	if (HawkeyeDirectory::Instance().getDriveId() == boost::filesystem::path(export_path).root_name())
	{
		if (!FileSystemUtilities::IsRequiredFreeDiskSpaceAvailable(required_size_in_bytes, FileSystemUtilities::eDiskInfoFormat::eBytes))
			return false;
	}
	else
	{
		if (!(FileSystemUtilities::getDiskFreeSpace(export_path, FileSystemUtilities::eDiskInfoFormat::eBytes) > required_size_in_bytes))
			return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "CheckAvailableMemoryToExportData: <exit>");

	return true;
}

#ifndef DATAIMPORTER

//*****************************************************************************
ExportData::ExportData (std::shared_ptr<HawkeyeServices> pDataManagerServices)
	: pDataManagerServices_(pDataManagerServices)
{
}

//*****************************************************************************
void ExportData::Export (
	ExportSequence sequence,
	std::shared_ptr<const ExportData::Inputs> pExportDataInputs,
	std::shared_ptr<ExportData::Params> pExportDataParams)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Export: <enter, state: " + EnumConversion<ExportSequence>::enumToString(sequence) + ">");

	if (!pExportDataInputs || !pExportDataParams)
	{
		// Kill application 
		//
		// there should be a way to resolve this problem without killing the application
		//
		Logger::L().Log(MODULENAME, severity_level::critical, 
			"Export: (!pExportDataInputs || !pExportDataParams) assert false: " + 
			EnumConversion<ExportSequence>::enumToString(sequence) + ">");
		Sleep(250);		
		assert(false); // Kill the app
	}

	auto execute_next_state = [this](ExportSequence exportState,
		HawkeyeError he,
		std::shared_ptr<const ExportData::Inputs> pExportDataInputs,
		std::shared_ptr<ExportData::Params> pExportDataParams)
	{
		pExportDataParams->he = he;
		pDataManagerServices_->enqueueInternal([=]()
			{
				Export (exportState, pExportDataInputs, pExportDataParams);
			});
	};

	switch (sequence)
	{
		case ExportSequence::eCreateFileNamesAndStagingDir:
		{
			exportInProgress_ = true;

			std::string sDateTime = ChronoUtilities::ConvertToString(std::chrono::system_clock::now(), "%Y%m%d-%H%M%S");

			//Temporary path in export location to keep the temp copy of files during export operation.
			pExportDataParams->archive_file_path = boost::str(boost::format("%s\\%s_ViCELLBLU_SN%s.zip")
				% pExportDataInputs->export_path
				% sDateTime
				% HawkeyeConfig::Instance().get().instrumentSerialNumber.get());

			// Replace spaces with underscores in archive file name
			std::replace(pExportDataParams->archive_file_path.begin(), pExportDataParams->archive_file_path.end(), ' ', '_');

//TODO: could make sure that the first file in the list is the metadata file...
			pExportDataParams->metadata_file_path = HawkeyeDirectory::Instance().getMetaDataFile();

			// Add encrypted metadata filename to the list of files to export.
			std::string encrypted_path = {};
			if (HDA_GetEncryptedFileName (pExportDataParams->metadata_file_path, encrypted_path))
			{
				pExportDataParams->data_files_export.push_back (encrypted_path);
			}

			// Create a file that is the version of the export.
			std::string dllVersion = GetDLLVersion();

			// Add empty file with the name of the software version that created the export.
			std::fstream versionFile;
			std::string versionFilename = boost::str (boost::format ("%s\\v%s") % HawkeyeDirectory::Instance().getInstrumentDir() % dllVersion);
			versionFile.open (versionFilename, std::ios::out);
			if (!versionFile)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to create version file: " + versionFilename);
			}
			else
			{
				versionFile.close();
				pExportDataParams->data_files_export.push_back (versionFilename);
			}

			execute_next_state (ExportSequence::eExportMetadata, HawkeyeError::eSuccess, pExportDataInputs, pExportDataParams);
		}
		break;

		case ExportSequence::eExportMetadata:
		{
			auto cb = [=](uuid__t completed_id)
			{
				pExportDataInputs->progress_cb(HawkeyeError::eSuccess, completed_id);
			};

			ExportUtilities::ExportInputs inputs_to_export = {};
			inputs_to_export.username = pExportDataInputs->username;
			inputs_to_export.image_selection = pExportDataInputs->image_selection;
			inputs_to_export.nth_image = pExportDataInputs->nth_image;
			inputs_to_export.export_metadata_file_path = pExportDataParams->metadata_file_path;
			inputs_to_export.rs_uuid_list = pExportDataInputs->rs_uuid_list;

			ExportUtilities metaData = {};
			metaData.metadataFilePath_ = inputs_to_export.export_metadata_file_path;
			bool is_cancelled = false;

			if (!metaData.ExportMetaData(inputs_to_export, is_cancelled, cancelExportHandler_, pExportDataParams->data_files_export, pExportDataParams->audit_log_str, cb))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "exportData <exit -Failed to Create the Metadata>");
				execute_next_state(ExportSequence::eClearOnCancelOrFailure, HawkeyeError::eStorageFault, pExportDataInputs, pExportDataParams);
				return;
			}

			// Trigger completion if data export is cancelled.
			if (is_cancelled)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, "exportData <exit -Cancel Export Metadata Called>");
				execute_next_state (ExportSequence::eClearOnCancelOrFailure, HawkeyeError::eSuccess, pExportDataInputs, pExportDataParams);
				return;
			}

			execute_next_state (ExportSequence::eCheckForDiskSpace, HawkeyeError::eSuccess, pExportDataInputs, pExportDataParams);
		}
		break;

		case ExportSequence::eCheckForDiskSpace:
		{
			if (!checkAvailableStorageSpace (pExportDataInputs->export_path, pExportDataParams->data_files_export))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "exportData <exit - Low disk space>");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_storagenearcapacity,
					instrument_error::instrument_storage_instance::none,
					instrument_error::severity_level::error));
				execute_next_state (ExportSequence::eClearOnCancelOrFailure, HawkeyeError::eLowDiskSpace, pExportDataInputs, pExportDataParams);
				return;
			}

			execute_next_state (ExportSequence::eArchiveData, HawkeyeError::eSuccess, pExportDataInputs, pExportDataParams);
		}
		break;

		case ExportSequence::eArchiveData:
		{
			bool is_cancelled = false;

			// Remove duplicate file names from files_to_archive.
			std::sort(pExportDataParams->data_files_export.begin(), pExportDataParams->data_files_export.end());
			auto dup = std::unique(pExportDataParams->data_files_export.begin(), pExportDataParams->data_files_export.end());

			//NOTE: this is probablly not needed any more, kept just in case...
			if (dup != pExportDataParams->data_files_export.end())
				pExportDataParams->data_files_export.resize(std::distance(pExportDataParams->data_files_export.begin(), dup));

			auto he = archiveDataFiles (pExportDataParams->archive_file_path, pExportDataParams->data_files_export, is_cancelled);

			if (he != HawkeyeError::eSuccess || is_cancelled)
			{
				execute_next_state(ExportSequence::eClearOnCancelOrFailure, he, pExportDataInputs, pExportDataParams);
				return;
			}

			execute_next_state (ExportSequence::eClearOnSuccess, HawkeyeError::eSuccess, pExportDataInputs, pExportDataParams);
		}
		break;

		case ExportSequence::eClearOnSuccess:
		{
			execute_next_state (ExportSequence::eExportComplete, HawkeyeError::eSuccess, pExportDataInputs, pExportDataParams);
		}
		break;

		case ExportSequence::eClearOnCancelOrFailure:
		{
			pExportDataParams->audit_log_str.clear();                                                                           // Clear audit log list 
			FileSystemUtilities::RemoveAll(pExportDataParams->archive_file_path);                                               // Remove the inflight archive file
			pExportDataParams->archive_file_path = {};                                                                          // Clear archive file path
			execute_next_state (ExportSequence::eExportComplete, pExportDataParams->he, pExportDataInputs, pExportDataParams);
		}
		break;

		case ExportSequence::eExportComplete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "exportData <Triggered completion callback>");

			std::string encryptedFilename;
			HDA_GetEncryptedFileName (pExportDataParams->metadata_file_path, encryptedFilename);
			if (!FileSystemUtilities::RemoveAll (encryptedFilename))
			{
				Logger::L().Log (MODULENAME, severity_level::warning, 
					"exportData <failed to remove: " + HawkeyeDirectory::Instance().getMetaDataFile()+ ">");
			}

			if (!FileSystemUtilities::RemoveAll (HawkeyeDirectory::Instance().getResultBinariesDir()))
			{
				Logger::L().Log(MODULENAME, severity_level::warning, "Failed to delete ResultBinaries directory");
			}

			//Trigger the completion callback( it could be failure or success)
			pDataManagerServices_->enqueueInternal([=]()
				{
					exportInProgress_ = false;
					pExportDataInputs->completion_cb (pExportDataParams->he, pExportDataParams->archive_file_path, pExportDataParams->audit_log_str);
				});
		}
		break;

		default:
		{
			// Kill application 
			//
			// there should be a way to resolve this problem without killing the application
			//
			Logger::L().Log(MODULENAME, severity_level::critical,
				"Export: bad state assert false: " +
				EnumConversion<ExportSequence>::enumToString(sequence) + ">");
			Sleep(250);
			assert(false);
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "Export: <exit>");
}

//*****************************************************************************
HawkeyeError ExportData::archiveDataFiles (const std::string& archive_file, const std::vector<std::string>& data_files_to_export, bool& isCancelled)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "archiveDataFiles <enter>");

	isCancelled = false;

	//Archive all the data files listed in the file export_file_table_of_contents.
	auto ret_code = FileSystemUtilities::ArchiveData (archive_file, data_files_to_export);

	Logger::L().Log (MODULENAME, severity_level::debug1, "archiveDataFiles <Return code : " 
		+ std::to_string(std::underlying_type<FileSystemUtilities::eArchiveErrorCode>::type(ret_code)) + ">");

	if (ret_code == FileSystemUtilities::eArchiveErrorCode::eComplete)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "archiveDataFiles <exit -Successfully completed>");
		return HawkeyeError::eSuccess;
	}
	else if (ret_code == FileSystemUtilities::eArchiveErrorCode::eFatalError)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "archiveDataFiles <exit - Low disk space OR Fatal error>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::result,
			instrument_error::severity_level::warning));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_storagenearcapacity,
			instrument_error::instrument_storage_instance::none,
			instrument_error::severity_level::warning));
		return HawkeyeError::eStorageFault;
	}
	else if (ret_code == FileSystemUtilities::eArchiveErrorCode::eLowMemory)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "archiveDataFiles <exit - Low disk space>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_storagenearcapacity,
			instrument_error::instrument_storage_instance::none,
			instrument_error::severity_level::warning));
		return HawkeyeError::eLowDiskSpace;
	}
	else //Any other failure return code
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("archiveDataFiles <exit - Failed to archive the data, error_code: %ld>") % (int32_t)ret_code));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::result,
			instrument_error::severity_level::warning));
		return HawkeyeError::eStorageFault;
	}

	assert(false);
}

#endif

//*****************************************************************************
HawkeyeError ExportData::Cancel()
{
	if (!cancelExportHandler_)
		return HawkeyeError::eNotPermittedAtThisTime;

	cancelExportHandler_();
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool ExportData::IsExportInProgress()
{ 	
	return (exportInProgress_);
}


#ifndef DATAIMPORTER

static ExportUtilities s_metaData = {};
static ExportData::Params s_exportDataParams = {};
static ExportUtilities::ExportInputs s_inputs_to_export = {};
static std::string s_exportPath = {};


//*****************************************************************************
bool ExportData::Export_Start(const Inputs& inputs)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Start: <enter>");
	exportInProgress_ = true;

	std::string sDateTime = ChronoUtilities::ConvertToString(std::chrono::system_clock::now(), "%Y%m%d-%H%M%S");

	//Temporary path in export location to keep the temp copy of files during export operation.
	s_exportDataParams.archive_file_path = boost::str(boost::format("%s\\%s_CydemVTCHM_SN%s.zip")
		% inputs.export_path
		% sDateTime
		% HawkeyeConfig::Instance().get().instrumentSerialNumber.get());

	// Replace spaces with underscores in archive file name
	std::replace(s_exportDataParams.archive_file_path.begin(), s_exportDataParams.archive_file_path.end(), ' ', '_');

	//TODO: could make sure that the first file in the list is the metadata file...
	s_exportDataParams.metadata_file_path = HawkeyeDirectory::Instance().getMetaDataFile();

	// Add encrypted metadata filename to the list of files to export.
	std::string encrypted_path = {};
	if (HDA_GetEncryptedFileName(s_exportDataParams.metadata_file_path, encrypted_path))
	{
		s_exportDataParams.data_files_export.push_back(encrypted_path);
	}

	// Create a file that is the version of the export.
	std::string dllVersion = GetDLLVersion();

	// Add empty file with the name of the software version that created the export.
	std::fstream versionFile;
	std::string versionFilename = boost::str(boost::format("%s\\v%s") % HawkeyeDirectory::Instance().getInstrumentDir() % dllVersion);
	versionFile.open(versionFilename, std::ios::out);
	if (!versionFile)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Failed to create version file: " + versionFilename);
	}
	else
	{
		versionFile.close();
		s_exportDataParams.data_files_export.push_back(versionFilename);
	}

	
	s_inputs_to_export.username = inputs.username;
	s_inputs_to_export.image_selection = inputs.image_selection;
	s_inputs_to_export.nth_image = inputs.nth_image;
	s_inputs_to_export.export_metadata_file_path = s_exportDataParams.metadata_file_path;
	s_inputs_to_export.rs_uuid_list = inputs.rs_uuid_list;
	s_exportPath = inputs.export_path;

	s_metaData.metadataFilePath_ = s_inputs_to_export.export_metadata_file_path;
	if (s_metaData.ExportMeta_Start(s_inputs_to_export))
	{
		// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Start: <exit>");
		return true;
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Start: <exit ExportMeta_Start() FAIL>");
	exportInProgress_ = false;
	return false;

}

//*****************************************************************************
std::vector<std::string> ExportData::GetAuditEntries()
{
	return s_exportDataParams.audit_log_str;
}

//*****************************************************************************
bool ExportData::Export_MetaData(uint32_t index, uint32_t delayms)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_MetaData: <enter>");
	if (s_metaData.ExportMeta_Next(index, s_exportDataParams.audit_log_str, s_exportDataParams.data_files_export, delayms))
	{
		if (index >= (s_inputs_to_export.rs_uuid_list.size() - 1))
		{
			// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_MetaData: <exit> call ExportMeta_Finalize");
			return s_metaData.ExportMeta_Finalize();
		}
		return true;
	}
	Logger::L().Log(MODULENAME, severity_level::debug1, "Export_MetaData: <exit - FAIL>");
	return false;
}

//*****************************************************************************
bool ExportData::Export_IsStorageSpaceAvailable()
{
	return checkAvailableStorageSpace(s_exportPath, s_exportDataParams.data_files_export);
}

//*****************************************************************************
bool ExportData::Export_ArchiveData(const std::string& filename, char*& outname)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_ArchiveData <enter>");

	// Remove duplicate file names from files_to_archive.
	std::sort(s_exportDataParams.data_files_export.begin(), s_exportDataParams.data_files_export.end());
	auto dup = std::unique(s_exportDataParams.data_files_export.begin(), s_exportDataParams.data_files_export.end());

	//NOTE: this is probablly not needed any more, kept just in case...
	if (dup != s_exportDataParams.data_files_export.end())
		s_exportDataParams.data_files_export.resize(std::distance(s_exportDataParams.data_files_export.begin(), dup));

	bool is_cancelled = false;

	// leave for debug
	//Logger::L().Log(MODULENAME, severity_level::debug1, "Export_ArchiveData call archiveDataFiles : " + 
	//	s_exportDataParams.archive_file_path + " num files: " + std::to_string(s_exportDataParams.data_files_export.size()));

	std::string outfile = "";
	if (filename.length() > 0)
	{
		outfile = filename;
	}
	else 
	{
		outfile = s_exportDataParams.archive_file_path;	
	}
	DataConversion::convertToCharPointer(outname, outfile);

	auto he = archiveDataFiles(outfile, s_exportDataParams.data_files_export, is_cancelled);
	if (he != HawkeyeError::eSuccess || is_cancelled)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_ArchiveData <exit FAIL>");
		return false;
	}
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_ArchiveData <exit>");
	return true;
}

//*****************************************************************************
bool ExportData::Export_Cleanup(bool removeFile)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Cleanup <enter>");

	std::string encryptedFilename;
	HDA_GetEncryptedFileName(s_exportDataParams.metadata_file_path, encryptedFilename);
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Cleanup <remove: " + encryptedFilename + ">");

	if (!FileSystemUtilities::RemoveAll(encryptedFilename))
	{
		Logger::L().Log(MODULENAME, severity_level::warning,
			"Export_Cleanup <failed to remove: " + encryptedFilename + ">");
	}

	std::string bindir = HawkeyeDirectory::Instance().getResultBinariesDir();

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Cleanup <remove binaries: " + bindir + ">");

	if (!FileSystemUtilities::RemoveAll(bindir))
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "Export_Cleanup - Failed to delete ResultBinaries directory: " + bindir);
	}

	if (removeFile)
	{
		s_exportDataParams.audit_log_str.clear(); // Clear audit log list 
		FileSystemUtilities::RemoveAll(s_exportDataParams.archive_file_path); // Remove the inflight archive file
		s_exportDataParams.archive_file_path = {}; // Clear archive file path
	}

	exportInProgress_ = false;
	s_exportPath = "";
	s_exportDataParams.data_files_export.clear();
	s_exportDataParams.metadata_file_path = "";
	s_exportDataParams.he = HawkeyeError::eDeprecated;

	return true;
}


#endif