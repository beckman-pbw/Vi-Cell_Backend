#include "stdafx.h"

#include "AppConfig.hpp"

#include "CommandParser.hpp"
#include "DataImporter.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeConfig.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeUUID.hpp"
#include "ImageAnalysisUtilities.hpp"
#include "InstrumentConfig.hpp"
#include "SignaturesDLL.hpp"
#include "SResultBinStorage.hpp"

#define POW_2_10 1024.0

//*****************************************************************************
struct sortSamplesByCarouselColumn
{
	bool operator() (const WorkQueueItemRecord& a, const WorkQueueItemRecord& b) const
	{
		SamplePosition positionA = a.sampleRecord.GetPosition().getSamplePosition();
		SamplePosition positionB = b.sampleRecord.GetPosition().getSamplePosition();
		return (positionA.col < positionB.col);
	}
};

struct sortSamplesByPlateRow
{
	bool operator() (const WorkQueueItemRecord& a, const WorkQueueItemRecord& b) const
	{
		SamplePosition positionA = a.sampleRecord.GetPosition().getSamplePosition();
		SamplePosition positionB = b.sampleRecord.GetPosition().getSamplePosition();

		if (positionA.row != positionB.row)
		{
			return (positionA.row < positionB.row);
		}
		return (positionA.col < positionB.col);
	}
};

struct sortSamplesByPlateColumn
{
	bool operator() (const WorkQueueItemRecord& a, const WorkQueueItemRecord& b) const
	{
		SamplePosition positionA = a.sampleRecord.GetPosition().getSamplePosition();
		SamplePosition positionB = b.sampleRecord.GetPosition().getSamplePosition();

		if (positionA.col != positionB.col)
		{
			return (positionA.col < positionB.col);
		}
		return (positionA.row < positionB.row);
	}
};

