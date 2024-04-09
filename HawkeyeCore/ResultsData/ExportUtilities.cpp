#include "stdafx.h"

#include "../../target/properties/HawkeyeCore/dllversion.hpp"

#include "DBif_Api.h"

#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeResultsDataManager.hpp"
#include "ExportUtilities.hpp"
#include "InstrumentConfig.hpp"
#include "QualityControlsDLL.hpp"
#include "ReagentPack.hpp"
#include "SignaturesDLL.hpp"
#include "SResultBinStorage.hpp"

#define MODULENAME "ExportUtilities"

using namespace PtreeConversionHelper;

std::vector<DataSignatureDLL>& GetDataSignatureDeinitions();

//*****************************************************************************
static std::string getAuditLogStr (const pt::ptree& sample_record_node, const pt::ptree& result_summary_node)
{
	if (sample_record_node.empty() || result_summary_node.empty())
	{
		return "";
	}

	auto sample_id = sample_record_node.get<std::string>(SAMPLE_ID_NODE, " ");
	auto username = sample_record_node.get<std::string>(USER_NODE, " ");
	auto timestamp = result_summary_node.get<uint64_t>(TIMESTAMP_NODE, 0);
	auto celltype_label = result_summary_node.get<std::string>(CELLTYPE_NODE + "." + LABEL_NODE, " ");
	auto audit_log = boost::str(boost::format("Result of Sample: <%s> User : <%s> Timestamp : <%s UTC> Celltype : <%s>")
		% sample_id
		% username
		% ChronoUtilities::ConvertToString(ChronoUtilities::ConvertToTimePoint < std::chrono::seconds>(timestamp))
		% celltype_label);

	return audit_log;
}

//*****************************************************************************
void ExportUtilities::ClearMetaData()
{
	mainMetaDataTree_.clear();
}

//*****************************************************************************
ExportUtilities::~ExportUtilities()
{
}

//*****************************************************************************
// Get Sample result record from specified uuid
//*****************************************************************************
bool GetAnalysisSResult (CellCounterResult::SResult &sresult, uuid__t uuid)
{
	DBApi::DB_AnalysisRecord dbAnalysisRecord = {};
	DBApi::eQueryResult dbStatus = DBApi::DbFindAnalysis (dbAnalysisRecord, uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("GetAnalysisSResult: <exit, DbFindAnalysis failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_storage_readerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		return false;
	}

	if (HawkeyeResultsDataManager::Instance().RetrieveSResult(dbAnalysisRecord.SResultId, sresult) != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "GetAnalysisSResult: <exit, Failed to retrieve SResult>");
	}

	return true;
}

