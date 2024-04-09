// ReSharper disable CppInconsistentNaming
#include "stdafx.h"

#include <cstdio>

#include <DBif_Api.h>
#include "AnalysisImagePersistHelper.hpp"
#include "AuditLog.hpp"
#include "CellCounterFactory.h"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "ExportUtilities.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeAssert.hpp"
#include "HawkeyeResultsDataManager.hpp"
#include "HawkeyeUUID.hpp"
#include "ImageAnalysisUtilities.hpp"
#include "ImageWrapperUtilities.hpp"
#ifndef DATAIMPORTER
#include "LegacyData.hpp"
#endif
#include "Logger.hpp"
#include "ReportsCommon.hpp"
#include "ResultDefinition.hpp"
#include "SaveResults.h"
#include "SignaturesDLL.hpp"
#include "SResultData.hpp"
#include "SystemErrors.hpp"
#include "SResultBinStorage.hpp"
#include "UserList.hpp"
#include "uuid__t.hpp"

static std::string MODULENAME = "HawkeyeResultsDataManager";
static uuid__t tempUUID = {};
static uuid__t dustRefImageSetId = {};

//*****************************************************************************
void HawkeyeResultsDataManager::Initialize (std::shared_ptr<boost::asio::io_service> pUpstream)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");

#ifndef DATAIMPORTER
	pDataManagerServices_ = std::make_shared<HawkeyeServices>(pUpstream, "HawkeyeResultsDataManager_Thread");

	exportData = std::make_unique<ExportData>(pDataManagerServices_);

	SaveResult::Initialize();
#endif

//TODO: remove when DB is complete...
	// Placeholder UUID to allow the record to be written to the DB.
	// This will be updated when the fields are filled in.
	HawkeyeUUID::Getuuid__tFromStr ("0102030405060708090A0B0C0D0E0FFF", tempUUID);

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");
}

#ifndef DATAIMPORTER
//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveImage (const uuid__t& uuid, std::function<void(cv::Mat, HawkeyeError)> cb)
{
	HAWKEYE_ASSERT (MODULENAME, cb);
	pDataManagerServices_->enqueueInternal([this, uuid, cb]()
	{
		cv::Mat image = {};
		int16_t sequenceNum = 0;
		auto he = retrieveImage (uuid, sequenceNum, image);

		pDataManagerServices_->enqueueExternal (cb,  std::move(image), he);
	});
}

//*****************************************************************************
// With v1.3 s/w there is a change in nomenclature:
// ImageSet_t data is now stored in DB_ImageSeqRecord (sequence) records.
// Renamed internal HawkeyeLogicImpl APIs to reflect this change in perspective.
//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveImageSequenceSet (const uuid__t& uuid, std::function<void(int16_t, ImageSet_t imageSet, HawkeyeError)> cb)
{
	HAWKEYE_ASSERT (MODULENAME, cb);
	pDataManagerServices_->enqueueInternal([this, uuid, cb]()
		{
			ImageSet_t imageSet = {};
			int16_t seqNumber = 0;
			auto status = retrieveImageSequenceSet (uuid, seqNumber, imageSet);

			pDataManagerServices_->enqueueExternal (cb, seqNumber, std::move(imageSet), status);
		});
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::RetrieveImageInfo(const uuid__t& id, int16_t& seqNumber, std::string& path, uuid__t &imgRecId, system_TP& timestamp)
{
	return retrieveImageInfo(id, seqNumber, path, imgRecId, timestamp);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveAnnotatedImage (const uuid__t& result_id, const uuid__t& image_id, DataRecordCallback<cv::Mat> cb)
{
	HAWKEYE_ASSERT (MODULENAME, cb);
	pDataManagerServices_->enqueueInternal([this, result_id, image_id, cb]()
	{
		cv::Mat image = {};
		auto status = retrieveAnnotatedImage (result_id, image_id, image);

		pDataManagerServices_->enqueueExternal (cb, std::move(image), status);
	});
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveBWImage (const uuid__t& uuid, DataRecordCallback<cv::Mat> cb)
{
	HAWKEYE_ASSERT (MODULENAME, cb);
	pDataManagerServices_->enqueueInternal([this, uuid, cb]()
	{
		cv::Mat image = {};
		auto status = retrieveBWImage (uuid, image);

		pDataManagerServices_->enqueueExternal (cb, std::move(image), status);
	});
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveDetailedMeasurement (const uuid__t& summaryResultUuid, DataRecordCallback<DetailedResultMeasurementsDLL> cb)
{
	HAWKEYE_ASSERT (MODULENAME, cb);
	pDataManagerServices_->enqueueInternal([this, summaryResultUuid, cb]()
	{
		DetailedResultMeasurementsDLL record = {};
		auto status = retrieveDetailedMeasurement (summaryResultUuid, record);

		pDataManagerServices_->enqueueExternal (cb, std::move(record), status);
	});
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveHistogram (const uuid__t& uuid, bool only_POI, Hawkeye::Characteristic_t measurement, uint8_t bin_count, DataRecordsCallback<histogrambin_t> cb)
{
	HAWKEYE_ASSERT (MODULENAME, cb);
	pDataManagerServices_->enqueueInternal([=]()
	{
		std::vector<histogrambin_t> records = {};
		auto status = retrieveHistogram (uuid, only_POI, measurement, bin_count, records);

		pDataManagerServices_->enqueueExternal (cb, std::move(records), status);
	});
}

//*****************************************************************************
void HawkeyeResultsDataManager::SaveAnalysisData(
	uuid__t worklistUUID,
	const SampleDefinitionDLL& sampleDef,
	std::shared_ptr<ImageCollection_t> imageCollection,
	const CellCounterResult::SResult& sresult,
    const std::vector<ReagentInfoRecordDLL>& reagentinfoRecords,
	boost::optional<ImageSet_t> dustReferenceImage, // Dust, background levels...,
	DBApi::DB_SampleRecord dbSampleRecord,
	DBApi::DB_AnalysisRecord dbAnalysisRecord,
	QcStatus qcStatus,
	std::function<void(SampleDefinitionDLL, bool)> onComplete)
{
	auto wrapper_cb = [=](SampleDefinitionDLL sampleDef, bool status)
	{
		if (!status)
		{
			Logger::L().Log (MODULENAME, severity_level::error, "SaveAnalysisData: <Failed to Save the analysis data>");
		}
		pDataManagerServices_->enqueueExternal (onComplete, std::move(sampleDef), status);
	};

	pDataManagerServices_->enqueueInternal([=]() 
	{
		saveAnalysisData(
			worklistUUID, 
			sampleDef,
			imageCollection, 
			sresult,
			reagentinfoRecords, 
			dustReferenceImage, 
			dbSampleRecord, 
			dbAnalysisRecord, 
			qcStatus,
			wrapper_cb);
	});
}

//*****************************************************************************
void HawkeyeResultsDataManager::SaveLegacyCellCountingData (std::shared_ptr<ImageCollection_t> imageCollection, std::string label, const CellCounterResult::SResult& stCumulativeOutput, BooleanCallback cb)
{
	pDataManagerServices_->enqueueInternal([=]() {
		
		auto status = saveLegacyCellCountingData (imageCollection, label, stCumulativeOutput);

		pDataManagerServices_->enqueueExternal (cb, status);
	});
}
#endif

//*****************************************************************************
void HawkeyeResultsDataManager::SignResult (const uuid__t& rr_uuid, const DataSignatureInstanceDLL& sig, std::function<void(HawkeyeError, std::string)> cb)
{
#ifndef DATAIMPORTER

	pDataManagerServices_->enqueueInternal([=]() {

		std::string audit_log_str = {};
		auto status = signResult (rr_uuid, sig, audit_log_str);

		pDataManagerServices_->enqueueExternal (cb, status, audit_log_str);
	});

#endif

}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::RetrieveSResult (const uuid__t& sampleUuid, CellCounterResult::SResult& sresult)
{
	return retrieveSResult (sampleUuid, sresult);
}

//*****************************************************************************
void HawkeyeResultsDataManager::DeleteSampleRecord (std::vector<uuid__t>& sampleRecordUuidList, bool retain_results_and_first_image, std::function<void(HawkeyeError, uuid__t, std::string)> callback)
{
#ifndef DATAIMPORTER
	if (ExportData::IsExportInProgress())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "DeleteSampleItem : <exit -Export data operation is in progress>");
		pDataManagerServices_->enqueueExternal (callback, HawkeyeError::eNotPermittedAtThisTime, uuid__t{}, std::string());
		return;
	}
#endif

	pDataManagerServices_->enqueueInternal([=]()
	{
		isDeleteInProgress_ = true;
		for (const auto& sampleRecordUuid : sampleRecordUuidList)
		{
			std::string msg = boost::str (boost::format ("DeleteSampleRecord: UUID: %s") % Uuid::ToStr (sampleRecordUuid));

			std::string audit_log_str = {};
			auto status = deleteSampleRecord (sampleRecordUuid, retain_results_and_first_image, audit_log_str);
			if (status == HawkeyeError::eStorageFault)
			{
				Logger::L().Log (MODULENAME, severity_level::error, msg + ", failed");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_readerror, 
					instrument_error::instrument_storage_instance::result, 
					instrument_error::severity_level::warning));
				continue;
			}
			else
			{
				Logger::L().Log (MODULENAME, severity_level::debug1, msg);
			}

			// Check if sample sets need to be deleted
			// Set up filtering criteria.
			std::vector<DBApi::eListFilterCriteria> filterTypes = {};
			std::vector<std::string> compareOperations = {};
			std::vector<std::string> compareValues = {};

			std::string sTimeStr;
			std::string eTimeStr;
			// Use start and end dates from previous RetrieveSampleRecords()
			ChronoUtilities::getListTimestampAsStr(retrieveSampleStart, retrieveSampleEnd, sTimeStr, eTimeStr);
			filterTypes.push_back(DBApi::eListFilterCriteria::RunDateRangeFilter);
			compareOperations.push_back("=");
			compareValues.push_back(boost::str(boost::format("%s;%s") % sTimeStr % eTimeStr));
			// Only get sample sets marked as complete
			filterTypes.push_back(DBApi::eListFilterCriteria::StatusFilter);
			compareOperations.push_back("=");
			compareValues.push_back(boost::str(boost::format("%d") % (int)DBApi::eSampleSetStatus::SampleSetComplete));
			uuid__t userUUID = {};
			if (!retrieveSampleUsername.empty())
			{
				filterTypes.push_back(DBApi::eListFilterCriteria::OwnerFilter);
				compareOperations.push_back("=");

				UserList::Instance().GetUserUUIDByName(retrieveSampleUsername, userUUID);
				std::string temp = boost::str(boost::format("'%s'") % Uuid::ToStr(userUUID));
				compareValues.push_back(temp);
			}
			std::string str = "DeleteSampleRecord: filter criteria...\n";
			for (int idx = 0; idx < filterTypes.size(); idx++)
			{
				str += boost::str(boost::format("%d : %s : %s\n")
					% (int)filterTypes[idx]
					% compareOperations[idx]
					% compareValues[idx]);
			}
			Logger::L().Log(MODULENAME, severity_level::debug1, str);

			Logger::L().Log(MODULENAME, severity_level::debug1, "DeleteSampleRecord - DbGetSampleSetListEnhanced: START QUERY...");

			// Get a list of sample sets filtering on the start and end dates and the username
			std::vector<DBApi::DB_SampleSetRecord> dbSampleSetList = {};
			DBApi::eQueryResult dbStatus = DBApi::DbGetSampleSetListEnhanced(
				dbSampleSetList,
				filterTypes,
				compareOperations,
				compareValues,
				-1,
				DBApi::eListSortCriteria::CreationDateSort,
				DBApi::eListSortCriteria::SortNotDefined,
				DBApi::eContainedObjectRetrieval::FirstLevelObjs,
				-1,		// Descending sort order
				"",
				-1,
				-1,
				"");
			Logger::L().Log(MODULENAME, severity_level::debug1, "DeleteSampleRecord - DbGetSampleSetListEnhanced: END QUERY...");
			if (DBApi::eQueryResult::QueryOk == dbStatus)
			{
				// Try to find sample sets that can be deleted (containing no 'complete' or 'template' sample items)
				// Sample items that were complete but there corresponding sample records were deleted are marked as 'deleted'
				bool allDeleted;
				for (auto& ssr : dbSampleSetList)
				{
					allDeleted = true;
					// loop throug all sample items in sample set and check their status
					for (auto& v : ssr.SSItemsList)
					{
						if ((v.SampleItemStatus == static_cast<int32_t>(DBApi::eSampleItemStatus::ItemComplete)) ||
							(v.SampleItemStatus == static_cast<int32_t>(DBApi::eSampleItemStatus::ItemTemplate)))
						{
							allDeleted = false;
							break;
						}
					}
					if (allDeleted)
					{
						// No sample items worth keeping in sample set, delete sample set				
						dbStatus = DBApi::DbRemoveSampleSet(ssr);
						if (DBApi::eQueryResult::QueryOk != dbStatus)
						{
							Logger::L().Log(MODULENAME, severity_level::warning,
								boost::str(boost::format("DeleteSampleRecord - DbRemoveSampleSet failed, status: %ld") % (int32_t)dbStatus));
						}
					}
				}
			}
			else
			{
				Logger::L().Log(MODULENAME, severity_level::warning,
					boost::str(boost::format("DeleteSampleRecord - DbGetSampleSetListEnhanced failed, status: %ld") % (int32_t)dbStatus));
			}

			pDataManagerServices_->enqueueExternal (callback, status, sampleRecordUuid, audit_log_str);
		}

		isDeleteInProgress_ = false;
	});
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::ExportForOfflineAnalysis (
	const std::vector<uuid__t>& rs_uuid_list,
    const std::string& username,
    const std::string& export_path,
    eExportImages exportImages,
    uint16_t export_nth_image,
    std::function<void(HawkeyeError, uuid__t)> exportdataProgressCb,
    std::function<void(HawkeyeError, std::string, std::vector<std::string>)> onExportCompletionCb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "ExportForOfflineAnalysis : <enter>");

#ifndef DATAIMPORTER
	//Before doing anything else check whether the delete is in progress or previous export is in progress, if yes don't proceed.
	if (isDeleteInProgress_ || ExportData::IsExportInProgress())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "ExportForOfflineAnalysis : <exit, Delete or Export operation in progress>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
#endif	

	if (export_path.empty() ||
		exportdataProgressCb == nullptr ||
		onExportCompletionCb == nullptr ||
		rs_uuid_list.empty() ||
		username.empty() ||
		!FileSystemUtilities::VerifyUserAccessiblePath(export_path) ||
		!FileSystemUtilities::IsDirectory(export_path))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ExportForOfflineAnalysis: <exit, invalid inputs or the export path is not user accessible: " + export_path + ">");
		return HawkeyeError::eInvalidArgs;
	}

	//Check whether 7zip is available.
	if (!FileSystemUtilities::FileExists(HawkeyeDirectory::getZipUtility()))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "7-Zip utility is not installed.");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_precondition_notmet, 
			instrument_error::instrument_precondition_instance::zip_utility_installed,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	auto cb_progress_wrapper = [=](HawkeyeError er, uuid__t uuid)
	{
		pDataManagerServices_->enqueueExternal (exportdataProgressCb, er, uuid);
	};

	auto cb_completion_wrapper = [=](HawkeyeError er, std::string archived_filepath, std::vector<std::string> audit_log_str_list)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "ExportForOfflineAnalysis : <exit>");
		pDataManagerServices_->enqueueExternal (onExportCompletionCb, er, archived_filepath, audit_log_str_list);
	};

	ExportData::Inputs export_data_inputs;
	export_data_inputs.username = username;
	export_data_inputs.export_path = export_path;
	export_data_inputs.image_selection = exportImages;
	export_data_inputs.nth_image = export_nth_image;
	export_data_inputs.rs_uuid_list = std::move(rs_uuid_list);
	export_data_inputs.progress_cb = cb_progress_wrapper;
	export_data_inputs.completion_cb = cb_completion_wrapper;

	//Shared Pointer 1
	std::shared_ptr<const ExportData::Inputs> pExportDataInputs = std::make_shared<ExportData::Inputs>(export_data_inputs);

	//Shared Pointer 2
	std::shared_ptr<ExportData::Params> pExportDataParams = std::make_shared<ExportData::Params>(ExportData::Params());

