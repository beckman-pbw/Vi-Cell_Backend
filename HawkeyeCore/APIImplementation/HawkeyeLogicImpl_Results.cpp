#include "stdafx.h"

#include "boost/foreach.hpp"

#include "Auditlog.hpp"
#include "AnalysisDefinitionsDLL.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "ImageProcessingUtilities.hpp"
#include "ImageWrapperDLL.hpp"
#include "Logger.hpp"
#include "QualityControlDLL.hpp"
#include "ResultDefinitionDLL.hpp"
#include "SResultBinStorage.hpp"

static std::string MODULENAME = "Results";

//*****************************************************************************
static void log_exit (const char* function_name, HawkeyeError status, uint32_t retrieved_size = 0)
{
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, \
			boost::str(boost::format("%s: %s - Records: %d <Exit>") % function_name % HawkeyeErrorAsString(status) % retrieved_size));
	}
}

//*****************************************************************************
static std::string getUUIDStrings (const std::vector<uuid__t>& uuids)
{
	std::string output;
	for (size_t index = 0; index < uuids.size(); index++)
	{
		output.append (boost::str (boost::format ("\nUUID[%d]: %s") % index % Uuid::ToStr (uuids[index])));
	}
	return output;
}

//*****************************************************************************
static std::string getUUIDStrings (const uuid__t& uuid)
{
	return getUUIDStrings(std::vector<uuid__t>{uuid});
}