//*****************************************************************************
// v1.2...
//*****************************************************************************
bool DataImporter::ImportData ()
{
	std::vector<WorkQueueDataRecord> wqDataRecords = {};

	bool status = RetrieveDataRecordsForUpgrade (wqDataRecords);
	if (!status)
	{
		Log ("Failed to retrieve the data...", severity_level::error);
		return false;
	}

	if (wqDataRecords.empty())
	{
		Log ("No Workqueue records to import...", severity_level::error);
		return true;
	}

	int index = 0;
	std::stringstream ss;
	ss << "\nWorkQueues to import:\n";
	for (const auto& wqDataRecord : wqDataRecords)
	{
		ss << ++index
			<< ") UUID: " << Uuid::ToStr (wqDataRecord.workqueueRecord.uuid)
			<< ", Label: " << (wqDataRecord.workqueueRecord.label.empty() ? "EMPTY" : wqDataRecord.workqueueRecord.label)
			<< ", Username: " << (wqDataRecord.workqueueRecord.username.empty() ? "EMPTY" : wqDataRecord.workqueueRecord.username)
			<< ", Timestamp: " << ChronoUtilities::ConvertToString(wqDataRecord.workqueueRecord.timestamp, "%Y-%m-%d %H:%M:%S")
			<< ", # Samples: " << wqDataRecord.workqueueItems.size()
			<< std::endl;
	}
	Log (ss.str());

	uint32_t wqr_index = 0;

	// Calculate the memory.
	// Total memory = Memory required for each sample images and result records.
	double totalMemoryMB = 0;
	std::string image_path = HawkeyeDirectory::Instance().getImagesBaseDir();

	std::string newInstrumentDir = HawkeyeDirectory::Instance().getInstrumentDir(false);

	for (auto& wqDataRecord : wqDataRecords)
	{
		for (auto& wqi : wqDataRecord.workqueueItems)
		{
			// This is kind of a hack...
			// The v1.3 export data format does not support ImageSets like in v1.2.
			// In reality, the v1.3 format needs to be refactored to better mirror the v1.3 data structures.
			CommandParser cp;
			cp.parse ("\\", wqi.imageDataRecords[0].imageRecord.path);

			std::string path = {};
			for (int i = 1; i <= 4; i++)
			{
				path.append ("\\" + cp.getByIndex(i));
			}
			path = std::regex_replace (path, std::regex("Instrument"), newInstrumentDir);

			auto images_size = FileSystemUtilities::GetSizeoFTheFileorDirectory(path) / (POW_2_10 * POW_2_10);

			totalMemoryMB += images_size;

			for (auto& rs : wqi.sampleRecord.result_summaries)
			{
				// Update the instrument path with the real one.
				std::string binfilePath = std::regex_replace (rs.path, std::regex("Instrument"), newInstrumentDir);
				binfilePath = std::regex_replace (binfilePath, std::regex(".bin"), ".ebin");

				auto rr_size = FileSystemUtilities::GetSizeoFTheFileorDirectory (binfilePath) / (POW_2_10 * POW_2_10);
				totalMemoryMB += rr_size;
			}
		}
	}

	boost::filesystem::space_info space_info = FileSystemUtilities::GetSpaceInfoOfDisk (HawkeyeDirectory::Instance().getDriveId());
	Log (boost::str (boost::format ("Used disk space: %f GB")
		% ((space_info.capacity - space_info.free) / (POW_2_10 * POW_2_10 * POW_2_10))));
	Log (boost::str (boost::format ("Free disk space: %f GB")
		% (space_info.free / (POW_2_10 * POW_2_10 * POW_2_10))));
	Log (boost::str (boost::format ("Disk space required to import the data: %f GB")
		% (totalMemoryMB / POW_2_10)));

	if (!FileSystemUtilities::IsRequiredFreeDiskSpaceInMBAvailable(static_cast<uint64_t>(totalMemoryMB)))
	{
		Log ("Insufficient storage space...", severity_level::error);
		return false;
	}

	Log ("Please wait importing data...");

	int wqCount = 1;
	for (const auto& wqDataRecord : wqDataRecords)
	{
		// Create a Worklist record.
		WorklistDLL::WorklistData worklist = {};
		worklist.uuid = wqDataRecord.workqueueRecord.uuid;
		worklist.username = wqDataRecord.workqueueRecord.username;
		worklist.runUsername = wqDataRecord.workqueueRecord.username;
		worklist.label = wqDataRecord.workqueueRecord.label;
		if (UserList::Instance().GetUserUUIDByName(worklist.username, worklist.runUserUuid) != HawkeyeError::eSuccess)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("ImportData: <username %s not found>") % worklist.username));
			//TODO: to use in place of the missing username???
		}

		worklist.userUuid = worklist.runUserUuid;
		worklist.timestamp = wqDataRecord.workqueueRecord.timestamp;

		// The WorkQueue label field is not written to the v1.2 HawkeyeMetaData file.
		if (worklist.label.empty())
		{
			worklist.label = boost::str (boost::format ("SampleSet %d") % wqCount);
		}
		Log ("Importing Worklist: " + worklist.label + ", timestamp: " + ChronoUtilities::ConvertToString (worklist.timestamp, "%Y-%m-%d %H:%M:%S"));

		worklist.orphanParameterSettings.postWash = eSamplePostWash::eNormalWash;
		worklist.orphanParameterSettings.dilutionFactor = 1;
		worklist.orphanParameterSettings.saveEveryNthImage = 1;
		worklist.orphanParameterSettings.analysis.uuid = {};
		worklist.orphanParameterSettings.analysis.analysis_index = 0;
		worklist.orphanParameterSettings.celltype.uuid = {};
		worklist.orphanParameterSettings.celltype.celltype_index = 0;
		worklist.useSequencing = false;
		worklist.sequencingDigit = 0;
		worklist.sequencingTextFirst = true;
		worklist.sequencingBaseLabel = "";
		worklist.sequencingNumberOfDigits = 0;

		// Create a single SampleSet for all of the WorkQueueItems in the WorkQueue record.
		// v1.2 data creates one SampleSet for each WorkQueue.
		SampleSetDLL sampleSet = {};
		sampleSet.index = 1;	// Use 1 here to skip the imaginary orphan SampleSet (index 0).
		sampleSet.worklistUuid = worklist.uuid;
		sampleSet.setLabel = worklist.label;
		sampleSet.worklistUuid = worklist.uuid;
		sampleSet.timestamp = worklist.timestamp;
		sampleSet.carrier = eCarrierType::eUnknown;
		sampleSet.name = worklist.label;
		sampleSet.username = worklist.username;
		uuid__t tUUID = {};
		if (UserList::Instance().GetUserUUIDByName(sampleSet.username, tUUID) != HawkeyeError::eSuccess)
		{
			sampleSet.userUuid = tUUID;
		}
		else
		{
			sampleSet.userUuid = tUUID;
		}
		sampleSet.setLabel = "";	// Tag field.
		sampleSet.status = DBApi::eSampleSetStatus::SampleSetComplete;
		sampleSet.samplesetIdNum = 0;	// Updated by the DB code.

		uint16_t sIndex = 0;
		for (const auto& wqi : wqDataRecord.workqueueItems)
		{
			bool usingNewSampleRecord = true;

			DBApi::DB_SampleRecord dbSampleRecord = {};
			SampleDefinitionDLL sampleDef = {};

			std::shared_ptr<ImageCollection_t> imageCollection = std::make_shared<ImageCollection_t>();
			ImageDataForSResultMap imageDataForSResult = {};

			HawkeyeDirectory::Instance().useImportInstrumentDir();

			for (const auto& imageDataRecord : wqi.imageDataRecords)
			{
				std::string imagePath = boost::str (boost::format ("%s%s")
					% HawkeyeDirectory::Instance().getDriveId()
					% std::regex_replace (imageDataRecord.imageRecord.path, std::regex("Instrument"), newInstrumentDir));

				cv::Mat image = {};
				if (!HDA_ReadEncryptedImageFile (imagePath, image))
				{
					Logger::L().Log(MODULENAME, severity_level::warning,
						boost::str (boost::format ("ImportData: image file not found, %s") % imagePath));
					continue;
				}

				imageDataForSResult[imageDataRecord.imagesetRecord.sequence_number].uuid = imageDataRecord.imagesetRecord.uuid;
				imageDataForSResult[imageDataRecord.imagesetRecord.sequence_number].timestamp = imageDataRecord.imagesetRecord.timestamp;

				imageCollection->push_back ({ image, {} });
			}

			// Create Result record meta data and copy all the result binaries.
			// The binary path is the real one.
			for (auto& summaryResult : wqi.sampleRecord.result_summaries)
			{
				DBApi::DB_AnalysisRecord dbAnalysisRecord = {};
				CellCounterResult::SResult sresult = {};

				AnalysisDefinitionsDLL::GetAnalysisByIndex (summaryResult.analysis_settings.analysis_index, sampleDef.parameters.analysis);
				sampleDef.parameters.celltype = CellTypesDLL::getCellTypeByIndex (summaryResult.cell_type_settings.celltype_index);

				// v1.2.43 actually deleted CellTypes from the CellType.info file.  This resulted in a CellType index of -1 when the imported into v1.3 and later.
				// This caused a crash (https://lsjira.beckman.com/browse/PC3549-5907)
				// Check for invalid CellType index returned from "getCellTypeByIndex".
				if (sampleDef.parameters.celltype.celltype_index == DELETED_CELLTYPE_INDEX)
				{					
					// getCellTypeByIndex returned -1 (4294967295) meaning the CellType is not found, recreate the CellType and save it to the database.
					ResultSummaryDLL sr = summaryResult;
					sr.cell_type_settings.retired = true;
					CellTypesDLL::Add (sr.cell_type_settings, true);
					
					AnalysisDefinitionsDLL::GetAnalysisByIndex (summaryResult.analysis_settings.analysis_index, sampleDef.parameters.analysis);
					sampleDef.parameters.celltype = CellTypesDLL::getCellTypeByIndex (sr.cell_type_settings.celltype_index);
				}
				
				if (usingNewSampleRecord)
				{
					// Create a SampleDefinition (SampleItem) for each sample record.
					// The first <Result> tag in the HawkeyeMetaData.xml file is the original result.
					// Other results in the WorkQueue record are reanalyses.

					sampleDef.index = sIndex++;
					sampleDef.sampleSetIndex = sampleSet.index;
					sampleDef.username = worklist.username;
					sampleDef.runUserID = sampleSet.userUuid;
					sampleDef.timestamp = wqi.sampleRecord.timestamp;

					sampleDef.position = { 'Z', 1 };

					sampleDef.status = SampleDefinitionDLL::FromSampleLogCompletionStatus (wqi.sampleRecord.sam_comp_status);

					// The Analysis and Celltype is set just before SaveAnalysisData is called to ensure
					// that reanalyized summary records are set correctly.
					sampleDef.parameters.tag = wqi.sampleRecord.comment;
					sampleDef.parameters.postWash = static_cast<eSamplePostWash>(wqi.sampleRecord.wash);
					sampleDef.parameters.dilutionFactor = wqi.sampleRecord.dilution_factor;

					//TODO: is this correct?  Possibly not... the field is not in the XML file.
					sampleDef.parameters.saveEveryNthImage = 1;		// Save all the images that are available.

					sampleDef.parameters.label = wqi.sampleRecord.sample_identifier;
					sampleDef.parameters.tag = wqi.sampleRecord.comment;
					if (!wqi.sampleRecord.bp_qc_identifier.empty())
					{
						QualityControlDLL qc = {};
						if (getQualityControlByName (wqi.sampleRecord.bp_qc_identifier, qc))
						{
							dbSampleRecord.QcId = qc.uuid;
						}
					}

					auto str = boost::str (boost::format ("%d: %s, %s")
						% sIndex
						% sampleDef.parameters.label
						% ChronoUtilities::ConvertToString (summaryResult.timestamp, "%Y-%m-%d %H:%M:%S"));
					Log (str);

					dbSampleRecord.AcquisitionDateTP = summaryResult.timestamp;

					if (!HawkeyeResultsDataManager::CreateSampleRecord (sampleDef, dbSampleRecord))
					{ // Errors are reported by CreateSampleRecord.
						return false;
					}

					// Update the SampleDefintion (SampleItem) with the just created Sample UUID.
					sampleDef.sampleDataUuid = dbSampleRecord.SampleId;

					sampleSet.samples.push_back (sampleDef);

					HawkeyeDirectory::Instance().useInstrumentDir();

					usingNewSampleRecord = false;

				} // End "if (usingNewSampleRecord)"

				dbAnalysisRecord.AnalysisDateTP = summaryResult.timestamp;
				dbAnalysisRecord.AnalysisUserId = {};
				UserList::Instance().GetUserUUIDByName(summaryResult.username, dbAnalysisRecord.AnalysisUserId);

				if (!HawkeyeResultsDataManager::CreateAnalysisRecord (dbSampleRecord, dbAnalysisRecord))
				{ // Errors are reported by CreateAnalysisRecord.
					return false;
				}

				// Get the SResult data.
				{
					HawkeyeDirectory::Instance().useImportInstrumentDir();

					std::string binfilePath = std::regex_replace (summaryResult.path, std::regex("Instrument"), newInstrumentDir);

					// Update the instrument path with the real one.
					binfilePath = boost::str (boost::format ("%s%s")
						% HawkeyeDirectory::Instance().getDriveId()
						% binfilePath);

					if (!SResultBinStorage::DeSerializeSResult (sresult, binfilePath))
					{
						Logger::L().Log (MODULENAME, severity_level::error,
							boost::str (boost::format ("ImportData: failure to read result file: %s") % summaryResult.path));
						continue;
					}

					HawkeyeDirectory::Instance().useInstrumentDir();
				}

				for (auto& sigSet : summaryResult.signature_set)
				{
					DBApi::db_signature_t dbSignature = {};

					dbSignature.userName = sigSet.signing_user;
					dbSignature.signatureTime = sigSet.timestamp;
					dbSignature.shortSignature = sigSet.signature.short_text;
					dbSignature.longSignature = sigSet.signature.long_text;

					// v1.2 does not export the signature hash so it is calculated here.
					auto time = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(dbSignature.signatureTime);
					std::string buf = dbSignature.userName + dbSignature.shortSignature + dbSignature.longSignature + std::to_string(time);
					size_t len = dbSignature.userName.length() + dbSignature.shortSignature.length() + dbSignature.longSignature.length() + sizeof(time);
					dbSignature.signatureHash = SignaturesDLL::SignText (buf);

					dbAnalysisRecord.SummaryResult.SignatureList.push_back (dbSignature);
				}

				SaveAnalysisData(
					worklist.uuid,
					sampleDef,
					imageCollection,
					imageDataForSResult,
					sresult,
					wqi.sampleRecord.reagent_info_records,
					{},
					dbSampleRecord,
					dbAnalysisRecord,
					static_cast<QcStatus>(dbAnalysisRecord.SummaryResult.QcStatus),
					nullptr);

			} // End "for (auto& summaryResult : wqi.sampleRecord.result_summaries)"

			// Delete the v1.2 image records for this WorkQueueItem.
			for (const auto& v : wqi.imageDataRecords)
			{
				std::string pathToDelete = std::regex_replace (v.imageRecord.path, std::regex("Instrument"), newInstrumentDir);
				pathToDelete = std::regex_replace (pathToDelete, std::regex(".png"), ".epng");

				// Update the instrument path with the real one.
				pathToDelete = boost::str (boost::format ("%s%s")
					% HawkeyeDirectory::Instance().getDriveId()
					% pathToDelete);

				if (!FileSystemUtilities::RemoveAll (pathToDelete))
				{
					DataImporter::Log ("Failed to delete ResultBinaries directory", severity_level::warning);
				}
			}

		} // End "for (const auto& wqi : wqDataRecord.workqueueItems)"

		worklist.sampleSetsDLL.push_back (sampleSet);

		DBApi::DB_WorklistRecord dbWorklist = WorklistDLL::WorklistData::ToDbStyle (worklist, DBApi::eWorklistStatus::WorklistComplete);
		{
			dbWorklist.InstrumentSNStr = HawkeyeConfig::Instance().get().instrumentSerialNumber;
			dbWorklist.RunDateTP = worklist.timestamp;

			// Since the v1.2 data does not contain the create or modify timestamps,
			// fix up the SampleSet create and modify timestamps to match the run timestamp.
			for (int i=0; i<dbWorklist.SSList.size(); i++)
			{
				dbWorklist.SSList[i].CreateDateTP = dbWorklist.SSList[i].RunDateTP;
				dbWorklist.SSList[i].ModifyDateTP = dbWorklist.SSList[i].RunDateTP;
			}
			
			DBApi::eQueryResult dbStatus = DBApi::DbAddWorklist (dbWorklist);
			if (dbStatus != DBApi::eQueryResult::QueryOk)
			{
				Log ("Worklist already exists", severity_level::error);
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("ImportData: <exit, DbAddWorklist failed, status: %ld>") % (int32_t)dbStatus));
				return false;
			}
		}

		wqCount++;

	} // End "for (const auto& wqr : wqrList_)"

	return true;
}

