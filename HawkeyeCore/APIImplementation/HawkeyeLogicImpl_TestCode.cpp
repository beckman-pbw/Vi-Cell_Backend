#include "stdafx.h"

#include <DBif_Api.h>

#include "DataConversion.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "SResultBinStorage.hpp"
#include "SResultData.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_TestCode";

//TODO: keeping this code until we're satisfied that Re-Analysis works correctly.
/*
	typedef struct blob_data_t_struct
	{
		std::vector<blob_info_pair>	blob_info;
		blob_point					blob_center;
		std::vector<blob_point>		blob_outline;
	} blob_data_t;

	typedef struct cluster_data_t_struct
	{
		int16_t					cell_count;
		std::vector<blob_point> cluster_polygon;	// all vertices of a (potentially) irregular outline
		blob_rect_t				cluster_box;
	} cluster_data_t;

	typedef struct DB_ImageResult
	{
		int64_t		ResultIdNum;
		uuid__t		ResultId;
		uuid__t		SampleId;
		uuid__t		ImageId;
		uuid__t		AnalysisId;
		int32_t		ImageSeqNum;

		DB_DetailedResultRecord			DetailedResult;

		std::map<int16_t, int16_t>		MaxNumOfPeaksFlChansMap;		// <chan number, max peaks>

		int16_t		NumBlobs;
		std::vector<blob_data_t>		BlobDataList;
		int16_t		NumClusters;
		std::vector<cluster_data_t>		ClusterDataList;
	} DB_ImageResultRecord;

	typedef struct DB_SResult
	{
		int64_t	SResultIdNum;
		uuid__t	SResultId;
		uuid__t	SampleId;
		uuid__t	AnalysisId;

		DB_AnalysisInputSettings			ProcessingSettings;
		DB_DetailedResultRecord				CumulativeDetailedResult;
		std::map<int16_t, int16_t>			CumulativeMaxNumOfPeaksFlChanMap;		// should be <fl-chan, max_num_peaks>
		std::vector<DB_ImageResultRecord>	ImageResultList;
	} DB_SResultRecord;

	struct SResult
	{
		SInputSettings Processing_Settings;													// Input process settings
		SImageStats Cumulative_results;														// Cumulative Cell counter result structure
		std::map<int, SImageStats> map_Image_Results;										// map containing the image statistic result
		std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>> map_Blobs_for_image;	// map containing the blob information per image
		std::map<int, std::map<int, int>> map_MaxNoOfPeaksFLChannel;						// map containing Maximum number of SubPeaks in each FL Channel
		std::map<int, std::vector<SLargeClusterData>> map_v_stLargeClusterData;				// Large cluster information
		std::map<int, int> map_MaxNoOfPeaksFLChannel_Cumulative;							// map containing the Max sub peak for sample
	};

	struct SInputSettings maps completely to the DBAnalysisInputSettingsRecord structure, with some additions for DB handling

	struct SInputSettings
	{
		std::map<ConfigParameters::E_CONFIG_PARAMETERS, double> map_InputConfigParameters;
		std::vector<std::tuple< Characteristic_t, float, E_POLARITY>> v_CellIdentificationParameters;
		std::vector<std::tuple< Characteristic_t, float, E_POLARITY>> v_POIIdentificationParameters;
	};

	struct SImageStats maps completely to the DB_DetailedresultRecord structure, with some additions for DB handling

*/

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method prototypes for SResult conversion operations:
//
#if 0
	bool TransferConfigMap( ConfigParamList_t & dbCfgMap, std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>& srecCfgMap );
	bool TransferCellIdentParamVector( v_IdentificationParams_t& dbMapV, v_IdentificationParams_t& srecMap );
	bool TransferCellIdentParamVector( std::vector<DBApi::analysis_input_params_storage>& dbMapV, v_IdentificationParams_t& srecMap );
	bool TransferBlobMapToVector( std::vector<DBApi::blob_info_pair>& dbMapV, std::map<Characteristic_t, float>& srecMap );
	bool TransferBlobMapToVector( std::vector<DBApi::blob_info_storage>& dbMapV, std::map<Characteristic_t, float>& srecMap );
	bool TransferBlobVector( std::vector<DBApi::blob_data_t> dbBlobs, std::vector<Blob::SBlobData> srecBlobs );
	bool TransferBlobVector( std::vector<DBApi::blob_data_storage> dbBlobs, std::vector<Blob::SBlobData> blobs );
	bool TransferPeakMap( std::map<int16_t, int16_t>& dbMap, std::map<int, int>& srecMap );
	bool MakeImageResultFromMaps( std::vector<DBApi::DB_ImageResultRecord>& imageresultList,
								  std::map<int, CellCounterResult::SImageStats>& map_Image_Results,
								  std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>>& map_Blobs_for_image,
								  std::map<int, std::map<int, int>>& map_MaxNoOfPeaksFLChannel,
								  std::map<int, std::vector<CellCounterResult::SLargeClusterData>>& map_v_stLargeClusterData );
	DBApi::blob_data_storage DbStorageRecordFromBlobSData( Blob::SBlobData bdata );
	DBApi::blob_data_t DbTRecordFromBlobSData( Blob::SBlobData bdata );
	DBApi::DB_AnalysisInputSettingsRecord DbRecordFromSInputSettings( CellCounterResult::SInputSettings settings );
	DBApi::DB_DetailedResultRecord DbRecordFromSImageStats( CellCounterResult::SImageStats result );
	DBApi::DB_SResultRecord DbRecordFromSResult( CellCounterResult::SResult srec );

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool HawkeyeLogicImpl::TransferConfigMap( ConfigParamList_t& dbCfgMap, std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>& srecCfgMap )
{
	for ( auto mapIter = srecCfgMap.begin(); mapIter != srecCfgMap.end(); ++mapIter )
	{
		ConfigParameters::E_CONFIG_PARAMETERS cfgEnum = ConfigParameters::E_CONFIG_PARAMETERS::eInvalidKey;
		double cfgVal = 0.0;

		cfgEnum = mapIter->first;
		cfgVal = mapIter->second;

		dbCfgMap.emplace( cfgEnum, cfgVal );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferCellIdentParamVector( v_IdentificationParams_t& dbMapV, v_IdentificationParams_t& srecMap )
{
	for ( auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter )
	{
		dbMapV.push_back( *mapIter );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferCellIdentParamVector( std::vector<DBApi::analysis_input_params_storage> & dbMapV, v_IdentificationParams_t& srecMap )
{
	for ( auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter )
	{
		DBApi::analysis_input_params_storage ap = {};
		Characteristic_t mc = {};

		mc = mapIter->_Myfirst._Val;
		ap.key = mc._Myfirst._Val;
		ap.s_key = mc._Get_rest()._Myfirst._Val;
		ap.s_s_key = mc._Get_rest()._Get_rest()._Myfirst._Val;
		ap.value = mapIter->_Get_rest()._Myfirst._Val;
		ap.polarity = static_cast<E_POLARITY>(mapIter->_Get_rest()._Get_rest()._Myfirst._Val);

		dbMapV.push_back( ap );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferBlobMapToVector( std::vector<DBApi::blob_info_pair>& dbMapV, std::map<Characteristic_t, float>& srecMap )
{
	for ( auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter )
	{
		DBApi::blob_info_pair blob_info = {};

		blob_info.first = mapIter->first;
		blob_info.second = mapIter->second;

		dbMapV.push_back( blob_info );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferBlobMapToVector( std::vector<DBApi::blob_info_storage>& dbMapV, std::map<Characteristic_t, float>& srecMap )
{
	for ( auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter )
	{
		DBApi::blob_info_storage blob_info;
		Characteristic_t mc;

		mc = mapIter->first;
		blob_info.key = mc._Myfirst._Val;
		blob_info.s_key = mc._Get_rest()._Myfirst._Val;
		blob_info.s_s_key = mc._Get_rest()._Get_rest()._Myfirst._Val;
		blob_info.value = mapIter->second;

		dbMapV.push_back( blob_info );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferClusterVector( std::vector<DBApi::cluster_data_t>& cluster_data_v, std::vector<CellCounterResult::SLargeClusterData> map_cluster_v )
{
	for ( auto clIter = map_cluster_v.begin(); clIter != map_cluster_v.end(); ++clIter )
	{
		DBApi::cluster_data_t cluster;

		cluster.cell_count = static_cast<int16_t>(clIter->nNoOfCellsInCluster);

		for ( auto ptIter = clIter->v_oCellBoundaryPoints.begin(); ptIter != clIter->v_oCellBoundaryPoints.end(); ++ptIter )
		{
			DBApi::blob_point pt;

			pt.startx = static_cast<int16_t>( ptIter->x );
			pt.starty = static_cast<int16_t>( ptIter->y );

			cluster.cluster_polygon.push_back( pt );
		}

		cluster.cluster_box.start.startx = static_cast<int16_t>( clIter->BoundingBox.x );
		cluster.cluster_box.start.starty = static_cast<int16_t>( clIter->BoundingBox.y );
		cluster.cluster_box.height = static_cast<int16_t>( clIter->BoundingBox.height );
		cluster.cluster_box.width = static_cast<int16_t>( clIter->BoundingBox.width );

		cluster_data_v.push_back( cluster );
	}

	return true;
}

DBApi::blob_data_t HawkeyeLogicImpl::DbTRecordFromBlobSData( Blob::SBlobData bdata )
{
	DBApi::blob_data_t dbBlob;

	dbBlob.blob_center.startx = static_cast<int16_t>( bdata.oPointCenter.x );
	dbBlob.blob_center.starty = static_cast<int16_t>( bdata.oPointCenter.y );

	TransferBlobMapToVector( dbBlob.blob_info, bdata.map_Characteristics );

	for ( auto ptIter = bdata.v_oCellBoundaryPoints.begin(); ptIter != bdata.v_oCellBoundaryPoints.end(); ++ptIter )
	{
		DBApi::blob_point pt;

		pt.startx = static_cast<int16_t>( ptIter->x );
		pt.starty = static_cast<int16_t>( ptIter->y );

		dbBlob.blob_outline.push_back( pt );
	}

	return dbBlob;
}

DBApi::blob_data_storage HawkeyeLogicImpl::DbStorageRecordFromBlobSData( Blob::SBlobData bdata )
{
	DBApi::blob_data_storage dbBlob;

	dbBlob.blob_center.startx = static_cast<int16_t>( bdata.oPointCenter.x );
	dbBlob.blob_center.starty = static_cast<int16_t>( bdata.oPointCenter.y );

	TransferBlobMapToVector( dbBlob.blob_info, bdata.map_Characteristics );

	for ( auto ptIter = bdata.v_oCellBoundaryPoints.begin(); ptIter != bdata.v_oCellBoundaryPoints.end(); ++ptIter )
	{
		DBApi::blob_point pt;

		pt.startx = static_cast<int16_t>( ptIter->x);
		pt.starty = static_cast<int16_t>( ptIter->y);

		dbBlob.blob_outline.push_back( pt );
	}

	return dbBlob;
}

bool HawkeyeLogicImpl::TransferBlobVector( std::vector<DBApi::blob_data_t>& dbBlobs, std::vector<Blob::SBlobData> srecBlobs )
{
	size_t listSize = srecBlobs.size();
	int listIndex = 0;

	for ( listIndex = 0; listIndex < listSize; listIndex++ )
	{
		Blob::SBlobData blob = srecBlobs.at( listIndex );

		DBApi::blob_data_t dbBlob = DbTRecordFromBlobSData( blob );

		dbBlobs.push_back( dbBlob );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferBlobVector( std::vector<DBApi::blob_data_storage>& dbBlobs, std::vector<Blob::SBlobData> blobs )
{
	for ( auto blobIter = blobs.begin(); blobIter != blobs.end(); ++blobIter )
	{
		Blob::SBlobData blob = *blobIter;

		DBApi::blob_data_storage dbBlob = DbStorageRecordFromBlobSData( blob );

		dbBlobs.push_back( dbBlob );
	}

	return true;
}

bool HawkeyeLogicImpl::MakeImageResultFromMaps( std::vector<DBApi::DB_ImageResultRecord>& imageResultList,
												std::map<int, CellCounterResult::SImageStats>& map_Image_Results,
												std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>>& map_Blobs_for_image,
												std::map<int, std::map<int, int>>& map_MaxNoOfPeaksFLChannel,
												std::map<int, std::vector<CellCounterResult::SLargeClusterData>>& map_v_stLargeClusterData )
{
	// this method assumes that the maps are ALL organized to contain one entry per image!!!
	size_t mapSize;

	mapSize = map_Image_Results.size();
	if ( map_Blobs_for_image.size() != mapSize )
	{
		return false;
	}

	if ( map_MaxNoOfPeaksFLChannel.size() != mapSize )
	{
		return false;
	}

	if ( map_v_stLargeClusterData.size() != mapSize )
	{
		return false;
	}

	int mapIndex = 0;
	int mapKey = mapIndex + 1;

	for ( mapIndex = 0; mapIndex < mapSize; mapIndex++ )
	{
		DBApi::DB_ImageResultRecord dbiResult = {};
		mapKey = mapIndex + 1;

		CellCounterResult::SImageStats stats = {};

		stats.ClearData();
		auto map_stats = map_Image_Results.find( mapKey );
		stats = map_stats->second;
		dbiResult.DetailedResult = DbRecordFromSImageStats( stats );

		auto map_peak_map = map_MaxNoOfPeaksFLChannel.find( mapKey );
		std::map<int, int> image_flPeakMap = map_peak_map->second;
		TransferPeakMap( dbiResult.MaxNumOfPeaksFlChansMap, image_flPeakMap );

		auto map_blob_ptr = map_Blobs_for_image.find( mapKey );
		std::shared_ptr<std::vector<Blob::SBlobData>> blob_v_ptr = map_blob_ptr->second;
		std::vector<Blob::SBlobData> blob_v = *blob_v_ptr;
//		std::vector<DBApi::blob_data_t> bdv;
//		std::vector<DBApi::blob_data_storage> bdv;

		TransferBlobVector( dbiResult.BlobDataList, blob_v );

		dbiResult.NumBlobs = static_cast<int16_t>( dbiResult.BlobDataList.size() );

		std::vector<CellCounterResult::SLargeClusterData> cluster_v;
		auto map_cluster_v = map_v_stLargeClusterData.find( mapKey );
		cluster_v = map_cluster_v->second;

		TransferClusterVector( dbiResult.ClusterDataList, cluster_v );

		dbiResult.NumClusters = static_cast<int16_t>( dbiResult.ClusterDataList.size() );

		dbiResult.ImageSeqNum = mapKey;

		imageResultList.push_back( dbiResult );
	}

	return true;
}

bool HawkeyeLogicImpl::TransferPeakMap( std::map<int16_t, int16_t>& dbMap, std::map<int, int>& srecMap )
{
	size_t mapSize;
	int mapIndex = 0;
	int mapKey = mapIndex + 1;

	mapSize = srecMap.size();

	for ( mapIndex = 0; mapIndex < mapSize; mapIndex++ )
	{
		int16_t key;
		int16_t value;

		auto srmap = srecMap.find( mapKey );
		key = static_cast<int16_t>( srmap->first );
		value = static_cast<int16_t>( srmap->second );

		dbMap.emplace( key, value );
	}

	return true;
}

DBApi::DB_DetailedResultRecord HawkeyeLogicImpl::DbRecordFromSImageStats( CellCounterResult::SImageStats result )
{
	DBApi::DB_DetailedResultRecord dbDRec = {};

	dbDRec.DetailedResultIdNum = 0;
	dbDRec.DetailedResultId = {};
	dbDRec.SampleId = {};
	dbDRec.ImageId = {};
	dbDRec.AnalysisId = {};
	dbDRec.OwnerUserId = {};

	dbDRec.TotalCumulativeImages = static_cast<uint16_t>( result.nTotalCumulative_Imgs );
	dbDRec.TotalCells_GP = static_cast<int32_t>( result.nTotalCellCount_GP );
	dbDRec.TotalCells_POI = static_cast<int32_t>( result.nTotalCellCount_POI );
	dbDRec.POI_PopPercent = result.dPopulationPercentage;
	dbDRec.CellConc_GP = result.dCellConcentration_GP;
	dbDRec.CellConc_POI = result.dCellConcentration_POI;
	dbDRec.AvgDiam_GP = result.dAvgDiameter_GP;
	dbDRec.AvgDiam_POI = result.dAvgDiameter_POI;
	dbDRec.AvgCircularity_GP = result.dAvgCircularity_GP;
	dbDRec.AvgCircularity_POI = result.dAvgCircularity_POI;
	dbDRec.AvgSharpness_GP = result.dAvgSharpness_GP;
	dbDRec.AvgSharpness_POI = result.dAvgSharpness_POI;
	dbDRec.AvgEccentricity_GP = result.dAvgEccentricity_GP;
	dbDRec.AvgEccentricity_POI = result.dAvgEccentricity_POI;
	dbDRec.AvgAspectRatio_GP = result.dAvgAspectRatio_GP;
	dbDRec.AvgAspectRatio_POI = result.dAvgAspectRatio_POI;
	dbDRec.AvgRoundness_GP = result.dAvgRoundness_GP;
	dbDRec.AvgRoundness_POI = result.dAvgRoundness_POI;
	dbDRec.AvgRawCellSpotBrightness_GP = result.dAvgRawCellSpotBrightness_GP;
	dbDRec.AvgRawCellSpotBrightness_POI = result.dAvgRawCellSpotBrightness_POI;
	dbDRec.AvgCellSpotBrightness_GP = result.dAvgCellSpotBrightness_GP;
	dbDRec.AvgCellSpotBrightness_POI = result.dAvgCellSpotBrightness_POI;
	dbDRec.AvgBkgndIntensity = result.dAvgBackgroundIntensity;
	dbDRec.TotalBubbleCount = static_cast<int32_t>( result.nTotalBubbleCount );
	dbDRec.LargeClusterCount = static_cast<int32_t>( result.nLargeClusterCount );

	return dbDRec;
}

DBApi::DB_AnalysisInputSettingsRecord HawkeyeLogicImpl::DbRecordFromSInputSettings( CellCounterResult::SInputSettings settings )
{
	DBApi::DB_AnalysisInputSettingsRecord dbSettings = {};

	dbSettings.SettingsIdNum = 0;
	dbSettings.SettingsId = {};

	TransferConfigMap( dbSettings.InputConfigParamMap, settings.map_InputConfigParameters );

	TransferCellIdentParamVector( dbSettings.CellIdentParamList, settings.v_CellIdentificationParameters );

	TransferCellIdentParamVector( dbSettings.POIIdentParamList, settings.v_POIIdentificationParameters );

	return dbSettings;
}

DBApi::DB_SResultRecord HawkeyeLogicImpl::DbRecordFromSResult( CellCounterResult::SResult srec )
{
	DBApi::DB_SResultRecord dbSRec = {};

	dbSRec.SResultIdNum = 0;
	dbSRec.SResultId = {};
	dbSRec.SampleId = {};
	dbSRec.AnalysisId = {};

	dbSRec.ProcessingSettings = DbRecordFromSInputSettings( srec.Processing_Settings );
	dbSRec.CumulativeDetailedResult = DbRecordFromSImageStats( srec.Cumulative_results );

	TransferPeakMap( dbSRec.CumulativeMaxNumOfPeaksFlChanMap,
					 srec.map_MaxNoOfPeaksFLChannel_Cumulative );

	MakeImageResultFromMaps( dbSRec.ImageResultList,
							 srec.map_Image_Results,
							 srec.map_Blobs_for_image,
							 srec.map_MaxNoOfPeaksFLChannel,
							 srec.map_v_stLargeClusterData );

	return dbSRec;
}
#endif