#ifndef DATAIMPORTER

	pDataManagerServices_->enqueueInternal([=]()
	{
		exportData->Export (ExportData::ExportSequence::eCreateFileNamesAndStagingDir, pExportDataInputs, pExportDataParams);
	});

#endif

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::CancelExportData()
{
#ifdef DATAIMPORTER
	return HawkeyeError::eSuccess;
#else
	return exportData->Cancel();
#endif
}

#ifndef DATAIMPORTER

static ExportData::Inputs s_export_data_inputs = { };

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::Export_Start(
	const std::vector<uuid__t>& rs_uuid_list,
	const std::string& username,
	const std::string& outPath,
	eExportImages exportImages,
	uint16_t export_nth_image)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Start : <enter>");

	//Before doing anything else check whether the delete is in progress or previous export is in progress, if yes don't proceed.
	if (isDeleteInProgress_ || ExportData::IsExportInProgress())
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "Export_Start : <exit, Delete or Export operation in progress>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (outPath.empty() ||
		rs_uuid_list.empty() ||
		username.empty() 
		||
		!FileSystemUtilities::VerifyUserAccessiblePath(outPath) ||
		!FileSystemUtilities::IsDirectory(outPath)
		
		)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Export_Start: <exit, invalid inputs or the export path is not user accessible: " + outPath + ">");
		return HawkeyeError::eInvalidArgs;
	}

	//Check whether 7zip is available.
	if (!FileSystemUtilities::FileExists(HawkeyeDirectory::getZipUtility()))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "7-Zip utility is not installed.");
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_precondition_notmet,
			instrument_error::instrument_precondition_instance::zip_utility_installed,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	s_export_data_inputs.username = username;
	s_export_data_inputs.export_path = outPath;
	s_export_data_inputs.image_selection = exportImages;
	s_export_data_inputs.nth_image = export_nth_image;
	s_export_data_inputs.rs_uuid_list = rs_uuid_list;

	if (exportData->Export_Start(s_export_data_inputs) == false)
	{
		return HawkeyeError::eSoftwareFault;
	}
	return HawkeyeError::eSuccess;
}



HawkeyeError HawkeyeResultsDataManager::Export_NextMetaData(uint32_t index, uint32_t delayms)
{
	if (isDeleteInProgress_)
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "Export_NextMetaData : <exit, Delete in progress>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	if (!ExportData::IsExportInProgress())
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "Export_NextMetaData : <exit, Not Exporting>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	if (exportData->Export_MetaData(index, delayms) == false)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_NextMetaData : <exit>");
		return HawkeyeError::eSoftwareFault;
	}
	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeResultsDataManager::Export_IsStorageAvailable()
{
	if (exportData->Export_IsStorageSpaceAvailable() == false)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_IsStorageAvailable : <exit - noy enough space>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeResultsDataManager::Export_ArchiveData(const std::string filename, char*& outname)
{
	if (exportData->Export_ArchiveData(filename, outname) == false)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_ArchiveData : <exit - failed>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeResultsDataManager::Export_Cleanup(bool removeFile)
{
	if (exportData->IsExportInProgress() == false)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Cleanup : <exit - not exporting>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	if (exportData->Export_Cleanup(removeFile) == false)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Cleanup : <exit - Export_Cleanup failed>");
		return HawkeyeError::eSoftwareFault;
	}
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
std::vector<std::string> HawkeyeResultsDataManager::GetAuditEntries()
{
	return exportData->GetAuditEntries();
}
#endif

//*****************************************************************************
void HawkeyeResultsDataManager::CacheReanalysisSResultRecord (uuid__t uuid, const CellCounterResult::SResult& sresult)
{
	cached_ReanalysisSResultRecord_ = std::pair<uuid__t, CellCounterResult::SResult>(uuid, sresult);
}

//*****************************************************************************
//NOTE: not currently called...
void  HawkeyeResultsDataManager::getBasicAnswerResults (const CellCounterResult::SResult& sresult, BasicResultAnswers &cumulative_result, std::vector<BasicResultAnswers>& per_image_result)
{
	cumulative_result = {};
	ImageAnalysisUtilities::getBasicResultAnswers (sresult.Cumulative_results, cumulative_result);

	per_image_result.clear();
	per_image_result.reserve (sresult.map_Image_Results.size());

	for (auto item : sresult.map_Image_Results)
	{
		BasicResultAnswers per_image_res;
		ImageAnalysisUtilities::getBasicResultAnswers (item.second, per_image_res);
		per_image_result.push_back (per_image_res);
	}
}

//*****************************************************************************
void  HawkeyeResultsDataManager::getDetailedMeasurementFromSResult (const CellCounterResult::SResult& sr, DetailedResultMeasurementsDLL& drm)
{
	for (const auto& im_blob_data_pair : sr.map_Blobs_for_image)
	{
		image_blobs_tDLL tmp_ib;
		tmp_ib.image_set_number = (uint16_t)im_blob_data_pair.first; // First param in the pair provides the image set number

		tmp_ib.blob_list.reserve(im_blob_data_pair.second->size());
		
		// Second param provides the blob details of each blobs in the image.
		for (auto blob : *im_blob_data_pair.second)
		{
			blob_measurements_tDLL tmp_bm;
			tmp_bm.x_coord = static_cast<uint16_t>(blob.oPointCenter.x);
			tmp_bm.y_coord = static_cast<uint16_t>(blob.oPointCenter.y);

			tmp_bm.measurements.reserve(blob.map_Characteristics.size());
			for (auto charc : blob.map_Characteristics)
			{
				measurement_t tmp_measurement;
				tmp_measurement.characteristic.key = std::get<0>(charc.first);
				tmp_measurement.characteristic.s_key = std::get<1>(charc.first);
				tmp_measurement.characteristic.s_s_key = std::get<2>(charc.first);
				tmp_measurement.value = charc.second;
				tmp_bm.measurements.push_back(tmp_measurement);
			}
			tmp_ib.blob_list.push_back(tmp_bm);
		}
		drm.blobs_by_image.push_back(tmp_ib);
	}

	for (auto im_lc_data_pair : sr.map_v_stLargeClusterData)
	{
		large_cluster_tDLL tmp_lc;
		
		// First param in the pair provides the image set number.
		tmp_lc.image_set_number = (uint16_t)im_lc_data_pair.first;
		tmp_lc.cluster_list.reserve(im_lc_data_pair.second.size());
		
		// Second param provides the clusters data in the image.
		for (const auto& lc : im_lc_data_pair.second)
		{
			large_cluster_data_t tmp_lcd;
			tmp_lcd.num_cells_in_cluster = (uint16_t)lc.nNoOfCellsInCluster;
			tmp_lcd.top_left_x = (uint16_t)lc.BoundingBox.x;
			tmp_lcd.top_left_y = (uint16_t)lc.BoundingBox.y;
			tmp_lcd.bottom_right_x = (uint16_t)(tmp_lcd.top_left_x + lc.BoundingBox.width);
			tmp_lcd.bottom_right_y = (uint16_t)(tmp_lcd.top_left_y + lc.BoundingBox.height);

			tmp_lc.cluster_list.push_back(tmp_lcd);
		}
		drm.large_clusters_by_image.push_back(tmp_lc);
	}
}

#ifndef DATAIMPORTER

std::vector<DataSignatureDLL>& GetDataSignatureDeinitions();

#endif

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::decode_DBSummaryResultRecord (const DBApi::DB_SummaryResultRecord& dbSummaryResultRecord, ResultSummaryDLL& summaryResultRecord)
{
#ifndef DATAIMPORTER

	BasicResultAnswers bra;
	bra.eProcessedStatus = static_cast<E_ERRORCODE>(dbSummaryResultRecord.ProcessingStatus);
	bra.nTotalCumulative_Imgs = dbSummaryResultRecord.TotalCumulativeImages;
	bra.count_pop_general = dbSummaryResultRecord.TotalCells_GP;
	bra.count_pop_ofinterest = dbSummaryResultRecord.TotalCells_POI;
	bra.concentration_general = dbSummaryResultRecord.CellConc_GP;
	bra.concentration_ofinterest = dbSummaryResultRecord.CellConc_POI;
	bra.percent_pop_ofinterest = dbSummaryResultRecord.POI_PopPercent;
	bra.avg_diameter_pop = dbSummaryResultRecord.AvgDiam_GP;
	bra.avg_diameter_ofinterest = dbSummaryResultRecord.AvgDiam_POI;
	bra.avg_circularity_pop = dbSummaryResultRecord.AvgCircularity_GP;
	bra.avg_circularity_ofinterest = dbSummaryResultRecord.AvgCircularity_POI;
	bra.coefficient_variance = dbSummaryResultRecord.CoefficientOfVariance;
	bra.average_cells_per_image = dbSummaryResultRecord.AvgCellsPerImage;
	bra.average_brightfield_bg_intensity = dbSummaryResultRecord.AvgBkgndIntensity;
	bra.bubble_count = dbSummaryResultRecord.TotalBubbleCount;
	bra.large_cluster_count = dbSummaryResultRecord.LargeClusterCount;

	summaryResultRecord.uuid = dbSummaryResultRecord.SummaryResultId;

	// This is used by RetrieveResultRecord to get the per image detailed results from the Analysis DB record.
	summaryResultRecord.analysisUuid = dbSummaryResultRecord.AnalysisId;
	summaryResultRecord.timestamp = dbSummaryResultRecord.ResultDateTP;
	summaryResultRecord.qcStatus= static_cast<QcStatus>(dbSummaryResultRecord.QcStatus);

	CellTypesDLL::getCellTypeByIndex (dbSummaryResultRecord.CellTypeIndex, summaryResultRecord.cell_type_settings);

	// v1.2.43 actually deleted CellTypes from the CellType.info file.  This resulted in a CellType index of -1 when imported into v1.3 and later.
	// This caused a crash (https://lsjira.beckman.com/browse/PC3549-5907).
	// Check if the CellType was deleted and force the CellType label to an empty string.
	// In the UI code if the CellType label is empty then it will set the CellType label to "Deleted" localized to the currently selected language.
	if (summaryResultRecord.cell_type_settings.celltype_index == DELETED_CELLTYPE_INDEX)
	{
		summaryResultRecord.cell_type_settings.label = "";
		// Nothing needs to be done with "summaryResultRecord.analysis_settings" since this check is only to support the UI
		// displaying "Deleted" in place of the CellType label.
	}
	else
	{
		//NOTE: there is only one AnalysisDefinitionDLL associated w/ a CellType.
		summaryResultRecord.analysis_settings = summaryResultRecord.cell_type_settings.analysis_specializations[0];
	}

	summaryResultRecord.cumulative_result = bra;

	for (auto& vv : dbSummaryResultRecord.SignatureList)
	{
		DataSignatureInstanceDLL sigDLL = {};
		sigDLL.signing_user = vv.userName;
		sigDLL.timestamp = vv.signatureTime;
		sigDLL.signature.short_text = vv.shortSignature;
		sigDLL.signature.long_text = vv.longSignature;

		const auto& ds = GetDataSignatureDeinitions();
		auto item = std::find_if(
			ds.begin(), ds.end(), [sigDLL](const auto& item)
			{ return sigDLL.signature.short_text == item.short_text; });

		//TODO: should the DB signature hash be validated?
		//TODO: If so, what should have if the hash is invalid???
		if (item == ds.end())
		{
			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format ("decode_DBSampleRecord: <short signature definition '%s' not found>") % sigDLL.signature.short_text));
		}
		else
		{
			sigDLL.signature.short_text_signature = item->short_text_signature;
			sigDLL.signature.long_text_signature = item->long_text_signature;
		}

		summaryResultRecord.signature_set.push_back (sigDLL);
	}

#endif

	return HawkeyeError::eSuccess;
}

