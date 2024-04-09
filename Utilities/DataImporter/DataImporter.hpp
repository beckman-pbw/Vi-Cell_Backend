#pragma once

#include "boost\filesystem.hpp"

#include "AnalysisDefinitionsDLL.hpp"
#include "CellTypesDLL.hpp"
#include "HawkeyeResultsDataManager.hpp"
#include "PtreeConversionHelper.hpp"
#include "ResultDefinitionDLL.hpp"
#include "SampleSetDLL.hpp"

#define MODULENAME "DataImporter"

struct ConfigurationData
{
	AnalysisDefinitionsDLL analysisDLL;
	CellTypesDLL cellTypesDLL;
};

struct ImageDataRecord
{
	SampleImageSetRecordDLL imagesetRecord;
	ImageRecordDLL imageRecord;
};

struct WorkQueueItemRecord
{
	SampleRecordDLL sampleRecord;
	std::vector<ResultSummaryDLL> resultsummaryRecords;
	std::vector<ImageDataRecord> imageDataRecords;
};

struct ImportedSampleSet
{
	SampleSetDLL sampleSetDLL;
	std::vector<WorkQueueItemRecord> workqueueItems;
	ePrecession precession;
};

struct WorkQueueDataRecord
{
	WorkQueueRecordDLL workqueueRecord;
	std::vector<WorkQueueItemRecord> workqueueItems;
	std::vector<SampleSetDLL> sampleSets;
	ConfigurationData configData;
};

class DataImporter
{
public:
	DataImporter() { isMetadataLoaded_ = false; }
	virtual ~DataImporter() {};
	
	// v1.2...
	bool RetrieveDataRecordsForUpgrade (std::vector<WorkQueueDataRecord>& workQueueRecords);

	// v1.3...
	bool RetrieveDataRecordsForOfflineAnalysis (std::vector<ImportedSampleSet>& sampleSets);

	bool ImportConfiguration(bool setSerNo);		// v1.2
	bool ImportData ();	// v1.2
	bool ImportDataForOfflineAnalysis (std::string serialNumber);	// v1.3
	bool ImportConfigurationForOfflineAnalysis();	// v1.3
	static void Log (std::string str, severity_level severityLevel = severity_level::normal);
	bool ReadMetaData (const std::string& resultsDataDir);

private:
	void SaveAnalysisData(
		uuid__t worklistUUID,
		const SampleDefinitionDLL& sampleDef,
		std::shared_ptr<ImageCollection_t> imageCollection,
		ImageDataForSResultMap& imageDataForSResult,
		const CellCounterResult::SResult& sresult,
		const std::vector<ReagentInfoRecordDLL>& reagentinfoRecords,
		boost::optional<ImageSet_t> dustReferenceImages, // Dust, background levels...,
		DBApi::DB_SampleRecord dbSampleRecord,
		DBApi::DB_AnalysisRecord dbAnalysisRecord,
		QcStatus qcStatus,
		std::function<void(SampleDefinitionDLL, bool)> onComplete);
	boost::property_tree::ptree& GetMetaData() { return metadataTree_; }
	void clearMetadataPtree();

	pt::ptree metadataTree_;
	bool isMetadataLoaded_;
};
