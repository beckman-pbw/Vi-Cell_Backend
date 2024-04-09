#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "AnalysisDefinitionDLL.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "ReportsCommon.hpp"
#include "SamplePositionDLL.hpp"
#include "WorkQueue.hpp"


/// Work Queue Item
///  This structure represents a single "sample" in the system.
///  All members with the name "____Index" are indices into the appropriate list of items
///   (ex: "celltypeIndex" is an index identifying a particular item in the system-wide list of 
///    cell types.)
///  A sample may select 1 or 2 reagents which will be mixed at a ratio of 1:1 (or (0.5 + 0.5):1 if two reagents are selected).
///  A sample may select 0 or more fluorescent illumination sources (Hunter).  The Brightfield source will always be used.
///  Within a queue, the following must be true at the time it is submitted for processing:
///   - WQI labels are not duplicated
///   - WQI locations are not duplicated
///   - All Indices must be valid (Valid: within range of the system-wide list AND 
///      where applicable, valid for the currently-logged-in user
///      (ex: If WQI.celltypeIndex is '4', then '4' must be a valid index into the system-wide celltype list
///       AND the logged-in-user must have '4' in their list of allowed celltypes)
///
typedef struct WorkQueueItemDLL
{
	WorkQueueItemDLL()
	{
		maxImageCount = (uint16_t)(0);
		isAdditionalSample = false;
		timestamp = {};
	}

	//Note: This conversion function will not convert cellTypeIndex and analysisIndex in "WorkQueueItem" TO  CellTypeDLL and AnalysisDefinitionDLL in  "WorkQueueItemDLL".
	WorkQueueItemDLL(WorkQueueItem wqi)
		: WorkQueueItemDLL()
	{
		DataConversion::convertToStandardString(label, wqi.label);
		DataConversion::convertToStandardString(comment, wqi.comment);

		location = SamplePositionDLL(wqi.location);
		//Need to Update celltype after completion of this function using getCelltype()
		saveEveryNthImage = wqi.saveEveryNthImage;
		dilutionFactor = wqi.dilutionFactor;
		postWash = wqi.postWash;
		DataConversion::convertToStandardString(bp_qc_name, wqi.bp_qc_name);
		//Need to Update analysis definition after completion of this function using getAnalysis();
		status = wqi.status;
	}

	WorkQueueItem ToCStyle()
	{
		WorkQueueItem wqi = {};

		DataConversion::convertToCharPointer(wqi.label, label);
		DataConversion::convertToCharPointer(wqi.comment, comment);

		wqi.location = location.getSamplePosition();
		wqi.celltypeIndex = celltype.celltype_index;
		wqi.saveEveryNthImage = saveEveryNthImage;
		wqi.dilutionFactor = dilutionFactor;
		wqi.postWash = postWash;

		DataConversion::convertToCharPointer(wqi.bp_qc_name, bp_qc_name);
		DataConversion::convertToCharPointer(wqi.label, label);
		DataConversion::convertToCharPointer(wqi.comment, comment);

		wqi.numAnalyses = 0;
		wqi.analysisIndex = analysis.analysis_index;
		wqi.status = status;

		return wqi;
	}

	uuid__t uuid;
	std::string label;
	std::string comment;
	SamplePositionDLL location;
	CellTypeDLL celltype;
	uint32_t saveEveryNthImage;
	uint32_t dilutionFactor;
	eSamplePostWash postWash;

	std::string bp_qc_name; // name of associated bioprocess or QC.  May be NULL.

	AnalysisDefinitionDLL analysis;

	eSampleStatus status;
	
	// fields locally used
	boost::optional<uint16_t> maxImageCount;
	bool isAdditionalSample;
	system_TP timestamp;

	// Convert to "sample_completion_status" from "eSampleStatus"
	sample_completion_status getsampleCompletionStatus() const
	{
		sample_completion_status to = sample_completion_status::sample_errored;
		switch (this->status)
		{
			case eSampleStatus::eCompleted:
				to = sample_completion_status::sample_completed;
				break;
			case eSampleStatus::eNotProcessed:
				to = sample_completion_status::sample_not_run;
				break;
			case eSampleStatus::eSkip_Manual:
				to = sample_completion_status::sample_skipped;
				break;
			case eSampleStatus::eSkip_Error:
			default:
				to = sample_completion_status::sample_errored;
		}
		return to;
	}

} WorkQueueItemDLL_t;

/// NOTE: The system will not necessarily begin processing the work queue at the first entry.
///       A carousel may not necessarily be oriented in a way that puts position Z1 ready to run.
///       For a carousel, the system will begin the queue at whichever entry corresponds to the location that is
///       closest to being in the "active" position.
///
///       Additionally, the system may re-order the workqueue as presented to "correct" the execution order for
///       an otherwise-valid queue.
typedef struct WorkQueueDLL
{
	WorkQueueDLL()
	{
		curWQIIndex = 0;
	}

	uuid__t uuid;
	std::string userId;
	std::string label;
	uint16_t curWQIIndex;
	std::vector<WorkQueueItemDLL> workQueueItems;

	eCarrierType carrier;
	ePrecession precession; // does not affect Carousel processing...

	WorkQueueItemDLL additionalWorkSettings; // For eCarousel: settings to use for processing of any additional samples encountered.
											 // .label will be used with an appended index ("label.1", "label.2"....)
											 // .location will be ignored.
	system_TP timestamp;
} WorkQueueDLL_t;
