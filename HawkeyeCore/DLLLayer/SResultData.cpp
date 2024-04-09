#include "stdafx.h"

#include <exception>

#include <DBif_Api.h>

#include "DataConversion.hpp"
#include "Logger.hpp"
#include "SResultBinStorage.hpp"
#include "SResultData.hpp"
#include "SystemErrors.hpp"

static const char MODULENAME[] = "SResultData";


//*****************************************************************************
HawkeyeError SResultData::Write(
	const uuid__t& sampleId, 
	const uuid__t& analysisId, 
	const CellCounterResult::SResult& sresult, 
	const ImageDataForSResultMap& imageDataForSResult,
	uuid__t& sresultId)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "SResultData::Write: <enter>");

	DBApi::DB_SResultRecord dbSResultRecord = {};

	dbSResultRecord.SResultIdNum = 0;
	dbSResultRecord.SResultId = {};
	dbSResultRecord.SampleId = sampleId;
	dbSResultRecord.AnalysisId = analysisId;

	dbSResultRecord.ProcessingSettings = DbRecordFromSInputSettings(sresult.Processing_Settings);
	dbSResultRecord.CumulativeDetailedResult = DbRecordFromSImageStats(sresult.Cumulative_results);

	dbSResultRecord.CumulativeDetailedResult.SampleId = sampleId;
	dbSResultRecord.CumulativeDetailedResult.AnalysisId = analysisId;

	Logger::L().Log (MODULENAME, severity_level::debug2, "SResultData::Write: <Before TransferPeakMap>");

	if (sresult.map_MaxNoOfPeaksFLChannel_Cumulative.size())
	{
		TransferPeakMap (dbSResultRecord.CumulativeMaxNumOfPeaksFlChanMap, sresult.map_MaxNoOfPeaksFLChannel_Cumulative);
	}

	Logger::L().Log (MODULENAME, severity_level::debug2, "SResultData::Write: <Before MakeImageResultFromMaps>");

	MakeImageResultFromMaps (dbSResultRecord.ImageResultList,
		sresult.map_Image_Results,
		sresult.map_Blobs_for_image,
		sresult.map_MaxNoOfPeaksFLChannel,
		sresult.map_v_stLargeClusterData,
		imageDataForSResult);

	Logger::L().Log (MODULENAME, severity_level::debug2, "SResultData::Write: <Before DbAddSResult>");

	// Use the timestamp of last item in the vector.
	// Reanalysis does not save images.  The images belong to the SampleRecord.
	if (!imageDataForSResult.empty())
	{
		if (!ChronoUtilities::IsZero(imageDataForSResult.at((--imageDataForSResult.end())->first).timestamp))
		{
			dbSResultRecord.CumulativeDetailedResult.ResultDateTP = imageDataForSResult.at((--imageDataForSResult.end())->first).timestamp;
		}
	}

	DBApi::eQueryResult dbStatus = DBApi::DbAddSResult (dbSResultRecord);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults != dbStatus)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format ("SResultData::Write: <exit, DbAddSResult failed; query status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::error));
			return HawkeyeError::eStorageFault;
		}
	}

	sresultId = dbSResultRecord.SResultId;

	Logger::L().Log (MODULENAME, severity_level::debug2, "SResultData::Write: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool SResultData::TransferConfigMap (ConfigParamList_t& dbCfgMap, std::map<ConfigParameters::E_CONFIG_PARAMETERS, double>& srecCfgMap)
{
	for (auto mapIter = srecCfgMap.begin(); mapIter != srecCfgMap.end(); ++mapIter)
	{
		ConfigParameters::E_CONFIG_PARAMETERS cfgEnum = ConfigParameters::E_CONFIG_PARAMETERS::eInvalidKey;
		double cfgVal = 0.0;

		cfgEnum = mapIter->first;
		cfgVal = mapIter->second;

		dbCfgMap.emplace(cfgEnum, cfgVal);
	}

	return true;
}

//*****************************************************************************
bool SResultData::TransferCellIdentParamVector (v_IdentificationParams_t& dbMapV, v_IdentificationParams_t& srecMap)
{
	for (auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter)
	{
		dbMapV.push_back(*mapIter);
	}

	return true;
}

//*****************************************************************************
bool SResultData::TransferCellIdentParamVector (std::vector<DBApi::analysis_input_params_storage>& dbMapV, v_IdentificationParams_t& srecMap)
{
	for (auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter)
	{
		DBApi::analysis_input_params_storage ap = {};
		Characteristic_t mc = {};

		mc = mapIter->_Myfirst._Val;
		ap.key = mc._Myfirst._Val;
		ap.s_key = mc._Get_rest()._Myfirst._Val;
		ap.s_s_key = mc._Get_rest()._Get_rest()._Myfirst._Val;
		ap.value = mapIter->_Get_rest()._Myfirst._Val;
		ap.polarity = static_cast<E_POLARITY>(mapIter->_Get_rest()._Get_rest()._Myfirst._Val);

		dbMapV.push_back(ap);
	}

	return true;
}

//*****************************************************************************
bool SResultData::TransferBlobMapToVector (std::vector<DBApi::blob_info_pair>& dbMapV, std::map<Characteristic_t, float>& srecMap)
{
	for (auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter)
	{
		DBApi::blob_info_pair blob_info = {};

		blob_info.first = mapIter->first;
		blob_info.second = mapIter->second;

		dbMapV.push_back(blob_info);
	}

	return true;
}

//*****************************************************************************
bool SResultData::TransferBlobMapToVector (std::vector<DBApi::blob_info_storage>& dbMapV, std::map<Characteristic_t, float>& srecMap)
{
	for (auto mapIter = srecMap.begin(); mapIter != srecMap.end(); ++mapIter)
	{
		DBApi::blob_info_storage blob_info;
		Characteristic_t mc;

		mc = mapIter->first;
		blob_info.key = mc._Myfirst._Val;
		blob_info.s_key = mc._Get_rest()._Myfirst._Val;
		blob_info.s_s_key = mc._Get_rest()._Get_rest()._Myfirst._Val;
		blob_info.value = mapIter->second;

		dbMapV.push_back(blob_info);
	}

	return true;
}

//*****************************************************************************
bool SResultData::TransferClusterVector (std::vector<DBApi::cluster_data_t>& cluster_data_v, std::vector<CellCounterResult::SLargeClusterData> map_cluster_v)
{
	for (auto clIter = map_cluster_v.begin(); clIter != map_cluster_v.end(); ++clIter)
	{
		DBApi::cluster_data_t cluster;

		cluster.cell_count = static_cast<int16_t>(clIter->nNoOfCellsInCluster);

		for (auto ptIter = clIter->v_oCellBoundaryPoints.begin(); ptIter != clIter->v_oCellBoundaryPoints.end(); ++ptIter)
		{
			DBApi::blob_point pt;

			pt.startx = static_cast<int16_t>(ptIter->x);
			pt.starty = static_cast<int16_t>(ptIter->y);

			cluster.cluster_polygon.push_back(pt);
		}

		cluster.cluster_box.start.startx = static_cast<int16_t>(clIter->BoundingBox.x);
		cluster.cluster_box.start.starty = static_cast<int16_t>(clIter->BoundingBox.y);
		cluster.cluster_box.height = static_cast<int16_t>(clIter->BoundingBox.height);
		cluster.cluster_box.width = static_cast<int16_t>(clIter->BoundingBox.width);

		cluster_data_v.push_back(cluster);
	}

	return true;
}

//*****************************************************************************
DBApi::blob_data_t SResultData::DbTRecordFromBlobSData (Blob::SBlobData bdata)
{
	DBApi::blob_data_t dbBlob;

	dbBlob.blob_center.startx = static_cast<int16_t>(bdata.oPointCenter.x);
	dbBlob.blob_center.starty = static_cast<int16_t>(bdata.oPointCenter.y);

	TransferBlobMapToVector (dbBlob.blob_info, bdata.map_Characteristics);

	for (auto ptIter = bdata.v_oCellBoundaryPoints.begin(); ptIter != bdata.v_oCellBoundaryPoints.end(); ++ptIter)
	{
		DBApi::blob_point pt;

		pt.startx = static_cast<int16_t>(ptIter->x);
		pt.starty = static_cast<int16_t>(ptIter->y);

		dbBlob.blob_outline.push_back(pt);
	}

	return dbBlob;
}

//*****************************************************************************
DBApi::blob_data_storage SResultData::DbStorageRecordFromBlobSData (Blob::SBlobData bdata)
{
	DBApi::blob_data_storage dbBlob;

	dbBlob.blob_center.startx = static_cast<int16_t>(bdata.oPointCenter.x);
	dbBlob.blob_center.starty = static_cast<int16_t>(bdata.oPointCenter.y);

	TransferBlobMapToVector(dbBlob.blob_info, bdata.map_Characteristics);

	for (auto ptIter = bdata.v_oCellBoundaryPoints.begin(); ptIter != bdata.v_oCellBoundaryPoints.end(); ++ptIter)
	{
		DBApi::blob_point pt;

		pt.startx = static_cast<int16_t>(ptIter->x);
		pt.starty = static_cast<int16_t>(ptIter->y);

		dbBlob.blob_outline.push_back(pt);
	}

	return dbBlob;
}

//*****************************************************************************
bool SResultData::TransferBlobVector (std::vector<DBApi::blob_data_t>& dbBlobs, const std::vector<Blob::SBlobData>& srecBlobs)
{
	size_t listSize = srecBlobs.size();
	int listIndex = 0;

	for (listIndex = 0; listIndex < listSize; listIndex++)
	{
		Blob::SBlobData blob = srecBlobs.at(listIndex);

		DBApi::blob_data_t dbBlob = DbTRecordFromBlobSData(blob);

		dbBlobs.push_back(dbBlob);
	}

	return true;
}

//*****************************************************************************
bool SResultData::TransferBlobVector (std::vector<DBApi::blob_data_storage>& dbBlobs, const std::vector<Blob::SBlobData>& blobs)
{
	for (auto blobIter = blobs.begin(); blobIter != blobs.end(); ++blobIter)
	{
		Blob::SBlobData blob = *blobIter;

		DBApi::blob_data_storage dbBlob = DbStorageRecordFromBlobSData(blob);

		dbBlobs.push_back(dbBlob);
	}

	return true;
}

//*****************************************************************************
bool SResultData::MakeImageResultFromMaps (std::vector<DBApi::DB_ImageResultRecord>& imageResultList,
	const std::map<int, CellCounterResult::SImageStats>& map_Image_Results,
	const std::map<int, std::shared_ptr<std::vector<Blob::SBlobData>>>& map_Blobs_for_image,
	const std::map<int, std::map<int, int>>& map_MaxNoOfPeaksFLChannel,
	const std::map<int, std::vector<CellCounterResult::SLargeClusterData>>& map_v_stLargeClusterData,
	const ImageDataForSResultMap& imageDataForSResult)
{
	// This method assumes that the maps are ALL organized to contain one entry per image.
	size_t mapSize = map_Image_Results.size();
	if (map_Blobs_for_image.size() != mapSize)
	{
		return false;
	}

	// Do not check this field.
	// When reanalyzing using CellType with the same cluster setting as the original,
	// the images are not reprocessed resulting in an SResult where this field is empty.
	//if (map_MaxNoOfPeaksFLChannel.size() != mapSize)
	//{
	//	return false;
	//}

	if (map_v_stLargeClusterData.size() != mapSize)
	{
		return false;
	}

	int mapIndex = 0;
	int mapKey = mapIndex + 1;

	for (mapIndex = 0; mapIndex < mapSize; mapIndex++)
	{
		DBApi::DB_ImageResultRecord dbiResult = {};
		mapKey = mapIndex + 1;

		CellCounterResult::SImageStats stats = {};

		stats.ClearData();
		auto map_stats = map_Image_Results.find(mapKey);
		stats = map_stats->second;
		dbiResult.DetailedResult = DbRecordFromSImageStats(stats);

		if (map_MaxNoOfPeaksFLChannel.size())
		{
			auto map_peak_map = map_MaxNoOfPeaksFLChannel.find(mapKey);
			if (map_peak_map->second.size())
			{
				TransferPeakMap (dbiResult.MaxNumOfPeaksFlChansMap, map_peak_map->second);
			}
		}

		auto map_blob_ptr = map_Blobs_for_image.find(mapKey);
			std::shared_ptr<std::vector<Blob::SBlobData>> blob_v_ptr = map_blob_ptr->second;
			std::vector<Blob::SBlobData> blob_v = *blob_v_ptr;

		TransferBlobVector(dbiResult.BlobDataList, blob_v);
		dbiResult.NumBlobs = static_cast<int16_t>(dbiResult.BlobDataList.size());

		std::vector<CellCounterResult::SLargeClusterData> cluster_v;
		auto map_cluster_v = map_v_stLargeClusterData.find (mapKey);
		cluster_v = map_cluster_v->second;

		TransferClusterVector (dbiResult.ClusterDataList, cluster_v);

		dbiResult.NumClusters = static_cast<int16_t>(dbiResult.ClusterDataList.size());

		dbiResult.ImageSeqNum = mapKey;

		if (imageDataForSResult.size())
		{
			try
			{
				dbiResult.ImageId = imageDataForSResult.at(mapKey).uuid;
				dbiResult.DetailedResult.ResultDateTP = imageDataForSResult.at(mapKey).timestamp;
			}
			catch (...)
			{
			}
		}

		imageResultList.push_back(dbiResult);

	} // End "for (mapIndex = 0; mapIndex < mapSize; mapIndex++)"

	return true;
}

//*****************************************************************************
bool SResultData::TransferPeakMap (std::map<int16_t, int16_t>& dbMap, const std::map<int, int>& srecMap)
{
	size_t mapSize;
	int mapIndex = 0;
	int mapKey = mapIndex + 1;

	mapSize = srecMap.size();

	for (mapIndex = 0; mapIndex < mapSize; mapIndex++)
	{
		int16_t key;
		int16_t value;

		auto srmap = srecMap.find(mapKey);
		key = static_cast<int16_t>(srmap->first);
		value = static_cast<int16_t>(srmap->second);

		dbMap.emplace(key, value);
	}

	return true;
}

//*****************************************************************************
DBApi::DB_DetailedResultRecord SResultData::DbRecordFromSImageStats (CellCounterResult::SImageStats result)
{
	DBApi::DB_DetailedResultRecord dbDRec = {};

	dbDRec.DetailedResultIdNum = 0;
	dbDRec.DetailedResultId = {};
	dbDRec.SampleId = {};
	dbDRec.ImageId = {};
	dbDRec.AnalysisId = {};
	dbDRec.OwnerUserId = {};

	dbDRec.ProcessingStatus = static_cast<int16_t>(result.eProcessStatus);
	dbDRec.TotalCumulativeImages = static_cast<uint16_t>(result.nTotalCumulative_Imgs);
	dbDRec.TotalCells_GP = static_cast<int32_t>(result.nTotalCellCount_GP);
	dbDRec.TotalCells_POI = static_cast<int32_t>(result.nTotalCellCount_POI);
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
	dbDRec.TotalBubbleCount = static_cast<int32_t>(result.nTotalBubbleCount);
	dbDRec.LargeClusterCount = static_cast<int32_t>(result.nLargeClusterCount);

	return dbDRec;
}

//*****************************************************************************
DBApi::DB_AnalysisInputSettingsRecord SResultData::DbRecordFromSInputSettings (CellCounterResult::SInputSettings settings)
{
	DBApi::DB_AnalysisInputSettingsRecord dbSettings = {};

	dbSettings.SettingsIdNum = 0;
	dbSettings.SettingsId = {};

	TransferConfigMap (dbSettings.InputConfigParamMap, settings.map_InputConfigParameters);

	TransferCellIdentParamVector (dbSettings.CellIdentParamList, settings.v_CellIdentificationParameters);

	TransferCellIdentParamVector (dbSettings.POIIdentParamList, settings.v_POIIdentificationParameters);

	return dbSettings;
}

//*****************************************************************************
static CellCounterResult::SImageStats DB_DetailedResultRecord_To_SImageStats (const DBApi::DB_DetailedResultRecord& drr)
{
	CellCounterResult::SImageStats imageStats = {};

	imageStats.eProcessStatus = static_cast<E_ERRORCODE>(drr.ProcessingStatus);
	imageStats.nTotalCumulative_Imgs = drr.TotalCumulativeImages;
	imageStats.nTotalCellCount_GP = drr.TotalCells_GP;
	imageStats.nTotalCellCount_POI = drr.TotalCells_POI;
	imageStats.dPopulationPercentage = drr.POI_PopPercent;
	imageStats.dCellConcentration_GP = drr.CellConc_GP;
	imageStats.dCellConcentration_POI = drr.CellConc_POI;
	imageStats.dAvgDiameter_GP = drr.AvgDiam_GP;
	imageStats.dAvgDiameter_POI = drr.AvgDiam_POI;
	imageStats.dAvgCircularity_GP = drr.AvgCircularity_GP;
	imageStats.dAvgCircularity_POI = drr.AvgCircularity_POI;
	imageStats.dAvgSharpness_GP = drr.AvgSharpness_GP;
	imageStats.dAvgSharpness_POI = drr.AvgSharpness_POI;
	imageStats.dAvgEccentricity_GP = drr.AvgEccentricity_GP;
	imageStats.dAvgEccentricity_POI = drr.AvgEccentricity_POI;
	imageStats.dAvgAspectRatio_GP = drr.AvgAspectRatio_GP;
	imageStats.dAvgAspectRatio_POI = drr.AvgAspectRatio_POI;
	imageStats.dAvgRoundness_GP = drr.AvgRoundness_GP;
	imageStats.dAvgRoundness_POI = drr.AvgRoundness_POI;
	imageStats.dAvgRawCellSpotBrightness_GP = drr.AvgRawCellSpotBrightness_GP;
	imageStats.dAvgRawCellSpotBrightness_POI = drr.AvgRawCellSpotBrightness_POI;
	imageStats.dAvgCellSpotBrightness_GP = drr.AvgCellSpotBrightness_GP;
	imageStats.dAvgCellSpotBrightness_POI = drr.AvgCellSpotBrightness_POI;
	imageStats.dAvgBackgroundIntensity = drr.AvgBkgndIntensity;
	imageStats.nTotalBubbleCount = drr.TotalBubbleCount;
	imageStats.nLargeClusterCount = drr.LargeClusterCount;

	return imageStats;
}

//*****************************************************************************
CellCounterResult::SResult SResultData::FromDBStyle (const DBApi::DB_SResultRecord& dbSResultRecord)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "FromDBStyle: <enter>");

	CellCounterResult::SResult sresult = {};

	// Convert DB record to CellCounterResult::SResult.
	sresult.Processing_Settings.map_InputConfigParameters = dbSResultRecord.ProcessingSettings.InputConfigParamMap;

	for (auto& v : dbSResultRecord.ProcessingSettings.CellIdentParamList)
	{
		auto char_tuple = std::make_tuple (v.key, v.s_key, v.s_s_key);
		auto param_tuple = std::make_tuple (char_tuple, v.value, v.polarity);
		sresult.Processing_Settings.v_CellIdentificationParameters.push_back (param_tuple);
	}

	for (auto& v : dbSResultRecord.ProcessingSettings.POIIdentParamList)
	{
		auto char_tuple = std::make_tuple (v.key, v.s_key, v.s_s_key);
		auto param_tuple = std::make_tuple (char_tuple, v.value, v.polarity);
		sresult.Processing_Settings.v_POIIdentificationParameters.push_back (param_tuple);
	}

	sresult.Cumulative_results = DB_DetailedResultRecord_To_SImageStats (dbSResultRecord.CumulativeDetailedResult);

	int mapIdx = 1;
	for (auto& v : dbSResultRecord.ImageResultList)
	{
		CellCounterResult::SImageStats imageStats = DB_DetailedResultRecord_To_SImageStats (v.DetailedResult);
		sresult.map_Image_Results.insert (sresult.map_Image_Results.end(), std::pair<int, CellCounterResult::SImageStats>(mapIdx, imageStats));

		{ // Populate "map_Blobs_for_image".
			std::vector<Blob::SBlobData> vBlobData = {};

			for (auto& vv : v.BlobDataList)
			{
				Blob::SBlobData blobData;

				for (auto& vvv : vv.blob_info)
				{
					blobData.map_Characteristics.insert (blobData.map_Characteristics.end(), std::pair<Characteristic_t, float>(vvv));
				}

				blobData.oPointCenter.x = vv.blob_center.startx;
				blobData.oPointCenter.y = vv.blob_center.starty;

				for (auto& vvv : vv.blob_outline)
				{
					blobData.v_oCellBoundaryPoints.emplace_back (vvv.startx, vvv.starty);
				}

				vBlobData.push_back (blobData);
			}

			sresult.map_Blobs_for_image.insert(
				sresult.map_Blobs_for_image.end(),
				std::pair <int, std::shared_ptr<std::vector<Blob::SBlobData>>>
				(mapIdx, std::make_shared<std::vector<Blob::SBlobData>> (vBlobData))
			);
		}

		{ // Populate "map_v_stLargeClusterData".
			std::vector<CellCounterResult::SLargeClusterData> vlargeClusterData = {};

			for (auto& vv : v.ClusterDataList)
			{
				CellCounterResult::SLargeClusterData largeClusterData = {};

				largeClusterData.nNoOfCellsInCluster = vv.cell_count;

				for (auto& vvv : vv.cluster_polygon)
				{
					cv::Point point;
					point.x = vvv.startx;
					point.y = vvv.starty;
					largeClusterData.v_oCellBoundaryPoints.push_back (point);
				}

				largeClusterData.BoundingBox.x = vv.cluster_box.start.startx;
				largeClusterData.BoundingBox.y = vv.cluster_box.start.starty;
				largeClusterData.BoundingBox.height = vv.cluster_box.height;
				largeClusterData.BoundingBox.width = vv.cluster_box.width;

				vlargeClusterData.push_back (largeClusterData);
			}

			sresult.map_v_stLargeClusterData.insert(
				sresult.map_v_stLargeClusterData.end(),
				std::pair <int, std::vector<CellCounterResult::SLargeClusterData>>
				(mapIdx, vlargeClusterData));
		}

		//TODO: Hunter code, this has not been tested.
		{ // Populate "map_MaxNoOfPeaksFLChannel".
			for (auto& vv : v.MaxNumOfPeaksFlChansMap)
			{
				std::map<int, int> intType = {};
				intType.insert (std::make_pair<int, int>((int)vv.first, (int)vv.second));

				sresult.map_MaxNoOfPeaksFLChannel.insert(
					sresult.map_MaxNoOfPeaksFLChannel.end(),
					std::pair <int, std::map<int, int>>
					(mapIdx, intType));
			}
		}

		//TODO: Hunter code, this has not been tested.
		// Populate "map_MaxNoOfPeaksFLChannel_Cumulative".
		for (size_t idx = 1; idx <= v.MaxNumOfPeaksFlChansMap.size(); idx++)
		{
			sresult.map_MaxNoOfPeaksFLChannel_Cumulative.at(static_cast<int16_t>(idx)) = v.MaxNumOfPeaksFlChansMap.at(static_cast<int16_t>(idx));
		}


		mapIdx++;
	}

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "FromDBStyle: <exit>");

	return sresult;
}
