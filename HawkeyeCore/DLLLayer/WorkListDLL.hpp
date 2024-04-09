#pragma once

#include <cstdint>
#include <string>
#include <queue>
#include <vector>

#include <DBif_Api.h>

#include "AnalysisDefinitionDLL.hpp"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "HawkeyeError.hpp"
#include "ImageCollection.hpp"
#include "ImageWrapperDLL.hpp"
#include "ResultDefinitionCommon.hpp"
#include "SampleDefinitionDLL.hpp"
#include "Worklist.hpp"

/// NOTE: The system will not necessarily begin processing the work queue at the first entry.
///       A carousel may not necessarily be oriented in a way that puts position Z1 ready to run.
///       For a carousel, the system will begin the queue at whichever entry corresponds to the location that is
///       closest to being in the "active" position.
///
///       Additionally, the system may re-order the Worklist as presented to "correct" the execution order for
///       an otherwise-valid queue.
class WorklistDLL
{
public:
	enum class eWorklistDLLState : uint8_t
	{
		eEntryPoint = 0,
		eValidateWorklist,
		eInitializeStage,
		eProbeUp,
		eComplete,
		eError,
	};

	enum class QueueCarouselState : uint8_t
	{
		eEntryPoint = 0,
		eSearchForTube,
		eGotoNextTube,
		eCheckIsOrphanSample, // Defined sample OR Orphan sample
		eHandleOrphanSample,
		eProcessItem,
		eWaitAllDone,
		eComplete,
		eError
	};

	struct SampleIndices {
		uint16_t sampleSetIndex;
		uint16_t sampleIndex;
		uint16_t processingOrder;
	};

	typedef std::function<void (SampleDefinitionDLL)> sample_status_callback_DLL;
	typedef std::function<void (uuid__t /*work queue record identifier*/)> sample_completion_callback_DLL;
	typedef std::function<void (SampleDefinitionDLL,
                                uint16_t /*image sequence number*/,
                                ImageSetWrapperDLL /*image*/,
                                BasicResultAnswers /*cumulative*/,
                                BasicResultAnswers /*this_image*/)> sample_image_result_callback_DLL;
	typedef std::function<void(uuid__t /*work queue record identifier*/)> worklist_completion_callback_DLL;

	WorklistDLL (std::shared_ptr<HawkeyeServices> pHawkeyeServices);

	void Set (const Worklist& workList, std::function<void (HawkeyeError)> callback);
	HawkeyeError Get (Worklist*& wl);

	void AddSampleSet (
		const SampleSet& ss, 
		std::function<void(HawkeyeError)> onComplete);
	void CancelSampleSet (
		uint16_t sampleSetIndex,
		std::function<void(HawkeyeError)> onComplete);
	void Start (sample_status_callback_DLL onSampleStatus,
	            sample_image_result_callback_DLL onSampleImageProcessed,
	            sample_status_callback_DLL onSampleComplete,
	            worklist_completion_callback_DLL onWorklistComplete,
	            std::function<void (HawkeyeError)> onCompleteCallback);
	HawkeyeError Pause(bool force = false);
	void Resume (std::function<void (HawkeyeError)> callback);
	HawkeyeError Stop(bool lock = true);

	HawkeyeError GetCurrentSample (SampleDefinition*& sd);
	void worklistCompleted();
	void generateCarouselOrphanSample (const SamplePositionDLL& currentStagePos);
	void onSampleProcessingComplete (HawkeyeError he);
	void waitForDataWrapupToComplete();
	void searchForNextTube (boost::optional<uint32_t> finalTubePosition, std::function<void (bool)> callback);
	void queueCarouselSampleInternal (QueueCarouselState currentState, boost::optional<uint32_t> currentTubeLocation, std::function<void (HawkeyeError)> callback);
	void queuePlateSampleInternal (std::function<void (HawkeyeError)> callback);
	void queueACupSampleInternal (std::function<void (HawkeyeError)> callback);
	void queueSampleForProcessing();
	void handlePauseStopForCurrentItem();
	HawkeyeError isSampleValid (bool& reset, SampleDefinitionDLL& sdDLL);
	void onSampleCellCountingComplete (SampleIndices curSampleIndices, int imageNum, CellCounterResult::SResult ccResult);
	void SetImageOutputTypePreference (eImageOutputType image_type);
	