//*****************************************************************************
// v1.3 Instrument to v1.3 Offline Analysis.
//*****************************************************************************
#define ImageSetPosition 6
bool DataImporter::ImportDataForOfflineAnalysis (std::string serialNumber)
{
	std::vector<ImportedSampleSet> importedSampleSets = {};

	bool status = RetrieveDataRecordsForOfflineAnalysis (importedSampleSets);
	if (!status)
	{
		Log ("Failed to retrieve the data...", severity_level::error);
		return false;
	}

	if (importedSampleSets.empty())
	{
		Log ("No SampleSets to import...", severity_level::error);
		return false;
	}

	uint32_t wqr_index = 0;

	// Calculate the memory.
	// Total memory = Memory required for each sample images and result records.
	double totalMemoryMB = 0;
	std::string image_path = HawkeyeDirectory::Instance().getImagesBaseDir();

	std::string newInstrumentDir = HawkeyeDirectory::Instance().getInstrumentDir(false);

	for (auto& importedSampleSet : importedSampleSets)
	{
		for (auto& wqi : importedSampleSet.workqueueItems)
		{
			// This is kind of a hack...
			// The v1.3 export data format does not support ImageSets like in v1.2.
			// In reality, the v1.3 format needs to be refactored to better mirror the v1.3 data structures.
			CommandParser cp;
			cp.parse ("\\", wqi.imageDataRecords[0].imageRecord.path);

			std::string path = {};
			for (int i = 1; i <= ImageSetPosition; i++)
			{
				path.append ("\\" + cp.getByIndex(i));
			}
			path = std::regex_replace (path, std::regex("Instrument"), newInstrumentDir);

			auto images_size = FileSystemUtilities::GetSizeoFTheFileorDirectory(path) / (POW_2_10 * POW_2_10);
			
			totalMemoryMB += images_size;

			//Keep for debugging: Log (boost::str (boost::format("%s:    %lf, %lf") % path % images_size % totalMemoryMB));

			for (auto& rs : wqi.sampleRecord.result_summaries)
			{
				// Update the instrument path with the real one.
				std::string binfilePath = std::regex_replace (rs.path, std::regex("Instrument"), newInstrumentDir);
				binfilePath = std::regex_replace (binfilePath, std::regex(".bin"), ".ebin");

				auto rr_size = FileSystemUtilities::GetSizeoFTheFileorDirectory (binfilePath) / (POW_2_10 * POW_2_10);

				totalMemoryMB += rr_size;

				//Keep for debugging: Log (boost::str (boost::format("%s: %lf, %lf") % binfilePath % rr_size % totalMemoryMB));
			}
		}
	}

	boost::filesystem::space_info space_info = FileSystemUtilities::GetSpaceInfoOfDisk (HawkeyeDirectory::Instance().getDriveId());
	Log (boost::str (boost::format ("Used disk space: %f GB")
		% ((space_info.capacity - space_info.free) / (POW_2_10 * POW_2_10 * POW_2_10))));
	Log (boost::str (boost::format ("Free disk space: %f GB")
		% (space_info.free / (POW_2_10 * POW_2_10 * POW_2_10))));
	Log (boost::str (boost::format ("Disk space required to import the data: %f GB")
		% (totalMemoryMB / POW_2_10)));

	if (!FileSystemUtilities::IsRequiredFreeDiskSpaceInMBAvailable(static_cast<uint64_t>(totalMemoryMB)))
	{
		Log ("Insufficient storage space...", severity_level::error);
		return false;
	}

	Log ("Please wait importing data...");

	//NOTE: the serial number is not imported so as to not confuse users.
	//DBApi::DB_InstrumentConfigRecord& instConfig = InstrumentConfig::Instance().Get();
	//instConfig.InstrumentSNStr = serialNumber;
	//InstrumentConfig::Instance().Set();

	bool doOnce = true;
	int ssCount = 1;
	uint16_t ssIndex = 1;

	WorklistDLL::WorklistData worklist = {};

	for (const auto& importedSampleSet : importedSampleSets)
	{
		if (doOnce)
		{
			// Create the only Worklist record.  All SampleSets are put in this one Worklist.
			HawkeyeUUID::Generate().get_uuid__t (worklist.uuid);
			worklist.runUserUuid = worklist.uuid;
			worklist.username = importedSampleSet.sampleSetDLL.username;
			worklist.runUsername = worklist.username;
			if (UserList::Instance().GetUserUUIDByName(worklist.username, worklist.runUserUuid) != HawkeyeError::eSuccess)
			{
				Logger::L().Log (MODULENAME, severity_level::error,
					boost::str (boost::format ("ImportData: <username %s not found>") % worklist.username));
				return false;
			}

			worklist.userUuid = worklist.runUserUuid;

			// Use the timestamp from the first sample record.
			worklist.timestamp = importedSampleSet.workqueueItems[0].sampleRecord.timestamp;

			worklist.status = static_cast<int32_t>(DBApi::eWorklistStatus::WorklistRunning);
			worklist.orphanParameterSettings.postWash = eSamplePostWash::eNormalWash;
			worklist.orphanParameterSettings.dilutionFactor = 1;
			worklist.orphanParameterSettings.saveEveryNthImage = 1;
			worklist.orphanParameterSettings.analysis.uuid = {};
			worklist.orphanParameterSettings.analysis.analysis_index = 0;
			worklist.orphanParameterSettings.celltype.uuid = {};
			worklist.orphanParameterSettings.celltype.celltype_index = 0;
			worklist.useSequencing = false;
			worklist.sequencingDigit = 0;
			worklist.sequencingTextFirst = true;
			worklist.sequencingBaseLabel = "";
			worklist.sequencingNumberOfDigits = 0;

			doOnce = false;
		}

		// Create a single SampleSet for all of the WorkQueueItems in the WorkQueue record.
		SampleSetDLL sampleSetDLL = {};
		sampleSetDLL.index = 1;	// Use 1 here to skip the imaginary orphan SampleSet (index 0).
		sampleSetDLL.worklistUuid = worklist.uuid;
		sampleSetDLL.setLabel = importedSampleSet.sampleSetDLL.setLabel;	// Tag field.
		sampleSetDLL.uuid = importedSampleSet.sampleSetDLL.uuid;
		sampleSetDLL.timestamp = importedSampleSet.sampleSetDLL.timestamp;
		sampleSetDLL.carrier = eCarrierType::eUnknown;
		sampleSetDLL.name = importedSampleSet.sampleSetDLL.name;
		sampleSetDLL.username = importedSampleSet.sampleSetDLL.username;
		UserList::Instance().GetUserUUIDByName(importedSampleSet.sampleSetDLL.username, sampleSetDLL.userUuid);
		sampleSetDLL.status = static_cast<DBApi::eSampleSetStatus>(importedSampleSet.sampleSetDLL.status);

		auto str = boost::str (boost::format ("SampleSet %d: %s, %s")
			% ssIndex++
			% sampleSetDLL.name
			% ChronoUtilities::ConvertToString (sampleSetDLL.timestamp, "%Y-%m-%d %H:%M:%S"));
		Log (str);

		uint16_t sIndex = 0;
		for (const auto& wqi : importedSampleSet.workqueueItems)
		{
			bool usingNewSampleRecord = true;

			DBApi::DB_SampleRecord dbSampleRecord = {};
			SampleDefinitionDLL sampleDef = {};

			std::shared_ptr<ImageCollection_t> imageCollection = std::make_shared<ImageCollection_t>();
			ImageDataForSResultMap imageDataForSResult = {};

			HawkeyeDirectory::Instance().useImportInstrumentDir();

			for (const auto& imageDataRecord : wqi.imageDataRecords)
			{
				std::string imagePath = boost::str (boost::format ("%s%s")
					% HawkeyeDirectory::Instance().getDriveId()
					% std::regex_replace (imageDataRecord.imageRecord.path, std::regex("Instrument"), newInstrumentDir));

				cv::Mat image = {};
				if (!HDA_ReadEncryptedImageFile (imagePath, image))
				{
					Logger::L().Log(MODULENAME, severity_level::warning,
						boost::str (boost::format ("ImportData: image file not found, %s") % imagePath));
					continue;
				}

				imageDataForSResult[imageDataRecord.imagesetRecord.sequence_number].uuid = imageDataRecord.imagesetRecord.uuid;
				imageDataForSResult[imageDataRecord.imagesetRecord.sequence_number].timestamp = imageDataRecord.imagesetRecord.timestamp;

				imageCollection->push_back ({ image, {} });
			}

			// Create Result record meta data and copy all the result binaries.
			// The binary path is the real one.
			for (auto& summaryResult : wqi.sampleRecord.result_summaries)
			{
				DBApi::DB_AnalysisRecord dbAnalysisRecord = {};
				CellCounterResult::SResult sresult = {};

				AnalysisDefinitionsDLL::GetAnalysisByIndex (summaryResult.analysis_settings.analysis_index, sampleDef.parameters.analysis);
				sampleDef.parameters.celltype = CellTypesDLL::getCellTypeByIndex (summaryResult.cell_type_settings.celltype_index);

				if (usingNewSampleRecord)
				{
					// Create a SampleDefinition (SampleItem) for each sample record.
					// The first <Result> tag in the HawkeyeMetaData.xml file is the original result.
					// Other results in the WorkQueue record are reanalyses.

					sampleDef.index = sIndex++;
					sampleDef.sampleSetIndex = sampleSetDLL.index;
					sampleDef.username = summaryResult.username;
					sampleDef.runUserID = sampleSetDLL.userUuid;
					UserList::Instance().GetUserUUIDByName(summaryResult.username, sampleDef.runUserID);
					sampleDef.timestamp = wqi.sampleRecord.timestamp;
					sampleDef.position = wqi.sampleRecord.GetPosition();
					sampleDef.status = SampleDefinitionDLL::FromSampleLogCompletionStatus (wqi.sampleRecord.sam_comp_status);

					sampleDef.parameters.tag = wqi.sampleRecord.comment;
					sampleDef.parameters.postWash = wqi.sampleRecord.wash;
					sampleDef.parameters.dilutionFactor = wqi.sampleRecord.dilution_factor;

					//TODO: is this correct?  Possibly not... the field is not in the XML file.
					sampleDef.parameters.saveEveryNthImage = 1;		// Save all the images that are available.

					sampleDef.parameters.label = wqi.sampleRecord.sample_identifier;
					sampleDef.parameters.tag = wqi.sampleRecord.comment;
					if (!wqi.sampleRecord.bp_qc_identifier.empty())
					{
						QualityControlDLL qc = {};
						if (getQualityControlByName (wqi.sampleRecord.bp_qc_identifier, qc))
						{
							dbSampleRecord.QcId = qc.uuid;
						}
					}

					auto str = boost::str (boost::format ("\t%d: %s, %s")
						% sIndex
						% sampleDef.parameters.label
						% ChronoUtilities::ConvertToString (summaryResult.timestamp, "%Y-%m-%d %H:%M:%S"));
					Log (str);

					dbSampleRecord.AcquisitionDateTP = summaryResult.timestamp;

					if (!HawkeyeResultsDataManager::CreateSampleRecord (sampleDef, dbSampleRecord))
					{ // Errors are reported by CreateSampleRecord.
						return false;
					}

					HawkeyeDirectory::Instance().useInstrumentDir();

					// Update the SampleDefintion (SampleItem) with the just created Sample UUID.
					sampleDef.sampleDataUuid = dbSampleRecord.SampleId;

					sampleSetDLL.samples.push_back (sampleDef);

					usingNewSampleRecord = false;

				} // End "if (usingNewSampleRecord)"

				dbAnalysisRecord.AnalysisDateTP = summaryResult.timestamp;
				dbAnalysisRecord.AnalysisUserId = {};
				UserList::Instance().GetUserUUIDByName(summaryResult.username, dbAnalysisRecord.AnalysisUserId);

				if (!HawkeyeResultsDataManager::CreateAnalysisRecord (dbSampleRecord, dbAnalysisRecord))
				{ // Errors are reported by CreateAnalysisRecord.
					return false;
				}

				// Get the SResult data.
				{
					HawkeyeDirectory::Instance().useImportInstrumentDir();

					std::string binfilePath = std::regex_replace (summaryResult.path, std::regex("Instrument"), newInstrumentDir);

					// Update the instrument path with the real one.
					binfilePath = boost::str (boost::format ("%s%s")
						% HawkeyeDirectory::Instance().getDriveId()
						% binfilePath);

					if (!SResultBinStorage::DeSerializeSResult (sresult, binfilePath))
					{
						Logger::L().Log (MODULENAME, severity_level::error,
							boost::str (boost::format ("ImportData: failure to read result file: %s") % summaryResult.path));
						continue;
					}

					HawkeyeDirectory::Instance().useInstrumentDir();
				}

				for (auto& sigSet : summaryResult.signature_set)
				{
					DBApi::db_signature_t dbSigSet = {};
					
					dbSigSet.userName = sigSet.signing_user;
					dbSigSet.signatureTime = sigSet.timestamp;
					dbSigSet.shortSignature = sigSet.signature.short_text;
					dbSigSet.longSignature = sigSet.signature.long_text;
					dbSigSet.signatureHash = sigSet.signature.sig_hash;
					
					dbAnalysisRecord.SummaryResult.SignatureList.push_back (dbSigSet);
				}

				SaveAnalysisData(
					worklist.uuid,
					sampleDef,
					imageCollection,
					imageDataForSResult,
					sresult,
					wqi.sampleRecord.reagent_info_records,
					{},
					dbSampleRecord,
					dbAnalysisRecord,
					static_cast<QcStatus>(dbAnalysisRecord.SummaryResult.QcStatus),
					nullptr);

			} // End "for (auto& summaryResult : wqi.sampleRecord.result_summaries)"

			// Delete the v1.2 image records for this WorkQueueItem.
			for (const auto& v : wqi.imageDataRecords)
			{
				std::string pathToDelete = std::regex_replace (v.imageRecord.path, std::regex("Instrument"), newInstrumentDir);
				pathToDelete = std::regex_replace (pathToDelete, std::regex(".png"), ".epng");

				// Update the instrument path with the real one.
				pathToDelete = boost::str (boost::format ("%s%s")
					% HawkeyeDirectory::Instance().getDriveId()
					% pathToDelete);

				if (!FileSystemUtilities::RemoveAll (pathToDelete))
				{
					DataImporter::Log ("Failed to delete ResultBinaries directory", severity_level::warning);
				}
			}

		} // End "for (const auto& wqi : wqDataRecord.workqueueItems)"

		worklist.sampleSetsDLL.push_back (sampleSetDLL);

		ssCount++;

	} // End "for (const auto& sampleSet : sampleSets)"

	DBApi::DB_WorklistRecord dbWorklist = WorklistDLL::WorklistData::ToDbStyle (worklist, DBApi::eWorklistStatus::WorklistComplete);
	{
		dbWorklist.InstrumentSNStr = HawkeyeConfig::Instance().get().instrumentSerialNumber;
		dbWorklist.RunDateTP = worklist.timestamp;

		DBApi::eQueryResult dbStatus = DBApi::DbAddWorklist (dbWorklist);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Log ("Worklist already exists", severity_level::error);
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("ImportData: <exit, DbAddWorklist failed, status: %ld>") % (int32_t)dbStatus));
			return false;
		}
	}

	return true;
}

