#pragma once 

#include <iostream> 
#include <vector> 
#include "ChronoUtilities.hpp"
#include "CellTypesDLL.hpp" 
#include "DataConversion.hpp"
#include "ReportsCommon.hpp"
#include "ResultDefinition.hpp" 
#include "SamplePositionDLL.hpp"
#include "SignatureDLL.hpp" 
#include "uuid__t.hpp"

typedef struct ImageRecordDLL
{
	ImageRecord ToCStyle()
	{
		ImageRecord rec = {};

		rec.uuid = uuid;
		DataConversion::convertToCharPointer(rec.username, username);
		rec.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);

		return rec;
	}

	uuid__t uuid; 
	std::string username;

	system_TP timestamp;

	std::string path;
} ImageRecordDLL;

typedef struct SampleImageSetRecordDLL
{
	SampleImageSetRecord ToCStyle()
	{
		SampleImageSetRecord rec = {};

		rec.uuid = uuid;
		DataConversion::convertToCharPointer(rec.username, username);

		rec.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		rec.sequence_number = sequence_number;
		rec.brightfield_image = brightfield_image;

		rec.num_fl_channels = (uint8_t)flImagesAndChannelNumlist.size();
		if (rec.num_fl_channels > 0)
		{
			rec.fl_images = new uuid__t[rec.num_fl_channels];
			rec.fl_channel_numbers = new uint16_t[rec.num_fl_channels];
			for (uint8_t i = 0; i < rec.num_fl_channels; i++)
			{
				//fetching the data form vector<pair>
				rec.fl_channel_numbers[i] = flImagesAndChannelNumlist[i].first;
				rec.fl_images[i] = flImagesAndChannelNumlist[i].second;
			}
		}
		else
		{
			rec.fl_images = nullptr;
			rec.fl_channel_numbers = nullptr;
		}

		return rec;
	}

	uuid__t uuid;
	std::string username;
	system_TP timestamp;
	uint16_t sequence_number;
	uuid__t brightfield_image;
	std::vector<std::pair<uint16_t, uuid__t>> flImagesAndChannelNumlist;

} SampleImageSetRecordDLL;

typedef struct ReagentInfoRecordDLL
{
	ReagentInfoRecord ToCStyle()
	{
		ReagentInfoRecord rec = {};

		DataConversion::convertToCharPointer(rec.pack_number, pack_number);
		rec.lot_number = lot_number;
		DataConversion::convertToCharPointer(rec.reagent_label, reagent_label);

		rec.expiration_date = expiration_date;
		rec.in_service_date = in_service_date;
		rec.effective_expiration_date = effective_expiration_date;

		return rec;
	}

	std::string pack_number;
	uint32_t lot_number;
	std::string reagent_label;
	uint64_t expiration_date; // Days since 1/1/1970
	uint64_t in_service_date; // Days since 1/1/1970
	uint64_t effective_expiration_date; // Days since 1/1/1970
} ReagentInfoRecordDLL;

typedef struct ResultSummaryDLL
{
	ResultSummary ToCStyle()
	{
		ResultSummary rec = {};

		rec.uuid = uuid;
		DataConversion::convertToCharPointer(rec.username, username);
		rec.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		rec.cell_type_settings = cell_type_settings.ToCStyle();
		rec.analysis_settings = analysis_settings.ToCStyle();
		rec.cumulative_result = cumulative_result;
		DataConversion::convert_vecOfDllType_to_listOfCType (signature_set, rec.signature_set, rec.num_signatures);
		rec.qcStatus = qcStatus;
		
		return rec;
	}

	uuid__t uuid;
	std::string username;
	system_TP timestamp;

	// Copy of analysis settings at time of creation 
	CellTypeDLL cell_type_settings;
	AnalysisDefinitionDLL analysis_settings;	// use for labeling population of interest ("Viable", "Apoptotic"...) 
	
	// Answers for the complete result 
	BasicResultAnswers cumulative_result; 

	// Signature Set 
	std::vector<DataSignatureInstanceDLL> signature_set;

	QcStatus qcStatus = QcStatus::eNotApplicable;

	uuid__t analysisUuid;
	uuid__t sresultUuid;

	// Used in DATAIMPORTER.
	std::string path;

} ResultSummaryDLL;


typedef struct ResultRecordDLL
{
	ResultRecord ToCStyle()
	{
		ResultRecord rec = {};
		rec.summary_info = summary_info.ToCStyle();

		DataConversion::create_Clist_from_vector (per_image_result, rec.per_image_result, rec.num_image_results);

		return rec;
	}

	ResultSummaryDLL summary_info;
	// Answers calculated per-image	 
	std::vector<BasicResultAnswers> per_image_result;

} ResultRecordDLL;

typedef struct InstrumentRecordDLL
{
	std::string version;
} InstrumentRecordDLL;

