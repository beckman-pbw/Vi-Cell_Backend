#pragma once

#include <iostream>
#include <boost\thread\future.hpp>
#include <boost\any.hpp>
#include "HawkeyeLogic.hpp"
#include <boost\property_tree\ptree.hpp>
#include <opencv2\opencv.hpp>
#include <boost\thread\thread.hpp>

#include "CellCounterFactory.h"
#include "ChronoUtilities.hpp"
#include "SystemStatusCommon.hpp"

static cv::Mat ConvertToMat(ImageWrapper_t& input)
{
	using namespace cv;
	Mat image = Mat((int)input.rows, (int)input.cols, (int)input.type);
	auto length = image.rows * image.step;
	image.data = new uchar[length];

	memcpy_s(image.data, length, input.data, length);

	return image;
}

static IplImage ConvertToIplImage(ImageWrapper_t& input)
{
	IplImage image = ConvertToMat(input);
	return image;
}

class TestAPIs
{

private:
	static void Validate(HawkeyeError status, const char* name)
	{
		std::string statusAsString = std::string(GetErrorAsStr(status));		
		if (status != HawkeyeError::eSuccess)
		{
			auto text = std::string(name) + " : " + statusAsString;
			failedCalls_.push_back(text);
			std::cout << text << std::endl;
		}
	}
	static void setPromise(int value)
	{
		if (promise_.first)
		{
			return;
		}
		promise_.first = true;
		promise_.second.set_value(value);
	}

	static std::pair<bool, boost::promise<int>> promise_;
	static std::vector<std::string> failedCalls_;
	static bool showImages_;

public:
	static std::vector<std::string> Test_GetFailedCalls()
	{
		std::vector<std::string> failedCalls;
		failedCalls.resize(failedCalls_.size());
		std::copy(failedCalls_.begin(), failedCalls_.end(), failedCalls.begin());
		failedCalls_.clear();
		return failedCalls;
	}

	static void Test_Analysis()
	{
		std::cout << "*****Test_Analysis()*****" << std::endl;
		{
			uint32_t num_ad = 1;
			AnalysisDefinition* analyses;
			Validate(GetFactoryAnalysisDefinitions(num_ad, analyses), "GetFactoryAnalysisDefinitions()");
			FreeAnalysisDefinitions(analyses, num_ad);

			num_ad = 0;
			analyses = nullptr;
			Validate(GetUserAnalysisDefinitions(num_ad, analyses), "GetUserAnalysisDefinitions()");
			FreeAnalysisDefinitions(analyses, num_ad);

			AnalysisDefinition* ad1 = new AnalysisDefinition();
			ad1->label = "New Analysis Definition #1";
			ad1->population_parameter = new AnalysisParameter();
			ad1->population_parameter->label = "Population param label #1";
			uint16_t addIndex1;
			Validate(AddAnalysisDefinition(*ad1, addIndex1), "AddAnalysisDefinition()");
			ad1->analysis_index = addIndex1;
			Validate(ModifyBaseAnalysisDefinition(*ad1, true), "ModifyBaseAnalysisDefinition()");
			Validate(RemoveAnalysisDefinition(addIndex1), "RemoveAnalysisDefinition()");

			AnalysisDefinition* ad2 = new AnalysisDefinition();
			ad2->label = "New Analysis Definition #2";
			uint16_t addIndex2;
			Validate(AddAnalysisDefinition(*ad2, addIndex2), "AddAnalysisDefinition()");
			ad2->analysis_index = addIndex2;
			Validate(ModifyBaseAnalysisDefinition(*ad2, true), "ModifyBaseAnalysisDefinition()");
			Validate(RemoveAnalysisDefinition(addIndex2), "RemoveAnalysisDefinition()");
		}

		{
			uint32_t num_analyses;
			AnalysisDefinition* analyses;
			Validate(GetAllAnalysisDefinitions(num_analyses, analyses), "GetAllAnalysisDefinitions()");
			FreeAnalysisDefinitions(analyses, num_analyses);
		}

		{
			uint32_t nparameters;
			char** parameters;
			Validate(GetSupportedAnalysisParameterNames(nparameters, parameters), "GetSupportedAnalysisParameterNames()");

			uint32_t ncharacteristics = 0;
			Hawkeye::Characteristic_t* characteristics = nullptr;
			Validate(GetSupportedAnalysisCharacteristics(ncharacteristics, characteristics), "GetSupportedAnalysisCharacteristics()");
			if (ncharacteristics > 0 && characteristics != nullptr)
			{
				std::cout << GetNameForCharacteristic(characteristics[0]);
			}
		}
		std::cout << "*************************" << std::endl;
	}

	static void Test_Bioprocess()
	{
		HawkeyeError status = HawkeyeError::eSuccess;
		std::cout << "*****Test_Bioprocess()*****" << std::endl;

		Bioprocess_t* bioprocesses = nullptr;
		uint32_t num_bioprocesses = 0;
		Validate(GetBioprocessList(bioprocesses, num_bioprocesses), "GetBioprocessList()");
		FreeListOfBioprocess(bioprocesses, num_bioprocesses);
		Validate(GetActiveBioprocessList(bioprocesses, num_bioprocesses), "GetActiveBioprocessList()");

		Bioprocess_t bioprocess = Bioprocess_t();
		if (num_bioprocesses > 0)
		{
			bioprocess = bioprocesses[num_bioprocesses - 1];
		}
		Validate(AddBioprocess(bioprocess), "AddBioprocess()");
		Validate(SetBioprocessActivation(bioprocess.bioprocess_name, false), "SetBioprocessActivation()");
		Validate(RemoveBioprocess(bioprocess.bioprocess_name), "RemoveBioprocess()");
		if (num_bioprocesses > 0)
		{
			FreeListOfBioprocess(bioprocesses, num_bioprocesses - 1);
		}
		std::cout << "***************************" << std::endl;
	}

