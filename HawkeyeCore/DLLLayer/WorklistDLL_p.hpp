#pragma once

#include <cstdint>
#include <string>
#include <queue>
#include <vector>

#include "AnalysisDefinitionDLL.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "HawkeyeError.hpp"
#include "ImageCollection.hpp"
#include "ImageWrapperDLL.hpp"
#include "ResultDefinitionCommon.hpp"
#include "SampleDefinitionDLL.hpp"
#include "WorkflowController.hpp"
#include "Worklist.hpp"

class WorklistDLLPrivate
{
public:
	enum class eWorklistStates : uint8_t
	{
		eEntryPoint = 0,
		eCreateWL,
		eInitializeStage,
		eProbeUp,
		eComplete,
		eError,
	};

	WorklistDLLPrivate() {
		bool isWorklistRunning_ = false;  // Set to true when actual sample processing begins or is resumed.
	                                      // Set to false whenever sample processing has stopped (queue paused/stopped, error...).
		isTempAnalysis = false;
		imageOutputType = eImageOutputType::eImageAnnotated;
		imageCnt = 0;
		isAborted = false;
		isWorklistSet = false;
		sampleToSampleTiming = boost::none;
	}

	bool isWorklistRunning;  // Set to true when actual sample processing begins or is resumed.
	                         // Set to false whenever sample processing has stopped (queue paused/stopped, error...).
	bool isTempAnalysis;
	eImageOutputType imageOutputType;
	uint32_t imageCnt;
	bool isAborted;
	boost::optional<std::pair<SamplePositionDLL, system_TP>> sampleToSampleTiming;
	bool isWorklistSet;
	std::mutex worklistMutex;
	std::mutex sampleStatusMutex;
	std::shared_ptr<ImageCollection_t> imageCollection;
	std::shared_ptr<DeadlineTimerUtilities> deadlineTimer;
	std::shared_ptr<HawkeyeServices> pHawkeyeServices;
};
