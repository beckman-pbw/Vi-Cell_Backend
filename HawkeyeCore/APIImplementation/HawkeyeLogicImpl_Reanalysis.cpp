#include "stdafx.h"

#include "boost/foreach.hpp"

#include "Auditlog.hpp"
#include "AnalysisDefinitionsDLL.hpp"
#include "DataConversion.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "HawkeyeUUID.hpp"
#include "ImageProcessingUtilities.hpp"
#include "ImageWrapperDLL.hpp"
#include "Logger.hpp"
#include "QualityControlsDLL.hpp"
#include "ResultDefinitionDLL.hpp"

static const char MODULENAME[] = "Reanalysis";

static std::shared_ptr<CellCounterResult::SResult> reanalysisResult_;

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ReanalyzeSample(
	uuid__t sample_id,
	uint32_t celltype_index,
	uint16_t analysis_index,
	bool from_images,
	sample_analysis_callback_DLL onReanalysisComplete)
{
	HAWKEYE_ASSERT (MODULENAME, onReanalysisComplete);

	Logger::L().Log (MODULENAME, severity_level::normal, 
		boost::str (boost::format ("ReanalyzeSample <enter>\n\tSample UUID: %s\n\tCellType: %d\n\tAnalysis: %d\n\tFromImages: %s")
		% Uuid::ToStr (sample_id) 
		% celltype_index 
		% analysis_index
		% (from_images ? "true" : "false")));

	SampleDefinition* sampleDef;
	const HawkeyeError he = HawkeyeLogicImpl::GetSampleDefinitionBySampleId (sample_id, sampleDef);
	if (he != HawkeyeError::eSuccess)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ReanalyzeSample: <exit, sample not found>");
		return he;
	}

	const SampleDefinitionDLL sdDLL(*sampleDef);
	
	ReanlyzeSample_DataParams data_params = {};
	data_params.completion_status = HawkeyeError::eSuccess;
	data_params.loggedInUsername = UserList::Instance().GetLoggedInUsername();
	data_params.sampleDef = sdDLL;
	data_params.cellType_index = celltype_index;
	data_params.analysis_index = analysis_index;
	data_params.from_images = from_images;
	data_params.cb = onReanalysisComplete;

	pHawkeyeServices_->enqueueInternal([this, data_params]()
		{
			reanlyzeSampleInternal (data_params, ReanalyzeSampleStates::rs_GetExistingRecords);
		});

	Logger::L().Log (MODULENAME, severity_level::debug1, "ReanalyzeSample: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
void HawkeyeLogicImpl::reanlyzeSampleInternal (ReanlyzeSample_DataParams data_params, ReanalyzeSampleStates state)
{
	auto onCompleteCB = [this](ReanlyzeSample_DataParams data_params, bool status, ReanalyzeSampleStates nextState)
	{
		if (!status)
		{
			nextState = ReanalyzeSampleStates::rs_Error;
		}

		pHawkeyeServices_->enqueueInternal([=]()
			{
				reanlyzeSampleInternal (data_params, nextState);
			});
	};

	switch (state)
	{
		case ReanalyzeSampleStates::rs_GetExistingRecords:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_GetExistingRecords");
			auto wrapper = std::bind (onCompleteCB, std::placeholders::_1, std::placeholders::_2, ReanalyzeSampleStates::rs_GetCellTypeAndAnalysis);
			pHawkeyeServices_->enqueueInternal ([=]() 
				{ reanlyzeSample_GetRecord (data_params, wrapper); }
			);
			return;
		}

		case ReanalyzeSampleStates::rs_GetCellTypeAndAnalysis:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_GetCellTypeAndAnalysis");
			auto wrapper = std::bind (onCompleteCB, std::placeholders::_1, std::placeholders::_2, ReanalyzeSampleStates::rs_LoadImages);
			pHawkeyeServices_->enqueueInternal ([=]() 
				{ reanlyzeSample_GetCellTypeAnalysis (data_params, wrapper); }
			);
			return;
		}

		case ReanalyzeSampleStates::rs_LoadImages:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_LoadImages");
			if (data_params.from_images)
			{
				auto wrapper = std::bind (onCompleteCB, std::placeholders::_1, std::placeholders::_2, ReanalyzeSampleStates::rs_LoadNormalizationImages);
				pHawkeyeServices_->enqueueInternal ([=]()
					{ reanlyzeSample_LoadImages (data_params, wrapper); }
				);
			}
			else
			{
				pHawkeyeServices_->enqueueInternal ([=]() 
					{ onCompleteCB (data_params, true, ReanalyzeSampleStates::rs_DoReanalysis); }
				);
			}
			return;
		}

		case ReanalyzeSampleStates::rs_LoadNormalizationImages:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_LoadNormalizationImages");
			auto wrapper = std::bind (onCompleteCB, std::placeholders::_1, std::placeholders::_2, ReanalyzeSampleStates::rs_DoReanalysis);
			pHawkeyeServices_->enqueueInternal ([=]()
				{ reanlyzeSample_LoadNormalizationImages (data_params, wrapper); }
			);
			return;
		}

		case ReanalyzeSampleStates::rs_DoReanalysis:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_DoReanalysis");
			auto wrapper = std::bind (onCompleteCB, std::placeholders::_1, std::placeholders::_2, ReanalyzeSampleStates::rs_GenerateResultRecord);
			pHawkeyeServices_->enqueueInternal([=]()
				{ reanalyzeSample_DoReanalysis (data_params, wrapper); }
			);
			return;
		}

		case ReanalyzeSampleStates::rs_GenerateResultRecord:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_GenerateResultRecord");
			auto wrapper = std::bind (onCompleteCB, std::placeholders::_1, std::placeholders::_2, ReanalyzeSampleStates::rs_Complete);
			pHawkeyeServices_->enqueueInternal ([=]()
				{ reanalyzeSample_GenerateResultRecord (data_params, wrapper); }
			);
			return;
		}

		case ReanalyzeSampleStates::rs_Complete:
		{
			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSampleInternal: rs_Complete, Successfully performed reanalysis");
			// Report success and data to UI...
			pHawkeyeServices_->enqueueExternal (data_params.cb, data_params.completion_status, data_params.sampleDef.sampleDataUuid, data_params.resultRecord);
			return;
		}

		case ReanalyzeSampleStates::rs_Error:
		{
			Logger::L().Log (MODULENAME, severity_level::error, "reanlyzeSampleInternal: rs_Error, Failed to perform reanalysis");
			// Report error to UI...
			pHawkeyeServices_->enqueueExternal (data_params.cb, data_params.completion_status, data_params.sampleDef.sampleDataUuid, ResultRecordDLL{});
//TODO: should the Vi-Cell be used ???
			return;
		}
	}

	//unreachable code
	HAWKEYE_ASSERT (MODULENAME, false);
}