	static void Test_CellType()
	{
		std::cout << "*****Test_CellType()*****" << std::endl;
		{
			uint32_t num_ct;
			CellType* celltypes;
			Validate(GetFactoryCellTypes(num_ct, celltypes), "GetFactoryCellTypes()");
			FreeListOfCellType(celltypes, num_ct);

			num_ct = 0;
			celltypes = nullptr;
			Validate(GetUserCellTypes(num_ct, celltypes), "GetUserCellTypes()");
			FreeListOfCellType(celltypes, num_ct);

			num_ct = 0;
			CellType* ct = new CellType();
			ct->label = "Test #CellType 1";
			ct->celltype_index = 0x80000004;
			Validate(AddCellType(*ct, num_ct), "AddCellType()");
			Validate(ModifyCellType(*ct), "ModifyCellType()");
			Validate(RemoveCellType(num_ct), "RemoveCellType()");
		}

		{
			uint32_t ad_index = 0x0001;
			uint32_t ct_index = 0x00000001;
			bool is_specialized;
			Validate(IsAnalysisSpecializedForCellType(ad_index, ct_index, is_specialized), "IsAnalysisSpecializedForCellType()");
			AnalysisDefinition* ad = nullptr;
			Validate(GetAnalysisForCellType(ad_index, ct_index, ad), "GetAnalysisForCellType()");
			if (ad != nullptr)
			{
				Validate(SpecializeAnalysisForCellType(*ad, ct_index), "SpecializeAnalysisForCellType()");
			}
			Validate(RemoveAnalysisSpecializationForCellType(ad_index, ct_index), "RemoveAnalysisSpecializationForCellType()");
		}

		{
			uint32_t num_celltypes = 0;
			CellType* celltypes = nullptr;
			Validate(GetAllCellTypes(num_celltypes, celltypes), "GetAllCellTypes()");
			FreeListOfCellType(celltypes, num_celltypes);
		}
		std::cout << "*************************" << std::endl;
	}

	static void Test_QualityControl()
	{
		std::cout << "*****Test_QualityControl()*****" << std::endl;
		QualityControl_t* qualitycontrols;
		uint32_t num_qcs;
		Validate(GetQualityControlList(qualitycontrols, num_qcs), "GetQualityControlList()");
		FreeListOfQualityControl(qualitycontrols, num_qcs);

		num_qcs = 0;
		qualitycontrols = nullptr;
		Validate(GetActiveQualityControlList(qualitycontrols, num_qcs), "GetActiveQualityControlList()");
		FreeListOfQualityControl(qualitycontrols, num_qcs);

		QualityControl_t qualityControl = *(new QualityControl_t());
		qualityControl.qc_name = "Test #QC name1";
		qualityControl.lot_information = "Test #QC LI1";
		qualityControl.comment_text = "Test #QC CT1";
		Validate(AddQualityControl(qualityControl), "AddQualityControl()");
		Validate(RemoveQualityControl(qualityControl.qc_name), "RemoveQualityControl()");
		std::cout << "*******************************" << std::endl;
	}