bool getQualityControlByUUID (const uuid__t uuid, QualityControlDLL& qc);

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::decode_DBSampleRecord (const DBApi::DB_SampleRecord& dbSampleRecord, SampleRecordDLL& sampleRecord)
{
	HawkeyeError he = HawkeyeError::eSuccess;

#ifndef DATAIMPORTER

	sampleRecord.uuid = dbSampleRecord.SampleId;
	sampleRecord.username = dbSampleRecord.RunUserNameStr;
	sampleRecord.timestamp = dbSampleRecord.AcquisitionDateTP;
	sampleRecord.sample_identifier = dbSampleRecord.SampleNameStr;
	sampleRecord.bp_qc_identifier = {};

	QualityControlDLL qc = {};
	if (getQualityControlByUUID(dbSampleRecord.QcId, qc))
	{
		sampleRecord.bp_qc_identifier = qc.qc_name;
	}

	sampleRecord.dilution_factor = dbSampleRecord.Dilution;
	sampleRecord.wash = static_cast<eSamplePostWash>(dbSampleRecord.WashTypeIndex);
	sampleRecord.comment = dbSampleRecord.Label;

	sampleRecord.reagent_info_records = {};	//TODO: currently not written by DB code...
	if (dbSampleRecord.ReagentTypeNameList.size() > 0)
	{
		for (uint16_t i = 0; i < dbSampleRecord.NumReagents; i++)
		{
			ReagentInfoRecordDLL reagentInfo;
			reagentInfo.reagent_label = dbSampleRecord.ReagentTypeNameList[i];
			reagentInfo.pack_number = dbSampleRecord.ReagentPackNumList[i];
			reagentInfo.lot_number = std::stol (dbSampleRecord.PackLotNumList[i]);
			reagentInfo.effective_expiration_date = dbSampleRecord.PackLotExpirationList[i];
			reagentInfo.in_service_date = dbSampleRecord.PackInServiceList[i];
			reagentInfo.expiration_date = dbSampleRecord.PackServiceExpirationList[i];
			sampleRecord.reagent_info_records.push_back (reagentInfo);
		}
	}

	sampleRecord.imageNormalizationData = dbSampleRecord.DustRefImageSetId;

	// Find all the analysis records for this sample id.
	std::vector<DBApi::DB_AnalysisRecord> dbAnalysisRecords = {};
	he = read_DBAnalysisRecords (dbSampleRecord.SampleId, dbAnalysisRecords);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	// Populate the ResultSummary records for each analysis.
	for (auto& v : dbAnalysisRecords)
	{
		ResultSummaryDLL resultSummary = {};
		decode_DBSummaryResultRecord (v.SummaryResult, resultSummary);
		resultSummary.sresultUuid = v.SResultId;
		UserList::Instance().GetUsernameByUUID (v.AnalysisUserId, resultSummary.username);
		sampleRecord.result_summaries.push_back (resultSummary);
	}

	sampleRecord.sam_comp_status = {};	//TODO: need converter: dbSampleRecord.SampleStatus;
//	sampleRecord.sam_comp_status = static_cast<sample_completion_status>(dbSampleRecord.SampleStatus);

	if (!Uuid::IsValid(dbSampleRecord.ImageSetId))
	{
		Logger::L().Log (MODULENAME, severity_level::warning,
			boost::str (boost::format ("decode_DBSampleRecord: <DBSampleRecord %s has invalid ImageSetId: %s>")
				% Uuid::ToStr(dbSampleRecord.SampleId)
				% Uuid::ToStr(dbSampleRecord.ImageSetId)));
		return he;
	}

	DBApi::DB_ImageSetRecord imageSetRecord = {};
	he = read_DBImageSetRecord (dbSampleRecord.ImageSetId, imageSetRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	for (auto& v : imageSetRecord.ImageSequenceList)
	{
		sampleRecord.imageSequences.push_back (v.ImageSequenceId);
	}

#endif

	return he;
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleSResult (const uuid__t& uuid, std::function<void(CellCounterResult::SResult sresult, HawkeyeError)> retrieve_cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleSResult: <enter>");

	// Find all the analysis records for this sample id.
	// These are returned in chronological order.
	std::vector<DBApi::DB_AnalysisRecord> dbAnalysisRecords = {};
	HawkeyeError he = read_DBAnalysisRecords (uuid, dbAnalysisRecords);
	if (he != HawkeyeError::eSuccess)
	{ // Errors are reported by called method.
		retrieve_cb ({}, he);
		return;
	}

//TODO: check for empty SResult UUID...

	// USing the original analysis record (the oldest) get the SResult.
	CellCounterResult::SResult sresult;
	he = retrieveSResult (dbAnalysisRecords[0].SResultId, sresult);
	if (he != HawkeyeError::eSuccess)
	{ // Errors are reported by called method.
		retrieve_cb ({}, he);
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleSResult: <exit>");

	retrieve_cb (sresult, he);
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::RetrieveDbImageSetRecord (const uuid__t& uuid, DBApi::DB_ImageSetRecord& dbImageSetRecord)
{
	return read_DBImageSetRecord (uuid, dbImageSetRecord);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveDbSampleRecord (const uuid__t& uuid, std::function<void(DBApi::DB_SampleRecord, HawkeyeError)> retrieve_cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDbSampleRecord: <enter>");

	DBApi::DB_SampleRecord dbSampleRecord = {};
	auto he = read_DBSampleRecord (uuid, dbSampleRecord);
	if (he != HawkeyeError::eSuccess)
	{ // Errors are reported by called method.
		retrieve_cb ({}, he);
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDbSampleRecord: <exit>");

	retrieve_cb (dbSampleRecord, he);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleRecord (const uuid__t& uuid, std::function<void(SampleRecordDLL, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecord: <enter>");
	SampleRecordDLL sampleRecord = {};
	if (!RetrieveSampleRecord(uuid, sampleRecord))
	{ // Errors are reported by called method.
		retrieve_cb ({}, HawkeyeError::eEntryNotFound);
	}
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecord: <exit>");
	retrieve_cb (sampleRecord, HawkeyeError::eSuccess);
}

//*****************************************************************************
bool HawkeyeResultsDataManager::RetrieveSampleRecord(const uuid__t& uuid, SampleRecordDLL& sampleRecord)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "RetrieveSampleRecord: <enter>");

	sampleRecord = {};

	DBApi::DB_SampleRecord dbSampleRecord = {};
	auto he = read_DBSampleRecord(uuid, dbSampleRecord);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "RetrieveSampleRecord: <exit, unable to find sample record in database>");
		return false;
	}

	if (decode_DBSampleRecord(dbSampleRecord, sampleRecord) != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "RetrieveSampleRecord: <exit, decode_DBSampleRecord failed>");
		return false;
	}
	// Now get the sample item for this sample 
	// This allows us to get the sample position
	// We need to use the filtered method to find the sample Id
	// We don't have the sample item ID or we could find it directly
	//
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOperations = {};
	std::vector<std::string> compareValues = {};

	filterTypes.push_back(DBApi::eListFilterCriteria::SampleIdFilter);
	compareOperations.push_back("=");
	std::string uuidStr = boost::str(boost::format("'%s'") % Uuid::ToStr(uuid));
	compareValues.push_back(uuidStr);

	std::vector<DBApi::DB_SampleItemRecord> dbSampleItemList = {};
	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleItemListEnhanced(
		dbSampleItemList,
		filterTypes,
		compareOperations,
		compareValues,
		1, // Looking for a single record
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		-1,		// Descending sort order
		"",
		-1,
		-1,
		"");

	if ((dbStatus == DBApi::eQueryResult::QueryOk) && (dbSampleItemList.size() == 1))
	{
		// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "RetrieveSampleRecord: <exit>");
		sampleRecord.SetPosition(dbSampleItemList[0].SampleRow, dbSampleItemList[0].SampleCol);
		return true;
	}
	Logger::L().Log(MODULENAME, severity_level::warning, "RetrieveSampleRecord: sample item not found for " + uuidStr);
	return false;
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleRecords(
	uint64_t start,
	uint64_t end,
	const std::string username,
	std::function<void(std::vector<SampleRecordDLL>, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecords: <enter>");

	std::string sTimeStr;
	std::string eTimeStr;
	ChronoUtilities::getListTimestampAsStr(start, end, sTimeStr, eTimeStr);

	// Set up filtering criteria.
	std::vector<DBApi::eListFilterCriteria> filterTypes = {};
	std::vector<std::string> compareOperations = {};
	std::vector<std::string> compareValues = {};
	// Save start, end and username for delete sample records
	retrieveSampleStart = start;
	retrieveSampleEnd = end;
	retrieveSampleUsername = username;

	filterTypes.push_back (DBApi::eListFilterCriteria::RunDateRangeFilter);
	compareOperations.push_back ("=");
	compareValues.push_back (boost::str (boost::format ("%s;%s") % sTimeStr % eTimeStr));

	filterTypes.push_back(DBApi::eListFilterCriteria::StatusFilter);
	compareOperations.push_back("=");
	compareValues.push_back(boost::str(boost::format("%d") % (int)DBApi::eSampleItemStatus::ItemComplete));

	std::vector<DBApi::DB_SampleItemRecord> dbSampleItemList = {};
	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleItemListEnhanced(
		dbSampleItemList,
		filterTypes,
		compareOperations,
		compareValues,
		-1,
		DBApi::eListSortCriteria::RunDateSort, // use RunDateSort not all others are supported
		DBApi::eListSortCriteria::SortNotDefined,
		-1,
		"",
		-1,
		-1,
		"");

	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			retrieve_cb ({}, HawkeyeError::eEntryNotFound);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("RetrieveSampleRecords: <exit, DbGetSamplesList failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning

		retrieve_cb ({}, HawkeyeError::eStorageFault);
		return;
	}

//TODO: temporary, for debugging sort order...
	//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecords: list of SampleRecords");
	//for (auto& v : dbSampleList)
	//{
	//	auto stime = ChronoUtilities::ConvertToString (v.AcquisitionDateTP, "%Y-%m-%d %H:%M:%S");
	//	Logger::L().Log (MODULENAME, severity_level::debug1, stime);
	//}

	uuid__t userUUID = {};
	if (!username.empty())
		UserList::Instance().GetUserUUIDByName(username, userUUID);
	Uuid usrId(userUUID);

	std::vector<SampleRecordDLL> sampleRecords = {};

	for (auto& v : dbSampleItemList)
	{

		//
		// It is possible for records to be in the sample item table and not the sample properties table
		// return all valid samples that can be found
		//
		DBApi::DB_SampleRecord sr = {};
		if (DBApi::DbFindSample(sr, v.SampleId) == DBApi::eQueryResult::QueryOk)
		{
			// Filter by user if requested
			if (!Uuid::IsClear(userUUID) && (usrId != Uuid(sr.RunUserId)))
    			continue;
			
			SampleRecordDLL srdll = {};
			auto he = decode_DBSampleRecord(sr, srdll);
			if (he != HawkeyeError::eSuccess)
			{
				Logger::L().Log(MODULENAME, severity_level::warning, "RetrieveSampleRecords: decode_DBSampleRecord failed <exit>");
				retrieve_cb({}, he);
				return;
			}
			srdll.SetPosition(v.SampleRow, v.SampleCol);
			sampleRecords.push_back(srdll);
		}
	}

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecords: <exit>");

	retrieve_cb (sampleRecords, HawkeyeError::eSuccess);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleRecordsByQCUuid(
	const uuid__t qcUuid,
	std::function<void(std::vector<SampleRecordDLL>, HawkeyeError)> retrieve_cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordsByQCUuid: <enter>");

	if (!Uuid::IsValid(qcUuid) || Uuid::IsClear(qcUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "RetrieveSampleRecordsByQCUuid: <exit: invalid UUID>");
		retrieve_cb({}, HawkeyeError::eInvalidArgs);
		return;
	}

	std::vector<DBApi::DB_SampleRecord> dbSampleList = {};
	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleList(
		dbSampleList,
		DBApi::eListFilterCriteria::QcFilter,
		"=",
		boost::str (boost::format ("'%s'") % Uuid::ToStr(qcUuid)),
		-1,
		DBApi::eListSortCriteria::CreationDateSort,
		DBApi::eListSortCriteria::SortNotDefined,
		0,
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			retrieve_cb ({}, HawkeyeError::eEntryNotFound);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("RetrieveSampleRecordsByQCUuid: <exit, DbGetSamplesList failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		retrieve_cb ({}, HawkeyeError::eDatabaseError);
		return;
	}

	//TODO: temporary, for debugging sort order...
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordsByQCUuid: list of SampleRecords");
	for (auto& v : dbSampleList)
	{
		auto stime = ChronoUtilities::ConvertToString (v.AcquisitionDateTP, "%Y-%m-%d %H:%M:%S");
		Logger::L().Log (MODULENAME, severity_level::debug1, stime);
	}

	std::vector<SampleRecordDLL> sampleRecords = {};

	for (auto& v : dbSampleList)
	{
		SampleRecordDLL sampleRecord = {};
		HawkeyeError he = decode_DBSampleRecord (v, sampleRecord);
		if (HawkeyeError::eSuccess != he)
		{ // Errors are reported by called method.
			retrieve_cb ({}, he);
			return;
		}
		sampleRecords.push_back (sampleRecord);
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordsByQCUuid: <exit>");

	retrieve_cb (sampleRecords, HawkeyeError::eSuccess);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleRecordList (const std::vector<uuid__t>& uuids, std::function<void(std::vector<SampleRecordDLL>, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordList: <enter>");

	std::vector<SampleRecordDLL> sampleRecordsDLL;

	auto data_cb = [this, &sampleRecordsDLL](SampleRecordDLL record, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			sampleRecordsDLL.push_back (record);
		}
	};

	for (auto& v : uuids)
	{
		RetrieveSampleRecord (v, data_cb);
	}

	if (sampleRecordsDLL.size())
	{
		retrieve_cb (sampleRecordsDLL, HawkeyeError::eSuccess);
	}
	else
	{
		retrieve_cb (sampleRecordsDLL, HawkeyeError::eEntryNotFound);
	}

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordList: <exit>");
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveResultRecord (const uuid__t& uuid, std::function<void(ResultRecordDLL, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecord: <enter>");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "RetrieveResultRecord: <exit: invalid UUID>");
		retrieve_cb(ResultRecordDLL{}, HawkeyeError::eInvalidArgs);
		return;
	}

	auto wrapper_cb = [this, retrieve_cb](ResultSummaryDLL resultSummary, HawkeyeError status) -> void
	{
		ResultRecordDLL resultRecord = {};

		if (status == HawkeyeError::eSuccess)
		{
			resultRecord.summary_info = resultSummary;

			std::vector<DBApi::DB_DetailedResultRecord> dbDetailedResultsRecords = {};
			HawkeyeError he = read_DBDetailedResultsRecords (resultSummary.analysisUuid, dbDetailedResultsRecords);
			if (he != HawkeyeError::eSuccess)
			{ // Errors are reported by called method.
				retrieve_cb ({}, he);
			}

			// Find the cumulative record in the list.
			auto item = std::find_if (dbDetailedResultsRecords.begin(), dbDetailedResultsRecords.end(),
				[](const auto& item) { return item.TotalCumulativeImages > 1; });

			// If there is a cumulative record, delete it.  It is not part of the detailed image results.
			if (item != dbDetailedResultsRecords.end())
			{
				dbDetailedResultsRecords.erase (item);
			}

			resultRecord.per_image_result.clear();

			for (auto& v : dbDetailedResultsRecords)
			{
				BasicResultAnswers bra = {};
				bra = DB_DetailedResultRecord_To_BasicResultAnswers (v);
				resultRecord.per_image_result.push_back (bra);
			}
		}

		// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecord: <exit>");

		retrieve_cb (resultRecord, status);
	};

	RetrieveResultSummaryRecord (uuid, wrapper_cb);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveResultRecords (uint64_t start, uint64_t end, std::string sUsername, std::function<void(std::vector<ResultRecordDLL>, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecords: <enter>");

	std::string sTimeStr;
	std::string eTimeStr;
	ChronoUtilities::getListTimesAsStr (start, end, sTimeStr, eTimeStr);

//NOTE: implemented though not currently used in the UI.

	retrieve_cb ({}, HawkeyeError::eDeprecated);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecords: <exit>");
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveResultRecordList (const std::vector<uuid__t>& uuids, std::function<void(std::vector<ResultRecordDLL>, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecordList: <enter>");

	std::vector<ResultRecordDLL> resultRecordsDLL;

	auto data_cb = [this, &resultRecordsDLL](ResultRecordDLL record, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			resultRecordsDLL.push_back (record);
		}
	};

	for (auto& v : uuids)
	{
		RetrieveResultRecord (v, data_cb);
	}

	retrieve_cb (resultRecordsDLL, HawkeyeError::eSuccess);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecordList: <exit>");
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveResultSummaryRecord (const uuid__t& summaryResultUuid, std::function<void(ResultSummaryDLL, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultSummaryRecord: <enter, summaryResultUuid: " + Uuid::ToStr (summaryResultUuid) + ">");

	if (!Uuid::IsValid(summaryResultUuid) || Uuid::IsClear(summaryResultUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "RetrieveResultSummaryRecord: <exit: invalid UUID>");
		retrieve_cb(ResultSummaryDLL{}, HawkeyeError::eInvalidArgs);
		return;
	}

	DBApi::DB_SummaryResultRecord dbSummaryResultRecord = {};
	HawkeyeError he = read_DBSummaryResultRecord (summaryResultUuid, dbSummaryResultRecord);
	if (he != HawkeyeError::eSuccess)
	{
		retrieve_cb (ResultSummaryDLL{}, he);
	}

	// Need the DB_SampleRecord to get the username...
	DBApi::DB_SampleRecord dbSampleRecord = {};
	he = read_DBSampleRecord (dbSummaryResultRecord.SampleId, dbSampleRecord);
	if (he != HawkeyeError::eSuccess)
	{ // Errors are reported by called method.
		retrieve_cb (ResultSummaryDLL{}, he);
	}

	ResultSummaryDLL summaryRecord;
	decode_DBSummaryResultRecord (dbSummaryResultRecord, summaryRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultSummaryRecord: <exit>");

	retrieve_cb (summaryRecord, he);
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveDetailedMeasurement (const uuid__t& summaryResultUuid, DetailedResultMeasurementsDLL& drm)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveDetailedMeasurement: <enter, summaryResultUuid: " + Uuid::ToStr (summaryResultUuid) + ">");

	if (!Uuid::IsValid(summaryResultUuid) || Uuid::IsClear(summaryResultUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveDetailedMeasurement: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	HawkeyeError he = HawkeyeError::eSuccess;

	CellCounterResult::SResult sresult = {};

	// A bit of slight of hand here...
	// In Reanalysis a fake UUID is used to cache the SResult of the first reanalysis pass.
	// Check to see if the fake UUID matches the one used to cache the SResult.
	if (isReanalysisSResultCached(summaryResultUuid))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveDetailedMeasurement: <exit, using cached data>");
		sresult = cached_ReanalysisSResultRecord_.second;
	}
	else
	{
		he = read_DBSummaryResultRecordReturnSResult (summaryResultUuid, sresult);
		if (he != HawkeyeError::eSuccess)
		{ // Errors are reported by called method.
			return he;
		}
	}

	drm.uuid = summaryResultUuid;
	getDetailedMeasurementFromSResult (sresult, drm);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveDetailedMeasurement: <exit>");

	return he;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveHistogram (const uuid__t & summaryResultUuid, bool only_POI, Hawkeye::Characteristic_t measurement, uint8_t bin_count, std::vector<histogrambin_t>& histogram)
{

 	DetailedResultMeasurementsDLL drm = {};
	auto status = retrieveDetailedMeasurement (summaryResultUuid, drm);
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveHistogram: <enter, error summaryResultUuid: " + Uuid::ToStr(summaryResultUuid) + ">");
		return status;
	}

	if (drm.blobs_by_image.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveRecordHistogram: <exit, validation failed>");
		return HawkeyeError::eValidationFailed; // TODO : Is this condition is validation failed or enone found ?
	}

	auto findItem = [only_POI, &measurement](const blob_measurements_tDLL& element) -> boost::optional<float>
	{
		bool isCell = false;
		bool isPOI = false;
		boost::optional<float> value;

		for (auto item : element.measurements)
		{
			auto e_key = static_cast<BaseCharacteristicKey_t>(item.characteristic.key);

			if (e_key == BaseCharacteristicKey_t::IsCell)
			{
				isCell = item.value > 0.0;
			}

			else if (e_key == BaseCharacteristicKey_t::IsPOI)
			{
				isPOI = item.value > 0.0;
			}

			else if (e_key == measurement.key)
			{
				value.reset(item.value);
			}
		}

		if (!isCell || (only_POI && !isPOI))
		{
			value.reset();
		}

		return value;
	};

	std::vector<float> matches;
	for (auto item : drm.blobs_by_image)
	{
		for (int index = 0; index < item.blob_list.size(); index++)
		{
			auto match = findItem(item.blob_list[index]);
			if (match)
			{
				matches.push_back(match.get());
			}
		}
	}

	if (matches.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveHistogram: <exit, none found>");
		return HawkeyeError::eNoneFound;
	}

	// Sorting is done to get the low and high values for histogram plot, and to
	// ensure that bin counts can be found sequentially
	std::sort(matches.begin(), matches.end());
	auto stepSize = (matches[matches.size() - 1] - matches[0]) / bin_count;

	histogram.reserve(bin_count);

	auto half_step_size = stepSize / 2.0f;
	auto iter = matches.begin();
	auto right_edge_value = matches[0];
	for (auto index = 0; index < bin_count; index++)
	{
		histogrambin_t bin = {}; //Initialize structure
		right_edge_value += stepSize;

		// Bin values are *central* (midpoint between left and right edges)
		bin.bin_nominal_value = right_edge_value - half_step_size;

		auto start_next_bin = std::upper_bound(iter, matches.end(), right_edge_value);

		// Account for some error in the last bin that might accumulate from rounding errors.
		if (index == (bin_count - 1))
			start_next_bin = matches.end();

		bin.count = static_cast<uint32_t>(start_next_bin - iter);
		iter = start_next_bin;

		histogram.push_back (std::move(bin));
	}

	matches.clear();

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveImage (const uuid__t& uuid, int16_t& sequenceNum, cv::Mat& image)
{
	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveImage: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}


	DBApi::DB_ImageRecord dbImageRecord = {};
	HawkeyeError he = read_DBImageRecord (uuid, dbImageRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	// Get the path to the encrypted image file.
	DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
	he = read_DBImageSeqRecord (dbImageRecord.ImageSequenceId, dbImageSeqRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	DBApi::DB_ImageSetRecord dbImageSetRecord = {};
	he = read_DBImageSetRecord (dbImageSeqRecord.ImageSetId, dbImageSetRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	sequenceNum = dbImageSeqRecord.SequenceNum;

	std::string imagePath = boost::str (boost::format("%s\\%s\\%s")
		% dbImageSetRecord.ImageSetPathStr
		% dbImageSeqRecord.ImageSequenceFolderStr
		% dbImageRecord.ImageFileNameStr);

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveImage: <reading:  " + imagePath + ">");

	if (!HDA_ReadEncryptedImageFile (imagePath, image))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveImage: <exit: failed to read image:  " + imagePath + ">");
		return HawkeyeError::eSoftwareFault;  //TODO: is this correct...
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveImageSequenceSet (const uuid__t& uuid, int16_t& seqNumber, ImageSet_t& imageSet)
{
	//Keep for debugging Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveImageSequenceSet: <enter, uuid: " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveImageSequenceSet: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	// Get the path to the encrypted image file.
	DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
	HawkeyeError he = read_DBImageSeqRecord (uuid, dbImageSeqRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "dbImageSeqRecord.ImageSetId: " + Uuid::ToStr (dbImageSeqRecord.ImageSetId));

	DBApi::DB_ImageSetRecord dbImageSetRecord = {};
	he = read_DBImageSetRecord (dbImageSeqRecord.ImageSetId, dbImageSetRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	std::string imageSeqPath = boost::str (boost::format("%s\\%s\\")
		% dbImageSetRecord.ImageSetPathStr
		% dbImageSeqRecord.ImageSequenceFolderStr);

	for (auto& v : dbImageSeqRecord.ImageList)
	{
		
		//Logger::L().Log (MODULENAME, severity_level::debug1, "dbImageSeqRecord.read_DBImageRecord: " + Uuid::ToStr (v.ImageId));

		DBApi::DB_ImageRecord dbImageRecord = {};
		HawkeyeError he = read_DBImageRecord (v.ImageId, dbImageRecord);
		if (he != HawkeyeError::eSuccess)
		{
			return he;
		}

		std::string imagePath = boost::str (boost::format("%s\\%s")
			% imageSeqPath
			% dbImageRecord.ImageFileNameStr);

		// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveImageSequenceSet: <reading:  " + imagePath + ">");

		cv::Mat* image;

		if (v.ImageChannel == 1)
		{ // Brightfield.
			image = &imageSet.first;
		}
		else 
		{ // Fluorescence.
			image = &imageSet.second[v.ImageChannel - 1];
		}

		if (!HDA_ReadEncryptedImageFile (imagePath, *image))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "retrieveImage: <exit: failed to read image:  " + imagePath + ">");
			return HawkeyeError::eSoftwareFault;  //TODO: is this correct...
		}
	}

	seqNumber = dbImageSeqRecord.SequenceNum;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveImageSequenceSet: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveImageInfo(const uuid__t& uuid, int16_t& seqNumber, std::string& imagePath, uuid__t& imgRecId, system_TP& timestamp)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveImageInfo: <enter, uuid: " + Uuid::ToStr(uuid) + ">");
	
	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveImageInfo: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	// Get the path to the encrypted image file.
	DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
	HawkeyeError he = read_DBImageSeqRecord(uuid, dbImageSeqRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}
	seqNumber = dbImageSeqRecord.SequenceNum;

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "dbImageSeqRecord.ImageSetId: " + Uuid::ToStr(dbImageSeqRecord.ImageSetId));

	DBApi::DB_ImageSetRecord dbImageSetRecord = {};
	he = read_DBImageSetRecord(dbImageSeqRecord.ImageSetId, dbImageSetRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	timestamp = dbImageSetRecord.CreationDateTP;

	std::string imageSeqPath = boost::str(boost::format("%s\\%s\\")
		% dbImageSetRecord.ImageSetPathStr
		% dbImageSeqRecord.ImageSequenceFolderStr);
	DBApi::DB_ImageRecord dbImageRecord = {};

	if (!(dbImageSeqRecord.ImageList.size() > 0))
	{
		return HawkeyeError::eEntryNotFound;
	}
	he = read_DBImageRecord(dbImageSeqRecord.ImageList[0].ImageId, dbImageRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}
	imgRecId = dbImageRecord.ImageId;
	imagePath = boost::str(boost::format("%s%s")
		% imageSeqPath
		% dbImageRecord.ImageFileNameStr);

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveImageInfo: <reading:  " + imagePath + ">");

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveImageInfo: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveAnnotatedImage (const uuid__t& summaryResultUuid, const uuid__t& imageSequenceUuid, cv::Mat& annotatedImage)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveAnnotatedImage: <enter, summaryResultUuid: " + Uuid::ToStr (summaryResultUuid) + ">");

	auto valid = true; // use a flag so that we log ALL errors not just the first
	if (!Uuid::IsValid(summaryResultUuid) || Uuid::IsClear(summaryResultUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveAnnotatedImage: <exit: invalid summaryResultUuid>");
		valid = false;
	}

	if (!Uuid::IsValid(imageSequenceUuid) || Uuid::IsClear(imageSequenceUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveAnnotatedImage: <exit: invalid imageSequenceUuid>");
		valid = false;
	}
	if (!valid) 
	{
		return HawkeyeError::eInvalidArgs;
	}

#ifndef DATAIMPORTER

	cv::Mat raw_image = {};
	int16_t sequenceNum = 0;
	auto he = retrieveImage (imageSequenceUuid, sequenceNum, raw_image);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	CellCounterResult::SResult sresult = {};
	if (isReanalysisSResultCached(summaryResultUuid))
	{ // Retrieve cached Reanalysis SResult data...
		Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveDetailedMeasurement: <exit, using cached data>");
		sresult = cached_ReanalysisSResultRecord_.second;
	}
	else
	{
		he = read_DBSummaryResultRecordReturnSResult (summaryResultUuid, sresult);
		if (he != HawkeyeError::eSuccess)
		{
			return he;
		}
	}

	if (!ImageWrapperUtilities::AnnotateImage (raw_image, annotatedImage, sresult, sequenceNum, ImageWrapperUtilities::AnnotationType::NonCells))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "retrieveAnnotatedImage: <exit, failed to annotate the image>");
		return HawkeyeError::eEntryInvalid;
	}

#endif

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveAnnotatedImage: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveBWImage (const uuid__t & imageUuid, cv::Mat & img)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveBWImage: <enter, imageUuid: " + Uuid::ToStr (imageUuid) + ">");

	if (!Uuid::IsValid(imageUuid) || Uuid::IsClear(imageUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "retrieveBWImage: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

#ifndef DATAIMPORTER

	cv::Mat raw_image = {};
	int16_t sequenceNum = 0;
	auto he = retrieveImage (imageUuid, sequenceNum, raw_image);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	const int bwImageThreshold = 127;

	if (!ImageWrapperUtilities::ConvertToBWImage (raw_image, img, bwImageThreshold))
	{
		return HawkeyeError::eValidationFailed;
	}

#endif

	Logger::L().Log (MODULENAME, severity_level::debug1, "retrieveBWImage: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeResultsDataManager::saveAnalysisData(
	uuid__t worklistUUID,
	const SampleDefinitionDLL& sampleDef,	//TODO: is this needed???
	std::shared_ptr<ImageCollection_t> imageCollection,
	const CellCounterResult::SResult& sresult,
	const std::vector<ReagentInfoRecordDLL>& reagentinfoRecords, 
	boost::optional<ImageSet_t> dustReferenceImages,
	DBApi::DB_SampleRecord dbSampleRecord,
	DBApi::DB_AnalysisRecord dbAnalysisRecord,
	QcStatus qcStatus,
	std::function<void(SampleDefinitionDLL, bool)> onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "saveAnalysisData: <enter>");

	HAWKEYE_ASSERT (MODULENAME, onComplete);

	// Check if this data if from a Sample or Reanalysis.
	dbSampleRecord.NumReagents = static_cast<uint16_t>(reagentinfoRecords.size());
	dbSampleRecord.QcId = sampleDef.parameters.qc_uuid;

	dbSampleRecord.RunUserId = sampleDef.runUserID;
	UserList::Instance().GetUsernameByUUID( dbSampleRecord.RunUserId, dbSampleRecord.RunUserNameStr );
	dbSampleRecord.OwnerUserNameStr = sampleDef.username;
	UserList::Instance().GetUserUUIDByName( dbSampleRecord.OwnerUserNameStr, dbSampleRecord.OwnerUserId );

	//TODO: currently these are not written by the DB code...
	for (auto& reagentPackInfo : reagentinfoRecords)
	{
		dbSampleRecord.ReagentTypeNameList.push_back (reagentPackInfo.reagent_label);
		dbSampleRecord.ReagentPackNumList.push_back (reagentPackInfo.pack_number);
		dbSampleRecord.PackLotNumList.push_back (boost::str (boost::format ("%06d") % reagentPackInfo.lot_number));
		dbSampleRecord.PackLotExpirationList.push_back (reagentPackInfo.effective_expiration_date);
		dbSampleRecord.PackInServiceList.push_back (reagentPackInfo.in_service_date);
		dbSampleRecord.PackServiceExpirationList.push_back (reagentPackInfo.expiration_date);
	}

	// Work around since "sampleDef" is const.
	SampleDefinitionDLL sampleToWrite = sampleDef;
	sampleToWrite.sampleDataUuid = dbSampleRecord.SampleId;

	std::string dateStr = ChronoUtilities::ConvertToString (ChronoUtilities::CurrentTime(), "%Y-%m");

	// imageDataForSResult is only used in the DataImporter to get the image times into the DetailedResult record.
	ImageDataForSResultMap imageDataForSResult = {};

	uuid__t imageSetUUID = {};
	if (!saveAnalysisImages (dateStr, worklistUUID, dbSampleRecord, dbAnalysisRecord, sampleToWrite, imageCollection, sresult, imageDataForSResult, imageSetUUID))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "saveAnalysisData: <exit, failed to save the raw images>");
		if (onComplete) {
			onComplete (sampleDef, false);
		}
		return;
	}

	// The Dust Reference image(s) are only saved once per Worklist run since a Dust Reference can only be generated.
	// All sample data references the same Dust Reference that was defined at the beginning of the Worklist.
//TODO: is there a way to refer to an existing Dust Reference???
//TODO: dust reference image(s) must be saved in a common location and loaded from that point...  and then written to the DB when saved...
	if (doSaveDustReference_ && dustReferenceImages)
	{
		if (!saveDustReferenceImages (dateStr, worklistUUID, dbSampleRecord, sampleToWrite, dustReferenceImages, dustRefImageSetId))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "saveAnalysisData: <exit, failed to save the dust reference images>");
			if (onComplete) {
				onComplete (sampleDef, false);
			}
			return;
		}

		doSaveDustReference_ = false;
	}

	dbSampleRecord.DustRefImageSetId = dustRefImageSetId;

	{ // Write DB_SummaryResultRecord.
		BasicResultAnswers bra = {};
		ImageAnalysisUtilities::getBasicResultAnswers (sresult.Cumulative_results, bra);

		auto& summaryResultRecord = dbAnalysisRecord.SummaryResult;

		summaryResultRecord.SampleId              = dbSampleRecord.SampleId;
		summaryResultRecord.ImageSetId            = imageSetUUID;
		summaryResultRecord.AnalysisId            = dbAnalysisRecord.AnalysisId;
		summaryResultRecord.AnalysisParamId = {};	//TODO: this is not needed...
		summaryResultRecord.AnalysisDefId         = sampleDef.parameters.analysis.uuid;
		summaryResultRecord.ImageAnalysisParamId = {};	//TODO: this is not needed...
		summaryResultRecord.CellTypeId            = sampleDef.parameters.celltype.uuid;
		summaryResultRecord.CellTypeIndex         = sampleDef.parameters.celltype.celltype_index;
		summaryResultRecord.ProcessingStatus      = static_cast<int16_t>(sampleDef.status);		//TODO: is this correct???
		summaryResultRecord.TotalCumulativeImages = static_cast<int16_t>(bra.nTotalCumulative_Imgs);
		summaryResultRecord.TotalCells_GP         = bra.count_pop_general;
		summaryResultRecord.TotalCells_POI        = bra.count_pop_ofinterest;
		summaryResultRecord.POI_PopPercent        = bra.percent_pop_ofinterest;
		summaryResultRecord.CellConc_GP           = bra.concentration_general;
		summaryResultRecord.CellConc_POI          = bra.concentration_ofinterest;
		summaryResultRecord.AvgDiam_GP            = bra.avg_diameter_pop;
		summaryResultRecord.AvgDiam_POI           = bra.avg_diameter_ofinterest;
		summaryResultRecord.AvgCircularity_GP     = bra.avg_circularity_pop;
		summaryResultRecord.AvgCircularity_POI    = bra.avg_circularity_ofinterest;
		summaryResultRecord.CoefficientOfVariance = bra.coefficient_variance;
		summaryResultRecord.AvgCellsPerImage      = bra.average_cells_per_image;
		summaryResultRecord.AvgBkgndIntensity     = bra.average_brightfield_bg_intensity;
		summaryResultRecord.TotalBubbleCount      = bra.bubble_count;
		summaryResultRecord.LargeClusterCount     = bra.large_cluster_count;
		summaryResultRecord.QcStatus              = static_cast<uint16_t>(qcStatus);
	}

	dbAnalysisRecord.SampleId = dbSampleRecord.SampleId;
	dbAnalysisRecord.ImageSetId = imageSetUUID;
	dbAnalysisRecord.QcId = dbSampleRecord.QcId;

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveAnalysisData: START SRESULT WRITE...");

	uuid__t sresultId = {};
	if (SResultData::Write (dbSampleRecord.SampleId, dbAnalysisRecord.AnalysisId, sresult, imageDataForSResult, sresultId) != HawkeyeError::eSuccess)
	{ // Error is reported in "SResultData::Write".
		onComplete (sampleDef, false);
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveAnalysisData: END SRESULT WRITE...");

	dbAnalysisRecord.SResultId = sresultId;

	DBApi::eQueryResult dbStatus = DBApi::DbModifyAnalysis (dbAnalysisRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveAnalysisData: <exit, update of DB_SampleRecord failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		if (onComplete) {
			onComplete (sampleToWrite, false);
		}
		return;
	}

	// Update DB_SampleRecord with the image set record UUID and mark it as complete.
	dbSampleRecord.ImageSetId = imageSetUUID;
	dbSampleRecord.SampleStatus = static_cast<int32_t>(DBApi::eSampleStatus::SampleComplete);

	dbStatus = DBApi::DbModifySample (dbSampleRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveAnalysisData: <exit, update of DB_SampleRecord failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		if (onComplete) {
			onComplete (sampleToWrite, false);
		}
		return;
	}

	// An empty "sampleDefUuid" indicates that this data is from the v1.2 DataImporter process.
	// v1.2 imported data does not have the concept of a SampleItem record, at least not one that is meaningful.
	if (!Uuid::IsClear(sampleToWrite.sampleDefUuid))
	{
		// Read the SampleItemRecord using the UUID from the SampleDefinitionDLL object.
		DBApi::DB_SampleItemRecord dbSampleItemRecord = {};
		dbStatus = DBApi::DbFindSampleItem (dbSampleItemRecord, sampleToWrite.sampleDefUuid);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("saveAnalysisData: <exit, read DB_SampleItemRecord failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::error));
			if (onComplete) {
				onComplete (sampleToWrite, false);
			}
			return;
		}

		dbSampleItemRecord.SampleId = dbSampleRecord.SampleId;
		dbSampleItemRecord.RunDateTP = ChronoUtilities::CurrentTime();

		dbStatus = DBApi::DbModifySampleItem (dbSampleItemRecord);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("saveAnalysisData: <exit, write DbModifySampleItem failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::sample,
				instrument_error::severity_level::error));
			if (onComplete) {
				onComplete (sampleToWrite, false);
			}
			return;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveAnalysisData: <exit>");

	if (onComplete) {
		onComplete (sampleToWrite, true);
	}
}

///*****************************************************************************
void HawkeyeResultsDataManager::SaveReanalysisData(
	const CellCounterResult::SResult& sresult,
	DBApi::DB_SampleRecord dbSampleRecord,
	DBApi::DB_AnalysisRecord dbAnalysisRecord,
	QcStatus qcStatus,
	std::function<void(bool)> onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "SaveReanalysisData: <enter>");
	HAWKEYE_ASSERT (MODULENAME, onComplete);

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveReanalysisData: START SRESULT WRITE...");

	uuid__t sresultId = {};
	if (SResultData::Write (dbSampleRecord.SampleId, dbAnalysisRecord.AnalysisId, sresult, std::map<int, ImageDataForSResult>(), sresultId) != HawkeyeError::eSuccess)
	{ // Error is reported in "SResultData::Write".
		if (onComplete) {
			onComplete (false);
		}
	}

	// Invalidate this cache record so that the SResult will be retrieved on the next read.
	cached_DBSResultRecord_.first = {};

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveReanalysisData: END SRESULT WRITE...");

	dbAnalysisRecord.SResultId = sresultId;

	dbSampleRecord.DustRefImageSetId = dustRefImageSetId;

	{ // Write DB_SummaryResultRecord.
		BasicResultAnswers bra = {};
		ImageAnalysisUtilities::getBasicResultAnswers (sresult.Cumulative_results, bra);

		auto& summaryResultRecord = dbAnalysisRecord.SummaryResult;

		summaryResultRecord.SampleId              = dbSampleRecord.SampleId;
		summaryResultRecord.ImageSetId            = dbSampleRecord.ImageSetId;
		summaryResultRecord.AnalysisId            = dbAnalysisRecord.AnalysisId;
		summaryResultRecord.AnalysisParamId       = {};	//TODO: this is not needed...
		summaryResultRecord.AnalysisDefId         = dbSampleRecord.AnalysisDefId;
		summaryResultRecord.ImageAnalysisParamId  = {};	//TODO: this is not needed...
		summaryResultRecord.CellTypeId            = dbSampleRecord.CellTypeId;
		summaryResultRecord.CellTypeIndex         = dbSampleRecord.CellTypeIndex;
		summaryResultRecord.ProcessingStatus      = static_cast<int16_t>(dbSampleRecord.SampleStatus);		//TODO: is this correct???
		summaryResultRecord.TotalCumulativeImages = static_cast<int16_t>(bra.nTotalCumulative_Imgs);
		summaryResultRecord.TotalCells_GP         = bra.count_pop_general;
		summaryResultRecord.TotalCells_POI        = bra.count_pop_ofinterest;
		summaryResultRecord.POI_PopPercent        = bra.percent_pop_ofinterest;
		summaryResultRecord.CellConc_GP           = bra.concentration_general;
		summaryResultRecord.CellConc_POI          = bra.concentration_ofinterest;
		summaryResultRecord.AvgDiam_GP            = bra.avg_diameter_pop;
		summaryResultRecord.AvgDiam_POI           = bra.avg_diameter_ofinterest;
		summaryResultRecord.AvgCircularity_GP     = bra.avg_circularity_pop;
		summaryResultRecord.AvgCircularity_POI    = bra.avg_circularity_ofinterest;
		summaryResultRecord.CoefficientOfVariance = bra.coefficient_variance;
		summaryResultRecord.AvgCellsPerImage      = bra.average_cells_per_image;
		summaryResultRecord.AvgBkgndIntensity     = bra.average_brightfield_bg_intensity;
		summaryResultRecord.TotalBubbleCount      = bra.bubble_count;
		summaryResultRecord.LargeClusterCount     = bra.large_cluster_count;
		summaryResultRecord.QcStatus              = static_cast<uint16_t>(qcStatus);
	}

	dbAnalysisRecord.SampleId = dbSampleRecord.SampleId;
	dbAnalysisRecord.ImageSetId = dbSampleRecord.ImageSetId;

	DBApi::eQueryResult dbStatus = DBApi::DbModifyAnalysis (dbAnalysisRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveReanalysisData: <exit, update of DB_SampleRecord failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		if (onComplete) {
			onComplete (false);
		}
		return;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveReanalysisData: <exit>");

	if (onComplete) {
		onComplete (true);
	}
}

#ifndef DATAIMPORTER
//*****************************************************************************
bool HawkeyeResultsDataManager::saveLegacyCellCountingData (
	std::shared_ptr<ImageCollection_t> imageCollection, 
	std::string sampleLabel,
	CellCounterResult::SResult sresult)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "saveLegacyCellCountingData: <enter>");

	// Create the OutPut Directory
	boost::posix_time::ptime pt = boost::posix_time::second_clock::local_time();
	std::string  timeStamp = to_iso_extended_string(pt);
	std::replace(timeStamp.begin(), timeStamp.end(), ':', '-');
	std::replace(timeStamp.begin(), timeStamp.end(), 'T', '_');
	timeStamp.erase(std::remove(timeStamp.begin(), timeStamp.end(), '-'), timeStamp.end());

	std::string outputBaseDir = boost::str(boost::format("%s\\%s_%s")
										   % HawkeyeDirectory::Instance().getLegacyCellCountingDataDir()
										   % timeStamp
										   % sampleLabel);
	std::string strCompleteAnalysisFolderPath = outputBaseDir + "\\ScoutCumulativeResult";
	std::string strImagesFolderPath = outputBaseDir + "\\ScoutAnnotateImages";
	std::string strFinalDestinationFolderForSingleImage = outputBaseDir + "\\ScoutSingleImageAnalysis";

	if (!FileSystemUtilities::CreateDirectories(strCompleteAnalysisFolderPath) ||
		!FileSystemUtilities::CreateDirectories(strImagesFolderPath) ||
		!FileSystemUtilities::CreateDirectories(strFinalDestinationFolderForSingleImage))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "saveLegacyCellCountingData: <exit, Failed to Create the Leagacy Cell Counting Directories>");
		return false;
	}

	// Create & write the FCS version 3.0 data to specified path from the user.
	ImageAnalysisUtilities::writeFCSData (outputBaseDir, sresult);

	Logger::L().Log(MODULENAME, severity_level::debug1, "Writing ExcelResults...");

	std::string cumulativeResultFileName = boost::str(boost::format("%s_%s") % timeStamp % sampleLabel);

	// Function to save the consolidated analysis results in excel sheet.
	SaveResult::SaveAnalysisInfo (sresult, strCompleteAnalysisFolderPath, cumulativeResultFileName);

	if (!imageCollection)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "saveLegacyCellCountingData: <exit -No images to save>");
		return true;
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "Writing annotated images and single-image results...");

	for (std::size_t imgno = 1; imgno <= imageCollection->size(); imgno++)
	{
		if (sresult.map_Image_Results[(int)imgno].eProcessStatus == eSuccess)
		{
			std::stringstream ss;

			// Function to draw the blob information on the image.
			ss << "(Annotate...";
			cv::Mat oMatDrawImage;
			if (!ImageWrapperUtilities::AnnotateImage((*imageCollection)[imgno - 1].first, oMatDrawImage, sresult, (int)imgno, ImageWrapperUtilities::AnnotationType::Text | ImageWrapperUtilities::AnnotationType::NonCells))
			{
				Logger::L().Log (MODULENAME, severity_level::error, "Failed to annotate image");
			}

			ss << "save image...";
			std::stringstream ostrStream;
			ostrStream << std::setw(4) << std::setfill('0') << (imgno);
			std::string strImageName = strImagesFolderPath + "\\anotated_" + "_" + ostrStream.str() + ".png";

			//To write the drawn image to the local folder
			imwrite(strImageName, oMatDrawImage);

			ss << "save csv...";
			std::string strPathToSave = strFinalDestinationFolderForSingleImage + "\\imageInfo_" + std::to_string(imgno) + ".csv";

			//To save single image analysis info for Hunter.
			SaveResult::SaveSingleImageInfo (sresult, strPathToSave, (int)imgno);

			ss << ")";
			Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());
		}
	}

	uint32_t imageNum = 0;
	for (auto v : (*imageCollection))
	{
		//TODO: now to save the FL images???
		//TODO: get the *dataType* based on the LED wavelength...
		std::string dataPath;
		std::string dataType;
		if (!createLegacyImageDataDirectories(imageNum, outputBaseDir) ||
			!getLegacyDataPath(HawkeyeConfig::LedType::LED_BrightField, dataPath) ||
			!getLegacyDataType(HawkeyeConfig::LedType::LED_BrightField, dataType))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "saveLegacyCellCountingData: <exit, failed to Create the Image Data Directories>");
			return false;
		}

		std::string imagePath = boost::str(boost::format("%s\\%s_%d.png") % dataPath % dataType % imageNum);

		// Must use the C language API as the C++ API causes the application to crash
		// when building for debug.
		// Refer to: http://answers.opencv.org/question/93302/save-png-with-imwrite-read-access-violation/

		const auto ipl_img = IplImage(v.first);
		cvSaveImage(imagePath.c_str(), &ipl_img);
		// This C++ API crashes in debug mode.
		//	imwrite ("testimage.png", image);
		Logger::L().Log (MODULENAME, severity_level::debug1, "saveLegacyCellCountingData: writing " + imagePath);

		imageNum++;
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "saveLegacyCellCountingData: <exit>");
	return true;
}
#endif

#ifndef DATAIMPORTER

std::string SignSignatureText (const std::string& inputText);
//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::signResult (const uuid__t& rr_uuid, const DataSignatureInstanceDLL& sig, std::string& audit_log_str)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "signResult: <enter>");
	Logger::L().Log (MODULENAME, severity_level::debug1, "signResult: " + Uuid::ToStr (rr_uuid));

	audit_log_str = {};

	DBApi::DB_SummaryResultRecord dbSummaryResultRecord = {};
	HawkeyeError he = read_DBSummaryResultRecord (rr_uuid, dbSummaryResultRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	CellTypeDLL celltype = {};

	DBApi::DB_SampleRecord dbSampleRecord = {};
	DBApi::eQueryResult dbStatus = DBApi::DbFindSample (dbSampleRecord, dbSummaryResultRecord.SampleId);
	if (dbStatus == DBApi::eQueryResult::QueryOk)
	{
		celltype = CellTypesDLL::getCellTypeByUUID (dbSampleRecord.CellTypeId);
	}

	audit_log_str = boost::str (boost::format ("|Signature: %s, Applied to: %s, User: %s, Timestamp: %s UTC, CellType: %s")
		% sig.signature.short_text
		% dbSampleRecord.SampleNameStr
		% sig.signing_user
		% ChronoUtilities::ConvertToString(sig.timestamp)
		% celltype.label);

	DBApi::db_signature_t dbSignature = {};

	dbSignature.userName       = sig.signing_user;
	dbSignature.signatureTime  = sig.timestamp;
	dbSignature.shortSignature = sig.signature.short_text;
	dbSignature.longSignature  = sig.signature.long_text;

	auto time = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(dbSignature.signatureTime);
	std::string buf = dbSignature.userName + dbSignature.shortSignature + dbSignature.longSignature + std::to_string(time);
	size_t len = dbSignature.userName.length() + dbSignature.shortSignature.length() + dbSignature.longSignature.length() + sizeof(time);

	dbSignature.signatureHash  = SignaturesDLL::SignText (buf);

	dbSummaryResultRecord.SignatureList.push_back (dbSignature);

	dbStatus = DBApi::DbModifySummaryResult (dbSummaryResultRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("signResult: <exit, update of DB_SummaryResultRecord failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		return he;
	}

	// Invalidate this cache record so that the signature data will be retrieved on the next read.
	cached_DBSummaryResultRecord_.first = {};

	Logger::L().Log (MODULENAME, severity_level::debug1, "signResult: <exit>");
	return HawkeyeError::eSuccess;
}

#endif

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::deleteSampleRecord (const uuid__t& sampleRecordUuid, bool retain_results_and_first_image, std::string& audit_log_str)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "deleteSampleRecord: <enter, SampleRecord:" + Uuid::ToStr (sampleRecordUuid) + ">");

	audit_log_str = {};
	bool partial_failure = false;

	DBApi::eQueryResult dbStatus = DBApi::eQueryResult::QueryFailed;

	// Find the SampleItem record for the SampleProperties record so that its status can be updated.
	std::vector<DBApi::DB_SampleItemRecord> dbSampleItemList = {};

	dbStatus = DBApi::DbGetSampleItemList(
		dbSampleItemList,
		DBApi::eListFilterCriteria::SampleIdFilter,
		"=",
		boost::str (boost::format ("'%s'") % Uuid::ToStr(sampleRecordUuid)),
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0,
		"",
		-1,
		-1);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log (MODULENAME, severity_level::warning,
			boost::str (boost::format ("deleteSampleRecord: <exit, DbGetSampleItemList failed, status: %ld>") % (int32_t)dbStatus));
		return HawkeyeError::eStorageFault;
	}

	std::string retain_results_and_first_image_str = {};
	
	if (retain_results_and_first_image)
	{
		DBApi::DB_SampleRecord dbSampleRecord = {};
		auto he = read_DBSampleRecord (sampleRecordUuid, dbSampleRecord);
		if (he != HawkeyeError::eSuccess)
		{ // Errors are reported by called method.
			return he;
		}

		// Delete all the images except the first one.
		Logger::L().Log (MODULENAME, severity_level::debug1, "deleteSampleRecord: <removing all but one Image files from: " + Uuid::ToStr (dbSampleRecord.ImageSetId) + ">");

		// Check for invalid image set ID
		if (Uuid::IsClear(dbSampleRecord.ImageSetId) || !Uuid::IsValid(dbSampleRecord.ImageSetId))
		{
			Logger::L().Log(MODULENAME, severity_level::warning,
				std::string("deleteSampleRecord: <Invalid Image Set ID for sample record>"));
		}
		else
		{
			// Get the ImageSet to be deleted.
			DBApi::DB_ImageSetRecord imageSetRecord = {};
			dbStatus = DBApi::DbFindImageSet(imageSetRecord, dbSampleRecord.ImageSetId);
			if (DBApi::eQueryResult::QueryOk != dbStatus)
			{
				Logger::L().Log(MODULENAME, severity_level::error,
					boost::str(boost::format("deleteSampleRecord: <exit, DbFindImageSet failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_deleteerror,
					instrument_error::instrument_storage_instance::sample_item,
					instrument_error::severity_level::warning));
				partial_failure = true;
			}

			std::vector<uuid__t> imageUuidsToDelete = {};
			for (auto iter = imageSetRecord.ImageSequenceList.begin(); iter != imageSetRecord.ImageSequenceList.end();)
			{
				if (iter->SequenceNum == 1)
				{
					++iter;
				}
				else
				{
					std::string filePathToDelete = boost::str(boost::format("%s\\%s")
						% imageSetRecord.ImageSetPathStr
						% iter->ImageSequenceFolderStr);

					// Delete ImageSet files.
					if (!FileSystemUtilities::RemoveAll(filePathToDelete))
					{
						Logger::L().Log(MODULENAME, severity_level::error,
							boost::str(boost::format("deleteSampleRecord: <error deleting ImageSequence %s>") % filePathToDelete));
						ReportSystemError::Instance().ReportError(BuildErrorInstance(
							instrument_error::instrument_storage_deleteerror,
							instrument_error::instrument_storage_instance::sample_item,
							instrument_error::severity_level::warning));
					}

					imageUuidsToDelete.push_back(iter->ImageSequenceId);
					iter = imageSetRecord.ImageSequenceList.erase(iter);
				}
			}

			imageSetRecord.ImageSequenceCount = 1;

			dbStatus = DBApi::DbModifyImageSet(imageSetRecord);
			if (DBApi::eQueryResult::QueryOk != dbStatus)
			{
				Logger::L().Log(MODULENAME, severity_level::error,
					boost::str(boost::format("deleteSampleRecord: <error updating ImageSet record: %s>") % Uuid::ToStr(dbSampleRecord.ImageSetId)));
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_deleteerror,
					instrument_error::instrument_storage_instance::sample_item,
					instrument_error::severity_level::warning));
				partial_failure = true;
			}
			// Check if there are any UUIDs to delete.
			if (imageUuidsToDelete.size())
			{
				// Keep the first ImageSequence and delete all the rest.
				dbStatus = DBApi::DbRemoveImageSequencesByUuidList(imageUuidsToDelete);
				if (DBApi::eQueryResult::QueryOk != dbStatus)
				{
					Logger::L().Log(MODULENAME, severity_level::error,
						boost::str(boost::format("deleteSampleRecord: <error deleting ImageSequences: %s>") % imageSetRecord.ImageSetPathStr));
					ReportSystemError::Instance().ReportError(BuildErrorInstance(
						instrument_error::instrument_storage_deleteerror,
						instrument_error::instrument_storage_instance::sample_item,
						instrument_error::severity_level::warning));
					partial_failure = true;
				}
			}
		}
		retain_results_and_first_image_str = " Results and first image are retained";

//TODO: need to delete records from ImageResults and DetailedResults.

	}
	else
	{
		// If sample item found, mark for deletion
		if (dbSampleItemList.size() > 0)
		{
			DBApi::DB_SampleItemRecord& sampleItemRec = dbSampleItemList[0];

			// Mark the SampleItem record as deleted.
			sampleItemRec.SampleItemStatus = static_cast<int32_t>(DBApi::eSampleItemStatus::ItemDeleted);
			dbStatus = DBApi::DbModifySampleItem(sampleItemRec);
			if (DBApi::eQueryResult::QueryOk != dbStatus)
			{
				Logger::L().Log(MODULENAME, severity_level::error,
					boost::str(boost::format("deleteSampleRecord: <exit, DbModifySampleItem failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_deleteerror,
					instrument_error::instrument_storage_instance::sample_item,
					instrument_error::severity_level::warning));
				partial_failure = true;
			}

			Logger::L().Log(MODULENAME, severity_level::debug1, "deleteSampleRecord: <SampleItem: " + Uuid::ToStr(sampleItemRec.SampleItemId) + ">");
		}
		// Find ALL the Analysis record(s) for the Sample record.
		std::vector<DBApi::DB_AnalysisRecord> dbAnalysisList = {};

		dbStatus = DBApi::DbGetAnalysesList(
			dbAnalysisList,
			DBApi::eListFilterCriteria::SampleIdFilter,
			"=",
			boost::str (boost::format ("'%s'") % Uuid::ToStr(sampleRecordUuid)),
			-1,
			DBApi::eListSortCriteria::SortNotDefined,
			DBApi::eListSortCriteria::SortNotDefined,
			0,
			"",
			-1,
			-1);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("deleteSampleRecord: <exit, DbGetAnalysesList failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::sample_item,
				instrument_error::severity_level::warning));
			partial_failure = true;
		}

		// Delete all the SummaryResults and Analyses for this SampleRecord.
		// Dust References are never deleted as they may point to other records.
		// Analysis record contains: SampleId, SummaryResultId, SResultId.
		for (const auto& analysis : dbAnalysisList)
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "deleteSampleRecord: <Analysis: " + Uuid::ToStr (analysis.AnalysisId) + ">");

			Logger::L().Log (MODULENAME, severity_level::debug1, "deleteSampleRecord: <removing ImageSet files: " + Uuid::ToStr (analysis.ImageSetId) + ">");

			DBApi::DB_ImageSetRecord imageSetRecord = {};

			// Check for invalid image set ID
			if (Uuid::IsClear(analysis.ImageSetId) || !Uuid::IsValid(analysis.ImageSetId))
			{
				Logger::L().Log(MODULENAME, severity_level::warning,
					std::string("deleteSampleRecord: <Invalid Image Set ID for analysis record>"));
			}
			else
			{
				// Get the ImageSet to be deleted.
				dbStatus = DBApi::DbFindImageSet(imageSetRecord, analysis.ImageSetId);
				if (DBApi::eQueryResult::QueryOk != dbStatus)
				{
					Logger::L().Log(MODULENAME, severity_level::error,

						boost::str(boost::format("deleteSampleRecord: <exit, DbFindImageSet failed, status: %ld>") % (int32_t)dbStatus));
					ReportSystemError::Instance().ReportError(BuildErrorInstance(
						instrument_error::instrument_storage_deleteerror,
						instrument_error::instrument_storage_instance::sample_item,
						instrument_error::severity_level::warning));
					partial_failure = true;
				}
				// Delete ImageSet files.
				if (!FileSystemUtilities::RemoveAll(imageSetRecord.ImageSetPathStr))
				{
					Logger::L().Log(MODULENAME, severity_level::error,
						boost::str(boost::format("deleteSampleRecord: <error deleting ImageSet: %s>") % imageSetRecord.ImageSetPathStr));
					ReportSystemError::Instance().ReportError(BuildErrorInstance(
						instrument_error::instrument_storage_deleteerror,
						instrument_error::instrument_storage_instance::sample_item,
						instrument_error::severity_level::warning));
					partial_failure = true;
				}
			}

			Logger::L().Log (MODULENAME, severity_level::debug1, "deleteSampleRecord: <removing Analysis record: " + Uuid::ToStr (analysis.AnalysisId) + ">");

			// Delete Analysis, SummaryResult and SResult records.
			dbStatus = DBApi::DbRemoveAnalysisByUuid (analysis.AnalysisId);
			if (DBApi::eQueryResult::QueryOk != dbStatus)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("deleteSampleRecord: <exit, DbRemoveAnalysisByUuid failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_deleteerror,
					instrument_error::instrument_storage_instance::sample_item,
					instrument_error::severity_level::warning));
				partial_failure = true;
			}

		} // End "for (auto& analysis : dbAnalysisList)"

		dbStatus = DBApi::DbRemoveSampleByUuid(sampleRecordUuid);
		if (DBApi::eQueryResult::QueryOk != dbStatus)
		{
			Logger::L().Log(MODULENAME, severity_level::error,
				boost::str(boost::format("deleteSampleRecord: <exit, DbRemoveSampleByUuid failed, status: %ld>") % (int32_t)dbStatus));
				ReportSystemError::Instance().ReportError(BuildErrorInstance(
					instrument_error::instrument_storage_deleteerror,
					instrument_error::instrument_storage_instance::sample_item,
					instrument_error::severity_level::warning));
				partial_failure = true;
		}
	} // End "if (retain_results_and_first_image)"


	if (!partial_failure && (dbSampleItemList.size() > 0))
	{
		UserList::Instance().GetUsernameByUUID (dbSampleItemList[0].OwnerId, dbSampleItemList[0].OwnerNameStr);
		audit_log_str = boost::str(boost::format("Deleted sample: <Sample Name: %s, User: %s, Timestamp: %s UTC: %s> ")
			% dbSampleItemList[0].SampleItemNameStr
			% dbSampleItemList[0].OwnerNameStr
			% ChronoUtilities::ConvertToString(dbSampleItemList[0].RunDateTP)
			% retain_results_and_first_image_str);
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "deleteSampleRecord: <exit>");

	return partial_failure ? HawkeyeError::eStorageFault : HawkeyeError::eSuccess;
}

//*****************************************************************************
// Wrapper API used by DataImporter.
//*****************************************************************************
bool HawkeyeResultsDataManager::SaveAnalysisImages(
	const std::string dateStr,
	const uuid__t& worklistUUID,
	const DBApi::DB_SampleRecord& dbSampleRecord,
	const DBApi::DB_AnalysisRecord& dbAnalysisRecord,
	const SampleDefinitionDLL& sampleDefinition,
	std::shared_ptr<ImageCollection_t> imageCollection,
	const CellCounterResult::SResult& cellCounterResult,
	ImageDataForSResultMap& imageDataForSResult,
	uuid__t& imageSetUUID)
{
	return saveAnalysisImages(
		dateStr,
		worklistUUID,
		dbSampleRecord,
		dbAnalysisRecord,
		sampleDefinition,
		imageCollection,
		cellCounterResult,
		imageDataForSResult,
		imageSetUUID);
}

//*****************************************************************************
/**
 * \brief Create and persists the analysis images created from samples
 * \param dateStr Used in creating path to image files
 * \param worklistUUID Used in creating path to image files
 * \param dbSampleRecord The SampleId from the dbSampleRecord becomes part of the data written to the database
 * \param dbAnalysisRecord is not currently used. It is present in commented out code.
 * \param sampleDefinition The sampleDataUuid in the sampleDefinition is used in creating path to image files
 * \param imageCollection Having no images is a valid case, this may have been called from reanalysis which does not generate any images. The images are not yet annotated.
 * \param cellCounterResult is not currently used. It is present in commented out code.
 * \param imageSetUUID This reference to a UUID will be set after the image data is written to the database and the value is created.
 * \return true if successful, false if there was an error creating directories or writing the encrypted image files.
 */
bool HawkeyeResultsDataManager::saveAnalysisImages(
	const std::string dateStr,
	const uuid__t& worklistUUID,
	const DBApi::DB_SampleRecord& dbSampleRecord,
	const DBApi::DB_AnalysisRecord& dbAnalysisRecord,
	const SampleDefinitionDLL& sampleDefinition,
	std::shared_ptr<ImageCollection_t> imageCollection,
	const CellCounterResult::SResult& cellCounterResult,
	ImageDataForSResultMap& imageDataForSResult,
	uuid__t& imageSetUUID)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "saveAnalysisImages: <enter>");

	// Having no images is a valid case, this may have been called from reanalysis which does not generate any images.
	if (!imageCollection || imageCollection->empty())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "saveAnalysisImages: <exit, no images to save>");
		return true;
	}

	AnalysisImagePersistHelper persistHelper(dateStr, worklistUUID, sampleDefinition.sampleDataUuid, dbSampleRecord.SampleId);
	
	bool isNewData = false;

	if (imageDataForSResult.size() == 0)
	{ // New sample data...
		isNewData = true;

		Logger::L ().Log (MODULENAME, severity_level::normal, "saveAnalysisImages: <start saving images>");

		try
		{
			const size_t saveEveryNthImage = sampleDefinition.parameters.saveEveryNthImage;
			const size_t total_num_of_imagesetdata = imageCollection->size();

			if (total_num_of_imagesetdata > 0) {

				// Save first image data.
				persistHelper.storeImage (1, imageCollection->at(0));

				if (saveEveryNthImage > 0) {
					for (size_t image_set_data_index = saveEveryNthImage; image_set_data_index <= total_num_of_imagesetdata - 1; image_set_data_index += saveEveryNthImage)
					{
						if (image_set_data_index == 1) { // First image data is already saved so skip here
							continue;
						}
						persistHelper.storeImage (image_set_data_index, imageCollection->at(image_set_data_index - 1));
					}
				}

				if (total_num_of_imagesetdata > 1) {
					// Save last image data.
					persistHelper.storeImage (imageCollection->size(), imageCollection->at(imageCollection->size() - 1));
				}
			}

			Logger::L ().Log (MODULENAME, severity_level::debug1, "saveAnalysisImages: <end saving images>");

		}
		catch (std::exception& e)
		{
			Logger::L().Log(MODULENAME, severity_level::error, e.what());
			return false;
		}
	}
	else
	{ // Imported sample data...
		size_t imageIndex = 0;
		for (const auto& v : imageDataForSResult)
		{
			persistHelper.storeImage (v.first, imageCollection->at(imageIndex++));
		}
	}

	DBApi::DB_ImageSetRecord dbImageSetRecord = persistHelper.GetImageSetRecord();
	dbImageSetRecord.ImageSequenceCount = static_cast<int16_t>(dbImageSetRecord.ImageSequenceList.size());
	dbImageSetRecord.CreationDateTP = dbSampleRecord.AcquisitionDateTP;

	Logger::L ().Log (MODULENAME, severity_level::debug2, "saveAnalysisImages: <start DbAddImageSet>");

	DBApi::eQueryResult dbStatus = DBApi::DbAddImageSet (dbImageSetRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveAnalysisImages: <exit, DB add image write failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		return false;
	}

	Logger::L ().Log (MODULENAME, severity_level::debug2, "saveAnalysisImages: <end DbAddImageSet>");

	if (isNewData)
	{
		// Collect the ImageIds for sending to SResult::write.
		system_TP zeroTP;
		for (const auto& v : dbImageSetRecord.ImageSequenceList)
		{
			imageDataForSResult[v.SequenceNum].uuid = v.ImageList[0].ImageId;
			imageDataForSResult[v.SequenceNum].timestamp = zeroTP;
		}
	}

	imageSetUUID = dbImageSetRecord.ImageSetId;

	Logger::L().Log (MODULENAME, severity_level::debug2, "saveAnalysisImages: <exit>");

	return true;
}

