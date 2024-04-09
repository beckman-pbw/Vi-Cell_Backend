#pragma once

#include<functional>
#include<vector>

#include<boost/format.hpp>

#include "DataConversion.hpp"
#include "ExportData.hpp"
#include "HawkeyeError.hpp"
#include "HawkeyeServices.hpp"
#include "ImageCollection.hpp"
#include "ResultDefinitionDLL.hpp"
#include "SampleDefinitionDLL.hpp"
#include "SResultData.hpp"
#include "WorkListDLL.hpp"
#include "uuid__t.hpp"

#include <ICellCounter.h>

typedef std::function<void(bool)> BooleanCallback;
typedef std::function<void (HawkeyeError, uuid__t /*sample_id*/, ResultRecordDLL /*Analysis Data*/)> sample_analysis_callback_DLL;

// Callbacks.
template<class T>
using DataRecordCallback = std::function<void(T, HawkeyeError)>;
template<class T>
using DataRecordsCallback = std::function<void(std::vector<T>, HawkeyeError)>;

class HawkeyeResultsDataManager
{
public:
	static HawkeyeResultsDataManager& Instance()
	{
		static HawkeyeResultsDataManager instance;
		return instance;
	}

	void Initialize (std::shared_ptr<boost::asio::io_service> pUpstream);

	void RetrieveDetailedMeasurement(const uuid__t& id, DataRecordCallback<DetailedResultMeasurementsDLL> cb);
	void RetrieveHistogram(const uuid__t& id, bool only_POI, Hawkeye::Characteristic_t measurement, uint8_t bin_count, DataRecordsCallback<histogrambin_t> cb);
	void RetrieveImage (const uuid__t& id, std::function<void(cv::Mat, HawkeyeError)> cb);
	void RetrieveImageSequenceSet (const uuid__t& id, std::function<void(int16_t, ImageSet_t, HawkeyeError)> cb);
	HawkeyeError RetrieveImageInfo(const uuid__t& id, int16_t& seqNumber, std::string &path, uuid__t& imgRecId, system_TP& timestamp);
	void RetrieveAnnotatedImage(const uuid__t & result_id, const uuid__t& image_id, DataRecordCallback<cv::Mat> cb);
	void RetrieveBWImage(const uuid__t& id, DataRecordCallback<cv::Mat> cb);
	
	// Data Write APIs.
	void SaveAnalysisData(
		uuid__t worklistUUID,
		const SampleDefinitionDLL& sample,
		std::shared_ptr<ImageCollection_t> imageCollection,
		const CellCounterResult::SResult& stCumulativeOutput,
		const std::vector<ReagentInfoRecordDLL>& reagentinfoRecords,
		boost::optional<ImageSet_t> imageNormalizationData, // Dust, background levels...
		DBApi::DB_SampleRecord dbSampleRecord,
		DBApi::DB_AnalysisRecord dbAnalysisRecord,
		QcStatus qcStatus,
		std::function<void(SampleDefinitionDLL, bool)> onComplete);
	
	void SaveReanalysisData(
		const CellCounterResult::SResult& sresult,
		DBApi::DB_SampleRecord dbSampleRecord,
		DBApi::DB_AnalysisRecord dbAnalysisRecord,
		QcStatus qcStatus,
		std::function<void(bool)> onComplete);

	static bool SaveAnalysisImages (
		const std::string dateStr,
		const uuid__t& worklistUUID,
		const DBApi::DB_SampleRecord& dbSampleRecord,
		const DBApi::DB_AnalysisRecord& dbAnalysisRecord,
		const SampleDefinitionDLL& sampleDefinition,
		std::shared_ptr<ImageCollection_t>imageCollection,
		const CellCounterResult::SResult& cellCounterResult,
		ImageDataForSResultMap& imageDataForSResult,
		uuid__t& imageSetUUID);

#ifndef DATAIMPORTER
	void SaveLegacyCellCountingData (std::shared_ptr<ImageCollection_t> imageCollection, std::string label, const CellCounterResult::SResult& stCumulativeOutput, BooleanCallback cb);
#endif
	void SignResult(const uuid__t& rr_uuid, const DataSignatureInstanceDLL& sig, std::function<void(HawkeyeError, std::string)> cb);
	HawkeyeError RetrieveSResult (const uuid__t& sresultUuid, CellCounterResult::SResult& sresult);
	void ResetSaveDustReference() { doSaveDustReference_ = true; }

	void DeleteSampleRecord (std::vector<uuid__t>& sampleRecordUuidList, bool retain_results_and_first_image, std::function<void(HawkeyeError, uuid__t, std::string)> callback);