	static SampleSetDLL& getSampleSetByIndex (uint16_t ssIndex);
	static SampleDefinitionDLL& getSampleByIndices (WorklistDLL::SampleIndices indices);
	void skipSample(SampleDefinitionDLL& sample);
	static SampleIndices getIndicesFromSample (const SampleDefinitionDLL& sample);
	std::vector<SampleIndices> getIndicesVectorFromSamples (const std::vector<SampleDefinitionDLL>& samples);
	static void Log (const std::vector<SampleSetDLL>& sampleSets);

	struct WorklistData { // These are the elements of the the WorklistDLL data object.
	public:
		//*****************************************************************************
		WorklistData()
		{
			uuid = {};
			userUuid = {};
			runUserUuid = {};
			username;
			runUsername;
			label;
			timestamp;				// Set when Worklist is started.
			status;
			acquireSample = true;
			carrier = eCarrierType::eUnknown;
			precession = ePrecession::eRowMajor;
			useSequencing = false;
			SampleParametersDLL orphanParameterSettings;
			std::vector<SampleSetDLL> sampleSetsDLL;
		}

		//*****************************************************************************
		WorklistData (Worklist wl)
		{
			// Make sure there is Worklist label.
			if (strlen(wl.label) == 0)
			{
				label = ChronoUtilities::ConvertToString(ChronoUtilities::CurrentTime(), "%Y%m%d_%H%M%S");
			}
			else
			{
				DataConversion::convertToStandardString (label, wl.label);
			}

			carrier = wl.carrier;
			precession = wl.precession;
			DataConversion::convertToStandardString (username, wl.username);
			DataConversion::convertToStandardString (runUsername, wl.runUsername);

			useSequencing = wl.useSequencing;
			sequencingTextFirst = wl.sequencingTextFirst;
			DataConversion::convertToStandardString (sequencingBaseLabel, wl.sequencingBaseLabel);
			sequencingDigit = wl.sequencingStartingDigit;
			sequencingNumberOfDigits = wl.sequencingNumberOfDigits;
			orphanParameterSettings = wl.defaultParameterSettings;
		}