//*****************************************************************************
//NOTE: Dust Reference is only written once per Worklist.
//*****************************************************************************
bool HawkeyeResultsDataManager::saveDustReferenceImages(
	const std::string dateStr,
	const uuid__t& worklistUUID,
	const DBApi::DB_SampleRecord& sampleRecord,
	const SampleDefinitionDLL& sample,
	const boost::optional<ImageSet_t>& dustReferenceImageSet,
	uuid__t& dustReferenceImageSetUUID)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "saveDustReferenceImages: <enter>");

	DBApi::DB_ImageSetRecord dbImageSetRecord = {};

	dbImageSetRecord.ImageSetPathStr = boost::str (boost::format ("%s\\%s\\%s")
		% HawkeyeDirectory::Instance().getImagesBaseDir()
		% dateStr
		% Uuid::ToStr(worklistUUID));

	dbImageSetRecord.ImageSequenceCount = 1;
	dbImageSetRecord.SampleId = sampleRecord.SampleId;

	int imageNum = 0;
	{
		DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};

		dbImageSeqRecord.FlChannels = 0;
		dbImageSeqRecord.ImageCount = 1 + dbImageSeqRecord.FlChannels;  // Brightfield plus any FL images.
		dbImageSeqRecord.ImageSequenceFolderStr = Uuid::ToStr(worklistUUID);
		dbImageSeqRecord.ImageSequenceId = {};
		dbImageSeqRecord.SequenceNum = static_cast<int16_t>(imageNum + 1);	//TODO: is this correct???

		// Brightfield image.
		DBApi::DB_ImageRecord dbImageRecord = {};

		dbImageRecord.ImageChannel = 1;	// Brightfield image is always channel 1.
		dbImageRecord.ImageFileNameStr = "dr_bfimage.png";
		dbImageRecord.ImageId = {};		// Filled in by DB when written.

		dbImageSeqRecord.ImageList.push_back (dbImageRecord);

		std::string imagePath = boost::str (boost::format ("%s\\%s") % dbImageSetRecord.ImageSetPathStr % dbImageSeqRecord.ImageSequenceFolderStr);

		if (!FileSystemUtilities::CreateDirectories (imagePath, false)) {
			Logger::L().Log (MODULENAME, severity_level::error, "<exit, failed to create directory: " + imagePath + ">");
			return false;
		}

		imagePath = boost::str (boost::format ("%s\\%s") % imagePath % dbImageRecord.ImageFileNameStr);

		Logger::L().Log (MODULENAME, severity_level::debug1, "Dust Reference: " + imagePath);

		if (!HDA_WriteEncryptedImageFile (dustReferenceImageSet.get().first, imagePath))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "saveDustReferenceImages: <exit: failed to write image :  " + imagePath + ">");
			return false;
		}

		//TODO: this code block has not been tested as there are no FL images available.
		if (!dustReferenceImageSet.get().second.empty())
		{
			for (auto& v : dustReferenceImageSet.get().second)
			{
				dbImageRecord = {};

				dbImageRecord.ImageChannel = static_cast<uint8_t>(v.first);
				dbImageRecord.ImageFileNameStr = boost::str (boost::format ("dr_flimage_%d.png") % dbImageRecord.ImageChannel);
				dbImageRecord.ImageId = {};		// Filled in when written.

				dbImageSeqRecord.ImageList.push_back (dbImageRecord);

				std::string imagePath = boost::str (boost::format ("%s\\%s")
					% dbImageSetRecord.ImageSetPathStr
					% dbImageRecord.ImageFileNameStr);

				if (!HDA_WriteEncryptedImageFile (v.second, imagePath))
				{
					Logger::L().Log(MODULENAME, severity_level::error, "saveDustReferenceImages: <exit: failed to write image :  " + imagePath + ">");
					return false;
				}
			}
		}

		imageNum++;

		dbImageSetRecord.ImageSequenceList.push_back (dbImageSeqRecord);

	} // End "for (auto& v : tempImageCollection)"

	dbImageSetRecord.ImageSequenceCount = static_cast<int16_t>(dbImageSetRecord.ImageSequenceList.size());

	DBApi::eQueryResult dbStatus = DBApi::DbAddImageSet (dbImageSetRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveDustReferenceImages: <exit, DB add image write failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		return false;
	}

	dustReferenceImageSetUUID = dbImageSetRecord.ImageSequenceList[0].ImageSequenceId;

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveDustReferenceImages: <exit>");

	return true;
}

