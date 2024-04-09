#pragma once

#include <cstdint>

#include "SamplePosition.hpp"

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
///       AND the logged-in-user must have '4' in her list of allowed celltypes)
///
struct WorkQueueItem
{
	char* label;
	char* comment;
	SamplePosition location;
	uint32_t celltypeIndex;
	uint32_t saveEveryNthImage;
	uint32_t dilutionFactor;
	eSamplePostWash postWash;

	char* bp_qc_name; // name of associated bioprocess or QC.  May be NULL.

	uint8_t numAnalyses;
	uint16_t analysisIndex;	// Ref: AnalysisDefinition::analysis_index
	
	eSampleStatus status;
};

/// NOTE: The system will not necessarily begin processing the work queue at the first entry.
///       A carousel may not necessarily be oriented in a way that puts position Z1 ready to run.
///       For a carousel, the system will begin the queue at whichever entry corresponds to the location that is
///       closest to being in the "active" position.
///
///       Additionally, the system may re-order the workqueue as presented to "correct" the execution order for
///       an otherwise-valid queue.
struct WorkQueue
{
	WorkQueue() {
		numWQI = 0;
		curWQIIndex = 0;
	}
	
	char* label;
	uint16_t numWQI;
	uint16_t curWQIIndex;
	WorkQueueItem* workQueueItems;

	eCarrierType carrier;
	ePrecession precession; // does not affect Carousel processing...

	WorkQueueItem additionalWorkSettings; // For eCarousel: settings to use for processing of any additional samples encountered.
	                                      // .label will be used with an appended index ("label.1", "label.2"....)
	                                      // .location will be ignored.
};