//*****************************************************************************
void DataImporter::SaveAnalysisData(
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
	std::function<void(SampleDefinitionDLL, bool)> onComplete)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "DataImporterSaveAnalysisData: <enter>");

	// Check if this data if from a Sample or Reanalysis.
	dbSampleRecord.NumReagents = static_cast<uint16_t>(reagentinfoRecords.size());
	dbSampleRecord.QcId = sampleDef.parameters.qc_uuid;

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

	std::string dateStr = ChronoUtilities::ConvertToString (sampleToWrite.timestamp, "%Y-%m");

	uuid__t imageSetUUID = {};
	if (!HawkeyeResultsDataManager::SaveAnalysisImages (dateStr, worklistUUID, dbSampleRecord, dbAnalysisRecord, sampleToWrite, imageCollection, sresult, imageDataForSResult, imageSetUUID))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "saveAnalysisData: <exit, failed to save the raw images>");
		if (onComplete) {
			onComplete (sampleDef, false);
		}
		return;
	}

	// *** NOTE ****
	// Dust reference data is not available in the v1.2 or v1.3 HawkeyeMetaData.xml file.
	// *** NOTE ***

	{ // Write DB_SummaryResultRecord.
		BasicResultAnswers bra = {};
		ImageAnalysisUtilities::getBasicResultAnswers (sresult.Cumulative_results, bra);

		auto& summaryResultRecord = dbAnalysisRecord.SummaryResult;

		summaryResultRecord.SampleId = dbSampleRecord.SampleId;
		summaryResultRecord.ImageSetId = imageSetUUID;
		summaryResultRecord.AnalysisId = dbAnalysisRecord.AnalysisId;
		summaryResultRecord.AnalysisParamId = {};	//TODO: this is not needed...
		summaryResultRecord.AnalysisDefId = sampleDef.parameters.analysis.uuid;
		summaryResultRecord.ImageAnalysisParamId = {};	//TODO: this is not needed...
		summaryResultRecord.CellTypeId = sampleDef.parameters.celltype.uuid;
		summaryResultRecord.CellTypeIndex = sampleDef.parameters.celltype.celltype_index;
		summaryResultRecord.ProcessingStatus = static_cast<int16_t>(sampleDef.status);		//TODO: is this correct???
		summaryResultRecord.TotalCumulativeImages = static_cast<int16_t>(bra.nTotalCumulative_Imgs);
		summaryResultRecord.TotalCells_GP = bra.count_pop_general;
		summaryResultRecord.TotalCells_POI = bra.count_pop_ofinterest;
		summaryResultRecord.POI_PopPercent = bra.percent_pop_ofinterest;
		summaryResultRecord.CellConc_GP = bra.concentration_general;
		summaryResultRecord.CellConc_POI = bra.concentration_ofinterest;
		summaryResultRecord.AvgDiam_GP = bra.avg_diameter_pop;
		summaryResultRecord.AvgDiam_POI = bra.avg_diameter_ofinterest;
		summaryResultRecord.AvgCircularity_GP = bra.avg_circularity_pop;
		summaryResultRecord.AvgCircularity_POI = bra.avg_circularity_ofinterest;
		summaryResultRecord.CoefficientOfVariance = bra.coefficient_variance;
		summaryResultRecord.AvgCellsPerImage = bra.average_cells_per_image;
		summaryResultRecord.AvgBkgndIntensity = bra.average_brightfield_bg_intensity;
		summaryResultRecord.TotalBubbleCount = bra.bubble_count;
		summaryResultRecord.LargeClusterCount = bra.large_cluster_count;
		summaryResultRecord.QcStatus = static_cast<uint16_t>(qcStatus);
		summaryResultRecord.ResultDateTP = dbAnalysisRecord.AnalysisDateTP;
	}

	dbAnalysisRecord.SampleId = dbSampleRecord.SampleId;
	dbAnalysisRecord.ImageSetId = imageSetUUID;
	dbAnalysisRecord.QcId = dbSampleRecord.QcId;

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveAnalysisData: START SRESULT WRITE...");

	uuid__t sresultId = {};
	if (SResultData::Write (dbSampleRecord.SampleId, dbAnalysisRecord.AnalysisId, sresult, imageDataForSResult, sresultId) != HawkeyeError::eSuccess)
	{ // Error is reported in "SResultData::Write".
		if (onComplete) {
			onComplete (sampleDef, false);
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "saveAnalysisData: END SRESULT WRITE...");

	dbAnalysisRecord.SResultId = sresultId;

	DBApi::eQueryResult dbStatus = DBApi::DbModifyAnalysis (dbAnalysisRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveAnalysisData: <exit, DbModifyAnalysis failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::sample,
			instrument_error::severity_level::error));
		if (onComplete) {
			onComplete (sampleToWrite, false);
		}
		return;
	}

	// Update DB_SampleRecord with the image set record UUID.
	dbSampleRecord.ImageSetId = imageSetUUID;

	dbStatus = DBApi::DbModifySample (dbSampleRecord);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("saveAnalysisData: <exit, DbModifySample failed, status: %ld>") % (int32_t)dbStatus));
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
				boost::str (boost::format ("saveAnalysisData: <exit, DbFindSampleItem failed, status: %ld>") % (int32_t)dbStatus));
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

		dbStatus = DBApi::DbModifySampleItem (dbSampleItemRecord);
		if (dbStatus != DBApi::eQueryResult::QueryOk)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("saveAnalysisData: <exit, DbModifySampleItem failed, status: %ld>") % (int32_t)dbStatus));
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