		//*****************************************************************************
		static DBApi::DB_Worklist WorklistDLL::WorklistData::ToDbStyle (const WorklistData& worklistData, DBApi::eWorklistStatus wlStatus)
		{
			DBApi::DB_WorklistRecord wlDb = {};

			wlDb.WorklistStatus = static_cast<int32_t>(wlStatus);

			wlDb.WorklistIdNum = 0;

			wlDb.WorklistId = worklistData.uuid;
			wlDb.WorklistNameStr = worklistData.label;
			wlDb.ListComments = "";		//TODO: not used, remove...
			if (wlDb.InstrumentSNStr.length() == 0)
			{
				wlDb.InstrumentSNStr = SystemStatus::Instance().systemVersion.system_serial_number;
			}

			DBApi::DB_UserRecord dbUser;

			wlDb.CreationUserNameStr = worklistData.username;
			wlDb.CreationUserId = worklistData.userUuid;

			wlDb.RunUserNameStr = worklistData.runUsername;
			wlDb.RunUserId = worklistData.userUuid;

			wlDb.RunDateTP = worklistData.timestamp;
			wlDb.AcquireSample = worklistData.acquireSample;
			wlDb.CarrierType = static_cast<uint16_t>(worklistData.carrier);
			wlDb.ByColumn = false;
			if (worklistData.carrier == eCarrierType::ePlate_96)
			{
				wlDb.ByColumn = (worklistData.precession == ePrecession::eColumnMajor) ? true : false;
			}

			wlDb.SaveNthImage = (int16_t)worklistData.orphanParameterSettings.saveEveryNthImage;
			wlDb.WashTypeIndex = worklistData.orphanParameterSettings.postWash;
			wlDb.Dilution = (int16_t)worklistData.orphanParameterSettings.dilutionFactor;

			wlDb.SampleSetNameStr.clear();	// Not used.
			wlDb.SampleItemNameStr.clear();	// Not used.
			wlDb.ImageAnalysisParamId = {};
			wlDb.AnalysisDefId = worklistData.orphanParameterSettings.analysis.uuid;
			wlDb.AnalysisDefIndex = worklistData.orphanParameterSettings.analysis.analysis_index;
			wlDb.AnalysisParamId = {};
			wlDb.CellTypeId = worklistData.orphanParameterSettings.celltype.uuid;
			wlDb.CellTypeIndex = worklistData.orphanParameterSettings.celltype.celltype_index;

			// TODO: implement when/if worklist incorporates this information
			wlDb.BioProcessId = {};	//TODO: not used.
			wlDb.BioProcessNameStr.clear();	//TODO: not used.
			wlDb.QcProcessId = {};
			wlDb.QcProcessNameStr.clear();
			// TODO: find location of workflowId used for the samples
			wlDb.WorkflowId = {};

			// If worklist has been saved and retrieved here, sample set count may differ;
			// always initialize to the count of the source worklist object;
			wlDb.SampleSetCount = (int16_t)worklistData.sampleSetsDLL.size();
			wlDb.ProcessedSetCount = 0;

			SampleSetDLL ssd;
			DBApi::DB_SampleSetRecord dbss;

			for (auto& ssd : worklistData.sampleSetsDLL)
			{
				dbss = ssd.ToDbStyle();
				if (dbss.SampleSetStatus != static_cast<int32_t>(DBApi::eSampleSetStatus::NoSampleSetStatus) &&
					dbss.SampleSetStatus != static_cast<int32_t>(DBApi::eSampleSetStatus::SampleSetNotRun) &&
					dbss.SampleSetStatus != static_cast<int32_t>(DBApi::eSampleSetStatus::SampleSetTemplate))
				{
					wlDb.ProcessedSetCount++;
				}
				wlDb.SSList.push_back(dbss);
			}

			return wlDb;
		}

		uuid__t uuid;
		uuid__t userUuid;
		uuid__t runUserUuid;
		std::string username;
		std::string runUsername;
		std::string label;
		system_TP timestamp;				// Set when Worklist is started.
		int32_t status;
		bool acquireSample;					// Set when Worklist is started.
		eCarrierType carrier;
		ePrecession precession; // Only applies to plate processing.
		bool usingACup;

		bool        useSequencing;
		bool        sequencingTextFirst;
		std::string sequencingBaseLabel;
		uint16_t	sequencingDigit;
		uint16_t    sequencingNumberOfDigits;

		SampleParametersDLL orphanParameterSettings; // For eCarousel: settings to use for processing of any additional samples encountered.
										 // .label will be used with an appended index ("label.1", "label.2"....)
										 // .location will be ignored.
		std::vector<SampleSetDLL> sampleSetsDLL;

		class SamplesToProcess
		{
		public:

			std::vector<SampleIndices> Get()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				std::vector<SampleIndices> toReturn = samplesToProcess_;
				return toReturn;
			}

			size_t CurrentSampleIndex()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				size_t toReturn = curSampleIndex_;
				Logger::L().Log ("WorklistDLL", severity_level::debug2, boost::str(boost::format("CurrentSampleIndex: %d") % curSampleIndex_));
				return toReturn;
			}