	HawkeyeError ExportForOfflineAnalysis (const std::vector<uuid__t>& rs_uuid_list,
                            const std::string& username, 
                            const std::string& export_path, 
                            eExportImages exportImages,
                            uint16_t export_nth_image,
                            std::function<void(HawkeyeError, uuid__t)> exportdataProgressCb, 
                            std::function<void(HawkeyeError, std::string archived_filepath, std::vector<std::string>)> onExportCompletionCb);

	HawkeyeError CancelExportData();


	HawkeyeError Export_Start(
		const std::vector<uuid__t>& rs_uuid_list,
		const std::string& username,
		const std::string& outPath,
		eExportImages exportImages,
		uint16_t export_nth_image);

	HawkeyeError  Export_NextMetaData(uint32_t index, uint32_t delayms);
	HawkeyeError  Export_IsStorageAvailable();
	HawkeyeError  Export_ArchiveData(const std::string filename, char*& outname);
	HawkeyeError  Export_Cleanup(bool removeFile);

	std::vector<std::string> GetAuditEntries();

	void RetrieveResultRecord (const uuid__t& uuid, std::function<void(ResultRecordDLL, HawkeyeError)> retrieve_cb);
	void RetrieveResultRecords (uint64_t start, uint64_t end, std::string sUsername, std::function<void(std::vector<ResultRecordDLL>, HawkeyeError)> retrieve_cb);
	void RetrieveResultRecordList (const std::vector<uuid__t>& uuids, std::function<void(std::vector<ResultRecordDLL>, HawkeyeError)> retrieve_cb);

	void RetrieveDbSampleRecord (const uuid__t& uuid, std::function<void(DBApi::DB_SampleRecord, HawkeyeError)> retrieve_cb);
	void RetrieveSampleRecord (const uuid__t& uuid, std::function<void(SampleRecordDLL, HawkeyeError)> retrieve_cb);
	bool RetrieveSampleRecord (const uuid__t& uuid, SampleRecordDLL& sampleRecDll);
	void RetrieveSampleRecords (uint64_t start, uint64_t end, const std::string username, std::function<void(std::vector<SampleRecordDLL>, HawkeyeError)> retrieve_cb);
	void RetrieveSampleRecordList (const std::vector<uuid__t>& uuids, std::function<void(std::vector<SampleRecordDLL>, HawkeyeError)> retrieve_cb);
	void RetrieveSampleRecordsByQCUuid (const uuid__t qcUuid, std::function<void(std::vector<SampleRecordDLL>, HawkeyeError)> retrieve_cb);
	
	void RetrieveSampleImageSetRecord (uuid__t uuid, std::function<void(SampleImageSetRecordDLL, HawkeyeError)> retrieve_cb);
	HawkeyeError RetrieveSampleImageSetRecord(uuid__t uuid, SampleImageSetRecordDLL& sampleImageSetRecord);
	void RetrieveSampleImageSetRecords (uint64_t start, uint64_t end, std::string sUsername, std::function<void(std::vector<SampleImageSetRecordDLL>, HawkeyeError)> retrieve_cb);

	void RetrieveResultSummaryRecord (const uuid__t& uuid, std::function<void(ResultSummaryDLL, HawkeyeError)> retrieve_cb);
	void RetrieveResultSummaryRecordList (const std::vector<uuid__t>& uuids, std::function<void(std::vector<ResultSummaryDLL>, HawkeyeError)> retrieve_cb);
	
	void RetrieveSampleImageSetRecordList (const std::vector<uuid__t>& uuids, std::function<void(std::vector<SampleImageSetRecordDLL>, HawkeyeError)> retrieve_cb);

	void RetrieveSampleSResult (const uuid__t& uuid, std::function<void(CellCounterResult::SResult sresult, HawkeyeError)> retrieve_cb);

	HawkeyeError RetrieveDbImageSetRecord (const uuid__t& uuid, DBApi::DB_ImageSetRecord& dbImageSetRecord);

	// Used in Reanalysis.
	void CacheReanalysisSResultRecord (uuid__t uuid, const CellCounterResult::SResult& sresult);

	static bool HawkeyeResultsDataManager::CreateSampleRecord (
		const SampleDefinitionDLL& sampleDef, 
		DBApi::DB_SampleRecord& dbSampleRecord);
	static bool HawkeyeResultsDataManager::CreateAnalysisRecord (
		DBApi::DB_SampleRecord& dbSampleRecord, 
		DBApi::DB_AnalysisRecord& dbAnalysisRecord);