	static void Test_ReagentPack()
	{
		std::cout << "*****Test_ReagentPack()********" << std::endl;
		{
			uint16_t container_num = 0;
			ReagentContainerState* status = nullptr;
			Validate(GetReagentContainerStatus(container_num, status), "GetReagentContainerStatus()");

			uint16_t num_reagents = 0;
			ReagentContainerState* statusList = nullptr;
			Validate(GetReagentContainerStatusAll(num_reagents, statusList), "GetReagentContainerStatusAll()");
			
			if (status != nullptr)
			{
				//FreeReagentState(status->reagent_states, status->num_reagents);
			}
			FreeListOfReagentContainerState(statusList, num_reagents);
		}

		{
			uint32_t num_reagents = 0;
			ReagentDefinition* reagents = nullptr;
			Validate(GetReagentDefinitions(num_reagents, reagents), "GetReagentDefinitions()");
			FreeReagentDefinitions(reagents, num_reagents);
		}

		{
			auto callback = [](eFlushFlowCellState state)
			{
				eFlushFlowCellState getState;
				Validate(GetFlushFlowCellState(getState), "GetFlushFlowCellState()");
				if (state == getState)
				{
					std::cout << "eFlushFlowCellState : " << (int32_t)state << std::endl;
				}
				else
				{
					std::cout << "eFlushFlowCellState : Something went wrong!!!" << std::endl;
				}
				if (state == eFlushFlowCellState::ffc_Failed
					|| state == eFlushFlowCellState::ffc_Idle
					|| state == eFlushFlowCellState::ffc_Completed)
				{
					setPromise(int32_t(state));
				}
			};

			promise_ = std::make_pair(false, boost::promise<int>());
			auto future = promise_.second.get_future();
			Validate(StartFlushFlowCell(callback), "StartFlushFlowCell()");
			future.get();

			std::cout << std::endl << std::endl;

			promise_ = std::make_pair(false, boost::promise<int>());
			future = promise_.second.get_future();
			Validate(StartFlushFlowCell(callback), "StartFlushFlowCell()");
			Sleep(500);
			Validate(CancelFlushFlowCell(), "CancelFlushFlowCell()");
			future.get();
		}

		{
			std::cout << std::endl << std::endl;
			auto callback = [](eDecontaminateFlowCellState state)
			{
				eDecontaminateFlowCellState getState;
				Validate(GetDecontaminateFlowCellState(getState), "GetDecontaminateFlowCellState()");
				if (state == getState)
				{
					std::cout << "eDecontaminateFlowCellState : " << (int32_t)state << std::endl;
				}
				else
				{
					std::cout << "eDecontaminateFlowCellState : Something went wrong!!!" << std::endl;
				}
				if (state == eDecontaminateFlowCellState::dfc_Failed
					|| state == eDecontaminateFlowCellState::dfc_Idle
					|| state == eDecontaminateFlowCellState::dfc_Completed)
				{
					setPromise(int32_t(state));
				}
			};
			promise_ = std::make_pair(false, boost::promise<int>());
			auto future = promise_.second.get_future();
			Validate(StartDecontaminateFlowCell(callback), "StartDecontaminateFlowCell()");
			future.get();

			std::cout << std::endl << std::endl;

			promise_ = std::make_pair(false, boost::promise<int>());
			Validate(StartDecontaminateFlowCell(callback), "StartDecontaminateFlowCell()");
			Sleep(500);
			Validate(CancelDecontaminateFlowCell(), "CancelDecontaminateFlowCell()");
		}

		{
			std::cout << std::endl << std::endl;
			auto callback = [](ePrimeReagentLinesState state)
			{
				ePrimeReagentLinesState getState;
				Validate(GetPrimeReagentLinesState(getState), "GetPrimeReagentLinesState()");
				if (state == getState)
				{
					std::cout << "ePrimeReagentLinesState : " << (int32_t)state << std::endl;
				}
				else
				{
					std::cout << "ePrimeReagentLinesState : Something went wrong!!!" << std::endl;
				}
				if (state == ePrimeReagentLinesState::prl_Failed
					|| state == ePrimeReagentLinesState::prl_Idle
					|| state == ePrimeReagentLinesState::prl_Completed)
				{
					setPromise(int32_t(state));
				}
			};
			promise_ = std::make_pair(false, boost::promise<int>());
			auto future = promise_.second.get_future();
			Validate(StartPrimeReagentLines(callback), "StartPrimeReagentLines()");
			future.get();

			std::cout << std::endl << std::endl << std::endl;

			promise_ = std::make_pair(false, boost::promise<int>());
			Validate(StartPrimeReagentLines(callback), "StartPrimeReagentLines()");
			Sleep(500);
			Validate(CancelPrimeReagentLines(), "CancelPrimeReagentLines()");
		}


		std::cout << "*******************************" << std::endl;
	}