//*****************************************************************************

void HawkeyeResultsDataManager::RetrieveSampleImageSetRecord (uuid__t imageSequenceUuid, std::function<void(SampleImageSetRecordDLL, HawkeyeError)> retrieve_cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecord: " + Uuid::ToStr (imageSequenceUuid));

	SampleImageSetRecordDLL sampleImageSetRecord = {};

	DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
	HawkeyeError he = read_DBImageSeqRecord (imageSequenceUuid, dbImageSeqRecord);
	if (he != HawkeyeError::eSuccess)
	{ // Errors are reported by called method.
		Logger::L().Log(MODULENAME, severity_level::warning, "RetrieveSampleImageSetRecord: read_DBImageSeqRecord failed" + Uuid::ToStr(imageSequenceUuid));
		retrieve_cb ({}, he);
		return;
	}

	sampleImageSetRecord.uuid = dbImageSeqRecord.ImageSequenceId;

	sampleImageSetRecord.sequence_number = dbImageSeqRecord.SequenceNum;
	if (dbImageSeqRecord.ImageList.size() > 0)
	{
		sampleImageSetRecord.brightfield_image = dbImageSeqRecord.ImageList[0].ImageId;
	}

	for (size_t idx = 0; idx < dbImageSeqRecord.FlChannels; idx++)
	{
		std::pair<uint16_t, uuid__t> flImage = 
			std::pair<uint16_t, uuid__t>(static_cast<uint16_t>(idx + 1), dbImageSeqRecord.ImageList[idx + 1].ImageId);
		sampleImageSetRecord.flImagesAndChannelNumlist.push_back (flImage);
	}

	DBApi::DB_ImageSetRecord dbImageSetRecord = {};
	{
		he = read_DBImageSetRecord (dbImageSeqRecord.ImageSetId, dbImageSetRecord);
		if (he != HawkeyeError::eSuccess)
		{ // Errors are reported by called method.
			retrieve_cb ({}, he);
			return;
		}

		sampleImageSetRecord.timestamp = dbImageSetRecord.CreationDateTP;

		DBApi::DB_SampleRecord dbSampleRecord = {};
		{
			he = read_DBSampleRecord (dbImageSetRecord.SampleId, dbSampleRecord);
			if (he != HawkeyeError::eSuccess)
			{ // Errors are reported by called method.
				retrieve_cb ({}, he);
				return;
			}

//TODO: need to retrieve the user base on the UUID instead...
			sampleImageSetRecord.username = dbSampleRecord.RunUserNameStr;
		}
	}

	retrieve_cb (sampleImageSetRecord, he);
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::RetrieveSampleImageSetRecord(uuid__t imageSequenceUuid, SampleImageSetRecordDLL& sampleImageSetRecord)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecord: " + Uuid::ToStr(imageSequenceUuid));

	sampleImageSetRecord = {};

	DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
	HawkeyeError he = read_DBImageSeqRecord(imageSequenceUuid, dbImageSeqRecord);
	if (he != HawkeyeError::eSuccess)
	{ 
		Logger::L().Log(MODULENAME, severity_level::error, "RetrieveSampleImageSetRecord: <Exit, failed to read image sequence record>");
		return HawkeyeError::eEntryNotFound;
	}

	sampleImageSetRecord.uuid = dbImageSeqRecord.ImageSequenceId;

	sampleImageSetRecord.sequence_number = dbImageSeqRecord.SequenceNum;
	if (dbImageSeqRecord.ImageList.size() > 0)
	{
		sampleImageSetRecord.brightfield_image = dbImageSeqRecord.ImageList[0].ImageId;
	}

	for (size_t idx = 0; idx < dbImageSeqRecord.FlChannels; idx++)
	{
		std::pair<uint16_t, uuid__t> flImage =
			std::pair<uint16_t, uuid__t>(static_cast<uint16_t>(idx + 1), dbImageSeqRecord.ImageList[idx + 1].ImageId);
		sampleImageSetRecord.flImagesAndChannelNumlist.push_back(flImage);
	}

	DBApi::DB_ImageSetRecord dbImageSetRecord = {};
	{
		he = read_DBImageSetRecord(dbImageSeqRecord.ImageSetId, dbImageSetRecord);
		if (he != HawkeyeError::eSuccess)
		{ 
			Logger::L().Log(MODULENAME, severity_level::error, "RetrieveSampleImageSetRecord: <Exit, failed to read image set record>");
			return HawkeyeError::eEntryNotFound;
		}

		sampleImageSetRecord.timestamp = dbImageSetRecord.CreationDateTP;
	}
	return HawkeyeError::eSuccess;
}