//*****************************************************************************
void HawkeyeLogicImpl::reanlyzeSample_GetRecord (ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback)
{
	auto retrieve_cb = [this, callback, &data_params](DBApi::DB_SampleRecord dbSampleRecord, HawkeyeError he) -> void
	{
		bool retStatus = false;
		if (he == HawkeyeError::eSuccess)
		{
			data_params.dbSampleRecord = dbSampleRecord;
			retStatus = true;
		}

		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), retStatus);
	};

	HawkeyeResultsDataManager::Instance().RetrieveDbSampleRecord (data_params.sampleDef.sampleDataUuid, retrieve_cb);
}

//*****************************************************************************
HawkeyeError GetAnalysisForCellTypeDLL (uint16_t ad_index, uint32_t ct_index, AnalysisDefinitionDLL& ad);
void HawkeyeLogicImpl::reanlyzeSample_GetCellTypeAnalysis (ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback)
{
	if (!CellTypesDLL::isCellTypeIndexValid (data_params.cellType_index) || !HawkeyeLogicImpl::GetCellTypeByIndex (data_params.cellType_index, data_params.celltype))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ReanalyzeSample: <exit, CellType not found>");
		data_params.completion_status = HawkeyeError::eInvalidArgs;
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
		return;
	}

	if (!AnalysisDefinitionsDLL::isAnalysisIndexValid (data_params.analysis.analysis_index) ||
		!(GetAnalysisForCellTypeDLL (data_params.analysis_index, data_params.cellType_index, data_params.analysis) == HawkeyeError::eSuccess))
	{
		Logger::L().Log (MODULENAME, severity_level::error, "ReanalyzeSample: <exit, Analysis not found>");
		data_params.completion_status = HawkeyeError::eInvalidArgs;
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
		return;
	}

	pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), true);
}