	static void Test_Results()
	{
		std::cout << "*****Test_Results()********" << std::endl;
		{
			/*uuid__t id;
			WorkQueueRecord* rec;
			RetrieveWorkQueue(id, rec);

			WorkQueueRecord* reclist;
			uint32_t list_size;
			RetrieveWorkQueues(rec->time_stamp, rec->time_stamp + 10, rec->user_id, reclist, list_size);

			FreeWorkQueueRecord(reclist, list_size);*/
		}

		{
			/*uuid__t id;
			SampleRecord* rec;
			RetrieveSampleRecord(id, rec);
			SampleRecord* reclist;
			uint32_t list_size;
			RetrieveSampleRecords(rec->time_stamp, rec->time_stamp + 10, rec->user_id, reclist, list_size);

			FreeSampleRecord(reclist, list_size);

			const char* bioprocess_name = rec->sample_identifier;
			RetrieveSampleRecordsForBioprocess(bioprocess_name, reclist, list_size);

			FreeSampleRecord(reclist, list_size);

			const char* QC_name = rec->bp_qc_identifier;
			RetrieveSampleRecordsForQualityControl(QC_name, reclist, list_size);

			FreeSampleRecord(reclist, list_size);*/
		}

		{
			/*uuid__t id;
			SampleImageSetRecord* rec;
			RetrieveSampleImageSetRecord(id, rec);
			SampleImageSetRecord* reclist;
			uint32_t list_size;
			RetrieveSampleImageSetRecords(rec->time_stamp, rec->time_stamp + 10, rec->user_id, reclist, list_size);

			FreeImageSetRecord(reclist, list_size);*/
		}

		{
			/*uuid__t id;
			ImageRecord* rec;
			RetrieveImageRecord(id, rec);
			ImageRecord* reclist;
			uint32_t list_size;
			RetrieveImageRecords(rec->time_stamp, rec->time_stamp + 10, rec->user_id, reclist, list_size);

			FreeImageRecord(reclist, list_size);*/
		}

		uuid__t id;
		std::string constId = "ABCDEFGHIJKLMNO";
		strncpy_s((char*)id.u, sizeof(id.u), constId.c_str(), constId.size());

		{
			ResultRecord* rec = nullptr;
			Validate(RetrieveResultRecord(id, rec), "RetrieveResultRecord()");
			ResultSummary* reclist = nullptr;
			uint32_t list_size = 0;
			if (rec != nullptr)
			{
				Validate(RetrieveResultSummaries(rec->summary_info.time_stamp, rec->summary_info.time_stamp + 10, rec->summary_info.user_id, reclist, list_size), "RetrieveResultRecords()");
			}

			uuid__t record_id = id;
			DataSignature_t* signature = new DataSignature_t();
			signature->short_text = "ShortText";
			signature->long_text = "This is long text!";
			AddSignatureDefinition(signature);
			Validate(SignResultRecord(record_id, signature->short_text, 0), "SignResultRecord()");
			RemoveSignatureDefinition(signature->short_text, 0);
			FreeListOfResultSummary(reclist, list_size);
		}

		{
			DetailedResultMeasurements* measurements;
			Validate(RetrieveDetailedMeasurementsForResultRecord(id, measurements), "RetrieveDetailedMeasurementsForResultRecord()");

			FreeDetailedResultMeasurement(measurements);
		}

		{
			uuid__t image_id;
			std::string constId = "image__id__1";
			strncpy_s((char*)image_id.u, sizeof(image_id.u), constId.c_str(), constId.size());

			ImageWrapper_t* orgImg = nullptr;
			auto status = RetrieveImage(image_id, orgImg);
			Validate(status, "RetrieveImage");
			if (status == HawkeyeError::eSuccess && showImages_)
			{
				cvShowImage("orgImg", &(ConvertToIplImage(*orgImg)));
			}

			ImageWrapper_t* annotatedImg = nullptr;
			status = RetrieveAnnotatedImage(id, image_id, annotatedImg);
			Validate(status, "RetrieveAnnotatedImage");
			if (status == HawkeyeError::eSuccess && showImages_)
			{
				cvShowImage("annotatedImg", &(ConvertToIplImage(*annotatedImg)));
			}

			ImageWrapper_t* bwImg = nullptr;
			status = RetrieveBWImage(image_id, bwImg);
			Validate(status, "RetrieveBWImage");
			if (status == HawkeyeError::eSuccess && showImages_)
			{
				cvShowImage("bwImg", &(ConvertToIplImage(*bwImg)));
			}

			if (showImages_)
			{
				cvWaitKey(0);
			}
		}

		{
			DataSignature_t* signatures;
			uint16_t num_signatures;
			Validate(RetrieveSignatureDefinitions(signatures, num_signatures), "RetrieveSignatureDefinitions()");
		}

		{
			bool poi = true;
			Hawkeye::Characteristic_t ch;
			ch.key = static_cast<uint16_t>(BaseCharacteristicKey_t::DiameterInMicrons);
			uint8_t num_bins = 8;
			histogrambin_t* binData = nullptr;

			Validate(RetrieveHistogramForResultRecord(id, poi, ch, num_bins, binData), "RetrieveHistogramForResultRecord()");

			poi = false;
			ch.key = static_cast<uint16_t>(BaseCharacteristicKey_t::AvgSpotBrightness);
			binData = nullptr;
			Validate(RetrieveHistogramForResultRecord(id, poi, ch, num_bins, binData), "RetrieveHistogramForResultRecord()");

			{
				uuid__t sample_id = {};
				uint32_t ctIndex = 0;
				uint32_t adIndex = 0;

				auto cb = [](HawkeyeError er,uuid__t sampleId, ResultRecord* ans)
				{
					std::cout << "ReanalyzeSample - callback" << std::endl;
					setPromise(0);
				};

				promise_ = std::make_pair(false, boost::promise<int>());
				auto future = promise_.second.get_future();
				Validate(ReanalyzeSample(sample_id, ctIndex, adIndex, false, cb), "ReanalyzeSample");
				future.get();
				
				SampleRecord* sr = nullptr;
				uint32_t size = 0;
				Validate(RetrieveSampleRecords(0, 0, nullptr, sr, size), "RetrieveSampleRecords");
				if (size > 0)
				{
					auto sampleRec = sr[0];
					ResultRecord* rr = nullptr;
					RetrieveResultRecord(sampleRec.result_summaries[0].uuid, rr);

					ctIndex = rr->summary_info.cell_type_settings.celltype_index;
					adIndex = rr->summary_info.analysis_settings.analysis_index;

					promise_ = std::make_pair(false, boost::promise<int>());
					future = promise_.second.get_future();
					Validate(ReanalyzeSample(sampleRec.uuid, ctIndex, adIndex, false, cb), "ReanalyzeSample");
					future.get();

					ResultRecord* newRr = nullptr;
					RetrieveResultRecord(sampleRec.result_summaries[sampleRec.num_result_summary - 1].uuid, newRr);

					if (memcmp(rr, newRr, sizeof(rr)))
					{
						Validate(HawkeyeError::eValidationFailed, "ReanalyzeSample");
					}
				}
			}
		}

		std::cout << "***************************" << std::endl;
	}