//*****************************************************************************
void HawkeyeLogicImpl::initializeResults()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeResults: <enter>");

	HawkeyeResultsDataManager::Instance().Initialize (pHawkeyeServices_->getMainIos());

	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeResults: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfBlobMeasurement(blob_measurements_t* list, uint32_t n_items)
{
	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		if (list[i].num_measurements > 0)
		{
			delete[] list[i].measurements;
			list[i].measurements = nullptr;
			list[i].num_measurements = 0;
		}
	}

	delete[] list;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfResultRecord(ResultRecord* list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeResultRecord: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		freeResultSummaryInternal(list[i].summary_info);

		if (list[i].num_image_results > 0)
		{
			delete[] list[i].per_image_result;
			list[i].per_image_result = nullptr;
			list[i].num_image_results = 0;
		}
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeResultRecord: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfImageRecord(ImageRecord* list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeImageRecord: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		if (list[i].username)
		{
			delete[] list[i].username;
			list[i].username = nullptr;
		}
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeImageRecord: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfImageSetRecord(SampleImageSetRecord* list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeImageSetRecord: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		if (list[i].username)
		{
			delete[] list[i].username;
			list[i].username = nullptr;
		}

		if (list[i].num_fl_channels > 0)
		{
			delete[]list[i].fl_channel_numbers;
			list[i].fl_channel_numbers = nullptr;

			delete[]list[i].fl_images;
			list[i].fl_images = nullptr;

			list[i].num_fl_channels = 0;
		}
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeImageSetRecord: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::freeListOfReagentInfoRecord(ReagentInfoRecord* list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeReagentInfoRecord: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		if (list[i].pack_number)
		{
			delete[] list[i].pack_number;
			list[i].pack_number = nullptr;
		}

		if (list[i].reagent_label)
		{
			delete[] list[i].reagent_label;
			list[i].reagent_label = nullptr;
		}
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeReagentInfoRecord: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfSampleRecord(SampleRecord* list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleRecord: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		if (list[i].username)
		{
			delete[] list[i].username;
			list[i].username = nullptr;
		}

		if (list[i].sample_identifier)
		{
			delete[] list[i].sample_identifier;
			list[i].sample_identifier = nullptr;
		}

		if (list[i].bp_qc_identifier)
		{
			delete[] list[i].bp_qc_identifier;
			list[i].bp_qc_identifier = nullptr;
		}

		if (list[i].comment)
		{
			delete[] list[i].comment;
			list[i].comment = nullptr;
		}

		if (list[i].num_image_sets > 0)
		{
			delete[] list[i].image_sequences;
			list[i].image_sequences = nullptr;
			list[i].num_image_sets = 0;
		}

		freeListOfReagentInfoRecord (list[i].reagent_info_records, list[i].num_reagent_records);
		FreeListOfResultSummary (list[i].result_summaries, list[i].num_result_summary);
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeSampleRecord: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfWorklistRecord (WorklistRecord* list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug3, "FreeListOfWorklistRecord: <enter>");

	if (list == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "FreeListOfWorklistRecord: <exit, null ptr>");
		return;
	}

	for (uint32_t i = 0; i < n_items; i++)
	{
		if (list[i].username)
		{
			delete[] list[i].username;
			list[i].username = nullptr;
		}

		if (list[i].label)
		{
			delete[] list[i].label;
			list[i].label = nullptr;
		}

		if (list[i].num_sample_records > 0)
		{
			delete[] list[i].sample_records;
			list[i].sample_records = nullptr;
			list[i].num_sample_records = 0;
		}
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug3, "FreeListOfWorklistRecord: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeDetailedResultMeasurement (DetailedResultMeasurements* meas)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDetailedResultMeasurement: <enter>");

	if (meas == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDetailedResultMeasurement: <exit, null ptr>");
		return;
	}

	for (auto i = 0; i < meas->num_image_sets; i++)
	{
		FreeListOfBlobMeasurement(meas->blobs_by_image[i].blob_list, meas->blobs_by_image[i].blob_count);

		if (meas->large_clusters_by_image[i].cluster_count > 0)
		{
			delete[] meas->large_clusters_by_image[i].cluster_list;
			meas->large_clusters_by_image[i].cluster_list = nullptr;
			meas->large_clusters_by_image[i].cluster_count = 0;
		}
	}

	if (meas->num_image_sets > 0)
	{
		delete[] meas->blobs_by_image;
		meas->blobs_by_image = nullptr;

		delete[] meas->large_clusters_by_image;
		meas->large_clusters_by_image = nullptr;

		meas->num_image_sets = 0;
	}

	delete[] meas;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDetailedResultMeasurement: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfResultSummary(ResultSummary * list, uint32_t n_items)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeListOfResultSummary: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < n_items; i++)
	{
		freeResultSummaryInternal(list[i]);
	}

	delete[] list;

	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "FreeListOfResultSummary: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleRecord (uuid__t uuid, SampleRecord*& rec, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecord: <enter>");

	rec = nullptr;

	if (!InitializationComplete())
	{
		pHawkeyeServices_->enqueueExternal(cb, HawkeyeError::eNotPermittedAtThisTime);
		return;
	}

	//	//NOTE: The following three lines have been removed to support a special use case in the UI.
	//	// In UI, when the user inactivity has timed out the user must log back in.  In the event of
	//	// more than five login failures the user's account is disabled.  When exporting of the results
	//	// is enabled the UI code that exports the results calls this API (RetrieveSampleRecord).  These
	//	// lines of code resulted in an error being returned instead of the requested data which caused
	//	// the Ui to through an exception since this API returned no data.
	//	//AuditLogger::L().Log (generateAuditWriteData (UserList::Instance().GetLoggedInUsername(), audit_event_type::evt_notAuthorized, "Retrieve Sample Record"));
	//	//pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eNotPermittedByUser);
	//	//return;
	//	// NOTE: Now this code fails through when a user is logged in and their account is disabled.
	//	// This case can only occur as a result of too many login failures after an inactivity timeout.
	//}

//TODO: needs to be Removed in production Software.
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecord: " + getUUIDStrings(uuid));

	auto retrieve_cb = [this, cb, &rec](SampleRecordDLL record, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType (record, rec);
			numItems = 1;
		}

		log_exit ("RetrieveSampleRecord", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleRecord (uuid, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleRecords (uint64_t start, uint64_t end, char* username, SampleRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecords: <enter>");

	list_size = 0;
	reclist = nullptr;


	std::string sUsername;
	DataConversion::convertToStandardString(sUsername, username);

	auto retrieve_cb = [this, cb, &reclist, &list_size](std::vector<SampleRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, reclist, list_size);
		}

		log_exit("RetrieveSampleRecords", status, list_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleRecords (start, end, sUsername, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleRecordList (uuid__t* uuids, uint32_t list_size, SampleRecord*& recs, uint32_t& retrieved_size, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordList: <enter>");

	recs = nullptr;
	retrieved_size = 0;

	auto uuid_list = DataConversion::create_vector_from_Clist (uuids, list_size);

	// leave for debug 
	// Print the UUID-string of the given UUID's
	//if (Logger::L().IsOfInterest(severity_level::debug1))
	//{
	//	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordList: " + getUUIDStrings (uuid_list));
	//}

	auto retrieve_cb = [this, cb, &recs, &retrieved_size](std::vector<SampleRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			if (!records.size())
			{
				pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eEntryNotFound);
				return;
			}

			DataConversion::convert_vecOfDllType_to_listOfCType(records, recs, retrieved_size);
		}

		log_exit("RetrieveSampleRecordList", status, retrieved_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleRecordList (uuid_list, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleImageSetRecord (uuid__t uuid, SampleImageSetRecord*& rec, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecord: <enter>");

	rec = nullptr;


	auto retrieve_cb = [this, cb, &rec](SampleImageSetRecordDLL record, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType(record, rec);
			numItems = 1;
		}

		log_exit ("RetrieveSampleImageSetRecord", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleImageSetRecord (uuid, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleImageSetRecords (
	uint64_t start, 
	uint64_t end, 
	char* username, 
	SampleImageSetRecord*& reclist,
	uint32_t& list_size, 
	HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecords: <enter>");

	list_size = 0;
	reclist = nullptr;


	std::string sUsername;
	DataConversion::convertToStandardString (sUsername, username);

	auto retrieve_cb = [this, cb, &reclist, &list_size](std::vector<SampleImageSetRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, reclist, list_size);
		}
		log_exit ("RetrieveSampleImageSetRecords", status, list_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleImageSetRecords (start, end, sUsername, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleImageSetRecordList (
	uuid__t* uuids, 
	uint32_t list_size, 
	SampleImageSetRecord*& recs, 
	uint32_t& retrieved_size, 
	HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecordList: <enter>");

	recs = nullptr;
	retrieved_size = 0;


	// Print the UUID-string of the given UUID's
	auto uuid_list = DataConversion::create_vector_from_Clist (uuids, list_size);
	//
	// leave for debug
	//
	//if (Logger::L().IsOfInterest(severity_level::debug1))
	//{
	//	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleImageSetRecordList: " + getUUIDStrings(uuid_list));
	//}

	auto retrieve_cb = [this, cb, &recs, &retrieved_size](std::vector<SampleImageSetRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, recs, retrieved_size);
		}

		log_exit ("RetrieveSampleImageSetRecordList", status, retrieved_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleImageSetRecordList (uuid_list, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveResultRecord (uuid__t uuid, ResultRecord*& rec, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecord: <enter, UUID " + Uuid::ToStr (uuid) + ">");

	rec = nullptr;


	auto retrieve_cb = [this, cb, &rec](ResultRecordDLL record, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType (record, rec);
			numItems = 1;
		}

		log_exit ("RetrieveResultRecord", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveResultRecord (uuid, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveResultRecords (uint64_t start, uint64_t end, char* username, ResultRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecords: <enter>");

	list_size = 0;
	reclist = nullptr;

	std::string sUsername;
	DataConversion::convertToStandardString(sUsername, username);
	bool discardUserId = sUsername.empty();
	bool discardEndTime = end == 0;

	auto retrieve_cb = [this, cb, &reclist, &list_size](std::vector<ResultRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, reclist, list_size);
		}

		log_exit("RetrieveResultRecords", status, list_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveResultRecords (start, end, sUsername, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveResultRecordList (uuid__t* uuids, uint32_t list_size, ResultRecord*& recs, uint32_t& retrieved_size, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultRecordList: <enter>");

	recs = nullptr;
	retrieved_size = 0;

	auto uuid_list = DataConversion::create_vector_from_Clist (uuids, list_size);

	auto retrieve_cb = [this, cb, &recs, &retrieved_size](std::vector<ResultRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, recs, retrieved_size);
		}

		log_exit("RetrieveResultRecordList", status, retrieved_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveResultRecordList (uuid_list, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveResultSummaryRecord (uuid__t uuid, ResultSummary*& rec, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultSummaryRecord: <enter, UUID " + Uuid::ToStr (uuid) + ">");

	rec = nullptr;

	auto retrieve_cb = [this, cb, &rec](ResultSummaryDLL record, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType (record, rec);
			numItems = 1;
		}

		log_exit ("RetrieveResultSummaryRecord", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveResultSummaryRecord (uuid, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveResultSummaryRecordList (
	uuid__t* uuids, 
	uint32_t list_size, 
	ResultSummary*& recs, 
	uint32_t& retrieved_size, 
	HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveResultSummaryRecordList: <enter>");

	recs = nullptr;
	retrieved_size = 0;


	auto uuid_list = DataConversion::create_vector_from_Clist (uuids, list_size);

	auto retrieve_cb = [this, cb, &recs, &retrieved_size](std::vector<ResultSummaryDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, recs, retrieved_size);
			Logger::L().Log (MODULENAME, severity_level::debug1,
				boost::str (boost::format ("RetrieveResultSummaryRecordList: <recs.timestamp: %s>") % recs[0].timestamp));

			if (recs[0].num_signatures > 0)
			{
				Logger::L().Log (MODULENAME, severity_level::debug1,
					boost::str (boost::format ("RetrieveResultSummaryRecordList: <recs[0].signature_set[0].timestamp: %s>") % recs[0].signature_set[0].timestamp));
			}
		}

		log_exit ("RetrieveResultSummaryRecordList", status, retrieved_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveResultSummaryRecordList (uuid_list, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveDetailedMeasurementsForResultRecord (uuid__t uuid, DetailedResultMeasurements*& measurements, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDetailedMeasurementsForResultRecord: <enter>");

	measurements = nullptr;

	auto retrieve_cb = [this, cb, &measurements](DetailedResultMeasurementsDLL drm, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status==HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType(drm, measurements);
			numItems = 1;
		}
		log_exit ("RetrieveDetailedMeasurementsForResultRecord", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveDetailedMeasurement (uuid, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveImage (uuid__t uuid, ImageWrapper_t*& img, HawkeyeErrorCallback cb)
{
	// leave for debug Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveImage: <enter>");
	img = nullptr;
	auto retrieve_cb = [this, cb, &img](cv::Mat matImage, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType(ImageWrapperDLL{ matImage }, img);
			numItems = 1;
		}
		log_exit ("RetrieveImage", status, numItems); 
		pHawkeyeServices_->enqueueExternal (cb, status);
	};
	HawkeyeResultsDataManager::Instance().RetrieveImage (uuid, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveAnnotatedImage (uuid__t result_id, uuid__t image_id, ImageWrapper_t*& img, HawkeyeErrorCallback cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveAnnotatedImage: <enter>");

	img = nullptr;

	auto retrieve_cb = [this, cb, &img](cv::Mat matImage, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType(ImageWrapperDLL{ matImage }, img);
			numItems = 1;
		}

		log_exit ("RetrieveAnnotatedImage", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveAnnotatedImage (result_id, image_id, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveBWImage (uuid__t image_id, ImageWrapper_t*& img, HawkeyeErrorCallback cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveBWImage: <enter>");

	img = nullptr;


	auto retrieve_cb = [this, cb, &img](cv::Mat matImage, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_dllType_to_cType (ImageWrapperDLL{ matImage }, img);
			numItems = 1;
		}
		log_exit ("RetrieveBWImage", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveBWImage (image_id, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveHistogramForResultRecord (uuid__t uuid, bool only_POI, Hawkeye::Characteristic_t measurement, uint8_t& bin_count, histogrambin_t*& bins, HawkeyeErrorCallback cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveHistogramForResultRecord: <enter>");

	if (bin_count == 0)
	{
		pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eInvalidArgs);
		return;
	}

	bins = nullptr;


	auto retrieve_cb = [this, cb, &bin_count, &bins](std::vector<histogrambin_t> histogram, HawkeyeError status) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			bin_count = static_cast<uint8_t>(histogram.size());
			bins = new histogrambin_t[bin_count];
			std::copy(histogram.begin(), histogram.end(), bins);
			numItems = 1;
		}
		log_exit ("RetrieveHistogramForResultRecord", status, numItems);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveHistogram (uuid, only_POI, measurement, bin_count, retrieve_cb);
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::FreeHistogramBins (histogrambin_t* list)
{
	if (!list)
		return HawkeyeError::eInvalidArgs;

	delete[] list;
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::RetrieveSampleRecordsForBPandQC (const char* bp_qc_name, SampleRecord*& reclist, uint32_t& list_size, HawkeyeErrorCallback cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSampleRecordsForBPandQC: <enter>");
	
	list_size = 0;
	reclist = nullptr;

	if (bp_qc_name == nullptr)
	{
		pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eInvalidArgs);
		return;
	}


	QualityControlDLL qc = {};
	if (!getQualityControlByName (std::string(bp_qc_name), qc))
	{
		pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eNoneFound);
	}

	auto retrieve_cb = [this, cb, &reclist, &list_size](std::vector<SampleRecordDLL> records, HawkeyeError status) -> void
	{
		if (status == HawkeyeError::eSuccess)
		{
			DataConversion::convert_vecOfDllType_to_listOfCType(records, reclist, list_size);
		}

		log_exit ("RetrieveSampleRecordsForBPandQC", status, list_size);
		pHawkeyeServices_->enqueueExternal (cb, status);
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleRecordsByQCUuid (qc.uuid, retrieve_cb);
}

//*****************************************************************************
std::vector<DataSignatureDLL>& GetDataSignatureDeinitions();
HawkeyeError HawkeyeLogicImpl::RetrieveSignatureDefinitions (DataSignature_t*& signatures, uint16_t& num_signatures)
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "RetrieveSignatureDefinitions: <enter>");
	
	num_signatures = 0;
	signatures = nullptr;

	auto status = UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eNormal);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) // No logged in user
			return status;

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetAttributableUserName(),
//TODO: 			UserList::Instance().GetLoggedInUsername(),			
			audit_event_type::evt_notAuthorized, 
			"Retrieve Signature Definitions"));
		return HawkeyeError::eNotPermittedByUser;
	}

	auto ds = GetDataSignatureDeinitions();

	DataConversion::convert_vecOfDllType_to_listOfCType (ds, signatures, num_signatures);
	if (0 == num_signatures)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveSignatureDefinitions <exit, none found>");
		return HawkeyeError::eNoneFound;
	}

	log_exit ("RetrieveSignatureDefinitions", HawkeyeError::eSuccess, num_signatures);
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::SignResultRecord (uuid__t record_id, char* signature_short_text, uint16_t short_text_len, HawkeyeErrorCallback cb)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "SignResultRecord: <enter>");

	// Restricted to Service Engineer.
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eNormal))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetAttributableUserName(),
//TODO: 			UserList::Instance().GetLoggedInUsername(),			
			audit_event_type::evt_notAuthorized, 
			"Sign Result Record"));
		pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eNotPermittedByUser);
		return;
	}

	// Print the UUID-string of the given UUID
	Logger::L().Log (MODULENAME, severity_level::debug1, "SignResultRecord: " + getUUIDStrings(record_id));

	const auto& ds = GetDataSignatureDeinitions();

	auto item = std::find_if(
		ds.begin(), ds.end(), [&signature_short_text](const auto& item) 
			{ return item.short_text == signature_short_text; });

	if (item == ds.end())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "SignResultRecord: <exit, invalid args>");
		pHawkeyeServices_->enqueueExternal (cb, HawkeyeError::eInvalidArgs);
		return;
	}

	DataSignatureInstanceDLL sig = {};
	sig.signature = *item;
	sig.signing_user = UserList::Instance().GetConsoleUsername();
	sig.timestamp = ChronoUtilities::CurrentTime();
	std::string signature(signature_short_text);

	HawkeyeResultsDataManager::Instance().SignResult (record_id, sig, 
		[this, sig, signature, record_id, cb](HawkeyeError status, std::string audit_log_str) -> void
	{
		uint32_t numItems = 0;
		if (status == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				sig.signing_user,
				audit_event_type::evt_datasignatureapplied,
				audit_log_str));
			numItems = 1;
		}

		log_exit ("SignResultRecord", status, numItems);

		pHawkeyeServices_->enqueueExternal (cb, status);
	});
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeImageWrapper (ImageWrapper_t*& image, uint16_t num_image_wrapper)
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeImageWrapper <enter>");

	if (image == nullptr)
	{
		return;
	}

	for (uint16_t index = 0; index < num_image_wrapper; index++)
	{
		freeImageWrapperInternal(image[index]);
	}

	delete[] image;
	image = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeImageWrapper <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeImageSetWrapper (ImageSetWrapper_t*& image_set_wrapper, uint16_t num_image_set_wrapper)
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeImageSetWrapper: <enter>");

	if (image_set_wrapper == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "FreeImageSetWrapper: <exit, null ptr>");
		return;
	}

	for (uint16_t i_img_set = 0; i_img_set < num_image_set_wrapper; i_img_set++)
	{
		freeImageWrapperInternal(image_set_wrapper[i_img_set].brightfield_image);

		if (image_set_wrapper[i_img_set].num_fl_channels > 0)
		{
			for (uint8_t i_fl_chans = 0; i_fl_chans < image_set_wrapper[i_img_set].num_fl_channels; i_fl_chans++)
			{
				freeImageWrapperInternal(image_set_wrapper[i_img_set].fl_images[i_fl_chans].fl_image);
			}

			delete[] image_set_wrapper[i_img_set].fl_images;
			image_set_wrapper[i_img_set].fl_images = nullptr;
		}
	}

	delete[] image_set_wrapper;
	image_set_wrapper = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeImageSetWrapper: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeAutofocusResults (AutofocusResults*& results, uint8_t num_result)
{
	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeAutofocusResults: <enter>");

	if (results == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug3, "FreeAutofocusResults: <exit, null ptr>");
		return;
	}

	for (uint8_t index = 0; index < num_result; index++)
	{
		if (results[index].nFocusDatapoints > 0)
		{
			delete[] results[index].dataset;
			results[index].dataset = nullptr;
			results[index].nFocusDatapoints = 0;
		}

		freeImageWrapperInternal(results[index].bestfocus_af_image);
	}

	delete[] results;
	results = nullptr;
	
	Logger::L().Log (MODULENAME, severity_level::debug3, "FreeAutofocusResults: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::DeleteSampleRecord (
	const char* username, 
	const char* password,
	uuid__t* sampleRecordUuidlist, 
	uint32_t num_uuid, 
	bool retain_results_and_first_image, 
	delete_results_callback_DLL onDeleteCompletion,
	std::function<void(HawkeyeError)> onComplete)
{
	auto uName = std::string(username);
	Logger::L().Log (MODULENAME, severity_level::debug1, "DeleteSampleRecord: <enter> " + uName);

	// This must be done by Administrator.
	if (!UserList::Instance().IsUserPermissionAtLeast(uName, UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			uName,
			audit_event_type::evt_notAuthorized, 
			"Delete Sample Record"));
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eNotPermittedByUser);
		return;
	}

	auto sampleItemUuids = DataConversion::create_vector_from_Clist (sampleRecordUuidlist, num_uuid);
	if (sampleItemUuids.empty())
	{
		pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eInvalidArgs);
		return;
	}

	if (Logger::L().IsOfInterest(severity_level::debug1))
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "DeleteSampleRecord: " + getUUIDStrings(sampleItemUuids));
	}

	auto deleteWrapper = [this, uName, onDeleteCompletion](HawkeyeError he, uuid__t uuid, std::string audit_log_str)->void
	{
		if (he == HawkeyeError::eSuccess)
		{
			AuditLogger::L().Log (generateAuditWriteData(
				uName,
				audit_event_type::evt_deletesamplerecord, 
				audit_log_str));
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, "DeleteSampleRecord: <exit>");

		onDeleteCompletion (he, uuid);
	};

	HawkeyeResultsDataManager::Instance().DeleteSampleRecord (sampleItemUuids, retain_results_and_first_image, deleteWrapper);

	pHawkeyeServices_->enqueueExternal (onComplete, HawkeyeError::eSuccess);
}



//*****************************************************************************
void HawkeyeLogicImpl::freeResultSummaryInternal (ResultSummary& rs)
{
	if (rs.username)
	{
		delete[] rs.username;
		rs.username = nullptr;
	}

	CellTypeDLL::freeCelltypeInternal (rs.cell_type_settings);
	AnalysisDefinitionDLL::freeAnalysisDefinitionInternal (rs.analysis_settings);
	FreeDataSignatureInstance (rs.signature_set, rs.num_signatures);
}

//*****************************************************************************
void HawkeyeLogicImpl::freeImageWrapperInternal (ImageWrapper_t& iw)
{
	if (iw.data)
	{
		delete[] iw.data;
		iw.data = nullptr;
	}
}

//*****************************************************************************
// *** WARNING *** *** WARNING *** *** WARNING *** *** WARNING ***
// This code no longer called  by the UI in ViCellBLU and CellHealth.

//TODO: Is this true ???
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ExportInstrumentData (const char* username, const char* password,
	                                                uuid__t * rs_uuid_list,
													uint32_t num_uuid,
													const  char* export_location,
													eExportImages exportImages,
													uint16_t export_nth_image,
													export_data_completion_callback_DLL onExportCompletionCb,
													export_data_progress_callback_DLL exportdataProgressCb)
{
	auto uName = std::string(username);
	Logger::L().Log (MODULENAME, severity_level::debug1, "ExportInstrumentData: <enter>  " + uName);

	if (!UserList::Instance().IsUserPermissionAtLeast(username, UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized, 
			"Export Instrument Data"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (isShutdownInProgress_)
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (export_location == nullptr ||
		num_uuid == 0 ||
		rs_uuid_list == nullptr ||
		onExportCompletionCb == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "ExportInstrumentData: <exit, Invalid input>");
		return HawkeyeError::eInvalidArgs;
	}

	auto export_progress_cb = [this, exportdataProgressCb](HawkeyeError status, uuid__t uuid) -> void
	{
		if (exportdataProgressCb)
			pHawkeyeServices_->enqueueExternal (std::bind(exportdataProgressCb, status, uuid));
	};

	auto export_completion_cb = [this, uName, onExportCompletionCb](HawkeyeError status, std::string archived_filepath_str, std::vector<std::string> audit_log_str_list) -> void
	{
		char *archived_filepath = nullptr;
		if (!archived_filepath_str.empty() && status == HawkeyeError::eSuccess)
		{
			DataConversion::convertToCharPointer(archived_filepath, archived_filepath_str);

			uint16_t index = 1;
			std::string cumulative_audit_log_str = "Exported the \n";
			for (const auto& audit_log_str : audit_log_str_list)
			{
				cumulative_audit_log_str += std::to_string(index++) + ". " + audit_log_str + ".\n";
			}

			// Generate the audit log only on success full data export completion.
			AuditLogger::L().Log (generateAuditWriteData(
				uName, 
				audit_event_type::evt_Instrumentdataexported, 
				cumulative_audit_log_str));

			Logger::L().Log (MODULENAME, severity_level::debug1, "ExportInstrumentData: <exit>");
		}

		pHawkeyeServices_->enqueueExternal (std::bind(onExportCompletionCb, status, archived_filepath));
	};

	auto uuid_list = DataConversion::create_vector_from_Clist(rs_uuid_list, num_uuid);
	auto status = HawkeyeResultsDataManager::Instance().ExportForOfflineAnalysis (
		uuid_list, 
		uName, 
		export_location, 
		exportImages, 
		export_nth_image, 
		export_progress_cb, 
		export_completion_cb);

	return status;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::CancelExportData()
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "CancelExportData: <enter>");
	auto status = HawkeyeResultsDataManager::Instance().CancelExportData();
	Logger::L().Log(MODULENAME, severity_level::debug1, "CancelExportData: <exit>");
	return status;
}

static std::string s_exportUser = ""; // used for the audit log when done
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::Export_Start(
	const char* username,
	const char* password,
	uuid__t* rs_uuid_list,
	uint32_t num_uuid,
	const  char* outPath,
	eExportImages exportImages,
	uint16_t export_nth_image)
{
	// leave for debug Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Start: <enter>");
	if (isShutdownInProgress_)
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (outPath == nullptr ||
		num_uuid == 0 ||
		rs_uuid_list == nullptr)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "Export_Start: <exit, Invalid input>");
		return HawkeyeError::eInvalidArgs;
	}

	s_exportUser = username;
	auto uuid_list = DataConversion::create_vector_from_Clist(rs_uuid_list, num_uuid);
	auto status = HawkeyeResultsDataManager::Instance().Export_Start(
		uuid_list,
		username,
		outPath,
		exportImages,
		export_nth_image);

	return status;
}


HawkeyeError HawkeyeLogicImpl::Export_NextMetaData(uint32_t index, uint32_t delayms)
{
	auto status = HawkeyeResultsDataManager::Instance().Export_NextMetaData(index, delayms);
	return status;
}

HawkeyeError HawkeyeLogicImpl::Export_IsStorageAvailable()
{
	auto status = HawkeyeResultsDataManager::Instance().Export_IsStorageAvailable();
	return status;
}



HawkeyeError HawkeyeLogicImpl::Export_ArchiveData(const char* filename, char*& outname)
{
	auto status = HawkeyeResultsDataManager::Instance().Export_ArchiveData(filename, outname);
	if (status != HawkeyeError::eSuccess)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Export_ArchiveData : <exit - failed>");
		return status;
	}

	uint16_t index = 1;
	std::string cumulative_audit_log_str = "Exported:\n";
	
	auto lst = HawkeyeResultsDataManager::Instance().GetAuditEntries();
	for (const auto& audit_log_str : lst)
	{
		cumulative_audit_log_str += std::to_string(index++) + ". " + audit_log_str + ".\n";
	}

	// Generate the audit log only on success full data export completion.
	AuditLogger::L().Log(generateAuditWriteData(
		s_exportUser,
		audit_event_type::evt_Instrumentdataexported,
		cumulative_audit_log_str));

	return status;
}

HawkeyeError HawkeyeLogicImpl::Export_Cleanup(bool removeFile)
{
	auto status = HawkeyeResultsDataManager::Instance().Export_Cleanup(removeFile);
	return status;
}