//*****************************************************************************
// From the database summary result record, extract relevant info for the exported metadata.xml file
//*****************************************************************************
bool ExtractSummaryResult (DBApi::DB_SummaryResultRecord &summaryResultRec, ResultSummaryDLL &summary_results)
{
//TODO: is this needed???
// the same data is available in the SResult...
	summary_results.uuid = summaryResultRec.SummaryResultId;
	summary_results.timestamp = summaryResultRec.ResultDateTP;

	// leave for debug 
	//Logger::L().Log(MODULENAME, severity_level::debug1,
	//	"ExtractSummaryResult: <summaryResult UUID: " +
	//	Uuid::ToStr (summaryResultRec.SummaryResultId) + ", " +
	//	ChronoUtilities::ConvertToString(summaryResultRec.ResultDateTP) + ">");

	summary_results.cumulative_result.eProcessedStatus = static_cast<E_ERRORCODE>(summaryResultRec.ProcessingStatus);
	summary_results.cumulative_result.nTotalCumulative_Imgs = summaryResultRec.TotalCumulativeImages;
	summary_results.cumulative_result.count_pop_general = summaryResultRec.TotalCells_GP;
	summary_results.cumulative_result.count_pop_ofinterest = summaryResultRec.TotalCells_POI;
	summary_results.cumulative_result.concentration_general = summaryResultRec.CellConc_GP;
	summary_results.cumulative_result.concentration_ofinterest = summaryResultRec.CellConc_POI;
	summary_results.cumulative_result.avg_diameter_pop = summaryResultRec.AvgDiam_GP;
	summary_results.cumulative_result.avg_diameter_ofinterest = summaryResultRec.AvgDiam_POI;
	summary_results.cumulative_result.avg_circularity_pop = summaryResultRec.AvgCircularity_GP;
	summary_results.cumulative_result.avg_circularity_ofinterest = summaryResultRec.AvgCircularity_POI;
	summary_results.cumulative_result.coefficient_variance = summaryResultRec.CoefficientOfVariance;
	summary_results.cumulative_result.average_cells_per_image = summaryResultRec.AvgCellsPerImage;
	summary_results.cumulative_result.average_brightfield_bg_intensity = summaryResultRec.AvgBkgndIntensity;
	summary_results.cumulative_result.bubble_count = summaryResultRec.TotalBubbleCount;
	summary_results.cumulative_result.large_cluster_count = summaryResultRec.LargeClusterCount;
	summary_results.qcStatus = static_cast<QcStatus>(summaryResultRec.QcStatus);

	CellCounterResult::SResult sresult;
	if (!GetAnalysisSResult(sresult, summaryResultRec.AnalysisId))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExtractSummaryResult: <Exit Failed to retrieve SResult>");
		return false;
	}
	
	std::string bindir = HawkeyeDirectory::Instance().getResultBinariesDir();

	std::string res_rec_uuid_str = Uuid::ToStr (summaryResultRec.SummaryResultId);
	summary_results.path = boost::str(boost::format("%s\\%s.bin")
		% bindir
		% res_rec_uuid_str);

	// Save the Serialize SResult
	if (!SResultBinStorage::SerializeSResult(sresult, summary_results.path))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExtractSummaryResult: <Exit, failed to Create SResult Binary File>");
		return false;
	}

	AnalysisDefinitionsDLL::GetAnalysisByIndex(0, summary_results.analysis_settings);

	for (auto& vv : summaryResultRec.SignatureList)
	{
		DataSignatureInstanceDLL sigDLL = {};
		sigDLL.signing_user = vv.userName;
		sigDLL.timestamp = vv.signatureTime;
		sigDLL.signature.short_text = vv.shortSignature;
		sigDLL.signature.long_text = vv.longSignature;
		sigDLL.signature.sig_hash = vv.signatureHash;

		const auto& ds = GetDataSignatureDeinitions();
		auto item = std::find_if(
			ds.begin(), ds.end(), [sigDLL](const auto& item)
			{ return sigDLL.signature.short_text == item.short_text; });

		//TODO: should the DB signature hash be validated?
		//TODO: If so, what should have if the hash is invalid???
		if (item == ds.end())
		{
			Logger::L().Log(MODULENAME, severity_level::debug1,
				boost::str(boost::format("ExtractSummaryResult: <short signature definition '%s' not found>") % sigDLL.signature.short_text));
		}
		else
		{
			sigDLL.signature.short_text_signature = item->short_text_signature;
			sigDLL.signature.long_text_signature = item->long_text_signature;
		}

		summary_results.signature_set.push_back(sigDLL);
	}

	return true;
}