	static void Test_Signatures()
	{
		std::cout << "*****Test_Signatures()********" << std::endl;

		DataSignature_t* sigs = nullptr;
		uint16_t num_sigs = 0;
		Validate(RetrieveSignatureDefinitions(sigs, num_sigs), "RetrieveSignatureDefinitions");
		std::cout << "Total Signatures : " << num_sigs << std::endl;		

		DataSignature_t* signature = new DataSignature_t();
		signature->short_text = "ShortText";
		signature->long_text = "This is long text!";

		Validate(AddSignatureDefinition(signature), "AddSignatureDefinition()");

		sigs = nullptr;
		num_sigs = 0;
		Validate(RetrieveSignatureDefinitions(sigs, num_sigs), "RetrieveSignatureDefinitions");
		std::cout << "Total Signatures : " << num_sigs << std::endl;

		std::string srtSig = std::string(signature->short_text);
		Validate(RemoveSignatureDefinition((char*)srtSig.c_str(), sizeof(srtSig)), "RemoveSignatureDefinition()");

		sigs = nullptr;
		num_sigs = 0;
		Validate(RetrieveSignatureDefinitions(sigs, num_sigs), "RetrieveSignatureDefinitions");
		std::cout << "Total Signatures : " << num_sigs << std::endl;

		std::cout << "******************************" << std::endl;
	}

	static void Test_SystemStatus()
	{
		std::cout << "*****Test_SystemStatus()********" << std::endl;
		SystemStatusData* status;
		GetSystemStatus(status);
		FreeSystemStatus(status);
		std::cout << "********************************" << std::endl;
	}

	static void Test_CommonUtilities()
	{
		std::cout << "*****Test_Commonutilities()********" << std::endl;		
    
		std::string timeStamp = "Jan 01 2000 00:00:00";
		auto tp = ChronoUtilities::ConvertToTimePoint(timeStamp);
		auto days = ChronoUtilities::ConvertFromTimePoint<duration_days>(tp);

		auto tp_sec = std::chrono::time_point_cast<std::chrono::seconds>(tp);
		auto sec = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(tp_sec);
    
		auto newTp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(sec);
		auto newDays = ChronoUtilities::ConvertFromTimePoint<duration_days>(newTp);

		assert(days == newDays);
		assert(tp == newTp);
		assert(tp == tp_sec);

		std::cout << ChronoUtilities::ConvertFromTimePoint<duration_days>(tp);

		std::cout << "***********************************" << std::endl;
	}

	/*static void Test_DeadlineTimerHelper()
	{
		std::cout << "*****Test_DeadlineTimerHelper()********" << std::endl;
		boost::this_thread::sleep_for(boost::chrono::seconds{ 2 });
		{
			auto timer_tick = [](uint32_t rTime, SupportedTimerCountdown tcd)
			{
				std::cout << "Remaining Time (sec) : " << rTime << std::endl;
			};

			uint32_t ms = 10000;
			auto wait = [&]()
			{
				std::cout << "Waiting for " << ms << " milisec" << std::endl;
				bool success = DeadlineTimerHelper::Instance().wait(
					ms, SupportedTimerCountdown::timer_seconds, timer_tick);
				std::cout << "Waiting Completed " << success << std::endl;
			};
			auto cancel = [&]()
			{
				std::cout << "Waiting for Cancelling" << std::endl;
				bool success = DeadlineTimerHelper::Instance().cancel();
				std::cout << "Cancellation Completed " << success << std::endl;
			};
			boost::thread t1{ wait };
			boost::this_thread::sleep_for(boost::chrono::seconds{ 2 });
			boost::thread t2{ wait };
			t1.join();
			t2.join();

			boost::thread t3{ wait };
			boost::this_thread::sleep_for(boost::chrono::seconds(5));
			boost::thread t4{ cancel };
			t3.join();
			t4.join();
		}

		std::cout << "***************************************" << std::endl;
	}*/

	static void CallBackAutoFocus(
		eAutofocusState state, AutofocusResults* result)
	{
		std::cout << "State : " << (int)state << std::endl;
		if (state == eAutofocusState::af_WaitingForFocusAcceptance)
		{			
			setPromise((int)state);
		}
		if (state == eAutofocusState::af_Idle)
		{
			std::cout << "Finished AutoFocus : " << (int)state << std::endl;
			FreeAutofocusResults(result, 1);
			setPromise((int)state);
		}
	}
  