//*****************************************************************************
bool DataImporter::ReadMetaData (const std::string& resultsDataDir)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "ReadMetaData: <enter>");

	if (isMetadataLoaded_)
	{
		return true;
	}

	try
	{
		std::string metadataFilename = resultsDataDir;
		if (!HDA_EncryptedFileExists (metadataFilename))
		{
			return true;
		}

		if (!HDA_ReadEncryptedPtreeFile (metadataFilename, metadataTree_))
		{
			Logger::L().Log (MODULENAME, severity_level::error, "ReadMetaData: <exit: failed to read the encrypted metadata XML file>");
			clearMetadataPtree();
			return false;
		}

		isMetadataLoaded_ = true;
	}
	catch (boost::property_tree::xml_parser_error er)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ReadMetaData: <exit, exception occurred while reading XML metadata file, exception " + std::string(er.what()) + " >");
		clearMetadataPtree();
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "ReadMetaData: <exit>");
	return true;
}

//*****************************************************************************
void DataImporter::clearMetadataPtree()
{
	metadataTree_.clear();
	isMetadataLoaded_ = false;
}

//*****************************************************************************
// v1.2
//*****************************************************************************
bool DataImporter::RetrieveDataRecordsForUpgrade (std::vector<WorkQueueDataRecord>& workQueueRecords)
{
	try
	{
		workQueueRecords = {};

		//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecordsForUpgrade: <Enter>");

		if (!ReadMetaData (HawkeyeDirectory::Instance().getMetaDataFile()))
		{
			//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecordsForUpgrade: <Exit failed to read the Existing data>");
			return false;
		}

		std::pair<pt_assoc_it, pt_assoc_it> wqr_range = metadataTree_.equal_range (WORKQUEUE_NODE);
		for (auto wqr_it = wqr_range.first; wqr_it != wqr_range.second; wqr_it++)
		{
			WorkQueueDataRecord wqRecord = {};

			if (!PtreeConversionHelper::ConvertToStruct(wqr_it, wqRecord.workqueueRecord))
			{
				return false;
			}

			std::pair<pt_assoc_it, pt_assoc_it> wqir_range = wqr_it->second.equal_range (WORKQUEUE_ITEM_NODE);
			for (auto wqir_it = wqir_range.first; wqir_it != wqir_range.second; wqir_it++)
			{
				WorkQueueItemRecord wqiRecord = {};

				if (!PtreeConversionHelper::ConvertToStruct(wqir_it, wqiRecord.sampleRecord))
				{
					return false;
				}

				std::pair<pt_assoc_it, pt_assoc_it> rs_range = wqir_it->second.equal_range (RESULT_NODE);
				for (auto rs_it = rs_range.first; rs_it != rs_range.second; rs_it++)
				{
					ResultSummaryDLL resultsummaryRecord = {};
					if (!PtreeConversionHelper::ConvertToStruct(rs_it, resultsummaryRecord))
						return false;

					wqiRecord.resultsummaryRecords.push_back (resultsummaryRecord);
					wqiRecord.sampleRecord.result_summaries.push_back(resultsummaryRecord);
				}

				std::pair<pt_assoc_it, pt_assoc_it> imsr_range = wqir_it->second.equal_range (IMAGE_SET_NODE);
				for (auto imsr_it = imsr_range.first; imsr_it != imsr_range.second; imsr_it++)
				{
					ImageDataRecord imageDataRecord = {};
					if (!PtreeConversionHelper::ConvertToStruct(imsr_it, imageDataRecord.imagesetRecord))
					{
						return false;
					}

					// There is only one Brightfield image.
					auto bfi_it = imsr_it->second.find (BF_IMAGE_NODE);
					if (imsr_it->second.not_found() != bfi_it)
					{
						if (!PtreeConversionHelper::ConvertToStruct(bfi_it, imageDataRecord.imageRecord))
							return false;

						wqiRecord.imageDataRecords.push_back (imageDataRecord);
					}

					//NOTE: fluorescence channel is not supported.

					wqiRecord.sampleRecord.imageSequences.push_back (imageDataRecord.imagesetRecord.uuid);
				}

				wqRecord.workqueueItems.push_back (wqiRecord);
			}

			workQueueRecords.push_back (wqRecord);
		}
	}
	catch (const pt::ptree_error& er)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "RetrieveDataRecordsForUpgrade: <exit, exception: " + std::string(er.what()) + ">");
		return false;
	}

	//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecordsForUpgrade: <Exit>");

	return true;
}