	// Set by RetrieveSampleUsers(), used by DeleteSampleRecord()
	uint64_t retrieveSampleStart;
	uint64_t retrieveSampleEnd;
	std::string retrieveSampleUsername;


private:
	HawkeyeError read_DBSampleRecord (const uuid__t& uuid, DBApi::DB_SampleRecord& dbSampleRecord);
	HawkeyeError read_DBSummaryResultRecord (const uuid__t& uuid, DBApi::DB_SummaryResultRecord& dbSummaryResultRecord);
	HawkeyeError read_DBAnalysisRecord (const uuid__t& uuid, DBApi::DB_AnalysisRecord& dbSummaryResultRecord);
	HawkeyeError read_DBAnalysisRecords (const uuid__t& uuid, std::vector<DBApi::DB_AnalysisRecord>& dbSummaryResultRecord);
	HawkeyeError read_DBImageSetRecord (const uuid__t& uuid, DBApi::DB_ImageSetRecord& dbImageSetRecord);
	HawkeyeError read_DBImageSeqRecord (const uuid__t& uuid, DBApi::DB_ImageSeqRecord& dbImageSeqRecord);
	HawkeyeError read_DBImageRecord (const uuid__t& uuid, DBApi::DB_ImageRecord& dbImageRecord);
	HawkeyeError read_DBDetailedResultsRecords (const uuid__t& uuid, std::vector<DBApi::DB_DetailedResultRecord>& dbDetailedResultsRecords);
	HawkeyeError read_DBSResultRecord (const uuid__t& sresultUUID, DBApi::DB_SResultRecord& dbSResultRecord);
	HawkeyeError read_DBSummaryResultRecordReturnSResult (const uuid__t& summaryResultUuid, CellCounterResult::SResult& sresult);
	HawkeyeError decode_DBSampleRecord (const DBApi::DB_SampleRecord& dbSampleRecord, SampleRecordDLL& sampleRecord);
	HawkeyeError decode_DBSummaryResultRecord (const DBApi::DB_SummaryResultRecord& dbSummaryResultRecord, ResultSummaryDLL& summaryResultRecord);

