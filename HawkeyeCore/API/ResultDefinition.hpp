#pragma once
#include <cstdint>

#include "uuid__t.hpp"

#include "CellType.hpp"
#include "ReportsCommon.hpp"
#include "ResultDefinitionCommon.hpp"
#include "SampleParameters.hpp"
#include "Signature.hpp"

typedef struct blob_measurements_t
{
	uint16_t x_coord;
	uint16_t y_coord;

	uint32_t num_measurements;
	measurement_t* measurements;
} blob_measurements_t;

typedef struct image_blobs_t
{
	uint16_t image_set_number;
	uint32_t blob_count;
	blob_measurements_t* blob_list;
} image_blobs_t;


typedef struct large_cluster_t
{
	uint16_t image_set_number;
	uint32_t cluster_count;
	large_cluster_data_t* cluster_list;

} large_cluster_t;

/*
 * The detailed measurements for a particular Result.
 *  Due to the size of this data, it should be requested only as necessary.
 */
struct DetailedResultMeasurements
{
	uuid__t uuid;	// UUID of associated result record

	uint16_t num_image_sets;
	image_blobs_t* blobs_by_image;	
	large_cluster_t* large_clusters_by_image;
};

enum class QcStatus : uint16_t
{
	eFail = 0,
	ePass,
	eNotApplicable
};

struct ResultSummary
{
	uuid__t uuid;
	char* username;

	uint64_t timestamp;

	// Copy of analysis settings at time of creation
	CellType cell_type_settings;
	AnalysisDefinition analysis_settings;	// use for labeling population of interest ("Viable", "Apoptotic"...)

	// Answers for the complete result
	BasicResultAnswers cumulative_result;

	// Signature Set
	uint16_t num_signatures;
	DataSignatureInstance_t * signature_set;

	QcStatus qcStatus = QcStatus::eNotApplicable;
};


struct ResultRecord
{
	// Answers for the complete result
	ResultSummary summary_info;

	// Answers calculated per-image
	uint16_t num_image_results;
	BasicResultAnswers* per_image_result;
};

struct ImageRecord
{
	uuid__t uuid;
	char* username;

	uint64_t timestamp;
};

struct SampleImageSetRecord
{
	uuid__t uuid;
	char* username;
	uint64_t timestamp;
	uint16_t sequence_number;
	uuid__t brightfield_image;
	uint8_t num_fl_channels;
	uint16_t* fl_channel_numbers;
	uuid__t* fl_images;
};

struct ReagentInfoRecord
{
	char* pack_number;
	uint32_t lot_number;
	char* reagent_label;
	uint64_t expiration_date; // Days since 1/1/1970
	uint64_t in_service_date; // Days since 1/1/1970
	uint64_t effective_expiration_date; // Days since 1/1/1970
};

struct SampleRecord
{
	uuid__t uuid;
	char* username;
	uint64_t timestamp;
	char* sample_identifier;
	char* bp_qc_identifier;
	uint16_t dilution_factor;
	eSamplePostWash wash;
	char* comment;

	//Reagents
	uint16_t num_reagent_records;
	ReagentInfoRecord* reagent_info_records;

	//Images
	uint16_t num_image_sets;
	uuid__t* image_sequences;
	
	//Results
	uint16_t num_result_summary;
	ResultSummary *result_summaries;

	sample_completion_status  sam_comp_status;
	SamplePosition   position;
};

struct WorklistRecord
{
	uuid__t  uuid;
	char*    username;
	char*    label;
	uint64_t timestamp;
	uint16_t num_sample_records;
	uuid__t* sample_records;
};