//*****************************************************************************
// Export v1.3 data...
//*****************************************************************************
bool ExportUtilities::ExportMetaData (
	ExportInputs& inputs_to_export,
	bool& is_cancelled,
	std::function<void(void)>& cancel_handler,
	std::vector<std::string>& files_to_export,
	std::vector<std::string>& audit_log_str,
	std::function<void(uuid__t)> per_result_completion_cb)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData <Enter>");

	// Check for valid inputs to perform 
	if (inputs_to_export.rs_uuid_list.empty() ||
		inputs_to_export.export_metadata_file_path.empty() || 
		inputs_to_export.username.empty())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData <Exit - Invalid input>");
		return false;
	}

	bool cancel_metadata_export = false;
	cancel_handler = [&cancel_metadata_export]()mutable ->void
	{
		cancel_metadata_export = true;
	};

	is_cancelled = false;
	bool first = true;

	// Create binary results directories if they don't exist.
	if (!FileSystemUtilities::CreateDirectories(HawkeyeDirectory::Instance().getResultBinariesDir()))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <Exit Failed to create results binary directory>");
		return false;
	}

	pt::ptree sampleNode = {};
	pt::ptree sampleSetNode = {};
	pt::ptree instrumentNode = {};	// Root ptree node.
	instrumentNode.add(VERSION_NODE, InstrumentConfig::GetDLLVersion());
	instrumentNode.add (INSTRUMENT_TYPE_NODE, InstrumentConfig::Instance().Get().InstrumentType);
	
	SampleSetDLL sampleSet = {};
	std::vector<DBApi::DB_SampleItemRecord> dbSampleItemList = {};
	uuid__t sampleSetUuid = {};
	uuid__t sampleUuid = {};


	for (auto summaryUuid : inputs_to_export.rs_uuid_list)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <summaryResult UUID: " + Uuid::ToStr (summaryUuid) + ">");
	}

	// Loop through all summary results uuids specified
	for (auto summaryUuid : inputs_to_export.rs_uuid_list)
	{
		// If export cancelled, abort
		if (cancel_metadata_export)
		{
			is_cancelled = true;
			cancel_handler = nullptr;
			return true;
		}

		// Get the database summary result record specified by this summary uuid.
		DBApi::DB_SummaryResultRecord dbSummaryResultRecord = {};
		DBApi::DB_SampleSetRecord dbSampleSet = {};
		DBApi::DB_WorklistRecord dbWorklist = {};
		{
			DBApi::eQueryResult dbStatus = DBApi::DbFindSummaryResult (dbSummaryResultRecord, summaryUuid);
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("ExportMetaData: <exit, DbFindSummaryResult failed, status: %ld>") % (int32_t)dbStatus));
				return false;
			}

			Logger::L().Log(MODULENAME, severity_level::debug1,
				"ExportMetaData: <summary result UUID: " + 
				Uuid::ToStr (summaryUuid) + ", " +
				ChronoUtilities::ConvertToString(dbSummaryResultRecord.ResultDateTP) + ">");

			Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <sample UUID: " + Uuid::ToStr (dbSummaryResultRecord.SampleId) + ">");

			// Retrieve the SampleSet for this Summary Result.
			// For offline analysis we're only going to use enough data fields to support displaying the 
			// samples as they are seen on the instrument.
			dbSampleItemList = {};
			dbStatus = DBApi::DbGetSampleItemList(
				dbSampleItemList,
				DBApi::eListFilterCriteria::SampleIdFilter,
				"=",
				boost::str (boost::format ("'%s'") % Uuid::ToStr(dbSummaryResultRecord.SampleId)),
				-1,
				DBApi::eListSortCriteria::SortNotDefined,
				DBApi::eListSortCriteria::SortNotDefined,
				0,
				"",
				-1,
				-1,
				"");
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("ExportMetaData: <exit, DbGetSampleItemList failed, status: %ld>") % (int32_t)dbStatus));
				return false;
			}

			dbStatus = DBApi::DbFindSampleSet (dbSampleSet, dbSampleItemList[0].SampleSetId);
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("ExportMetaData: <exit, DbFindSampleSet failed, status: %ld>") % (int32_t)dbStatus));
				return false;
			}

			dbStatus = DBApi::DbFindWorklist (dbWorklist, dbSampleSet.WorklistId);
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("ExportMetaData: <exit, DbFindSampleSet failed, status: %ld>") % (int32_t)dbStatus));
				return false;
			}
		}

		Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <sampleset uuid: " + Uuid::ToStr (sampleSetUuid/*sampleSet.uuid*/) + ">");
		Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <dbSampleSet.SampleSetId uuid: " + Uuid::ToStr (dbSampleSet.SampleSetId) + ">");

		// Check if "summaryUuid" belongs to a different sample.  If so, all summary records get assigned to this sample
		// until the summary result's sample UUID does not match the previous summary result's sample UUID.
		if (Uuid(dbSummaryResultRecord.SampleId) != Uuid(sampleUuid))
		{ // Found a new sample.

			// Add the previous Sample node to the SampleSet node.
			if (!first)
			{
				Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <adding sampleNode to sampleSetNode>");
				sampleSetNode.add_child (SAMPLE_NODE, sampleNode);
			}

			// Add the previous SampleSet node to the Instrument (root) node.
			if (Uuid(dbSampleSet.SampleSetId) != Uuid(sampleSetUuid))
			{ // Found a new sample.

				// Add the previous Sample node to the SampleSet node.
				if (!first)
				{
					Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <adding sampleSetNode to instrumentNode>");
					instrumentNode.add_child (SAMPLESET_NODE, sampleSetNode);
				}

				// Get the info for the new SampleSet.
				sampleSetUuid = dbSampleSet.SampleSetId;
				sampleSet.FromDbStyle (dbSampleSet);
				ConvertToPtree (sampleSet, sampleSetNode);
				sampleSetNode.add (CARRIER_TYPE_NODE, dbWorklist.CarrierType);
				sampleSetNode.add (BY_COLUMN_NODE, dbWorklist.ByColumn);
			}

			SampleRecordDLL sampleRecord = {};
			sampleRecord.uuid = sampleUuid = dbSummaryResultRecord.SampleId;

			Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <sampleProperties.sample_record.uuid: " + Uuid::ToStr (sampleRecord.uuid) + ">");

			// Extract the relevant info from the database sample record.
			if (!HawkeyeResultsDataManager::Instance().RetrieveSampleRecord (dbSummaryResultRecord.SampleId, sampleRecord))
			{
				Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <Exit, failed to retreive sample record>");
				return false;
			}

			// Reagent info not stored in database, get from memory.
			std::vector<ReagentInfoRecordDLL>& reagentInfo = ReagentPack::Instance().GetReagentInfoRecords();
			sampleRecord.reagent_info_records.assign (reagentInfo.begin(), reagentInfo.end());

			ConvertToPtree (sampleRecord, sampleNode);

			// Determine if Nth image specified.
			uint32_t nthImage = 1;
			if (inputs_to_export.image_selection != eExportImages::eAll || inputs_to_export.nth_image != 1)
			{
				nthImage = inputs_to_export.nth_image;
			}

			// Handle case when nth image is specified. Will always get first and last (per 1.2)
			uint32_t imageIdx = 1;

			// Loop through image sequences and get image records
			for (auto& it : sampleRecord.imageSequences)
			{
				if ((imageIdx == 1) || (imageIdx == sampleRecord.imageSequences.size()) || ((imageIdx % nthImage) == 0))
				{
					// Get all relevant image information
					SampleImageSetRecordDLL sampleImageSetRecord = {};

					if (HawkeyeResultsDataManager::Instance().RetrieveSampleImageSetRecord(it, sampleImageSetRecord) == HawkeyeError::eSuccess)
					{
						sampleImageSetRecord.username = inputs_to_export.username;

						uuid__t bf_imgRecId = {};
						int16_t seqNum = 0;
						std::string imgPath;
						system_TP timestamp = {};
						if (HawkeyeResultsDataManager::Instance().RetrieveImageInfo(it, seqNum, imgPath, bf_imgRecId, timestamp) == HawkeyeError::eSuccess)
						{
							ImageRecordDLL imageRecord = {};
							imageRecord.uuid = bf_imgRecId;
							imageRecord.path = imgPath.substr(2);	// Strip off the drive letter and colon.
							imageRecord.timestamp = timestamp;
							imageRecord.username = inputs_to_export.username;
							sampleImageSetRecord.sequence_number = seqNum;

							std::vector<ImageRecordDLL> fl_imr_list;
							pt::ptree image_node = {};
							ConvertToPtree (sampleImageSetRecord, imageRecord, fl_imr_list, image_node);
							sampleNode.add_child (IMAGE_SET_NODE, image_node);

							std::string encrypted_path = {};
							if (HDA_GetEncryptedFileName(imgPath, encrypted_path))
								files_to_export.push_back(encrypted_path);
							else
							{
								Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <exit, failed to get image results path>");
								return false;
							}
						}
						else
						{
							Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <exit, failed to retrieve image info>");
							return false;
						}
					}
					else
					{
						Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <exit, failed to retreive sample image set record>");
						return false;
					}
				}
				++imageIdx;
			}
	
		} // End "if (Uuid(dbSummaryResultRecord.SampleId) != Uuid(sampleUuid))"

		// Get summary results from DB record and store in summary_results struct
		ResultSummaryDLL summary_results = {};
		if (!ExtractSummaryResult(dbSummaryResultRecord, summary_results))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <exit, failed to extract summary results from database record>");
			return false;
		}

		Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <summary result UUID 2: " + Uuid::ToStr (summary_results.uuid) + ">");

		CellTypesDLL::getCellTypeByIndex (dbSummaryResultRecord.CellTypeIndex, summary_results.cell_type_settings);
		
		// Copy summary result info into tree node
		pt::ptree summary_result_node = {};
		ConvertToPtree (summary_results, summary_result_node);

		// Generate audit log string
		audit_log_str.push_back (getAuditLogStr (sampleNode, summary_result_node));

		// Add summary tree node to sample node.
		Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <adding summaryResultNode to sampleNode>");
		sampleNode.add_child (SUMMARY_RESULTS_NODE, summary_result_node);
		
		std::string encrypted_path = {};
		if (HDA_GetEncryptedFileName(summary_results.path, encrypted_path))
			files_to_export.push_back(encrypted_path);
		else
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ExportMetaData: <exit, failed get binary results path>");
			return false;
		}

		first = false;

	} // End "for (auto summaryID : inputs_to_export.rs_uuid_list)"

	// Wrap up the last summary result into it's SampleSet and Sample.
	Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <adding sampleNode to sampleSetNode>");
	sampleSetNode.add_child (SAMPLE_NODE, sampleNode);
	Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMetaData: <adding sampleSetNode to instrumentNode>");
	instrumentNode.add_child (SAMPLESET_NODE, sampleSetNode);

	// Export Configuration Data.
	pt::ptree configurationNode = {};
	std::pair<std::string, boost::property_tree::ptree> pair;

	pair = AnalysisDefinitionsDLL::Export();
	configurationNode.add_child (pair.first, pair.second);
	pair = CellTypesDLL::Export();
	configurationNode.add_child (pair.first, pair.second);
	pair = QualityControlsDLL::Export();
	configurationNode.add_child (pair.first, pair.second);
	pair = SignaturesDLL::Export();
	configurationNode.add_child (pair.first, pair.second);
	pair = UserList::Instance().Export();
	configurationNode.add_child (pair.first, pair.second);

	//LH6531-6532 - CHM modules should disable security on export of data so that customers don't have to know
	// about user accounts they've never seen.
	pair = InstrumentConfig::Instance().Export(InstrumentConfig::Instance().Get().InstrumentType == HawkeyeConfig::CellHealth_ScienceModule);
	configurationNode.add_child (pair.first, pair.second);

	instrumentNode.add_child (CONFIGURATION_NODE, configurationNode);

	// Write export tree to encrypted metadata file.
	pt::ptree export_metadata_ptree = {};
	export_metadata_ptree.add_child (INSTRUMENT_NODE, instrumentNode);

	try
	{
		if (!HDA_WriteEncryptedPtreeFile(export_metadata_ptree, inputs_to_export.export_metadata_file_path))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ExportMetadata: <Exit : Failed to write the Metadata to XML file:" + inputs_to_export.export_metadata_file_path + ">");
			return false;
		}
	}
	catch (const boost::property_tree::xml_parser_error & er)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMetadata: <exit, exception occurred while writing metadata to XML file: exception " + std::string(er.what()) + ">");
		return false;
	}

	return true;
}