//*****************************************************************************
void HawkeyeLogicImpl::reanlyzeSample_LoadImages (ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback)
{
//TODO: test using 100 images where saveNthImage is 10.

	// retrieve images
	// Fun part: we need to preserve ORDER and SEQUENCE NUMBER for the image sets that get fed into the algorithm
	//  so that the reanalyzed result synchs with the original images.
	/*
		At each image set...
		Is the sequence number higher than the existing size?  (Sequence numbers start at 1!!!)
		No: insert set at that index
		Yes: Add black image sets to get up to size, then insert set at correct index
		It's expected that the algorithm should "ignore" black images (too dark) and exclude them from processing
	*/
	auto addblankImages = [this, callback](ReanlyzeSample_DataParams data_params)
	{
		//There are two ways to insert the blank images
		//1. based on the image sequence number in the collection
		//2. Based on the number of images processed initially 
		// Here 2 way is considered
		ImageSet_t blank_imset = {};
		blank_imset.first = cv::Mat(2048, 2048, CV_8UC1, Scalar(0, 0, 0));

		// Get the expected number of images from the collection.
		// This relies on the idea that we'll always save the LAST image in the set.
		uint16_t num_images = 0;
		for (const auto& im : *data_params.tmp_image_collection)
		{
			if (im.first > num_images)
			{
				num_images = im.first;
			}
		}

		for (uint16_t seq = 1; seq <= num_images; seq++)
		{
			// Emplace will succeed ONLY if the key does not already exist in the map
			// This does NOT overwrite the existing data, only "fills in the blanks"
			data_params.tmp_image_collection->emplace (seq, blank_imset);
		}

		data_params.image_collection = std::make_shared<ImageCollection_t>();
		data_params.image_collection->reserve (num_images);
		for (auto image_set : *data_params.tmp_image_collection)
		{
			data_params.image_collection->push_back (std::move(image_set.second));
		}

		data_params.tmp_image_collection.reset();

		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), true);
	};


	data_params.tmp_image_collection = std::make_shared<ImageSetCollection_t>();

	DBApi::DB_ImageSetRecord imageSetRecord = {};
	HawkeyeError he = HawkeyeResultsDataManager::Instance().RetrieveDbImageSetRecord (data_params.dbSampleRecord.ImageSetId, imageSetRecord);
	if (he != HawkeyeError::eSuccess)
	{
//TODO: throw an error and log a message ...
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
	}

	auto retrieve_cb = [this, callback, imageSetRecord, data_params, addblankImages](int16_t seqNumber, ImageSet_t imageSet, HawkeyeError he) -> void
	{
		if (he == HawkeyeError::eSuccess)
		{
			data_params.tmp_image_collection->emplace (seqNumber, imageSet);

			if (data_params.tmp_image_collection->size() == imageSetRecord.ImageSequenceList.size())
			{
				pHawkeyeServices_->enqueueInternal([=]() {addblankImages(std::move(data_params)); });
			}

			return;
		}

//TODO: throw an error and log a message ...
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
	};

	for (auto& v : imageSetRecord.ImageSequenceList)
	{
		//Keep for debugging Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSample_LoadImages: " + Uuid::ToStr (v.ImageSequenceId));

		HawkeyeResultsDataManager::Instance().RetrieveImageSequenceSet (v.ImageSequenceId, retrieve_cb);
	}
}

//*****************************************************************************
// Retrieve image set used by original sample that has dust subtract and FL backgrounds.
//*****************************************************************************
void HawkeyeLogicImpl::reanlyzeSample_LoadNormalizationImages (ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback)
{
	if (Uuid::IsValid (data_params.dbSampleRecord.DustRefImageSetId))
	{
		pHawkeyeServices_->enqueueInternal([this, callback, data_params]()
		{
			auto retrieve_cb = [this, callback, data_params](int16_t seqNumber, ImageSet_t imageSet, HawkeyeError he) -> void
			{
				if (he == HawkeyeError::eSuccess)
				{
					// Get rid of the constness of "data_params".
					ReanlyzeSample_DataParams tmp = data_params;
					tmp.background_normalization_images = imageSet;
						
					pHawkeyeServices_->enqueueInternal (callback, std::move(tmp), true);
					return;
				}

//TODO: throw an error and log a message ...
				pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
			};

			Logger::L().Log (MODULENAME, severity_level::debug1, "reanlyzeSample_LoadNormalizationImages: " + Uuid::ToStr (data_params.dbSampleRecord.DustRefImageSetId));

			HawkeyeResultsDataManager::Instance().RetrieveImageSequenceSet (data_params.dbSampleRecord.DustRefImageSetId, retrieve_cb);
		});
	}
	else
	{
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), true);
	}
}