//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleImageSetRecords(
	uint64_t start, 
	uint64_t end, 
	std::string sUsername,
	std::function<void(std::vector<SampleImageSetRecordDLL>, HawkeyeError)> retrieve_cb)
{
	std::string sTimeStr;
	std::string eTimeStr;
	ChronoUtilities::getListTimesAsStr (start, end, sTimeStr, eTimeStr);

	std::vector<DBApi::DB_SampleRecord> dbSampleList = {};

	DBApi::eQueryResult dbStatus = DBApi::DbGetSampleList(
		dbSampleList,
		DBApi::eListFilterCriteria::AcquisitionDateRangeFilter,
		"",
		boost::str (boost::format ("%s;%s") % sTimeStr % eTimeStr),
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0, 
		"",
		-1, 
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			retrieve_cb ({}, HawkeyeError::eEntryNotFound);
			return;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("RetrieveSampleImageSetRecordsDLL: <exit, DbGetSampleList failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));

		retrieve_cb ({}, HawkeyeError::eStorageFault);
		return;
	}

//TODO: temporary, for debugging sort order...
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecords: list of SampleRecords");
	for (auto& v : dbSampleList)
	{
		auto stime = ChronoUtilities::ConvertToString (v.AcquisitionDateTP, "%Y-%m-%d %H:%M:%S");
		Logger::L().Log (MODULENAME, severity_level::debug1, stime);
	}

	std::vector<SampleImageSetRecordDLL> sampleImageSetRecordsDLL;
	for (auto& v : dbSampleList)
	{
		DBApi::DB_ImageSetRecord dbImageSetRecord = {};
		auto he = read_DBImageSetRecord (v.ImageSetId, dbImageSetRecord);
		if (he != HawkeyeError::eSuccess)
		{ // Errors are reported by called method.
			retrieve_cb ({}, he);
			return;
		}

		SampleImageSetRecordDLL sampleImageSetRecord = {};

//TODO: need to retrieve the user base on the UUID instead...
		sampleImageSetRecord.username = v.RunUserNameStr;

		for (auto& vv : dbImageSetRecord.ImageSequenceList)
		{
			DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
			HawkeyeError he = read_DBImageSeqRecord (vv.ImageSequenceId, dbImageSeqRecord);
			if (he != HawkeyeError::eSuccess)
			{ // Errors are reported by called method.
				retrieve_cb ({}, he);
				return;
			}

			sampleImageSetRecord.uuid = dbImageSeqRecord.ImageSetId;
			sampleImageSetRecord.timestamp = dbImageSetRecord.CreationDateTP;

			sampleImageSetRecord.sequence_number = dbImageSeqRecord.SequenceNum;
			if (dbImageSeqRecord.ImageList.size() > 0)
			{
				sampleImageSetRecord.brightfield_image = dbImageSeqRecord.ImageList[0].ImageId;
			}

			for (size_t idx = 0; idx < dbImageSeqRecord.FlChannels; idx++)
			{
				std::pair<uint16_t, uuid__t> flImage =
					std::pair<uint16_t, uuid__t>(static_cast<uint16_t>(idx + 1), dbImageSeqRecord.ImageList[idx + 1].ImageId);
				sampleImageSetRecord.flImagesAndChannelNumlist.push_back (flImage);
			}

			sampleImageSetRecordsDLL.push_back (sampleImageSetRecord);

		} // End "for (auto& vv : dbImageSetRecord.ImageSequenceList)"

	} // End "for (auto& v : dbSampleList)"

	retrieve_cb (sampleImageSetRecordsDLL, HawkeyeError::eSuccess);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveSampleImageSetRecordList(
	const std::vector<uuid__t>& uuids, 
	std::function<void(std::vector<SampleImageSetRecordDLL>, HawkeyeError)> retrieve_cb)
{
	std::vector<SampleImageSetRecordDLL> sampleImageSetRecordsDLL;

	auto data_cb = [this, &sampleImageSetRecordsDLL](SampleImageSetRecordDLL record, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			sampleImageSetRecordsDLL.push_back (record);
		}
	};

	for (auto& v : uuids)
	{
		RetrieveSampleImageSetRecord (v, data_cb);
	}

	retrieve_cb (sampleImageSetRecordsDLL, HawkeyeError::eSuccess);
}

//*****************************************************************************
void HawkeyeResultsDataManager::RetrieveResultSummaryRecordList(
	const std::vector<uuid__t>& uuids,
	std::function<void(std::vector<ResultSummaryDLL>, HawkeyeError)> retrieve_cb)
{
	std::vector<ResultSummaryDLL> resultSummaryRecordsDLL;

	auto data_cb = [this, &resultSummaryRecordsDLL](ResultSummaryDLL record, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			resultSummaryRecordsDLL.push_back (record);
		}
	};

	for (auto& v : uuids)
	{
		RetrieveResultSummaryRecord (v, data_cb);
	}

	retrieve_cb (resultSummaryRecordsDLL, HawkeyeError::eSuccess);
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::retrieveSResult (const uuid__t& sresultUuid, CellCounterResult::SResult& sresult)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveSResult: <enter, " + Uuid::ToStr (sresultUuid) + ">");

	DBApi::DB_SResultRecord dbSResultRecord = {};
	HawkeyeError he = read_DBSResultRecord (sresultUuid, dbSResultRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	sresult = SResultData::FromDBStyle (dbSResultRecord);
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "retrieveSResult: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
bool HawkeyeResultsDataManager::CreateSampleRecord (const SampleDefinitionDLL& sampleDef, DBApi::DB_SampleRecord& dbSampleRecord)
{
	// Write a dbSampleRecord that will be updated after all of the ancillary data has been written.
	dbSampleRecord.SampleStatus = SampleDefinitionDLL::SampleItemStatusToDbStyle (sampleDef.status);
	dbSampleRecord.SampleNameStr = sampleDef.parameters.label;
	dbSampleRecord.CellTypeId = sampleDef.parameters.celltype.uuid;
	dbSampleRecord.CellTypeIndex = sampleDef.parameters.celltype.celltype_index;
	dbSampleRecord.AnalysisDefId = sampleDef.parameters.analysis.uuid;
	dbSampleRecord.AnalysisDefIndex = sampleDef.parameters.analysis.analysis_index;
	dbSampleRecord.Label = sampleDef.parameters.tag;
	dbSampleRecord.WashTypeIndex = static_cast<uint16_t>(sampleDef.parameters.postWash);
	dbSampleRecord.Dilution = static_cast<uint16_t>(sampleDef.parameters.dilutionFactor);	//TODO: change the size to uint16_t...
	
	dbSampleRecord.OwnerUserNameStr = sampleDef.username;
	UserList::Instance().GetUserUUIDByName( dbSampleRecord.OwnerUserNameStr, dbSampleRecord.OwnerUserId );
	dbSampleRecord.RunUserId = sampleDef.runUserID;
	UserList::Instance().GetUsernameByUUID( dbSampleRecord.RunUserId, dbSampleRecord.RunUserNameStr );

	if (ChronoUtilities::IsZero(dbSampleRecord.AcquisitionDateTP))
	{
		dbSampleRecord.AcquisitionDateTP = ChronoUtilities::CurrentTime();
	}
	dbSampleRecord.ImageSetId = {};
	dbSampleRecord.DustRefImageSetId = {};
	dbSampleRecord.AcquisitionInstrumentSNStr = HawkeyeConfig::Instance().get().instrumentSerialNumber;

//TODO: REMOVE "tempUUID"	
	dbSampleRecord.ImageAnalysisParamId = tempUUID;
	dbSampleRecord.NumReagents = 0;

	DBApi::eQueryResult dbStatus = DBApi::DbAddSample (dbSampleRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("CreateSampleRecord: <exit, DB sample write failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		return false;
	}

	return true;
}

//*****************************************************************************
bool HawkeyeResultsDataManager::CreateAnalysisRecord (DBApi::DB_SampleRecord& dbSampleRecord, DBApi::DB_AnalysisRecord& dbAnalysisRecord)
{
	// Create an dbAnalysisRecord that will be updated after all of the ancillary data has been written.
	dbAnalysisRecord.SampleId = dbSampleRecord.SampleId;
	if (Uuid::IsClear(dbAnalysisRecord.AnalysisUserId))
	{
		dbAnalysisRecord.AnalysisUserId = dbSampleRecord.RunUserId;
	}

	//dbAnalysisRecord.AnalysisUserNameStr = username;	//NOTE: this is no longer in the SampleProperties table.

	if (ChronoUtilities::IsZero(dbAnalysisRecord.AnalysisDateTP))
	{
		dbAnalysisRecord.AnalysisDateTP = ChronoUtilities::CurrentTime();
	}

	Logger::L().Log (MODULENAME, severity_level::debug1,
		boost::str (boost::format ("CreateAnalysisRecord: timestamp: %s") % ChronoUtilities::ConvertToString (dbAnalysisRecord.AnalysisDateTP)));

	dbAnalysisRecord.InstrumentSNStr = HawkeyeConfig::Instance().get().instrumentSerialNumber;
	dbAnalysisRecord.BioProcessId = dbSampleRecord.BioProcessId;
	dbAnalysisRecord.QcId = dbSampleRecord.QcId;

	DBApi::eQueryResult dbStatus = DBApi::DbAddAnalysis (dbAnalysisRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("CreateAnalysisRecord: <exit, DB sample write failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		return false;
	}

	return true;
}

//*****************************************************************************
//*****************************************************************************
// DB Read Methods
//*****************************************************************************
//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBSampleRecord (const uuid__t& uuid, DBApi::DB_SampleRecord& dbSampleRecord)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSampleRecord: <enter, " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBSampleRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	if (isDBSampleRecordCached(uuid))
	{
		// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSampleRecord: <exit, using cached data>");
		dbSampleRecord = cached_DBSampleRecord_.second;
		return HawkeyeError::eSuccess;
	}

	dbSampleRecord = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindSample (dbSampleRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBSampleRecord: <exit, DbFindSample failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning));
		return HawkeyeError::eStorageFault;
	}

	cached_DBSampleRecord_ = std::pair<uuid__t, DBApi::DB_SampleRecord>(uuid, dbSampleRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSampleRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBSummaryResultRecord (const uuid__t& uuid, DBApi::DB_SummaryResultRecord& dbSummaryResultRecord)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSummaryResultRecord: <enter, " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBSummaryResultRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	if (isDBSummaryResultRecordCached(uuid))
	{
		// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSummaryResultRecord: <exit, using cached data>");
		dbSummaryResultRecord = cached_DBSummaryResultRecord_.second;
		return HawkeyeError::eSuccess;
	}

	dbSummaryResultRecord = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindSummaryResult (dbSummaryResultRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBSummaryResultRecord: <exit, DbFindSummaryResult failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	cached_DBSummaryResultRecord_ = std::pair<uuid__t, DBApi::DB_SummaryResultRecord>(uuid, dbSummaryResultRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSummaryResultRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBAnalysisRecord (const uuid__t& uuid, DBApi::DB_AnalysisRecord& dbAnalysisRecord)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBAnalysisRecord: <enter, " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBAnalysisRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	if (isDBAnalysisRecordCached(uuid))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBAnalysisRecord: <exit, using cached data>");
		dbAnalysisRecord = cached_DBAnalysisRecord_.second;
		return HawkeyeError::eSuccess;
	}

	dbAnalysisRecord = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindAnalysis (dbAnalysisRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBAnalysisRecord: <exit, DbFindAnalysis failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	cached_DBAnalysisRecord_ = std::pair<uuid__t, DBApi::DB_AnalysisRecord>(uuid, dbAnalysisRecord);

	Logger::L().Log (MODULENAME, severity_level::debug2, "read_DBAnalysisRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBImageSetRecord (const uuid__t& uuid, DBApi::DB_ImageSetRecord& dbImageSetRecord)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageSetRecord: <enter, " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBImageSetRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	if (isDBImageSetRecordCached(uuid))
	{
		// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageSetRecord: <exit, using cached data>");
		dbImageSetRecord = cached_DBImageSetRecord_.second;
		return HawkeyeError::eSuccess;
	}

	dbImageSetRecord = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindImageSet (dbImageSetRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBImageSetRecord: <exit, DbFindImageSet failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	cached_DBImageSetRecord_ = std::pair<uuid__t, DBApi::DB_ImageSetRecord>(uuid, dbImageSetRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageSetRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBImageSeqRecord (const uuid__t& uuid, DBApi::DB_ImageSeqRecord& dbImageSeqRecord)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageSeqRecord: <enter, " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBImageSeqRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	if (isDBImageSeqRecordCached(uuid))
	{
		// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageSeqRecord: <exit, using cached data>");
		dbImageSeqRecord = cached_DBImageSeqRecord_.second;
		return HawkeyeError::eSuccess;
	}

	DBApi::DB_ImageRecord dbImageRecord = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindImageSequence (dbImageSeqRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBImageSeqRecord: <exit, DbFindImageRecord failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	cached_DBImageSeqRecord_ = std::pair<uuid__t, DBApi::DB_ImageSeqRecord>(uuid, dbImageSeqRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageSeqRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBImageRecord (const uuid__t& uuid, DBApi::DB_ImageRecord& dbImageRecord)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageRecord: <enter, " + Uuid::ToStr (uuid) + ">");

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBImageRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}
	
	if (isDBImageRecordCached(uuid))
	{
		// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageRecord: <exit, using cached data>");
		dbImageRecord = cached_DBImageRecord_.second;
		return HawkeyeError::eSuccess;
	}
	
	dbImageRecord = {};
	
	DBApi::eQueryResult dbStatus = DBApi::DbFindImage (dbImageRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBImageRecord: <exit, DbFindImage failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	cached_DBImageRecord_ = std::pair<uuid__t, DBApi::DB_ImageRecord>(uuid, dbImageRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBImageRecord: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBAnalysisRecords (const uuid__t& uuid, std::vector<DBApi::DB_AnalysisRecord>& dbAnalysisRecords)
{
	// leave for debug  Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBAnalysisRecords: <enter>");
	
	dbAnalysisRecords.clear();

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBAnalysisRecords: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	DBApi::eQueryResult dbStatus = DBApi::DbGetAnalysesList(
		dbAnalysisRecords,
		DBApi::eListFilterCriteria::ParentFilter,
		"=",
		boost::str (boost::format ("'%s'") % Uuid::ToStr(uuid)),
		-1,
		DBApi::eListSortCriteria::CreationDateSort,
		DBApi::eListSortCriteria::SortNotDefined,
		1,		// Ascending order...
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBAnalysisRecords: <exit, DbGetAnalysesList failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBAnalysisRecords: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBDetailedResultsRecords (
	const uuid__t& uuid, 
	std::vector<DBApi::DB_DetailedResultRecord>& dbDetailedResultsRecords)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBDetailedResultsRecords: <enter>");

	dbDetailedResultsRecords.clear();

	if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBDetailedResultsRecords: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	DBApi::eQueryResult dbStatus = DBApi::DbGetDetailedResultsList(
		dbDetailedResultsRecords,
		DBApi::eListFilterCriteria::ParentFilter,
		"=",
		boost::str (boost::format ("'%s'") % Uuid::ToStr(uuid)),
		-1,
		DBApi::eListSortCriteria::IdNumSort,
		DBApi::eListSortCriteria::SortNotDefined,
		1,		// Ascending order...
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("read_DBDetailedResultsRecords: <exit, DbGetDetailedResultsList failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBDetailedResultsRecords: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
BasicResultAnswers HawkeyeResultsDataManager::DB_DetailedResultRecord_To_BasicResultAnswers (const DBApi::DB_DetailedResultRecord& drr)
{
	BasicResultAnswers bra = {};

	bra.nTotalCumulative_Imgs = drr.TotalCumulativeImages;
	bra.count_pop_general = drr.TotalCells_GP;
	if ( bra.nTotalCumulative_Imgs > 0 )
	{
		bra.average_cells_per_image = drr.TotalCells_GP / bra.nTotalCumulative_Imgs;
	}
	bra.count_pop_ofinterest = drr.TotalCells_POI;
	bra.percent_pop_ofinterest = static_cast<float>(drr.POI_PopPercent);
	bra.concentration_general = static_cast<float>(drr.CellConc_GP);
	bra.concentration_ofinterest = static_cast<float>(drr.CellConc_POI);
	bra.avg_diameter_pop = static_cast<float>(drr.AvgDiam_GP);
	bra.avg_diameter_ofinterest = static_cast<float>(drr.AvgDiam_POI);
	bra.avg_circularity_pop = static_cast<float>(drr.AvgCircularity_GP);
	bra.avg_circularity_ofinterest = static_cast<float>(drr.AvgCircularity_POI);
	//TODO: calculate CV...
	bra.coefficient_variance = 0;
	//TODO: it was decided that the UI will not display CVs.
	bra.average_brightfield_bg_intensity = static_cast<int16_t>(drr.AvgBkgndIntensity);
	bra.bubble_count = static_cast<uint16_t>(drr.TotalBubbleCount);
	bra.large_cluster_count = static_cast<uint16_t>(drr.LargeClusterCount);

	return bra;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBSResultRecord (const uuid__t& sresultUUID, DBApi::DB_SResultRecord& dbSResultRecord/*CellCounterResult::SResult& sresult*/)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSResultRecord: <enter, " + Uuid::ToStr (sresultUUID) + ">");

	if (!Uuid::IsValid(sresultUUID) || Uuid::IsClear(sresultUUID))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBSResultRecord: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	if (isDBSResultRecordCached(sresultUUID))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSResultRecord: <exit, using cached data>");
		dbSResultRecord = cached_DBSResultRecord_.second;
		return HawkeyeError::eSuccess;
	}

	dbSResultRecord = {};

	DBApi::eQueryResult dbStatus = DBApi::DbFindSResult (dbSResultRecord, sresultUUID);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults == dbStatus)
		{
			return HawkeyeError::eEntryNotFound;
		}

		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format ("Read: <exit, DbFindSResult failed; query status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::warning)); // this is NOT a non-recoverable system fault use warning
		return HawkeyeError::eStorageFault;
	}

	cached_DBSResultRecord_ = std::pair<uuid__t, DBApi::DB_SResultRecord>(sresultUUID, dbSResultRecord);

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSResultRecord: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeResultsDataManager::read_DBSummaryResultRecordReturnSResult (const uuid__t& summaryResultUuid, CellCounterResult::SResult& sresult)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "read_DBSummaryResultRecordReturnSResult: <enter, " + Uuid::ToStr (summaryResultUuid) + ">");

	if (!Uuid::IsValid(summaryResultUuid) || Uuid::IsClear(summaryResultUuid))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "read_DBSummaryResultRecordReturnSResult: <exit: invalid UUID>");
		return HawkeyeError::eInvalidArgs;
	}

	DBApi::DB_SummaryResultRecord dbSummaryResultRecord = {};
	HawkeyeError he = read_DBSummaryResultRecord (summaryResultUuid, dbSummaryResultRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	DBApi::DB_AnalysisRecord dbAnalysisRecord = {};
	he = read_DBAnalysisRecord (dbSummaryResultRecord.AnalysisId, dbAnalysisRecord);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	he = retrieveSResult (dbAnalysisRecord.SResultId, sresult);
	if (he != HawkeyeError::eSuccess)
	{
		return he;
	}

	return he;
}