//*****************************************************************************
static pt::ptree s_sampleNode = {};
static pt::ptree s_sampleSetNode = {};
static pt::ptree s_instrumentNode = {};
static SampleSetDLL s_sampleSet = {};
static std::vector<DBApi::DB_SampleItemRecord> s_dbSampleItemList = {};
static uuid__t s_sampleSetUuid = {};
static uuid__t s_sampleUuid = {};
static ExportUtilities::ExportInputs s_inputs_to_export = {};


//*****************************************************************************
bool ExportUtilities::ExportMeta_Start(ExportInputs& inputs_to_export)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Start <Enter>");
	// Check for valid inputs to perform 
	if (inputs_to_export.rs_uuid_list.empty() ||
		inputs_to_export.export_metadata_file_path.empty() ||
		inputs_to_export.username.empty())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Start <Exit - Invalid input>");
		return false;
	}

	s_inputs_to_export.export_metadata_file_path = inputs_to_export.export_metadata_file_path;
	s_inputs_to_export.image_selection = inputs_to_export.image_selection;
	s_inputs_to_export.nth_image = inputs_to_export.nth_image;
	s_inputs_to_export.username = inputs_to_export.username;
	s_inputs_to_export.rs_uuid_list = inputs_to_export.rs_uuid_list;
	s_inputs_to_export.results_to_export = inputs_to_export.results_to_export;

	// Create binary results directories if they don't exist.

	std::string bindir = HawkeyeDirectory::Instance().getResultBinariesDir();

	if (!FileSystemUtilities::CreateDirectories(bindir))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Start: <Exit Failed to create results binary directory>");
		return false;
	}

	s_sampleNode.clear();
	s_sampleSetNode.clear();
	s_instrumentNode.clear();
	s_instrumentNode.add(VERSION_NODE, InstrumentConfig::GetDLLVersion());
	s_instrumentNode.add (INSTRUMENT_TYPE_NODE, InstrumentConfig::Instance().Get().InstrumentType);

	s_sampleSet = {};
	s_dbSampleItemList = {};
	s_sampleSetUuid = {};
	s_sampleUuid = {};

	// leave for debug
	//for (auto summaryUuid : s_inputs_to_export.rs_uuid_list)
	//{
	//	Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Start: <summaryResult UUID: " + Uuid::ToStr(summaryUuid) + ">");
	//}
	return true;
}