	static void Test_Services(char* password)
	{
		std::cout << "*****Test_Services()********" << std::endl;
		Validate(svc_CameraFocusAdjust(true, true), "svc_CameraFocusAdjust");
		Validate(svc_CameraFocusAdjust(true, false), "svc_CameraFocusAdjust");
		Validate(svc_CameraFocusAdjust(false, true), "svc_CameraFocusAdjust");
		Validate(svc_CameraFocusAdjust(false, false), "svc_CameraFocusAdjust");
						
		Validate(svc_CameraFocusStoreCurrentPosition(password), "svc_CameraFocusStoreCurrentPosition");

		Validate(svc_CameraFocusRestoreToSavedLocation(), "svc_CameraFocusRestoreToSavedLocation");

		auto flowCellDepthvalidation = [&](const uint16_t fcd)
		{
			auto flowCellDepth = (uint16_t)fcd;
			Validate(svc_SetFlowCellDepthSetting(flowCellDepth), std::string("svc_SetFlowCellDepthSetting" + std::to_string(fcd)).c_str());

			uint16_t getFlowCellDepth = 0;
			Validate(svc_GetFlowCellDepthSetting(getFlowCellDepth), std::string("svc_GetFlowCellDepthSetting" + std::to_string(fcd)).c_str());

			if (flowCellDepth != getFlowCellDepth)
			{
				Validate(HawkeyeError::eValidationFailed, std::string("svc_GetFlowCellDepthSetting" + std::to_string(fcd)).c_str());
			}
		};

		for (uint16_t index = 50; index <= 100; index++)
		{
			flowCellDepthvalidation(index);
		}
		flowCellDepthvalidation(70);

		auto timer_tick = [](uint32_t rTime)
		{
			std::cout << "Remaining Time (sec) " << rTime << std::endl;
		};

		SamplePosition samplePosition = SamplePosition();
		samplePosition.setRowColumn('Z', 10);
		autofocus_state_callback_t callBack = &CallBackAutoFocus;

		promise_ = std::make_pair(false, boost::promise<int>());
		auto future = promise_.second.get_future();
		Validate(svc_CameraAutoFocus(samplePosition, callBack, timer_tick), "svc_CameraAutoFocus");
		boost::this_thread::sleep_for(boost::chrono::seconds(30));
		Validate(svc_CameraAutoFocus_ServiceSkipDelay(), "svc_CameraAutoFocus_ServiceSkipDelay");
		auto value = future.get();

		Sleep(100);
		promise_ = std::make_pair(false, boost::promise<int>());
		future = promise_.second.get_future();
		Validate(svc_CameraAutoFocus_FocusAcceptance(
			eAutofocusCompletion::afc_Accept), "svc_CameraAutoFocus_FocusAcceptance");
		value = future.get();
		std::cout << "Final State is : " << value << std::endl;

		promise_ = std::make_pair(false, boost::promise<int>());
		Validate(svc_CameraAutoFocus(samplePosition, callBack, nullptr), "svc_CameraAutoFocus_with_cancel");

		Sleep(2500);
		future = promise_.second.get_future();
		Validate(svc_CameraAutoFocus_Cancel(), "svc_CameraAutoFocus_Cancel");
		value = future.get();
		std::cout << "Final State is : " << value << std::endl;

		auto livehandler = [](HawkeyeError he, ImageWrapper_t* image)
		{
			//std::string windowName = "Live Scanning";
			if (he == HawkeyeError::eSuccess)
			{
				std::cout << "Live Scanning in Progress" << std::endl;
				//auto matImage = ConvertToMat(image);
				//cv::imshow(windowName, matImage);
				//cv::waitKey(10);
			}
			else
			{
				std::cout << "Live Scanning hardware fault" << std::endl;
				//cv::destroyWindow(windowName);
			}
		};

		promise_ = std::make_pair(false, boost::promise<int>());
		future = promise_.second.get_future();
		Validate(svc_CameraAutoFocus(samplePosition, callBack, nullptr), "svc_CameraAutoFocus");
		boost::this_thread::sleep_for(boost::chrono::seconds(10));

		Validate(svc_StartLiveImageFeed(livehandler), "svc_StartLiveImageFeed");
		boost::this_thread::sleep_for(boost::chrono::seconds(5));
		Validate(svc_StopLiveImageFeed(), "svc_StopLiveImageFeed");
		std::cout << "Live Scanning Stop in Progress" << std::endl;
		boost::this_thread::sleep_for(boost::chrono::seconds(3));

		//std::string windowName = "Live Scanning";
		//cv::destroyWindow(windowName);

		Validate(svc_CameraAutoFocus_Cancel(), "svc_CameraAutoFocus_Cancel");
		value = future.get();
		std::cout << "Final State is : " << value << std::endl;

		Validate(svc_ManualSample_Load(), "svc_ManualSample_Load");
		Validate(svc_ManualSample_Nudge(), "svc_ManualSample_Nudge");
		Validate(svc_ManualSample_Expel(), "svc_ManualSample_Expel");

		std::cout << "****************************" << std::endl;
	}

	static void Test_BrightfieldDustSubtract()
	{
		std::cout << "*****Test_BrightfieldDustSubtract()********" << std::endl;

		auto callBack = [](
			eBrightfieldDustSubtractionState state, ImageWrapper_t* dust_ref,
			uint16_t num_dust_images, ImageWrapper_t* source_dust_images)
		{
			std::cout << "BrightfieldDustSubtract State is :" << (int32_t)state << std::endl;
			if (state == eBrightfieldDustSubtractionState::bds_WaitingOnUserApproval)
			{
				setPromise(state);
			}

			if (state == eBrightfieldDustSubtractionState::bds_Idle)
			{
				setPromise(state);
			}

			if (state == eBrightfieldDustSubtractionState::bds_Failed)
			{
				std::cout << "---Failed!---" << std::endl;
				setPromise(state);
			}
		};

		{
			promise_ = std::make_pair(false, boost::promise<int>());
			auto future = promise_.second.get_future();
			Validate(StartBrightfieldDustSubtract(callBack), "StartBrightfieldDustSubtract");
			eBrightfieldDustSubtractionState state;
			Validate(GetBrightfieldDustSubtractState(state), "GetBrightfieldDustSubtractState");
			auto value = future.get();

			promise_ = std::make_pair(false, boost::promise<int>());
			future = promise_.second.get_future();
			Sleep(100);
			Validate(AcceptDustReference(true), "AcceptDustReference + true");
			value = future.get();
		}

		{
			promise_ = std::make_pair(false, boost::promise<int>());
			auto future = promise_.second.get_future();
			Validate(StartBrightfieldDustSubtract(callBack), "StartBrightfieldDustSubtract");
			auto value = future.get();

			promise_ = std::make_pair(false, boost::promise<int>());
			future = promise_.second.get_future();
			Sleep(100);
			Validate(AcceptDustReference(false), "AcceptDustReference + false");
			value = future.get();
		}

		{
			Validate(StartBrightfieldDustSubtract(callBack), "StartBrightfieldDustSubtract");
			Sleep(500);
			Validate(CancelBrightfieldDustSubtract(), "CancelBrightfieldDustSubtract");
		}

		std::cout << "*******************************************" << std::endl;
	}