			size_t ResetOrderNum()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				size_t toReturn = nextProcessingOrder_ = 1;
				Logger::L().Log( "WorklistDLL", severity_level::debug1, boost::str( boost::format( "ResetOrderNum: %d" ) % nextProcessingOrder_ ) );
				return toReturn;
			}

			size_t NextOrderNum()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				Logger::L().Log( "WorklistDLL", severity_level::debug1, boost::str( boost::format( "NextOrderNum: %d" ) % nextProcessingOrder_ ) );
				size_t toReturn = nextProcessingOrder_++;
				return toReturn;
			}

			void ResetProcessingListOrder()
			{
				std::lock_guard<std::mutex> m{ mutex_ };

				resetProcessingListOrderInternal();
			}

			SampleIndices GetSampleIndices (size_t index)
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				SampleIndices toReturn = samplesToProcess_[index];
				return toReturn;
			}

			SampleIndices GetCurrentSampleIndices()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				SampleIndices toReturn = samplesToProcess_[curSampleIndex_];
				return toReturn;
			}

			bool IsSampleDefined(uint32_t tubeLoc, uint16_t& sampleIdx)
			{
				bool locFound = false;
				uint16_t foundIdx = 0;
				uint16_t foundCnt = 0;
				uint16_t order = 0;

				for (size_t i = 0; i < Count(); i++)
				{
					auto& indices = GetSampleIndices( i );
					auto& sample = getSampleByIndices(indices);
					if (sample.position.getColumn() == tubeLoc)
					{
						foundCnt++;
						if (( foundCnt < 2 ) || ( indices.processingOrder > order ))
						{
							order = indices.processingOrder;
							foundIdx = (uint16_t) i;
							locFound = true;
						}
					}
				}

				if (locFound)
				{
					sampleIdx = foundIdx;
				}
				return locFound;
			}

			size_t Count()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				return samplesToProcess_.size();
			}

			void Insert (eCarrierType carrier, std::vector<SampleIndices>& indices)
			{
				std::lock_guard<std::mutex> m{ mutex_ };

				samplesToProcess_.insert (samplesToProcess_.end(), indices.begin(), indices.end());

				if ( carrier == eCarrierType::eCarousel )
				{	
					// If samples inserted, sort indices by carousel column
					if (samplesToProcess_.size())
					{
						// Save the indices associated with the curSampleIndex_ because
						// after the sort these indices may move to a different location in 
						// samplesToProcess and the curSampleIndex_ might have to be adjusted.
						SampleIndices curSampleIndices = samplesToProcess_[curSampleIndex_];

						std::sort (samplesToProcess_.begin(), samplesToProcess_.end(), sortSamplesByCarouselColumn());

						// Find indices that are associated with the curSampleIndex_ and
						// set curSampleIndex_ appropriately.
						for (int sIdx = 0; sIdx < samplesToProcess_.size(); sIdx++)
						{
							if ((samplesToProcess_[sIdx].sampleIndex == curSampleIndices.sampleIndex) &&
								(samplesToProcess_[sIdx].sampleSetIndex == curSampleIndices.sampleSetIndex) &&
								(samplesToProcess_[sIdx].processingOrder == curSampleIndices.processingOrder))
							{
								curSampleIndex_ = sIdx;
							}
						}

						resetProcessingListOrderInternal();
					}
				}
			}

			void InsertAfterCurrentSampleIndex (const SampleIndices& indices)
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				samplesToProcess_.insert (samplesToProcess_.begin() + curSampleIndex_, indices);
				resetProcessingListOrderInternal();
			}

			// Only used for 96 well plate
			void IncrementCurrentSampleIndex()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				curSampleIndex_++;
				Logger::L().Log ("WorklistDLL", severity_level::debug1, boost::str(boost::format("IncrementCurrentSampleIndex: new index: %d") % curSampleIndex_));
			}

			void SetCurrentSampleIndex (size_t value)
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				curSampleIndex_ = value;
			}

			// Only used for 96 well plate
			bool IsDone()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				if (curSampleIndex_ >= samplesToProcess_.size())
				{
					return true;
				}

				return false;
			}

			void Clear()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				samplesToProcess_.clear();
				curSampleIndex_ = 0;
				nextProcessingOrder_ = 1;		// NOT an index! '0' indicates the value has not been assigned!
			}

			void CarouselSort()
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				std::sort (samplesToProcess_.begin(), samplesToProcess_.end(), sortSamplesByCarouselColumn());
			}

			void PlateSort (ePrecession precession)
			{
				std::lock_guard<std::mutex> m{ mutex_ };
				if (precession == eRowMajor)
				{
					std::sort (samplesToProcess_.begin(), samplesToProcess_.end(), sortSamplesByPlateRow());
				}
				else
				{
					std::sort (samplesToProcess_.begin(), samplesToProcess_.end(), sortSamplesByPlateColumn());
				}
			}

		private:
			//*****************************************************************************
			struct sortSamplesByCarouselColumn
			{
				bool operator() (const WorklistDLL::SampleIndices& a, const WorklistDLL::SampleIndices& b) const
				{
					SampleDefinitionDLL sampleA = WorklistDLL::getSampleByIndices(a);
					SampleDefinitionDLL sampleB = WorklistDLL::getSampleByIndices(b);

					return ( sampleA.position.getColumn() < sampleB.position.getColumn() );
				}
			};

			//*****************************************************************************
			// Sort by row, then column.
			//*****************************************************************************
			struct sortSamplesByPlateRow
			{
				bool operator() (const WorklistDLL::SampleIndices& a, const WorklistDLL::SampleIndices& b) const
				{
					SampleDefinitionDLL sampleA = WorklistDLL::getSampleByIndices(a);
					SampleDefinitionDLL sampleB = WorklistDLL::getSampleByIndices(b);

					if (sampleA.position.getRow() != sampleB.position.getRow())
					{
						return (sampleA.position.getRow() < sampleB.position.getRow());
					}
					return (sampleA.position.getColumn() < sampleB.position.getColumn());
				}
			};

			//*****************************************************************************
			// Sort by column, then row.
			//*****************************************************************************
			struct sortSamplesByPlateColumn
			{
				bool operator() (const WorklistDLL::SampleIndices& a, const WorklistDLL::SampleIndices& b) const
				{
					SampleDefinitionDLL sampleA = WorklistDLL::getSampleByIndices(a);
					SampleDefinitionDLL sampleB = WorklistDLL::getSampleByIndices(b);

					if (sampleA.position.getColumn() != sampleB.position.getColumn())
					{
						return (sampleA.position.getColumn() < sampleB.position.getColumn());
					}
					return (sampleA.position.getRow() < sampleB.position.getRow());
				}
			};

			void resetProcessingListOrderInternal()
			{
				SampleIndices indices = {};
				uint16_t chkOrder = 1;
				size_t listSize = samplesToProcess_.size();

				// first determine the number of samples that have already been processed
				for ( size_t listIdx = 0; listIdx < listSize; ++listIdx )
				{
					auto& sample = getSampleByIndices( samplesToProcess_[listIdx] );
					if ( sample.status != eSampleStatus::eNotProcessed )
					{
						chkOrder++;
					}
				}

				// next, set the order of samples in the list that have NOT been processed and that sort later than the current sample
				for ( size_t listIdx = curSampleIndex_; listIdx < listSize; ++listIdx )
				{
					auto& indices = samplesToProcess_[listIdx];
					auto& sample = getSampleByIndices( indices );
					if ( sample.status == eSampleStatus::eNotProcessed )
					{
						indices.processingOrder = chkOrder++;
					}
				}

				// finally, set the order of samples in the list that have NOT been processed but sorted ahead of the current sample
				for ( size_t listIdx = 0; listIdx < curSampleIndex_; ++listIdx )
				{
					auto& indices = samplesToProcess_[listIdx];
					auto& sample = getSampleByIndices( indices );
					if ( sample.status == eSampleStatus::eNotProcessed )
					{
						indices.processingOrder = chkOrder++;
					}
				}
				nextProcessingOrder_ = chkOrder;
			}

			std::mutex mutex_;
			std::vector<SampleIndices> samplesToProcess_;
			size_t curSampleIndex_;
			size_t nextProcessingOrder_;
		};

		SamplesToProcess samplesToProcess;

		void clear()
		{
			uuid = {};
			userUuid = {};
			runUserUuid = {};
			status = static_cast<int32_t>(DBApi::eWorklistStatus::NoWorklistStatus);
			acquireSample = false;
		}
	};