#define MAX_DELAY_MS 200
//*****************************************************************************
bool ExportUtilities::ExportMeta_Next(uint32_t index, std::vector<std::string>& auditLogEntries, std::vector<std::string>& files_to_export, uint32_t delayms)
{
	auto summaryUuid = s_inputs_to_export.rs_uuid_list[index];
	// Get the database summary result record specified by this summary uuid.
	DBApi::DB_SummaryResultRecord dbSummaryResultRecord = {};
	DBApi::DB_SampleSetRecord dbSampleSet = {};
	DBApi::DB_WorklistRecord dbWorklist = {};
	
	DBApi::eQueryResult dbStatus = DBApi::DbFindSummaryResult(dbSummaryResultRecord, summaryUuid);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("ExportMeta_Next: <exit, DbFindSummaryResult failed, status: %ld>") % (int32_t)dbStatus));
		return false;
	}

	// leave for debug
	//Logger::L().Log(MODULENAME, severity_level::debug1,
	//	"ExportMeta_Next: <summary result UUID: " +
	//	Uuid::ToStr(summaryUuid) + ", " +
	//	ChronoUtilities::ConvertToString(dbSummaryResultRecord.ResultDateTP) + ">");

	//Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <sample UUID: " + Uuid::ToStr(dbSummaryResultRecord.SampleId) + ">");

	// Retrieve the SampleSet for this Summary Result.
	// For offline analysis we're only going to use enough data fields to support displaying the 
	// samples as they are seen on the instrument.
	s_dbSampleItemList = {};
	dbStatus = DBApi::DbGetSampleItemList(
		s_dbSampleItemList,
		DBApi::eListFilterCriteria::SampleIdFilter,
		"=",
		boost::str(boost::format("'%s'") % Uuid::ToStr(dbSummaryResultRecord.SampleId)),
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0,
		"",
		-1,
		-1,
		"");
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("ExportMeta_Next: <exit, DbGetSampleItemList failed, status: %ld>") % (int32_t)dbStatus));
		return false;
	}

	dbStatus = DBApi::DbFindSampleSet(dbSampleSet, s_dbSampleItemList[0].SampleSetId);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("ExportMeta_Next: <exit, DbFindSampleSet failed, status: %ld>") % (int32_t)dbStatus));
		return false;
	}

	dbStatus = DBApi::DbFindWorklist(dbWorklist, dbSampleSet.WorklistId);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("ExportMeta_Next: <exit, DbFindWorklist failed, status: %ld>") % (int32_t)dbStatus));
		return false;
	}

	// leave for debug
	//Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <sampleset uuid: " + Uuid::ToStr(s_sampleSetUuid/*sampleSet.uuid*/) + ">");
	//Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <dbSampleSet.SampleSetId uuid: " + Uuid::ToStr(dbSampleSet.SampleSetId) + ">");

	// Check if "summaryUuid" belongs to a different sample.  If so, all summary records get assigned to this sample
	// until the summary result's sample UUID does not match the previous summary result's sample UUID.
	if (Uuid(dbSummaryResultRecord.SampleId) != Uuid(s_sampleUuid))
	{ // Found a new sample.

		// Add the previous Sample node to the SampleSet node.
		if (index != 0)
		{
			// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <adding sampleNode to sampleSetNode>");
			s_sampleSetNode.add_child(SAMPLE_NODE, s_sampleNode);
		}

		// Add the previous SampleSet node to the Instrument (root) node.
		if (Uuid(dbSampleSet.SampleSetId) != Uuid(s_sampleSetUuid))
		{ // Found a new sample.

			// Add the previous Sample node to the SampleSet node.
			if (index != 0)
			{
				// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <adding sampleSetNode to instrumentNode>");
				s_instrumentNode.add_child(SAMPLESET_NODE, s_sampleSetNode);
			}

			// Get the info for the new SampleSet.
			s_sampleSetUuid = dbSampleSet.SampleSetId;
			s_sampleSet.FromDbStyle(dbSampleSet);
			ConvertToPtree(s_sampleSet, s_sampleSetNode);
			s_sampleSetNode.add(CARRIER_TYPE_NODE, dbWorklist.CarrierType);
			s_sampleSetNode.add(BY_COLUMN_NODE, dbWorklist.ByColumn);
		}

		SampleRecordDLL sampleRecord = {};
		sampleRecord.uuid = s_sampleUuid = dbSummaryResultRecord.SampleId;

		// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <sampleProperties.sample_record.uuid: " + Uuid::ToStr(sampleRecord.uuid) + ">");

		// Extract the relevant info from the database sample record.
		if (!HawkeyeResultsDataManager::Instance().RetrieveSampleRecord(dbSummaryResultRecord.SampleId, sampleRecord))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Next: <Exit, failed to retreive sample record>");
			return false;
		}

		// Reagent info not stored in database, get from memory.
		std::vector<ReagentInfoRecordDLL>& reagentInfo = ReagentPack::Instance().GetReagentInfoRecords();
		sampleRecord.reagent_info_records.assign(reagentInfo.begin(), reagentInfo.end());

		ConvertToPtree(sampleRecord, s_sampleNode);

		// Determine if Nth image specified.
		uint32_t nthImage = 1;
		if (s_inputs_to_export.image_selection != eExportImages::eAll || s_inputs_to_export.nth_image != 1)
		{
			nthImage = s_inputs_to_export.nth_image;
		}

		if (delayms < 1)
			delayms = 1;

		if (delayms > MAX_DELAY_MS)
			delayms = MAX_DELAY_MS;

		// Handle case when nth image is specified. Will always get first and last (per 1.2)
		uint32_t imageIdx = 1;

		// Loop through image sequences and get image records
		for (auto& it : sampleRecord.imageSequences)
		{
			Sleep(delayms);

			if ((imageIdx == 1) || (imageIdx == sampleRecord.imageSequences.size()) || ((imageIdx % nthImage) == 0))
			{
				// Get all relevant image information
				SampleImageSetRecordDLL sampleImageSetRecord = {};

				if (HawkeyeResultsDataManager::Instance().RetrieveSampleImageSetRecord(it, sampleImageSetRecord) == HawkeyeError::eSuccess)
				{
					sampleImageSetRecord.username = s_inputs_to_export.username;

					uuid__t bf_imgRecId = {};
					int16_t seqNum = 0;
					std::string imgPath;
					system_TP timestamp = {};
					if (HawkeyeResultsDataManager::Instance().RetrieveImageInfo(it, seqNum, imgPath, bf_imgRecId, timestamp) == HawkeyeError::eSuccess)
					{
						ImageRecordDLL imageRecord = {};
						imageRecord.uuid = bf_imgRecId;
						imageRecord.path = imgPath.substr(2);	// Strip off the drive letter and colon.
						imageRecord.timestamp = timestamp;
						imageRecord.username = s_inputs_to_export.username;
						sampleImageSetRecord.sequence_number = seqNum;

						std::vector<ImageRecordDLL> fl_imr_list;
						pt::ptree image_node = {};
						ConvertToPtree(sampleImageSetRecord, imageRecord, fl_imr_list, image_node);
						s_sampleNode.add_child(IMAGE_SET_NODE, image_node);

						std::string encrypted_path = {};
						if (HDA_GetEncryptedFileName(imgPath, encrypted_path))
							files_to_export.push_back(encrypted_path);
						else
						{
							Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Next: <exit, failed to get image results path>");
							return false;
						}
					}
					else
					{
						Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Next: <exit, failed to retrieve image info>");
						return false;
					}
				}
				else
				{
					Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Next: <exit, failed to retreive sample image set record>");
					return false;
				}
			}
			++imageIdx;
		}

	} // End "if (Uuid(dbSummaryResultRecord.SampleId) != Uuid(sampleUuid))"

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: calling ExtractSummaryResult(...)");

	// Get summary results from DB record and store in summary_results struct
	ResultSummaryDLL summary_results = {};
	if (!ExtractSummaryResult(dbSummaryResultRecord, summary_results))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Next: <exit, failed to extract summary results from database record>");
		return false;
	}

	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <summary result UUID 2: " + Uuid::ToStr(summary_results.uuid) + ">");

	CellTypesDLL::getCellTypeByIndex(dbSummaryResultRecord.CellTypeIndex, summary_results.cell_type_settings);

	// Copy summary result info into tree node
	pt::ptree summary_result_node = {};
	ConvertToPtree(summary_results, summary_result_node);

	// Generate audit log string
	auditLogEntries.push_back(getAuditLogStr(s_sampleNode, summary_result_node));

	// Add summary tree node to sample node.
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Next: <adding summaryResultNode to sampleNode>");
	s_sampleNode.add_child(SUMMARY_RESULTS_NODE, summary_result_node);

	std::string encrypted_path = {};
	if (HDA_GetEncryptedFileName(summary_results.path, encrypted_path))
		files_to_export.push_back(encrypted_path);
	else
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Next: <exit, failed get binary results path>");
		return false;
	}

	return true;
}