//*****************************************************************************
void HawkeyeLogicImpl::reanalyzeSample_DoReanalysis(
	ReanlyzeSample_DataParams data_params,
	std::function<void(ReanlyzeSample_DataParams, bool)> callback)
{
	reanalysisResult_ = std::make_shared<CellCounterResult::SResult>();

	auto retrieve_cb = [this, data_params, callback](CellCounterResult::SResult orgSresult, HawkeyeError) -> void
	{
		if (data_params.from_images)
		{
			auto reanalysisLambda = [this, data_params, orgSresult, callback]() -> void
			{
				ImageProcessingUtilities::CellCounterReanalysis(
					data_params.image_collection,
					data_params.dbSampleRecord.Dilution,
					data_params.background_normalization_images,
					data_params.analysis,
					data_params.celltype,
					orgSresult,
					*reanalysisResult_)
					.whenComplete([this, data_params, callback](HawkeyeError he)mutable -> void
						{
							data_params.completion_status = he;
							pHawkeyeServices_->enqueueInternal(callback, std::move(data_params), he == HawkeyeError::eSuccess);
						});
			};

			pHawkeyeServices_->enqueueInternal(reanalysisLambda);
		}
		else
		{
			auto reanalysisLambda = [this, data_params, orgSresult, callback]() -> void
			{
				//NOTE: Always the first result data will be considered for the reanalysis
				ImageProcessingUtilities::CellCounterReanalysis(
					data_params.analysis,
					data_params.celltype,
					orgSresult,
					*reanalysisResult_)
					.whenComplete([this, data_params, callback](HawkeyeError he)mutable -> void
						{
							data_params.completion_status = he;
							pHawkeyeServices_->enqueueInternal(callback, std::move(data_params), he == HawkeyeError::eSuccess);
						});
			};

			pHawkeyeServices_->enqueueInternal(reanalysisLambda);
		}
	};

	HawkeyeResultsDataManager::Instance().RetrieveSampleSResult (data_params.dbSampleRecord.SampleId, retrieve_cb);
}

