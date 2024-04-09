#pragma once

#include <DBif_Api.h>

#include "CellCountingOutputParams.h"
#include "DataConversion.hpp"
#include "HawkeyeError.hpp"

struct ImageDataForSResult
{
	uuid__t   uuid;
	system_TP timestamp;
};
typedef std::map<int, ImageDataForSResult> ImageDataForSResultMap;

class SResultData
{
public:
	static HawkeyeError Write(
		const uuid__t& sampleId, 
		const uuid__t& analysisId, 
		const CellCounterResult::SResult& sresult, 
		const ImageDataForSResultMap& imageDataForSResult,
		uuid__t& sresultId);

	static CellCounterResult::SResult FromDBStyle (const DBApi::DB_SResultRecord& dbSResultRecord);

private:
	static bool TransferConfigMap( ConfigParamList_t & dbCfgMap, std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>& srecCfgMap );
	static bool TransferCellIdentParamVector( v_IdentificationParams_t& dbMapV, v_IdentificationParams_t& srecMap );
	static bool TransferCellIdentParamVector( std::vector<DBApi::analysis_input_params_storage>& dbMapV, v_IdentificationParams_t& srecMap );
	static bool TransferBlobMapToVector( std::vector<DBApi::blob_info_pair>& dbMapV, std::map<Characteristic_t, float>& srecMap );
	static bool TransferBlobMapToVector( std::vector<DBApi::blob_info_storage>& dbMapV, std::map<Characteristic_t, float>& srecMap );
	static bool TransferBlobVector( std::vector<DBApi::blob_data_t>& dbBlobs, const std::vector<Blob::SBlobData>& srecBlobs );
	static bool TransferBlobVector( std::vector<DBApi::blob_data_storage>& dbBlobs, const std::vector<Blob::SBlobData>& blobs );
	static bool TransferClusterVector( std::vector<DBApi::cluster_data_t>& cluster_data_v, std::vector<CellCounterResult::SLargeClusterData> map_cluster_v );
	static bool TransferPeakMap( std::map<int16_t, int16_t>& dbMap, const std::map<int, int>& srecMap );
	static bool MakeImageResultFromMaps(
		std::vector<DBApi::DB_ImageResultRecord>& imageresultList,
		const std::map<int, CellCounterResult::SImageStats>& map_Image_Results,
		const std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>>& map_Blobs_for_image,
		const std::map<int, std::map<int, int>>& map_MaxNoOfPeaksFLChannel,
		const std::map<int, std::vector<CellCounterResult::SLargeClusterData>>& map_v_stLargeClusterData,
		const ImageDataForSResultMap& imageDataForSResult);
	static DBApi::blob_data_storage DbStorageRecordFromBlobSData(Blob::SBlobData bdata);
	static DBApi::blob_data_t DbTRecordFromBlobSData(Blob::SBlobData bdata);
	static DBApi::DB_AnalysisInputSettingsRecord DbRecordFromSInputSettings(CellCounterResult::SInputSettings settings);
	static DBApi::DB_DetailedResultRecord DbRecordFromSImageStats(CellCounterResult::SImageStats result);
};
