#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "HawkeyeError.hpp"
#include "HawkeyeServices.hpp"
#include "ResultDefinitionCommon.hpp"

class ExportData
{
public:
	enum class ExportSequence : uint16_t
	{
		eCreateFileNamesAndStagingDir = 0,
		//eCheckForConfigFiles,
		eExportMetadata,
		eCheckForDiskSpace,
		eArchiveData,
		eClearOnSuccess,
		eClearOnCancelOrFailure,
		eExportComplete
	};

	struct Params
	{
		std::string metadata_file_path;
		std::vector<std::string> data_files_export;
		std::vector<std::string> audit_log_str;
		std::string archive_file_path;
		HawkeyeError he;
	};

	struct Inputs
	{
		std::string username;
		std::string export_path;
		eExportImages image_selection;
		uint16_t nth_image;
		std::vector<uuid__t> rs_uuid_list;
		std::function<void(HawkeyeError, uuid__t)> progress_cb;
		std::function<void(HawkeyeError, std::string archived_filepath, std::vector<std::string>) > completion_cb;
	};

	ExportData (std::shared_ptr<HawkeyeServices> pDataManagerServices);
	void Export (
		ExportSequence exportState,
		std::shared_ptr<const Inputs> pInputs,
		std::shared_ptr<Params> pParams);
	HawkeyeError Cancel();

#ifndef DATAIMPORTER

	bool Export_Start(const Inputs& inputs);

	bool Export_MetaData(uint32_t index, uint32_t delayms);
	
	bool Export_IsStorageSpaceAvailable();
	bool Export_ArchiveData(const std::string& filename, char*& outname);
	bool Export_Cleanup(bool removeFile);
#endif

	static bool IsExportInProgress();
	std::vector<std::string> GetAuditEntries();

private:
	bool checkAvailableStorageSpace (const std::string& export_path, std::vector<std::string>& data_files_path_list);
	HawkeyeError archiveDataFiles (const std::string& archive_file, const std::vector<std::string>& data_files_to_export, bool& isCancelled);

	std::shared_ptr<HawkeyeServices> pDataManagerServices_;
	std::function<void(void)> cancelExportHandler_ = nullptr;
};