	// Cache data to avoid redundant lookups.
	std::pair<uuid__t, DBApi::DB_SampleRecord> cached_DBSampleRecord_;
	bool isDBSampleRecordCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBSampleRecord_.first, uuid);
	}
	std::pair<uuid__t, DBApi::DB_SummaryResultRecord> cached_DBSummaryResultRecord_;
	bool isDBSummaryResultRecordCached (const uuid__t & uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBSummaryResultRecord_.first, uuid);
	}
	std::pair<uuid__t, DBApi::DB_AnalysisRecord> cached_DBAnalysisRecord_;
	bool isDBAnalysisRecordCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBAnalysisRecord_.first, uuid);
	}
	std::pair<uuid__t, DBApi::DB_ImageSetRecord> cached_DBImageSetRecord_;
	bool isDBImageSetRecordCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBImageSetRecord_.first, uuid);
	}
	std::pair<uuid__t, DBApi::DB_ImageSeqRecord> cached_DBImageSeqRecord_;
	bool isDBImageSeqRecordCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBImageSeqRecord_.first, uuid);
	}
	std::pair<uuid__t, DBApi::DB_ImageRecord> cached_DBImageRecord_;
	bool isDBImageRecordCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBImageRecord_.first, uuid);
	}
	std::pair<uuid__t, DBApi::DB_SResultRecord> cached_DBSResultRecord_;
	bool isDBSResultRecordCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
			return false;
		return DataConversion::AreEqual (cached_DBSResultRecord_.first, uuid);
	}


	std::pair<uuid__t, CellCounterResult::SResult> cached_ReanalysisSResultRecord_;
	bool isReanalysisSResultCached (const uuid__t& uuid)
	{
		if (!Uuid::IsValid(uuid) || Uuid::IsClear(uuid))
		{
			Logger::L().Log("HawkeyeResultsDataManager.hpp", severity_level::error, "isReanalysisSResultCached: <invalid UUID - return false>");
			return false;
		}

		Logger::L().Log ("HawkeyeResultsDataManager", severity_level::debug1,
			"isReanalysisSResultCached: cached UUID: " + Uuid::ToStr (cached_ReanalysisSResultRecord_.first) +
			" input UUID: " + Uuid::ToStr (uuid));
		return DataConversion::AreEqual (cached_ReanalysisSResultRecord_.first, uuid);
	}

	// Local io service provider.
	std::shared_ptr<HawkeyeServices> pDataManagerServices_;

	bool doSaveDustReference_ = false;

	std::atomic<bool> isDeleteInProgress_ = false;

	DBApi::DB_ImageSeqRecord storeImage(const ImageSet_t& image, size_t imageIndex);
	
	static bool saveAnalysisImages (
		const std::string dateStr,
		const uuid__t& worklistUUID, 
		const DBApi::DB_SampleRecord& dbSampleRecord,
		const DBApi::DB_AnalysisRecord& dbAnalysisRecord,
		const SampleDefinitionDLL& sampleDefinition, 
		std::shared_ptr<ImageCollection_t>imageCollection, 
		const CellCounterResult::SResult& cellCounterResult,
		ImageDataForSResultMap& imageDataForSResult,
		uuid__t& imageSetUUID);

	bool saveDustReferenceImages (const std::string dateStr, const uuid__t& worklistUUID, const DBApi::DB_SampleRecord& dbsr, const SampleDefinitionDLL& sample, const boost::optional<ImageSet_t>& dustReferenceImageSet, uuid__t& dustReferenceImageSetUUID);

	//NOTE: not currently called...
	void getBasicAnswerResults (const CellCounterResult::SResult & sresult, BasicResultAnswers & cumulative_result, std::vector<BasicResultAnswers>& per_image_result);

	void getDetailedMeasurementFromSResult (const CellCounterResult::SResult & sr, DetailedResultMeasurementsDLL & drm);

	HawkeyeError retrieveDetailedMeasurement (const uuid__t& id, DetailedResultMeasurementsDLL& drm);
	HawkeyeError retrieveHistogram (const uuid__t& id, bool only_POI, Hawkeye::Characteristic_t measurement, uint8_t bin_count, std::vector<histogrambin_t>& histogram);
	HawkeyeError retrieveImage (const uuid__t& id, int16_t& sequenceNum, cv::Mat& img);
	HawkeyeError retrieveImageSequenceSet (const uuid__t& id, int16_t& seqNumber, ImageSet_t& imageSet);
	HawkeyeError retrieveImageInfo(const uuid__t& id, int16_t& seqNumber, std::string &path, uuid__t& imgRecId, system_TP& timestamp);
	HawkeyeError retrieveAnnotatedImage (const uuid__t & result_id, const uuid__t& image_id, cv::Mat& img);
	HawkeyeError retrieveBWImage (const uuid__t& id, cv::Mat& img);
	HawkeyeError retrieveSResult (const uuid__t& sresultUuid, CellCounterResult::SResult& sresult);

	void saveAnalysisData (uuid__t worklistUUID,
	                       const SampleDefinitionDLL& sample,
	                       std::shared_ptr<ImageCollection_t> imageCollection, 
	                       const CellCounterResult::SResult& stCumulativeOutput,
	                       const std::vector<ReagentInfoRecordDLL>& reagentinfoRecords,
	                       boost::optional<ImageSet_t> dustReferenceImage,   /*imageNormalizationData,*/
	                       DBApi::DB_SampleRecord dbSampleRecord,
	                       DBApi::DB_AnalysisRecord dbAnalysisRecord,
                           QcStatus qcStatus,
	                       std::function<void(SampleDefinitionDLL, bool)> onComplete);

#ifndef DATAIMPORTER
	bool saveLegacyCellCountingData (std::shared_ptr<ImageCollection_t> imageCollection, std::string label, CellCounterResult::SResult sresult);
#endif
	HawkeyeError signResult (const uuid__t& rr_uuid, const DataSignatureInstanceDLL& sig, std::string& audit_log_str);
	HawkeyeError deleteSampleRecord (const uuid__t& wqir_uuid, bool retain_results_and_first_image, std::string& audit_log_str);
	BasicResultAnswers DB_DetailedResultRecord_To_BasicResultAnswers (const DBApi::DB_DetailedResultRecord& drr);

	std::unique_ptr<ExportData> exportData;

	enum class eExportDataStates :uint16_t
	{
		eCreateFileNamesAndStagingDir = 0,
		eCheckForConfigFiles,
		eExportMetadata,
		eCheckForDiskSpace,
		eArchiveData,
		eClearOnSuccess,
		eClearOnCancelOrFailure,
		eExportComplete
	};
	
	struct ExportDataParams
	{
		std::string temp_staging_dir;
		std::string metadata_file_path;
		std::vector<std::string> data_files_export;
		std::vector<std::string> audit_log_str;
		std::string archive_file_path;
		HawkeyeError he;
	};

	struct ExportDataInputs
	{
		std::string username;
		std::string export_path;
		eExportImages image_selection;
		uint16_t nth_image;
		std::vector<uuid__t> rs_uuid_list;
		std::function<void(HawkeyeError, uuid__t)> progress_cb;
		std::function<void(HawkeyeError, std::string archived_filepath, std::vector<std::string>) > completion_cb;
	};
};
