#pragma once

#include <mutex>

#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "PtreeConversionHelper.hpp"

class ExportUtilities 
{
private:
	void ClearMetaData();

	pt::ptree mainMetaDataTree_;
	std::mutex mtx_MetaDataHandler;

public:
	struct ExportInputs
	{
		std::vector<std::tuple<uuid__t, uuid__t, uuid__t>> results_to_export;
		std::vector<uuid__t> rs_uuid_list;
		std::string export_metadata_file_path;
		std::string username;
		eExportImages image_selection;
		uint16_t nth_image;
	};

	ExportUtilities() {}
	virtual ~ExportUtilities();

	boost::property_tree::ptree& GetMetaData() { return mainMetaDataTree_; }
	bool ExportMetaData (ExportInputs& inputs_to_export, bool & is_cancelled, std::function<void(void)>& cancel_handler, std::vector<std::string>& files_to_export, std::vector<std::string>& audit_log_str, std::function<void(uuid__t)> per_result_completion_cb);

	bool ExportMeta_Start(ExportInputs& inputs_to_export);
	bool ExportMeta_Next(uint32_t index, std::vector<std::string>& auditLogEntries, std::vector<std::string>& files_to_export, uint32_t delayms);
	bool ExportMeta_Finalize();

	std::string metadataFilePath_;
};