//*****************************************************************************
bool ExportUtilities::ExportMeta_Finalize()
{
	// Wrap up the last summary result into it's SampleSet and Sample.
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Finalize: <adding sampleNode to sampleSetNode>");
	s_sampleSetNode.add_child(SAMPLE_NODE, s_sampleNode);
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "ExportMeta_Finalize: <adding sampleSetNode to instrumentNode>");
	s_instrumentNode.add_child(SAMPLESET_NODE, s_sampleSetNode);

	// Export Configuration Data.
	pt::ptree configurationNode = {};
	std::pair<std::string, boost::property_tree::ptree> pair;

	pair = AnalysisDefinitionsDLL::Export();
	configurationNode.add_child(pair.first, pair.second);
	pair = CellTypesDLL::Export();
	configurationNode.add_child(pair.first, pair.second);
	pair = QualityControlsDLL::Export();
	configurationNode.add_child(pair.first, pair.second);
	pair = SignaturesDLL::Export();
	configurationNode.add_child(pair.first, pair.second);
	pair = UserList::Instance().Export();
	configurationNode.add_child(pair.first, pair.second);

	// LH6531-6532 - Cell Health Modules should disable security on data exports for use in offline analysis tool
	// This removes our need to handle user accounts that the user will not know about.
	pair = InstrumentConfig::Instance().Export(InstrumentConfig::Instance().Get().InstrumentType == HawkeyeConfig::CellHealth_ScienceModule);
	configurationNode.add_child(pair.first, pair.second);

	s_instrumentNode.add_child(CONFIGURATION_NODE, configurationNode);

	// Write export tree to encrypted metadata file.
	pt::ptree export_metadata_ptree = {};
	export_metadata_ptree.add_child(INSTRUMENT_NODE, s_instrumentNode);

	try
	{
		if (!HDA_WriteEncryptedPtreeFile(export_metadata_ptree, s_inputs_to_export.export_metadata_file_path))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Finalize: <Exit : Failed to write the Metadata to XML file >");
			return false;
		}
	}
	catch (const boost::property_tree::xml_parser_error& er)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "ExportMeta_Finalize: <exit, exception occurred while writing metadata to XML file: exception " + std::string(er.what()) + ">");
		return false;
	}

	return true;
}