typedef struct SampleRecordDLL
{
	SampleRecord ToCStyle()
	{
		SampleRecord rec = {};

		rec.uuid = uuid;
		DataConversion::convertToCharPointer(rec.username, username);
		rec.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);

		DataConversion::convertToCharPointer(rec.sample_identifier, sample_identifier);
		DataConversion::convertToCharPointer(rec.bp_qc_identifier, bp_qc_identifier);
		rec.dilution_factor = dilution_factor;
		rec.wash = wash;
		DataConversion::convertToCharPointer(rec.comment, comment);

		DataConversion::convert_vecOfDllType_to_listOfCType (reagent_info_records, rec.reagent_info_records, rec.num_reagent_records);

		DataConversion::create_Clist_from_vector (imageSequences, rec.image_sequences, rec.num_image_sets);

		DataConversion::convert_vecOfDllType_to_listOfCType (result_summaries, rec.result_summaries, rec.num_result_summary);

		rec.sam_comp_status = sam_comp_status;

		rec.position = GetPosition().getSamplePosition();

		return rec;
	}

	uuid__t uuid; 
	std::string username;
	system_TP timestamp;

	std::string sample_identifier; 
	std::string bp_qc_identifier; 
	uint16_t dilution_factor;
	eSamplePostWash wash;
	std::string comment;
	std::vector<ReagentInfoRecordDLL> reagent_info_records;
	std::vector<uuid__t> imageSequences; 
	uuid__t imageNormalizationData;
	std::vector<ResultSummaryDLL> result_summaries; 
	sample_completion_status  sam_comp_status;	
	SamplePositionDLL sample_position{};

	const SamplePositionDLL GetPosition() const { return sample_position; }
	const bool SetPosition(char row, uint8_t col) { return sample_position.setRowColumn(row, col); }

} SampleRecordDLL;


typedef struct WorklistRecordDLL
{
	WorklistRecord ToCStyle()
	{
		WorklistRecord rec = {};

		rec.uuid = uuid;
		DataConversion::convertToCharPointer(rec.username, username);
		DataConversion::convertToCharPointer(rec.label, label);
		rec.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		DataConversion::create_Clist_from_vector (sample_records, rec.sample_records, rec.num_sample_records);
		
		return rec;
	}

	uuid__t uuid; 
	std::string username; 
	std::string label;
	system_TP timestamp; 
	std::vector<uuid__t> sample_records; 

} WorklistRecordDLL;

// Equate these two data types for convenience in the code.
typedef WorklistRecordDLL WorkQueueRecordDLL;

typedef struct blob_measurements_tDLL
{
	blob_measurements_t ToCStyle()
	{
		blob_measurements_t b_measurement = {};

		b_measurement.x_coord = x_coord;
		b_measurement.y_coord = y_coord;

		DataConversion::create_Clist_from_vector(
			measurements, b_measurement.measurements, b_measurement.num_measurements);
		return b_measurement;
	}

	uint16_t x_coord;
	uint16_t y_coord;
	std::vector<measurement_t> measurements;

} blob_measurements_tDLL;


typedef struct image_blobs_tDLL
{
	image_blobs_t ToCStyle()
	{
		image_blobs_t blobs = {};
		blobs.image_set_number = image_set_number;
		DataConversion::convert_vecOfDllType_to_listOfCType(
			blob_list, blobs.blob_list, blobs.blob_count);
		return blobs;
	}

	uint16_t image_set_number; 
	std::vector<blob_measurements_tDLL> blob_list; 

} image_blobs_tDLL;


typedef struct large_cluster_tDLL
{
	large_cluster_t ToCStyle()
	{
		large_cluster_t lc = {};
		lc.image_set_number = image_set_number;
		DataConversion::create_Clist_from_vector(
			cluster_list, lc.cluster_list, lc.cluster_count);
		return lc;
	}

	uint16_t image_set_number; 
	std::vector<large_cluster_data_t> cluster_list; 

} large_cluster_tDLL;

/* 
* The detailed measurements for a particular Result. 
*  Due to the size of this data, it should be requested only as necessary. 
*/
typedef struct DetailedResultMeasurementsDLL 
{ 
	DetailedResultMeasurements ToCStyle()
	{
		DetailedResultMeasurements measurement = {};

		measurement.uuid = uuid;

		DataConversion::convert_vecOfDllType_to_listOfCType(
			blobs_by_image, measurement.blobs_by_image, measurement.num_image_sets);

		DataConversion::convert_vecOfDllType_to_listOfCType(
			large_clusters_by_image, measurement.large_clusters_by_image, measurement.num_image_sets);

		return measurement;
	}

	uuid__t uuid;	// UUID of associated result record	 
	std::vector<image_blobs_tDLL> blobs_by_image; 
	std::vector<large_cluster_tDLL> large_clusters_by_image; 

} DetailedResultMeasurementsDLL;