//*****************************************************************************
// Read v1.3 for importing to v1.3 offline analysis.
//*****************************************************************************
bool DataImporter::RetrieveDataRecordsForOfflineAnalysis (std::vector<ImportedSampleSet>& sampleSets)
{
	//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecordsForOfflineAnalysis: <enter>");

	try
	{
		sampleSets = {};

		std::pair<pt_assoc_it, pt_assoc_it> sampleSetRange = {};

		boost::property_tree::ptree& metadata = GetMetaData();

		{ // Only one instrument 
			std::pair<pt_assoc_it, pt_assoc_it> instrumentRange = metadata.equal_range(INSTRUMENT_NODE);

			InstrumentRecordDLL instRecord = {};
			if (!PtreeConversionHelper::ConvertToStruct (instrumentRange.first, instRecord))
			{
				return false;
			}

			boost::property_tree::ptree& instrumentNode = metadata.get_child (INSTRUMENT_NODE);
			sampleSetRange = instrumentNode.equal_range(SAMPLESET_NODE);
		}

		for (auto ss_range_it = sampleSetRange.first; ss_range_it != sampleSetRange.second; ss_range_it++)
		{
			ImportedSampleSet sampleSet = {};

			if (!PtreeConversionHelper::ConvertToStruct (ss_range_it, sampleSet.sampleSetDLL))
			{
				return false;
			}

			bool state = ss_range_it->second.get<bool>(BY_COLUMN_NODE);
			if (state)
			{
				sampleSet.precession = ePrecession::eColumnMajor;
			}
			else
			{
				sampleSet.precession = ePrecession::eRowMajor;
			}

			std::pair<pt_assoc_it, pt_assoc_it> sampleRange = ss_range_it->second.equal_range (SAMPLE_NODE);
			for (auto wqir_it = sampleRange.first; wqir_it != sampleRange.second; wqir_it++)
			{
				WorkQueueItemRecord wqiRecord = {};

				if (!PtreeConversionHelper::ConvertToStruct(wqir_it, wqiRecord.sampleRecord))
				{
					return false;
				}

				std::pair<pt_assoc_it, pt_assoc_it> rs_range = wqir_it->second.equal_range (SUMMARY_RESULTS_NODE);
				for (auto rs_it = rs_range.first; rs_it != rs_range.second; rs_it++)
				{
					ResultSummaryDLL resultsummaryRecord = {};
					if (!PtreeConversionHelper::ConvertToStruct(rs_it, resultsummaryRecord))
						return false;

					resultsummaryRecord.username = wqiRecord.sampleRecord.username;
					wqiRecord.resultsummaryRecords.push_back (resultsummaryRecord);
					wqiRecord.sampleRecord.result_summaries.push_back(resultsummaryRecord);
				}

				std::pair<pt_assoc_it, pt_assoc_it> imsr_range = wqir_it->second.equal_range (IMAGE_SET_NODE);
				for (auto imsr_it = imsr_range.first; imsr_it != imsr_range.second; imsr_it++)
				{
					ImageDataRecord imageDataRecord = {};
					if (!PtreeConversionHelper::ConvertToStruct(imsr_it, imageDataRecord.imagesetRecord))
					{
						return false;
					}

					// There is only one Brightfield image.
					auto bfi_it = imsr_it->second.find (BF_IMAGE_NODE);
					if (imsr_it->second.not_found() != bfi_it)
					{
						if (!PtreeConversionHelper::ConvertToStruct(bfi_it, imageDataRecord.imageRecord))
							return false;

						wqiRecord.imageDataRecords.push_back (imageDataRecord);
					}

					//NOTE: fluorescence channel is not supported.
					wqiRecord.sampleRecord.imageSequences.push_back (imageDataRecord.imagesetRecord.uuid);
				}

				sampleSet.workqueueItems.push_back (wqiRecord);

			} // End "for (auto wqir_it = sampleRange.first; wqir_it != sampleRange.second; wqir_it++)"

			// The list of SummaryResults for export from the UI are sent in the order listed in the Settings/Storage screen.
			// The samples are in reverse chronological order while the reanalyszes are in chronological order within each sample.
			// In order to get the offline analysis Home screen display to match that of the instrument the samples must be sorted
			// appropriately.
			if (sampleSet.sampleSetDLL.carrier == eCarrierType::eCarousel)
			{
				std::sort (sampleSet.workqueueItems.begin(), sampleSet.workqueueItems.end(), sortSamplesByCarouselColumn());
			}
			else
			{
				if (sampleSet.precession == ePrecession::eColumnMajor)
				{
					std::sort (sampleSet.workqueueItems.begin(), sampleSet.workqueueItems.end(), sortSamplesByPlateColumn());
				}
				else
				{
					std::sort (sampleSet.workqueueItems.begin(), sampleSet.workqueueItems.end(), sortSamplesByPlateRow());
				}
			}

			sampleSets.push_back (sampleSet);

		} // End "for (auto wqir_it = sampleSetRange.first; wqir_it != sampleSetRange.second; wqir_it++)"

	}
	catch (const pt::ptree_error& er)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "RetrieveDataRecordsForOfflineAnalysis: <exit, exception: " + std::string(er.what()) + ">");
		return false;
	}
	//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecordsForOfflineAnalysis: <Exit>");

	return true;
}