	static void Test_Services_Analysis()
	{
		std::cout << "*****Test_Services_Analysis()********" << std::endl;

		{
			bool lampstate = true;
			float intensity = 50.0;
			Validate(svc_SetCameraLampState(lampstate, intensity), "svc_SetCameraLampState");

			bool newlampState = false;
			float newIntensity = 0.0;
			Validate(svc_GetCameraLampState(newlampState, newIntensity), "svc_SetCameraLampState");

			if (newlampState != lampstate || newIntensity != intensity)
			{
				Validate(HawkeyeError::eValidationFailed, "svc_GetCameraLampState");
			}

			intensity = 60.0;
			Validate(svc_SetCameraLampIntensity(intensity), "svc_SetCameraLampState");

			newlampState = false;
			newIntensity = 0.0;
			Validate(svc_GetCameraLampState(newlampState, newIntensity), "svc_SetCameraLampState");

			if (newlampState != lampstate || newIntensity != intensity)
			{
				Validate(HawkeyeError::eValidationFailed, "svc_GetCameraLampState");
			}
		}

		{
			uint32_t num_ad = 0;
			AnalysisDefinition* analyses = nullptr;
			Validate(GetAllAnalysisDefinitions(num_ad, analyses), "GetAllAnalysisDefinitions()");

			if (num_ad > 0 && analyses != nullptr)
			{
				AnalysisDefinition ad = analyses[num_ad - 1];
				Validate(SetTemporaryAnalysisDefinition(&ad), "SetTemporaryAnalysisDefinition");

				AnalysisDefinition* newAd = nullptr;
				Validate(GetTemporaryAnalysisDefinition(newAd), "GetTemporaryAnalysisDefinition");

				if (newAd == nullptr || newAd->analysis_index != ad.analysis_index)
				{
					Validate(HawkeyeError::eValidationFailed, "svc_GetTemporaryAnalysisDefinition");
				}

				Validate(SetTemporaryAnalysisDefinitionFromExisting(ad.analysis_index), "SetTemporaryAnalysisDefinitionFromExisting");

				newAd = nullptr;
				Validate(GetTemporaryAnalysisDefinition(newAd), "GetTemporaryAnalysisDefinition");

				if (newAd == nullptr || newAd->analysis_index != ad.analysis_index)
				{
					Validate(HawkeyeError::eValidationFailed, "GetTemporaryAnalysisDefinition_Index");
				}
			}

			FreeAnalysisDefinitions(analyses, num_ad);
		}

		{
			uint32_t num_ct = 0;
			CellType* celltypes = nullptr;
			Validate(GetAllCellTypes(num_ct, celltypes), "GetAllCellTypes()");

			if (num_ct > 0 && celltypes != nullptr)
			{
				auto ct = celltypes[num_ct - 1];
				Validate(SetTemporaryCellType(&ct), "SetTemporaryCellType");

				CellType* newCt = nullptr;
				Validate(GetTemporaryCellType(newCt), "GetTemporaryCellType");

				if (newCt == nullptr || newCt->celltype_index != ct.celltype_index)
				{
					Validate(HawkeyeError::eValidationFailed, "GetTemporaryCellType");
				}

				Validate(SetTemporaryCellTypeFromExisting(ct.celltype_index), "svc_SetTemporaryCellTypeFromExisting");

				newCt = nullptr;
				Validate(GetTemporaryCellType(newCt), "GetTemporaryCellType");

				if (newCt == nullptr || newCt->celltype_index != ct.celltype_index)
				{
					Validate(HawkeyeError::eValidationFailed, "GetTemporaryCellType");
				}
			}

			FreeListOfCellType(celltypes, num_ct);
		}

		auto cb = [](HawkeyeError he, BasicResultAnswers result, ImageWrapper_t* image)
		{
			if (he != HawkeyeError::eSuccess)
			{
				std::cout << "Single/Continuous Analysis error " << (int)he << std::endl;
			}
			std::cout << "Single/Continuous Analysis success " << result.average_cells_per_image << std::endl;
		};

		auto livehandler = [](HawkeyeError he, ImageWrapper_t* image)
		{
			//if (he != HawkeyeError::eSuccess)
			{
				std::cout << "Single/Continuous Live Image error " << (int)he << std::endl;
			}
		};

		SamplePosition samplePosition = SamplePosition();
		samplePosition.setRowColumn('Z', 10);
		autofocus_state_callback_t callBack = nullptr; // &CallBackAutoFocus;
		Validate(svc_CameraAutoFocus(samplePosition, callBack, nullptr), "svc_CameraAutoFocus");
		Sleep(10000);

		Validate(svc_StartLiveImageFeed(livehandler), "svc_StartLiveImageFeed");
		Sleep(3000);

		{
			std::cout << "Performing svc_GenerateSingleShotAnalysis......." << std::endl;
			Validate(svc_GenerateSingleShotAnalysis(cb), "svc_GenerateSingleShotAnalysis");
			Sleep(12000);
		}

		{
			std::cout << "Performing svc_GenerateContinuousAnalysis......." << std::endl;
			Validate(svc_GenerateContinuousAnalysis(cb), "svc_GenerateContinuousAnalysis");

			Sleep(30000);

			Validate(svc_StopContinuousAnalysis(), "svc_StopContinuousAnalysis");
			Sleep(2000);
		}

		Validate(svc_StopLiveImageFeed(), "svc_StopLiveImageFeed");
		Sleep(5000);

		Validate(svc_CameraAutoFocus_ServiceSkipDelay(), "svc_CameraAutoFocus_ServiceSkipDelay");
		Validate(svc_CameraAutoFocus_Cancel(), "svc_CameraAutoFocus_Cancel");
		std::cout << "*************************************" << std::endl;
	}