private:
	typedef std::function<void(HawkeyeError)> sample_completion_callback;
	void setWorklistInternal (eWorklistDLLState currentState, std::function<void (HawkeyeError)> callback, HawkeyeError errorStatus = HawkeyeError::eSuccess);
	HawkeyeError validate_analysis_and_celltype (SampleDefinitionDLL& sdDLL);
	void startWorklistInternal (std::function<void (HawkeyeError)> callback);
	void addSampleSetInternal (std::shared_ptr<SampleSetDLL> sampleSet, std::function<void(HawkeyeError)> callback, bool isOrphanSampleSet = false );
	void cancelSampleSetInternal (uint16_t sampleSetIndex, std::function<void(HawkeyeError)> callback);
	void addAdditionalSampleInfo (SampleDefinitionDLL& sdDLL);
	void processSample (SampleDefinitionDLL& sdDLL, sample_completion_callback cb_WQIComplete);
	void onCameraCaptureTrigger (cv::Mat image);
	void onSampleCompleted (HawkeyeError he);
	bool isDiskSpaceAvailableForAnalysis (const std::vector<SampleDefinitionDLL>& sdDLL_list) const;
	bool atLeastXXPercentDiskFree(uint8_t full_percent) const;
	void onSampleWorkflowStateChanged (eSampleStatus state, bool executionComplete);
	void verifySampleToSampleTiming (const SampleDefinitionDLL& sdDLL);
	bool sufficientReagentForSet (ReagentContainerPosition containerPosition, const int count) const;
	void setSystemStatus (eSystemStatus sysstatus);
	void resetWorklist();
	void updateSampleStatus (SampleDefinitionDLL& sampleDLL);	
	bool updateSampleSetStatus (SampleSetDLL& sampleSet);
	bool isSampleSetRunning (const SampleSetDLL& sampleSet);
	bool isSampleSetCompleted (const SampleSetDLL& sampleSet);
	bool isSampleCompleted (const SampleDefinitionDLL& sampleSet);
	void markCarouselSlotsInUseForSimulation (const std::vector<SampleIndices>& indices);
	void wrapupSampleProcessing (SampleDefinitionDLL& sample);
	bool isWorklistComplete();
	bool areAnySamplesNotProcessed();


	void setNextSampleSet (Worklist, uint16_t index, std::function<void(HawkeyeError)> callback);
	void setSampleSetAsync (Worklist worklist, uint16_t ssIndex, std::function<void (HawkeyeError)> callback);

	// DB intermediate methods
	bool writeWorklistToDB (DBApi::eWorklistStatus wlStatus);
	bool updateWorklistToDB();
	bool updateWorklistStatusToDB (DBApi::eWorklistStatus dbWlStatus);

	bool addWorklistSampleSet (const SampleSetDLL& sampleSet);

	//TODO: not currently used...
	bool writeSampleSetToDB (SampleSetDLL& sampleSet);

	bool updateSampleSetStatusToDB (SampleSetDLL& sampleSet, DBApi::eSampleSetStatus dbSampleSetStatus);
	bool updateSampleItemStatusToDB (SampleDefinitionDLL& sample);
	bool updateSampleDefinitionToDB (SampleDefinitionDLL& sample);

	static void updateSampleSetUuids (const DBApi::DB_SampleSetRecord& dbSampleSet, SampleSetDLL& sampleSet);
	static void updateSampleUuids (const DBApi::DB_SampleItemRecord& dbSample, SampleDefinitionDLL& sample);
	static const int OrphanSampleSetIndex = 0;
};
