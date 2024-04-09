#pragma once

#include <mutex>

#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeUUID.hpp"
#include "PtreeConversionHelper.hpp"
#include "ResultDefinitionDLL_scout.hpp"
#include "SignatureDLL.hpp"


typedef struct ImageData
{
	SampleImageSetRecordDLL imsr;
	ImageRecordDLL bf_imr;
	std::vector<ImageRecordDLL> fl_imr_list;
}ImageSetDataRecord_t;

class DataReplicator 
{
private:
	pt::ptree mainMetaDataTree_;

	bool isExistingMetaDataIsRead_;

	bool ReadMetaDataFromDisk();
	bool WriteMetaDataToDisk();
	void ClearMetaDataPtrees();

	std::mutex mtx_MetaDataHandler;

public:
	DataReplicator();
	~DataReplicator();
	
	bool SaveDataRecords(const WorkQueueRecordDLL& wqr,
						 const SampleRecordDLL& wqir,
						 const ResultSummaryDLL& rs,
						 const std::vector <ImageSetDataRecord_t>& imsr_bf_fl_imr_list,
						 const ImageSetDataRecord_t& background_img_data);

	virtual  bool RetrieveDataRecords(std::vector<WorkQueueRecordDLL>& wqr_list,
									   std::vector<SampleRecordDLL>& wqir_list,
									   std::vector<ResultSummaryDLL>& rr_list,
									   std::vector<SampleImageSetRecordDLL>& imsr_list,
									   std::vector<ImageRecordDLL>& imr_list);

	static void duplicateData();

};