	static void Test_UserManagement(char* password)
	{
		std::cout << "*****Test_UserManagement()********" << std::endl;
		const int numberOfDays = 50;
		Validate(SetUserPasswordExpiration(numberOfDays), "SetUserPasswordExpiration");
		char** userList; uint32_t nUsers = 0;
		GetUserList(false, userList, nUsers);
		for (size_t i = 0; i < nUsers; i++)
		{
			uint32_t nMembers;
			char** propertymembers;
			if (GetUserProperty(userList[i], "PasswordExpiry", nMembers, propertymembers)
				== HawkeyeError::eSuccess)
			{
				for (size_t j = 0; j < nMembers; j++)
				{
					uint16_t days = 0;
					GetUserPasswordExpiration_Setting(days);
					uint16_t rDays = 0;
					GetUserPasswordExpiration(rDays);

					if (rDays == days && rDays == numberOfDays)
					{
						continue;
					}

					std::cout << "User Name : " << userList[i] << " password expiry date not matching!"
						<< std::endl;
				}
			}
		}

		std::cout << "**********************************" << std::endl;
	}

	static void Test_Reports()
	{
		std::cout << "*****Test_Reports()********" << std::endl;

		auto printAuditLog = [=](audit_log_entry& entry) -> void
		{
			std::cout << std::endl;
			std::cout << "user_id : \n" << std::string(entry.active_user_id ? entry.active_user_id : "") << "\n";
			std::cout << "eventType : \n" << (entry.event_type) << "\n";
			std::cout << "timestamp : \n" << (entry.timestamp) << "\n";
			std::cout << std::endl;
			std::cout << "event_message : \n" << (entry.event_message) << "\n";
			std::cout << std::endl;
		};

		uint32_t num_entries;
		audit_log_entry* log_entries = nullptr;
		Validate(RetrieveAuditTrailLogRange(0, 0, num_entries, log_entries), "RetrieveAuditTrailLogRange");

		for (uint32_t index = 0; index < num_entries; index++)
		{
			printAuditLog(log_entries[index]);
		}
		FreeAuditLogEntry(log_entries, num_entries);

		std::cout << "---------------------------------------"<< std::endl;

		num_entries = 0;
		log_entries = nullptr;
		Validate(RetrieveAuditTrailLogRange(0, 1547836030, num_entries, log_entries), "RetrieveAuditTrailLogRange");

		for (uint32_t index = 0; index < num_entries; index++)
		{
			printAuditLog(log_entries[index]);
		}
		FreeAuditLogEntry(log_entries, num_entries);

		std::cout << "---------------------------------------" << std::endl;

		num_entries = 0;
		log_entries = nullptr;
		Validate(RetrieveAuditTrailLogRange(1547836030, 0, num_entries, log_entries), "RetrieveAuditTrailLogRange");

		for (uint32_t index = 0; index < num_entries; index++)
		{
			printAuditLog(log_entries[index]);
		}
		FreeAuditLogEntry(log_entries, num_entries);

		std::cout << "---------------------------------------" << std::endl;
		std::cout << "---------------------------------------" << std::endl;

		char* archiveLocation = nullptr;
		Validate(ArchiveAuditTrailLog(1547836030, "bci_admin", archiveLocation), "ArchiveAuditTrailLog");

		std::cout << "Archive Location for Audit Log : " << archiveLocation << std::endl;

		std::cout << "---------------------------------------" << std::endl;
		std::cout << "---------------------------------------" << std::endl;

		num_entries = 0;
		log_entries = nullptr;
		Validate(ReadArchivedAuditTrailLog(archiveLocation, num_entries, log_entries), "ReadArchivedAuditTrailLog");

		for (uint32_t index = 0; index < num_entries; index++)
		{
			printAuditLog(log_entries[index]);
		}
		FreeAuditLogEntry(log_entries, num_entries);

		std::cout << "***************************" << std::endl;
	}

	static void Test_StageController()
	{
		std::cout << "*****Test_StageController()********" << std::endl;

		InitializeCarrier();
		SamplePosition pos;
		pos.row = 'D';
		pos.col = 7;
		svc_SetSampleWellPosition(pos);

		bool down = true;
		svc_MoveProbe(down);
		svc_MoveProbe(!down);

		std::cout << "***********************************" << std::endl;
	}
};

std::pair<bool, boost::promise<int>> TestAPIs::promise_{};
std::vector<std::string> TestAPIs::failedCalls_;
bool TestAPIs::showImages_ = false;