//*****************************************************************************
void HawkeyeLogicImpl::reanalyzeSample_GenerateResultRecord (ReanlyzeSample_DataParams data_params, std::function<void(ReanlyzeSample_DataParams, bool)> callback)
{
	if (data_params.completion_status != HawkeyeError::eSuccess || !reanalysisResult_)
	{
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
		return;
	}

	ImageAnalysisUtilities::ApplyConcentrationAdjustmentFactor (data_params.celltype, *reanalysisResult_);

	data_params.resultRecord.summary_info.timestamp = ChronoUtilities::CurrentTime();
	data_params.resultRecord.summary_info.cell_type_settings = data_params.celltype;
	data_params.resultRecord.summary_info.analysis_settings = data_params.analysis;
	data_params.resultRecord.summary_info.username = data_params.loggedInUsername;

	// Get BasicResult data from SResult
	ImageAnalysisUtilities::getBasicResultAnswers (reanalysisResult_->Cumulative_results, data_params.resultRecord.summary_info.cumulative_result);

	// Get Per image data from SResult
	data_params.resultRecord.per_image_result.clear();
	data_params.resultRecord.per_image_result.reserve (reanalysisResult_->map_Image_Results.size());

	for (const auto& item : reanalysisResult_->map_Image_Results)
	{
		BasicResultAnswers bra;
		ImageAnalysisUtilities::getBasicResultAnswers (item.second, bra);
		data_params.resultRecord.per_image_result.push_back (bra);
	}

	// Save the Data only if CI and AD are not Temp.
	if (data_params.cellType_index == TEMP_CELLTYPE_INDEX ||
		data_params.analysis_index == TEMP_ANALYSIS_INDEX)
	{
		// Cache temp data for possible later reference.
		HawkeyeUUID::Generate().get_uuid__t(data_params.resultRecord.summary_info.uuid);
		Logger::L().Log (MODULENAME, severity_level::debug1, "reanalyzeSample_GenerateResultRecord: <cached UUID: " + Uuid::ToStr (data_params.resultRecord.summary_info.uuid) + ">");
		HawkeyeResultsDataManager::Instance().CacheReanalysisSResultRecord (data_params.resultRecord.summary_info.uuid, *reanalysisResult_);
		pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), true);
	}
	else
	{
		data_params.dbSampleRecord.CellTypeIndex = data_params.celltype.celltype_index;
		data_params.dbSampleRecord.AnalysisDefIndex = data_params.analysis_index;
		data_params.dbSampleRecord.AcquisitionDateTP = ChronoUtilities::CurrentTime();
		data_params.dbAnalysisRecord.AnalysisUserId = {};
		UserList::Instance().GetUserUUIDByName(data_params.loggedInUsername, data_params.dbAnalysisRecord.AnalysisUserId);

		if (!HawkeyeResultsDataManager::CreateAnalysisRecord (data_params.dbSampleRecord, data_params.dbAnalysisRecord))
		{
			pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
			return;
		}

		auto sampleRecordRetrieveCb = [this, callback, data_params](SampleRecordDLL rec, HawkeyeError status)mutable
		{
			if (status != HawkeyeError::eSuccess)
			{
				pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
				return;
			}

			data_params.resultRecord.summary_info.uuid = data_params.dbAnalysisRecord.SummaryResult.SummaryResultId;
			Logger::L().Log (MODULENAME, severity_level::debug1, "resultRecord.summary_info.uuid: " + Uuid::ToStr (data_params.resultRecord.summary_info.uuid));

			pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), true);
		};

		auto reanalysisDataSaveCompletionCb = [this, callback, data_params, sampleRecordRetrieveCb](bool completion_status)
		{
			if (!completion_status)
			{
				Logger::L().Log (MODULENAME, severity_level::error, "ReanalyzeSample: <failed to save reanalysis data for sample: " + Uuid::ToStr (data_params.dbSampleRecord.SampleId) + ">");
				ReportSystemError::Instance().ReportError (BuildErrorInstance(
					instrument_error::instrument_storage_writeerror,
					instrument_error::instrument_storage_instance::result,
					instrument_error::severity_level::error));
				pHawkeyeServices_->enqueueInternal (callback, std::move(data_params), false);
				return;
			}

			Logger::L().Log (MODULENAME, severity_level::debug1, "ReanalyzeSample: <completed the saving analysis data for sample: " + Uuid::ToStr (data_params.dbSampleRecord.SampleId) + ">");

			//Read the latest record info and provide actual uuid created for newly created reanalysis result record.			
			HawkeyeResultsDataManager::Instance().RetrieveSampleRecord (data_params.dbSampleRecord.SampleId, sampleRecordRetrieveCb);
		};

		// Reanalysis doesn't need to save the normalization data - it's retrieved from the SampleRecord.
		// this workflow does not require the user id, so do not use the transient user technique
		pLocalIosvc_->post([=]()
		{
			const QcStatus qcStatus = IsSampleWithinQCLimits (data_params.sampleDef.parameters.bp_qc_name, data_params.sampleDef.parameters.qc_uuid, *reanalysisResult_);

			HawkeyeResultsDataManager::Instance().SaveReanalysisData(
				*reanalysisResult_,
				data_params.dbSampleRecord,
				data_params.dbAnalysisRecord,
				qcStatus,
				reanalysisDataSaveCompletionCb);

			std::string extra = boost::str(boost::format(
				"Reanalysis of Sample: %s\n"
				"\tTag: %s\n"
				"\tDate: %s UTC\n"
				"With:\n"
				"\tCell Type: %s\n")
				% data_params.dbSampleRecord.Label
				% data_params.dbSampleRecord.CommentsStr
				% ChronoUtilities::ConvertToString(data_params.dbSampleRecord.AcquisitionDateTP)
				% data_params.celltype.label);

			if (Uuid::IsValid(data_params.dbSampleRecord.QcId))
			{
//TODO: extra += "QCProcess: " + data_params.wqir.bp_qc_identifier + "\n";
			}
			else if (Uuid::IsValid(data_params.dbSampleRecord.BioProcessId))
			{
//TODO: extra += "BioProcess: " + data_params.wqir.bp_qc_identifier + "\n";
			}

			AuditLogger::L().Log (generateAuditWriteData(
				data_params.loggedInUsername,
				audit_event_type::evt_sampleresultcreated,
				extra));

			auto onWritingLegacyDataComplete = [](bool status) {
				if (!status)
				{
					Logger::L().Log (MODULENAME, severity_level::error, "reanalyzeSample_GenerateResultRecord: <Failed to Save the legacy cell counting data>");
				}
			};

			if (HawkeyeConfig::Instance().get().generateLegacyCellCountingData)
			{
				auto save_legacy_data_lambda = [=]()
				{
					HawkeyeResultsDataManager::Instance().SaveLegacyCellCountingData(
						data_params.image_collection,
						data_params.dbSampleRecord.SampleNameStr,
						*reanalysisResult_,
						onWritingLegacyDataComplete);
				};
				pHawkeyeServices_->enqueueInternal (save_legacy_data_lambda);
			}
		});
	}
}

