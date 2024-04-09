#include "RetrieveAPIsTest.hpp"
#include "ChronoUtilities.hpp"
#include <boost/format.hpp>

static std::string get_string_from_char_ptr(char * data_ptr)
{
	if (data_ptr)
		return std::string(data_ptr);

	return "EMPTY";
}

static void print_ap(const AnalysisParameter* ap, uint32_t num_ct)
{
	for (size_t index = 0; index < num_ct; index++)
	{
		std::cout << "\n*********************************************************************************\n";
		std::cout << "Analysis Params # : " << index
			<< "\n                  label : " << get_string_from_char_ptr(ap[index].label)
			<< "\n     Characteristic_key : " << ap[index].characteristic.key
			<< "\n    Characteristic_skey : " << ap[index].characteristic.s_key
			<< "\n   Characteristic_sskey : " << ap[index].characteristic.s_s_key
			<< "\n         ThresholdValue : " << ap[index].threshold_value
			<< "\n         AboveThreshold : " << ap[index].above_threshold << std::endl;
	}
}

static void print_ad(const AnalysisDefinition* ad, uint32_t num_ct)
{
	for (size_t index = 0; index < num_ct; index++)
	{
		std::cout << "\n*********************************************************************************\n";
		std::cout << "Analysis Definitions # : " << index
			<< "\n          Index : " << ad[index].analysis_index
			<< "\n          Label : " << get_string_from_char_ptr(ad[index].label)
			<< "\n   MixingCycles : " << (int)ad[index].mixing_cycles << std::endl;

		for (size_t ri = 0; ri < ad[index].num_reagents; ri++)
		{
			std::cout << "\n   Reagent Indices : " << ad[index].reagent_indices[ri] << std::endl;
		}
		for (size_t fli = 0; fli < ad[index].num_fl_illuminators; fli++)
		{
			std::cout << "\tFl_illuminators : ";
			std::cout << "\n      Emission_Wl: " << ad[index].fl_illuminators[fli].emission_wavelength_nm << std::endl;
			std::cout << "\n      Exposure_Wl: " << ad[index].fl_illuminators[fli].exposure_time_ms << std::endl;
			std::cout << "\n   Illuminator_Wl: " << ad[index].fl_illuminators[fli].illuminator_wavelength_nm << std::endl;
		}
		print_ap(ad[index].analysis_parameters, ad[index].num_analysis_parameters);
	}
}

static void print_ct(const CellType* ct, uint32_t num_ct)
{
	for (size_t index = 0; index < num_ct; index++)
	{
		std::cout << "\n*********************************************************************************\n";
		std::cout << " Cell Type # : " << index
			<< "\n              Index : " << ct[index].celltype_index
			<< "\n              Label : " << ct[index].label
			<< "\n      MaxImageCount : " << ct[index].max_image_count
			<< "\n   AspirationCycles : " << ct[index].aspiration_cycles
			<< "\n       ROI_X_Pixels : " << ct[index].roi_x_pixels
			<< "\n       ROI_Y_Pixels : " << ct[index].roi_y_pixels
			<< "\n             MinDia : " << ct[index].minimum_diameter_um
			<< "\n             MaxDia : " << ct[index].maximum_diameter_um
			<< "\n     MinCircularity : " << ct[index].minimum_circularity
			<< "\n     SharpnessLimit : " << ct[index].sharpness_limit
			<< "\n   DeclusterSetting : " << ct[index].decluster_setting
			<< "\n      Fl_ROI_Extent : " << ct[index].fl_roi_extent;
		print_ap(ct[index].cell_identification_parameters, ct[index].num_cell_identification_parameters);
		print_ad(ct[index].analysis_specializations, ct[index].num_analysis_specializations);
	}
}

static void print_rs(const ResultSummary* p_rs, uint32_t rr_ct)
{
	std::cout << "\n*********************************************************************************\n";
	for (uint32_t index = 0; index < rr_ct; index++)
	{
		std::cout << "\nResult Summary Data[" << index << "]:"
			<< "\n                     uuid : " << RetrieveAPIsTest::getUUIDStr(p_rs[index].uuid)
			<< "\n                  User_id : " << get_string_from_char_ptr(p_rs[index].user_id)
			<< "\n               Time Stamp : " << p_rs[index].time_stamp
			<< "\n        Num of Signatures : " << p_rs[index].num_signatures
			<< "\n BasicResAns-ImgProStatus : " << p_rs[index].cumulative_result.eProcessedStatus
			<< "\n BasicResAns-TtlNumImages : " << p_rs[index].cumulative_result.nTotalCumulative_Imgs
			<< "\n      avg bf bg intensity : " << p_rs[index].cumulative_result.average_brightfield_bg_intensity
			<< "\n      avg cells per image : " << p_rs[index].cumulative_result.average_cells_per_image
			<< "\n avg circularity interest : " << p_rs[index].cumulative_result.avg_circularity_ofinterest
			<< "\n      avg circularity POP : " << p_rs[index].cumulative_result.avg_circularity_pop
			<< "\n avg diameter of interest : " << p_rs[index].cumulative_result.avg_diameter_ofinterest
			<< "\n         avg diameter pop : " << p_rs[index].cumulative_result.avg_diameter_pop
			<< "\n    concentration general : " << p_rs[index].cumulative_result.concentration_general
			<< "\n            AnalysisIndex : " << p_rs[index].cell_type_settings.celltype_index
			<< "\n            CellTypeIndex : " << p_rs[index].analysis_settings.analysis_index
			<< std::endl;

		//print_ct(&p_ir[index].cell_type_settings, 1);
		//print_ad(&p_ir[index].analysis_settings, 1);

		for (uint32_t sigIndex = 0; sigIndex < p_rs[index].num_signatures; sigIndex++)
		{
			std::cout << " Signature - " << sigIndex << std::endl;
			std::cout << "      User - " << get_string_from_char_ptr(p_rs[index].signature_set[sigIndex].signing_user) << std::endl;
			std::cout << "Time Stamp - " << p_rs[index].signature_set[sigIndex].timestamp << std::endl;
			std::cout << "Short Text - " << get_string_from_char_ptr(p_rs[index].signature_set[sigIndex].signature.short_text) << std::endl;
			std::cout << " Long Text - " << get_string_from_char_ptr(p_rs[index].signature_set[sigIndex].signature.long_text) << std::endl;
		}
		std::cout << "\n*********************************************************************************\n";
	}
}

static void print_rr(const ResultRecord* p_rr, uint32_t rr_ct)
{
	std::cout << "\n*********************************************************************************\n";
	for (uint32_t index = 0; index < rr_ct; index++)
	{
		std::cout << "\nResult Record Data[" << index << "]:" << std::endl;
		std::cout << "Num of Per Image Results : " << p_rr[index].num_image_results;
		print_rs(&p_rr[index].summary_info, 1);
	}
}

static void print_drm(const DetailedResultMeasurements* p_drm, uint32_t drm_ct)
{
	std::cout << "\n*********************************************************************************\n";
	for (uint32_t index = 0; index < drm_ct; index++)
	{
		std::cout << "\nDetailed Measurements Data[" << index << "]:" << std::endl;
		std::cout << "Num of Images : " << p_drm[index].num_image_sets << std::endl;
	}
}

static bool InBounds(uint32_t val, uint32_t lo, uint32_t high)
{
	return  val >= lo && val <= high;
}

void PrintSampleRecordList(SampleRecord* sr_list, uint32_t list_size)
{
	for (std::size_t i = 0; i < list_size; i++)
	{
		std::cout << boost::str(boost::format("%3d. Sample %s | %2d results | %3d image sets\n")
								% (i + 1) % sr_list[i].sample_identifier
								% sr_list[i].num_result_summary 
								% sr_list[i].num_image_sets);
	}
}

void RetrieveAPIsTest::WorkqueueRecordTest()
{
	uint16_t failed_count = 0;
	uint32_t listsize = 0;
	std::cout << "WorkQueue Record : <Test Start>" << std::endl;
	auto print = [](auto wqlist, uint32_t list_size)
	{
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < list_size; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "WorkQueue Data[" << index << "]:\n" << "uuid : " << getUUIDStr(wqlist[index].uuid)
				<< "\nUser_id: " << get_string_from_char_ptr(wqlist[index].user_id) << "\nLabel: " << get_string_from_char_ptr(wqlist[index].wqLabel) <<"\nTime Stamp: " << wqlist[index].time_stamp << std::endl;

			for (auto i = 0; i < wqlist[index].num_sample_records; i++)
			{
				std::cout << " \tSample record[" << i << "]:" << getUUIDStr(wqlist[index].sample_records[i]) << std::endl;
			}
		}
		std::cout << "\n*********************************************************************************\n";
	};
	WorkQueueRecord *p_wqr;
	char * user = "";
	auto he = RetrieveWorkQueues(0, 0, user, p_wqr, listsize);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveWorkQueues---<FAILED> " + std::string(std::string(HawkeyeErrorAsString(he))) << std::endl;
	}

	print(p_wqr, listsize);
	uuid__t* wqUUIDList = wqUUIDList = new uuid__t[listsize]();
	for (uint32_t index = 0; index < listsize; index++)
		wqUUIDList[index] = p_wqr[index].uuid;

	if (listsize > 0)
	{
		WorkQueueRecord *pWq;
		he = RetrieveWorkQueue(wqUUIDList[0], pWq);
		if (he != HawkeyeError::eSuccess)
		{
			failed_count++;
			std::cout << "\n RetrieveWorkQueue---<FAILED> " + std::string(std::string(HawkeyeErrorAsString(he))) << std::endl;
		}
		else
		{
			print(pWq, 1);
		}
	}

	WorkQueueRecord *pWqList;
	uint32_t retreived_size = 0;
	he = RetrieveWorkQueueList(wqUUIDList, listsize, pWqList, retreived_size);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveWorkQueueList---<FAILED> " + std::string(std::string(HawkeyeErrorAsString(he))) << std::endl;
	}

	print(pWqList, retreived_size);

	if(retreived_size> 0)
	FreeListOfWorkQueueRecord(pWqList, retreived_size);

	if(failed_count!= 0)
		std::cout << "WorkQueue Record : <Test End FAILED>" << std::endl;
	else
		std::cout << "WorkQueue Record : <Test End PASSED>" << std::endl;
}

void RetrieveAPIsTest::SampleRecordTest()
{
	uint16_t failed_count = 0;
	uint32_t listsize = 0;
	std::cout << "SampleRecord Data: <Test Start>" << std::endl;

	auto print = [=](SampleRecord* p_sr, uint32_t list_size) {
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < list_size; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "Sample Record Data[" << index << "]:\n" << "uuid : " << getUUIDStr(p_sr[index].uuid)
				<< "\nUser_id: " << get_string_from_char_ptr(p_sr[index].user_id) << "\nTime Stamp: " << p_sr[index].time_stamp
				<< "\nDilution factor: " << p_sr[index].dilution_factor 
				<< "\nWashType: " << (int)p_sr[index].wash 
				<< "\nComment: " << get_string_from_char_ptr(p_sr[index].comment)<< std::endl;

			std::cout << "Reagent Information <Start>" << std::endl;
			for (auto i = 0; i < p_sr[index].num_reagent_records; i++)
			{
				std::cout << " ------------REAGENT INFO  ------------" << i << std::endl;
				std::cout << "\tPack number: " << p_sr[index].reagent_info_records[i].pack_number
					<< "\n\tLot Number: " << p_sr[index].reagent_info_records[i].lot_number
					<< "\n\tLabel: " << get_string_from_char_ptr(p_sr[index].reagent_info_records[i].reagent_label)
					<< "\n\tExpirationDate: " << p_sr[index].reagent_info_records[i].expiration_date
					<< "\n\tInserviceDate: " << p_sr[index].reagent_info_records[i].in_service_date
					<< "\n\tEffectiveExpirationDate: " << p_sr[index].reagent_info_records[i].effective_expiration_date
					<< std::endl;
			}
			std::cout << "Reagent Information <End>" << std::endl;
			for (auto i = 0; i < p_sr[index].num_image_sets; i++)
			{
				std::cout << "\tImage Sets[" << i << "]:" << getUUIDStr(p_sr[index].image_sets[i]) << std::endl;
			}

			for (auto i = 0; i < p_sr[index].num_result_summary; i++)
			{
				std::cout << "\tResult record[" << i << "]:" << getUUIDStr(p_sr[index].result_summaries[i].uuid) << std::endl;
			}
		}
		std::cout << "\n*********************************************************************************\n";
	};
	// Retrieve Sample image Record
	SampleRecord *p_sr;
	char * user = "";
	auto he = RetrieveSampleRecords(0, 0, user, p_sr, listsize);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveSampleRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
	}
	print(p_sr, listsize);


	uuid__t* UUIDList = new uuid__t[listsize]();
	for (uint32_t index = 0; index < listsize; index++)
		UUIDList[index] = p_sr[index].uuid;

	SampleRecord *pSr;
	if (listsize > 0)
	{
		he = RetrieveSampleRecord(UUIDList[0], pSr);
		if (he != HawkeyeError::eSuccess)
		{
			failed_count++;
			std::cout << "\n RetrieveSampleRecord---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		}
		else
			print(pSr, 1);
	}

	SampleRecord *pSrList;
	uint32_t retreived_size = 0;
	he = RetrieveSampleRecordList(UUIDList, listsize, pSrList, retreived_size);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveSampleRecordList---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
	}
	print(pSrList, retreived_size);
	
	FreeListOfSampleRecord(pSrList, retreived_size);

	if (failed_count != 0)
		std::cout << "SampleRecord : <Test End FAILED>" << std::endl;
	else
		std::cout << "SampleRecord : <Test End PASSED>" << std::endl;

}

void RetrieveAPIsTest::SampleImageSetRecordTest()
{
	uint32_t listsize = 0;
	uint16_t failed_count = 0;
	std::cout << "SampleImageSetRecord Data: <Test Start>" << std::endl;

	auto print = [=](auto p_sir, uint32_t listsize) {
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < listsize; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "Sample image Set Record Data[" << index << "]:\n" << "uuid : " << getUUIDStr(p_sir[index].uuid)
				<< "\nUser_id: " << get_string_from_char_ptr(p_sir[index].user_id) << "\nTime Stamp: " << p_sir[index].time_stamp <<
				"\nSeq_Num: " << p_sir[index].sequence_number <<
				"\nBf_UUID: " << getUUIDStr(p_sir[index].brightfield_image) << std::endl;
			auto numOfChannels = p_sir[index].num_fl_channels;
			if (numOfChannels > 0)
			{
				for (auto i = 0; i < p_sir[index].num_fl_channels; i++)
				{
					std::cout << "Fl_Channel[" << i << "]: " << p_sir[index].fl_channel_numbers[i] << std::endl;
					std::cout << "Fl_UUID [" << i << "]:" << getUUIDStr(p_sir[index].fl_images[i]) << std::endl;
				}
			}
		}
		std::cout << "\n*********************************************************************************\n";
	};
	// Retrieve SampleImage Set record
	SampleImageSetRecord *p_sir;
	char * user = "";
	auto he = RetrieveSampleImageSetRecords(0, 0, user, p_sir, listsize);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveSampleImageSetRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
	}
	print(p_sir, listsize);

	uuid__t* UUIDList = new uuid__t[listsize]();
	for (uint32_t index = 0; index < listsize; index++)
		UUIDList[index] = p_sir[index].uuid;

	SampleImageSetRecord *pSr;
	if (listsize > 0)
	{
		he = RetrieveSampleImageSetRecord(UUIDList[0], pSr);
		if (he != HawkeyeError::eSuccess)
		{
			failed_count++;
			std::cout << "\n RetrieveSampleImageSetRecord---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		}
		else
			print(pSr, 1);
	}

	SampleImageSetRecord *pSrList;
	uint32_t retreived_size = 0;
	he = RetrieveSampleImageSetRecordList(UUIDList, listsize, pSrList, retreived_size);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveSampleImageSetRecordList---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
	}
	print(pSrList, retreived_size);

	FreeListOfImageSetRecord(pSrList, retreived_size);

	if (failed_count != 0)
		std::cout << "SampleImageSetRecord : <Test End FAILED>" << std::endl;
	else
		std::cout << "SampleImageSetRecord : <Test End PASSED>" << std::endl;
}

void RetrieveAPIsTest::ImageRecordTest()
{
	uint16_t failed_count = 0;
	uint32_t listsize = 0;
	std::cout << "Image Record Data: <Test Start>" << std::endl;

	auto print = [=](auto p_ir, uint32_t listsize) {
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < listsize; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "Image Record Data[" << index << "]:\n" << "uuid : " << getUUIDStr(p_ir[index].uuid)
				<< "\nUser_id: " << get_string_from_char_ptr(p_ir[index].user_id) << "\nTime Stamp: " << p_ir[index].time_stamp << std::endl;
		}
		std::cout << "\n*********************************************************************************\n";
	};
	// Retrieve ImageRecord
	ImageRecord *p_ir;
	char * user = "";
	auto he = RetrieveImageRecords(0, 0, user, p_ir, listsize);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveImageRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
	}
	print(p_ir, listsize);

	uuid__t* UUIDList = new uuid__t[listsize]();
	for (uint32_t index = 0; index < listsize; index++)
	{
		UUIDList[index] = p_ir[index].uuid;
		imageRecUUIDList.push_back(UUIDList[index]);
	}

	ImageRecord *pSr;
	if (listsize > 0)
	{
		he = RetrieveImageRecord(UUIDList[0], pSr);
		if (he != HawkeyeError::eSuccess)
		{
			failed_count++;
			std::cout << "\n RetrieveImageRecord---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		}
		else
			print(pSr, 1);
	}

	ImageRecord *pSrList;
	uint32_t retreived_size;
	he = RetrieveImageRecordList(UUIDList, listsize, pSrList, retreived_size);
	if (he != HawkeyeError::eSuccess)
	{
		failed_count++;
		std::cout << "\n RetrieveImageRecordList---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
	}
	print(pSrList, retreived_size);

	FreeListOfImageRecord(pSrList, retreived_size);

	if (failed_count != 0)
		std::cout << "Image Record : <Test End FAILED>" << std::endl;
	else
		std::cout << "Image Record : <Test End PASSED>" << std::endl;
}

void RetrieveAPIsTest::ResultrecordTest(bool print_results)
{
	uint32_t listsize_sr = 0;
	SampleRecord *p_sr;

	auto retrieveSampleRecord = [](SampleRecord *&p_sr, uint32_t &listsize_sr)->bool
	{
		auto he = RetrieveSampleRecords(0, 0, "", p_sr, listsize_sr);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n RetrieveSampleRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
			return false;
		}
		return true;
	};
	
	auto displayAvailableSampleRecords = [&]()
	{
		for (uint32_t index = 0; index < listsize_sr; index++)
		{
			std::cout << index << ". UUID" << getUUIDStr(p_sr[index].uuid) << "\t|Sample ID: " << get_string_from_char_ptr(p_sr[index].sample_identifier) << "\t|User ID: " << get_string_from_char_ptr(p_sr[index].user_id) << std::endl;

			for (auto i = 0; i < p_sr[index].num_result_summary; i++)
			{
				std::cout << "Result record[" << i << "]:" << RetrieveAPIsTest::getUUIDStr(p_sr[index].result_summaries[i].uuid) << std::endl;
			}
		}
	};
	
	auto selectResultRecord = [&](uuid__t &res_uuid)->bool
	{
		uint32_t sample_index_value = {};
		uint32_t result_index_value = {};

		if (listsize_sr > 0)
		{
			//select a sample a index
			while (true)
			{
				std::string sample_index;
				std::cout << "Select the sample between : |0 - " << listsize_sr - 1 << "|" << std::endl;
				std::cin >> sample_index;
				sample_index_value = std::stoul(sample_index);

				if (InBounds(sample_index_value, 0, listsize_sr - 1))
					break;
				std::cout << "Invalid selection, try again" << std::endl;
			}
			//select a result record index
			while (true)
			{
				std::string result_index;
				std::cout << "Select the result between : |0 - " << p_sr[sample_index_value].num_result_summary - 1 << "|" << std::endl;
				std::cin >> result_index;
				result_index_value = std::stoul(result_index);

				if (InBounds(result_index_value, 0, p_sr[sample_index_value].num_result_summary - 1))
					break;
				std::cout << "Invalid selection, try again" << std::endl;
			}

			res_uuid = p_sr[sample_index_value].result_summaries[result_index_value].uuid;
			return true;
		}
		else
		{
			std::cout << "No Sample records found" << std::endl;
			return false;
		}
	};

	auto retrieveResultRecord = [=](uuid__t res_uuid)
	{
		auto start = std::chrono::system_clock::now();
		ResultRecord *p_rr = nullptr;
		auto he = RetrieveResultRecord(res_uuid, p_rr);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n RetrieveResultRecord---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
			return false;
		}
		if (print_results)
			print_rr(p_rr, 1);

		FreeListOfResultRecord(p_rr, 1);
		std::cout << "Successfully retrieved the record and freed the memory" << std::endl;

		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

		return true;
	};

	auto retrieveDetailedMeasurement = [=](uuid__t res_uuid)
	{
		auto start = std::chrono::system_clock::now();
		DetailedResultMeasurements *p_drm = nullptr;
		auto he = RetrieveDetailedMeasurementsForResultRecord(res_uuid, p_drm);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n RetrieveDetailedMeasurementsForResultRecord---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
			return false;
		}
		if (print_results)
			print_drm(p_drm, 1);

		FreeDetailedResultMeasurement(p_drm);
		std::cout << "Successfully retrieved the record and freed the memory" << std::endl;

		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

		return true;
	};
	auto execute = [=](int input)->bool
	{
		switch (input)
		{
			case 1:
			{
				std::cout << "Retrieving all the results summary :" << std::endl;
				uint32_t listsize = 0;
				ResultSummary *p_rs;
				auto start = std::chrono::system_clock::now();

				auto he = RetrieveResultSummaries(0, 0, "", p_rs, listsize);
				if (he != HawkeyeError::eSuccess)
				{
					std::cout << "\n RetrieveResultRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
					return true;
				}
				if (print_results)
					print_rs(p_rs, listsize);

				FreeListOfResultSummary(p_rs, listsize);
				std::cout << "Successfully retrieved and freed the memory # " << listsize << " records" << std::endl;
				auto end = std::chrono::system_clock::now();

				std::chrono::duration<double> elapsed_seconds = end - start;

				std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";
			}
			return true;
			case 2:
			{
				std::cout << "Retrieving a  result record :" << std::endl;

				displayAvailableSampleRecords();
				uuid__t res_uuid = {};
				if (selectResultRecord(res_uuid))
					retrieveResultRecord(res_uuid);
			}
			return true;
			case 3:
			{
				/*ResultRecord *pSrList;
				uint32_t retreived_size = 0;
				he = RetrieveResultRecordList(UUIDList, listsize, pSrList, retreived_size);
				if (he != HawkeyeError::eSuccess)
				{
					std::cout << "\n RetrieveResultRecordList---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
				}
				print_rr(pSrList, retreived_size);

				FreeListOfResultRecord(pSrList, retreived_size);*/

				std::cout << "Currently feature not available :" << std::endl;
			}
			return true;
			case 4:
			{
				displayAvailableSampleRecords();
				uuid__t res_uuid = {};
				if (selectResultRecord(res_uuid))
				{
					std::cout << "SigningResultRecord ----------\n" << "Enter signature [APR or REV] : "<< std::endl;
					std::string sig_text = {};
					std::cin >> sig_text;
					auto he = SignResultRecord(res_uuid, (char*)sig_text.c_str(), (uint16_t)sig_text.length());
					if (he != HawkeyeError::eSuccess)
					{
						std::cout << "Failed to Apply Signature: " + std::string(HawkeyeErrorAsString(he)) << std::endl;
					}
					else
					{
						if (retrieveResultRecord(res_uuid))
							std::cout << "Applied signature successfully" << std::endl;
					}
				}
			}
			return true;
			case 5:
			{
				std::cout << "Retrieving detail measurement of a result record :" << std::endl;

				displayAvailableSampleRecords();
				uuid__t res_uuid = {};
				if (selectResultRecord(res_uuid))
					retrieveDetailedMeasurement(res_uuid);
			}
			return true;
			case 6:
			{
				PrintResults();
			}
			return true;
			case 7:
				return false;
			default:
				std::cout << " Invalid input try again" << std::endl;
				return true;
		}
	};

	auto testmenu = [execute]()->bool
	{
		std::cout << "Result Record Data: <Test Start>" << std::endl;

		std::cout << "1. Retrieve all the result summary data."
			<< "\n2. Retrieve a particular result record."
			<< "\n3. Retrieve the records between the range."
			<< "\n4. Sign result record"
			<< "\n5. Retrieve detailed measurement"
			<< "\n6. Print results." 
			<< "\n7. End ResultrecordTest" << std::endl;

		std::string inputstr = {};
		std::cin >> inputstr;
		return execute(std::stol(inputstr));
	};
	
	if (!retrieveSampleRecord(p_sr, listsize_sr))
		return;

	while (true)
	{
		if (!testmenu())
			break;
	}

}

void RetrieveAPIsTest::RetrieveImageTest()
{
	std::cout << "\n RetrieveImageTest Test START> " << std::endl;
	uint32_t listsize = 0;
	SampleRecord *p_sr;
	char * user = "";
	auto he = RetrieveSampleRecords(0, 0, user, p_sr, listsize);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "\n RetrieveSampleRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	for (uint32_t index = 0; index < listsize; index++)
	{
		std::cout << index << ". UUID" << getUUIDStr(p_sr[index].uuid) << "\t|Sample ID: " << get_string_from_char_ptr(p_sr[index].sample_identifier) << "\t|User ID: " << get_string_from_char_ptr(p_sr[index].user_id) << std::endl;
	}

	uint32_t sample_index_value = {};

	if (listsize > 0)
	{
		//Select the the Sample
		while (true)
		{
			std::string sample_index;

			std::cout << "Select the sample between : |0 - " << listsize - 1 << "| to Retrieve images" << std::endl;
			std::cin >> sample_index;
			sample_index_value = std::stoul(sample_index);

			if (InBounds(sample_index_value, 0, listsize - 1))
				break;
			std::cout << "Invalid selection, try again" << std::endl;
		}
	}

	//Retrieve all the image sets 
	SampleImageSetRecord *p_imsr;
	uint32_t num_imsr = 0;
	he = RetrieveSampleImageSetRecordList(p_sr[sample_index_value].image_sets, p_sr[sample_index_value].num_image_sets, p_imsr, num_imsr);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "\n RetrieveSampleImageSetRecordList---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	std::cout << "Image Set Records : " << std::endl;
	for (uint32_t index = 0; index < num_imsr; index++)
	{
		std::cout << index << ". UUID" << getUUIDStr(p_imsr[index].uuid) << "\t| BF UUID: " << getUUIDStr(p_imsr[index].brightfield_image) << std::endl;
	}

	uint32_t imsr_index_value = {};

	if (num_imsr > 0)
	{
		//Select the the Sample
		while (true)
		{
			std::string imsr_index;

			std::cout << "Select the sample image set record between : |0 - " << num_imsr - 1 << "| to retrieve the BF image" << std::endl;
			std::cin >> imsr_index;
			imsr_index_value = std::stoul(imsr_index);

			if (InBounds(imsr_index_value, 0, num_imsr - 1))
				break;
			std::cout << "Invalid selection, try again" << std::endl;
		}
	}

	std::cout << "Retrieving Single image\n";
	auto start = std::chrono::system_clock::now();

	ImageWrapper_t *bfimg = nullptr;
	he = RetrieveImage(p_imsr[imsr_index_value].brightfield_image, bfimg);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "\n RetrieveImage---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << "\t UUID :" << getUUIDStr(p_imsr[imsr_index_value].brightfield_image) << std::endl;
		return;
	}

	auto end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;

	std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

	std::cout << "Retrieving all the images: " << std::endl;
	start = std::chrono::system_clock::now();
	for (uint32_t index = 0; index < num_imsr; index++)
	{
		ImageWrapper_t *bfimg = nullptr;
		he = RetrieveImage(p_imsr[index].brightfield_image, bfimg);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n RetrieveImage---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << "\t UUID :" << getUUIDStr(p_imsr[index].brightfield_image) << std::endl;
			return;
		}
	}
	end = std::chrono::system_clock::now();

	elapsed_seconds = end - start;

	std::cout << "Elapsed time: " << elapsed_seconds.count() << "s\n";

}

void RetrieveAPIsTest::SampleRecordForBPQCTest(std::string BPQC)
{
	uint32_t listsize = 0;
	std::cout << "Retrieve Sample Records for BP/QC <Test Start>" << std::endl;
	auto print = [=](SampleRecord *p_sr, uint32_t list_size) {
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < list_size; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "Sample Record Data[" << index << "]:\n" << "uuid : " << getUUIDStr(p_sr[index].uuid)
				<< "\nUser_id: " << get_string_from_char_ptr(p_sr[index].user_id) << "\nTime Stamp: " << p_sr[index].time_stamp << "\nQC\\BP: " << get_string_from_char_ptr(p_sr[index].bp_qc_identifier) << std::endl;

			for (auto i = 0; i < p_sr[index].num_image_sets; i++)
			{
				std::cout << "\tImage Sets[" << i << "]:" << getUUIDStr(p_sr[index].image_sets[i]) << std::endl;
			}

			for (auto i = 0; i < p_sr[index].num_result_summary; i++)
			{
				std::cout << "\tResult record[" << i << "]:" << getUUIDStr(p_sr[index].result_summaries[i].uuid) << std::endl;
			}
		}
		std::cout << "\n*********************************************************************************\n";
	};
	
	SampleRecord *p_sr;
	RetrieveSampleRecordsForQualityControl(BPQC.c_str(), p_sr, listsize);
	print(p_sr, listsize);
	FreeListOfSampleRecord(p_sr, listsize);
	std::cout << "Retrieve Sample Records for BP/QC <Test Start>" << std::endl;
}

void RetrieveAPIsTest::BiprocessTest()
{
	auto printBioProcess = [&](Bioprocess_t * bp, uint32_t num_bp)
	{

		for (size_t index = 0; index < num_bp; index++)
		{
			std::cout << "----------------- BioProcess-" << index+1 <<" -------------------";
			std::cout << "\nBioProcessName : " << get_string_from_char_ptr(bp[index].bioprocess_name)
			<< "\nCell_Type_Index : " <<  bp[index].cell_type_index
			<< "\nComment_text : " << get_string_from_char_ptr(bp[index].comment_text)
			<< "\nIsActive : " << std::boolalpha <<  bp[index].is_active << std::noboolalpha
			<< "\nReactorName : " << get_string_from_char_ptr(bp[index].reactor_name)
			<< "\nStartTime : " << bp[index].start_time;
			std::cout << "\n------------------------------------\n\n";
		}

		std::cout << "\n";
	};
	auto str1 =  "GetBioProcessList : ";
	Bioprocess_t *bioprocessList;
	uint32_t num_bp = 0;
	if (HawkeyeError::eSuccess == GetBioprocessList(bioprocessList, num_bp))
	{
		std::cout << str1 << "Passed" << std::endl;
		printBioProcess(bioprocessList, num_bp);
	}
	else
	{
		std::cout << str1 << "Failed" << std::endl;
		
	}


	auto str2 = "GetActiveBioProcessList ";
	bioprocessList = {};
	if(HawkeyeError::eSuccess == GetActiveBioprocessList(bioprocessList, num_bp))
	{
		std::cout << str2 << "Passed" << std::endl;
		printBioProcess(bioprocessList, num_bp);
	}
	else
	{
		std::cout << str2 << "Failed" << std::endl;
	}
}

void RetrieveAPIsTest::QualityControlTest()
{
	auto printQualityControl = [&](QualityControl_t * qc, uint32_t num_qc)
	{

		for (size_t index = 0; index < num_qc; index++)
		{
			std::cout << "----------------- QualityControl- " << index + 1 << " -------------------";
			std::cout << "\nQualityControlName : " << get_string_from_char_ptr(qc[index].qc_name)
				<< "\nCell_Type_Index : " << qc[index].cell_type_index
				<< "\nAssay_Type : " << qc[index].assay_type
				<< "\nLotInformation : " << get_string_from_char_ptr(qc[index].lot_information)
				<< "\nAssay_Value : " << qc[index].assay_value
				<< "\nPlusMinusPercentage : " << qc[index].plusminus_percentage
				<< "\nExpirationDate : " << qc[index].expiration_date
				<< "\nComment_text : " << get_string_from_char_ptr(qc[index].comment_text);
			std::cout << "\n------------------------------------\n\n";
		}

		std::cout << "\n";
	};
	auto str1 = "GetQualityControlList : ";
	QualityControl_t *qcList;
	uint32_t num_qc = 0;
	if (HawkeyeError::eSuccess == GetQualityControlList(qcList, num_qc))
	{
		std::cout << str1 << "Passed" << std::endl;
		printQualityControl(qcList, num_qc);
	}
	else
	{
		std::cout << str1 << "Failed" << std::endl;
	}


	auto str2 = "GetActiveQualityControlList ";
	qcList = {};
	if (HawkeyeError::eSuccess == GetActiveQualityControlList(qcList, num_qc))
	{
		std::cout << str2 << "Passed" << std::endl;
		printQualityControl(qcList, num_qc);
	}
	else
	{
		std::cout << str2 << "Failed" << std::endl;
	}
}

void RetrieveAPIsTest::CellTypeTest()
{
	auto print_celltype = [&](const CellType* ct, uint32_t num_ct)
	{
		for (size_t index = 0; index < num_ct; index++)
		{
			std::cout << " Cell Type # : " << index
				<< "\n index : " << ct[index].celltype_index
				<< "\n label : " << get_string_from_char_ptr(ct[index].label);
			for (size_t ind = 0; ind < ct[index].num_analysis_specializations; ind++)
			{
				std::cout << "\n\tAnalysis Definitions # : " << ind
					<< "\n\t\t A_Index : " << ct[index].analysis_specializations[ind].analysis_index
					<< "\n\t\t label : " << ct[index].analysis_specializations[ind].label << std::endl;
			}
		}
	};

	auto print_bp = [&](const Bioprocess_t* bp, uint32_t num_bp)
	{
		for (size_t index = 0; index < num_bp; index++)
		{
			std::cout << " Bioprocess # : " << index
				<< "\t index : " << bp[index].cell_type_index
				<< "\t label : " << get_string_from_char_ptr(bp[index].bioprocess_name) << std::endl;;
		}
	};


	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### GetAllCellTypes  ###" << std::endl;
	CellType * pCellType = nullptr;
	uint32_t num_ct = 0;
	auto he = GetAllCellTypes(num_ct, pCellType);
	if (he != HawkeyeError::eSuccess)
	{		
		std::cout << "GetAllCellTypes Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
		
	print_celltype(pCellType, num_ct);

	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### AddCellType ###" << std::endl;
	uint32_t ct_index = 0;
	std::string ct_label = "NewCellType 3";
	memcpy(pCellType->label, ct_label.c_str(), ct_label.size());
	he = AddCellType(pCellType[0], ct_index);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "AddCellType Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### GetAllCellTypes  ###" << std::endl;
	pCellType = nullptr;
	num_ct = 0;
	he = GetAllCellTypes(num_ct, pCellType);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetAllCellTypes Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}

	//print_celltype(pCellType, num_ct);

	//std::cout << "------------------------------------------------------------------------------------" << std::endl;
	//std::cout << "### RemoveCellType  ###" << std::endl;
	//// Delete Cell Type
	//for (int i = 0; i < num_ct; i++)
	//{
	//	he = RemoveCellType(pCellType[i].celltype_index);
	//	if (he != HawkeyeError::eSuccess)
	//		std::cout << "RemoveCellType Failed <" << std::string(HawkeyeErrorAsString(he)) << ">: Index[" << pCellType[i].celltype_index <<"]\n";
	//	else
	//		std::cout << "RemoveCellType Passed <" << std::string(HawkeyeErrorAsString(he)) << ">: Index[" << pCellType[i].celltype_index << "]\n";
	//}
	//std::cout << "------------------------------------------------------------------------------------" << std::endl;
	//std::cout << "### GetAllCellTypes  ###" << std::endl;
	//pCellType = nullptr;
	//num_ct = 0;
	//he = GetAllCellTypes(num_ct, pCellType);
	//if (he != HawkeyeError::eSuccess)
	//{
	//	std::cout << "GetAllCellTypes Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
	//	return;
	//}
	//print_celltype(pCellType, num_ct);
	//std::cout << "------------------------------------------------------------------------------------" << std::endl;

#if 0
	Bioprocess_t *pbioprocess = nullptr;
	uint32_t num_bp = 0;
	he = GetBioprocessList(pbioprocess, num_bp);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetAllCellTypes Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	print_bp(pbioprocess, num_bp);
	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::string bp_name = "NewBioprocess2";
	memset(pbioprocess[0].bioprocess_name, 0, sizeof(pbioprocess[0].bioprocess_name));
	memcpy(pbioprocess[0].bioprocess_name, bp_name.c_str(), sizeof(bp_name));
	pbioprocess[0].cell_type_index = ct_index;
	he = AddBioprocess(pbioprocess[0]);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "AddBioprocess Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	pbioprocess = nullptr;
	num_bp = 0;
	he = GetBioprocessList(pbioprocess, num_bp);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetAllCellTypes Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	print_bp(pbioprocess, num_bp);
	std::cout << "------------------------------------------------------------------------------------" << std::endl;
#endif
}

void RetrieveAPIsTest::AnalysisTest()
{
	auto print_Analysis = [&](const AnalysisDefinition* ad, uint32_t num_ct)
	{
		for (size_t index = 0; index < num_ct; index++)
		{
			std::cout << "Analysis Definitions # : " << index
				<< "\n\t index : " << ad[index].analysis_index
				<< "\n\t label : " << ad[index].label
				<< "\n\t Num Analysis param : " << ad[index].num_analysis_parameters
				<< "\n\t Num of Fl: " << ad[index].num_fl_illuminators << std::endl;
		}
	};

	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### GetAllAnalysisDefinitions ###" << std::endl;
	AnalysisDefinition * pAnalysis = nullptr;
	uint32_t num_ad = 0;
	auto he = GetAllAnalysisDefinitions(num_ad, pAnalysis);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetAllAnalysisDefinitions Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}

	print_Analysis(pAnalysis, num_ad);

	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### AddAnalysisDefinition ###" << std::endl;
	uint16_t ad_index = 0;
	std::string ad_label = "NewAnalysis";
	memcpy(pAnalysis[0].label, ad_label.c_str(), ad_label.size());
	he = AddAnalysisDefinition(pAnalysis[0], ad_index);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "AddAnalysisDefinition Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}

	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### GetAllAnalysisDefinitions ###" << std::endl;
	delete[] pAnalysis;
	pAnalysis = nullptr;
	num_ad = 0;
	he = GetAllAnalysisDefinitions(num_ad, pAnalysis);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetAllAnalysisDefinitions Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	print_Analysis(pAnalysis, num_ad);

	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### RemoveAnalysisDefinition ###" << std::endl;
	// Delete Analysis
	for (uint32_t i = 0; i < num_ad; i++)
	{
		he = RemoveAnalysisDefinition(pAnalysis[i].analysis_index);
		if (he != HawkeyeError::eSuccess)
			std::cout << "RemoveCellType Failed <" << std::string(HawkeyeErrorAsString(he)) << ">: Index[" << pAnalysis[i].analysis_index << "]\n";
		else
			std::cout << "RemoveCellType Passed <" << std::string(HawkeyeErrorAsString(he)) << ">: Index[" << pAnalysis[i].analysis_index << "]\n";
	}

	std::cout << "------------------------------------------------------------------------------------" << std::endl;
	std::cout << "### GetAllAnalysisDefinitions ###" << std::endl;
	delete[] pAnalysis;
	pAnalysis = nullptr;
	num_ad = 0;
	he = GetAllAnalysisDefinitions(num_ad, pAnalysis);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetAllAnalysisDefinitions Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	print_Analysis(pAnalysis, num_ad);
	std::cout << "------------------------------------------------------------------------------------" << std::endl;

}

void RetrieveAPIsTest::reanalyzeCallback(HawkeyeError he, uuid__t sample_uuid, ResultRecord* rr )
{
	std::cout << "Triggered Callback From Reanalyze test" << std::endl;

	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "ReanalyzeCallback - FAILED : " << HawkeyeErrorAsString(he) << std::endl;
		return;
	}
	
	//Retrieve the data of the sample which is reanalyzed
	SampleRecord *p_sr = nullptr;
	he = RetrieveSampleRecord(sample_uuid, p_sr);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "ReanalyzeCallback - RetrieveSampleRecord : " << HawkeyeErrorAsString(he) << std::endl;
		return;
	}

	std::cout << "Reanalyzed sample UUID : " << RetrieveAPIsTest::getUUIDStr(sample_uuid)
		<< "\tUser_id: " << get_string_from_char_ptr(p_sr->user_id) << "\tTime Stamp: " << p_sr->time_stamp
		<< "\tComment: " << get_string_from_char_ptr(p_sr->comment) << std::endl;

	for (auto i = 0; i < p_sr->num_result_summary; i++)
	{
		std::cout << "Result record[" << i << "]:" << RetrieveAPIsTest::getUUIDStr(p_sr->result_summaries[i].uuid) << std::endl;
	}
	
	std::cout << "Reanalyzed result record rata"
		<< "\tUUID : " << getUUIDStr(rr->summary_info.uuid)
		<< "\tUser_id: " << get_string_from_char_ptr(rr->summary_info.user_id)
		<< "\tTime Stamp: " << rr->summary_info.time_stamp
		<< "\nNum of Per Image Results: " << rr->num_image_results
		<< "\nNum of Signatures: " << rr->summary_info.num_signatures
		<< "\nBasicResultAnswer-ImageProcessStatus: " << rr->summary_info.cumulative_result.eProcessedStatus
		<< "\nBasicResultAnswer-TotalNumImages: " << rr->summary_info.cumulative_result.nTotalCumulative_Imgs
		<< std::endl;

		print_ct(&rr->summary_info.cell_type_settings, 1);
		print_ad(&rr->summary_info.analysis_settings, 1);

		for (uint32_t sigIndex = 0; sigIndex < rr->summary_info.num_signatures; sigIndex++)
		{
			std::cout << "\nSignature Details : User-" << get_string_from_char_ptr(rr->summary_info.signature_set[sigIndex].signing_user)
			<< "\tTimeStamp-" << rr->summary_info.signature_set[sigIndex].timestamp
			<< "\tShortText-" << get_string_from_char_ptr(rr->summary_info.signature_set[sigIndex].signature.short_text)
			<< "\tLongText-" << get_string_from_char_ptr(rr->summary_info.signature_set[sigIndex].signature.long_text) << std::endl;
		}
}

void RetrieveAPIsTest::Reanalyzetest(bool from_images)
{
	std::cout << "\n ReanalyzeSample Test START> " << std::endl; 
	uint32_t listsize = 0;
	std::vector<uuid__t> sampleRecList;
	SampleRecord *p_sr;
	char * user = "";
	auto he = RetrieveSampleRecords(0, 0, user, p_sr, listsize);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "\n RetrieveSampleRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	for (uint32_t index = 0; index < listsize; index++)
	{
		sampleRecList.push_back(p_sr[index].uuid);
		std::cout << index << ". UUID" << getUUIDStr(p_sr[index].uuid) <<"\t|Sample ID: " << get_string_from_char_ptr(p_sr[index].sample_identifier) << "\t|User ID: " << get_string_from_char_ptr(p_sr[index].user_id) << std::endl;

		for (auto i = 0; i < p_sr[index].num_result_summary; i++)
		{
			std::cout << "Result record[" << i << "]:" << RetrieveAPIsTest::getUUIDStr(p_sr[index].result_summaries[i].uuid) << std::endl;
		}
	}

	uint32_t sample_index_value = {};
	uint32_t celltype_index_value = {};
	uint32_t analysis_index_value = {};

	if (listsize > 0)
	{
		//Select the the Sample
		while (true)
		{
			std::string sample_index;

			std::cout << "Select the sample between : |0 - " << listsize - 1 << "| to Perform reanalysis" << std::endl;
			std::cin >> sample_index;
			sample_index_value = std::stoul(sample_index);

			if (InBounds(sample_index_value, 0, listsize - 1))
				break;
			std::cout << "Invalid selection, try again" << std::endl;
		}
		
		//Select the cell type
		CellType* celltype = nullptr;
		uint32_t num_celltype = 0;
		he = GetAllCellTypes(num_celltype, celltype);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n ReanalyzeSample : GetAllCellTypes---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
			return;
		}
		for (size_t index = 0; index < num_celltype; index++)
		{
			std::cout << index
				<< ". Cell type Index : " << celltype[index].celltype_index
				<< "\t Label : " << get_string_from_char_ptr(celltype[index].label) << std::endl;
		}

		if (num_celltype == 0)
		{
			std::cout << "No Cell type found" << std::endl;
			return;
		}
		//Print the Cell type details here.
		while (true)
		{
			std::string celltype_index;
			std::cout << "Select the Cell type between : |0 - " << num_celltype - 1 << "| to Perform reanalysis" << std::endl;
			std::cin >> celltype_index;
			celltype_index_value = std::stoul(celltype_index);

			if (InBounds(celltype_index_value, 0, num_celltype - 1))
				break;
			std::cout << "Invalid selection, try again" << std::endl;
		}

		//Select the analysis
		//Select the cell type
		AnalysisDefinition* analysis = nullptr;
		uint32_t num_analysis = 0;
		he = GetAllAnalysisDefinitions(num_analysis, analysis);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n ReanalyzeSample : GetAllAnalysisDefinitions---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
			return;
		}
		if (num_analysis == 0)
		{
			std::cout << "No Analysis found" << std::endl;
			return;
		}
		//Print the Analysis  details here.
		for (size_t index = 0; index < num_analysis; index++)
		{
			std::cout << "Analysis Definitions # : " << index
				<< "\t Index : " << analysis[index].analysis_index
				<< "\t Label : " << get_string_from_char_ptr(analysis[index].label) << std::endl;
		}
		while (true)
		{
			std::string analysis_index;
			std::cout << "Select the Analysis between : |0 - " << num_analysis - 1 << "| to Perform reanalysis" << std::endl;
			std::cin >> analysis_index;
			analysis_index_value = std::stoul(analysis_index);

			if (InBounds(analysis_index_value, 0, num_analysis - 1))
				break;
			std::cout << "Invalid selection, try again" << std::endl;
		}
		std::cout << "Reanalyzing using Cell type : Index- " << celltype[celltype_index_value].celltype_index << "\t label- " << celltype[celltype_index_value].label << std::endl;
		std::cout << "Reanalyzing using Analysis : Index- " << analysis[analysis_index_value].analysis_index << "\t label- " << analysis[analysis_index_value].label << std::endl;
		he = ReanalyzeSample(sampleRecList[sample_index_value], celltype[celltype_index_value].celltype_index, analysis[analysis_index_value].analysis_index, from_images, reanalyzeCallback);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n ReanalyzeSample---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
		}
	}

}

void RetrieveAPIsTest::TempCellTypeTest()
{
	HawkeyeError he;
	auto printCelltype = [&](const CellType& ct) 
	{
		std::cout << "----------------------------------------------\n";
		std::cout << "CellType Index: " << ct.celltype_index << "\n"
			<< "Label : " << ct.label << "\n" << std::endl;
	};
	// Get all Types
	CellType* pCellTypes = nullptr;
	uint32_t num_CellType = 0;
	he = GetAllCellTypes(num_CellType, pCellTypes);
	if (HawkeyeError::eSuccess != he )
		std::cout << "Failed Get ALL CellTypes : " << std::string(HawkeyeErrorAsString(he)) <<  std::endl;

	std::cout << "Num of CellTypes Found: " << num_CellType << std::endl;
	for (int index = 0; index < (int)num_CellType; index++)
		printCelltype(pCellTypes[index]);

	// Get Temporary Cell Type
	CellType* temp_cellType2 = nullptr;
	he = GetTemporaryCellType(temp_cellType2);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed GetTemporaryCellType:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
	}

	// Set Temporary Cell Type
	pCellTypes[0].celltype_index = 26;
	he = SetTemporaryCellType(&pCellTypes[0]);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed Get ALL CellTypes: " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	// Get Temporary Cell Type
	CellType* temp_cellType = nullptr;
	he = GetTemporaryCellType(temp_cellType);
	if (HawkeyeError::eSuccess != he )
	{
		std::cout << "Failed GetTemporaryCellType:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	printCelltype(*temp_cellType);

	// Set Temporary Cell Type from existing cell index
	he = SetTemporaryCellTypeFromExisting(6);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed Get ALL CellTypes:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	// Get Temporary Cell Type
	CellType* temp_cellType1 = nullptr;
	he = GetTemporaryCellType(temp_cellType1);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed GetTemporaryCellType:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	printCelltype(*temp_cellType1);


}

void RetrieveAPIsTest::TempAnalysisTest()
{
	HawkeyeError he;
	auto printAnalysis = [&](const AnalysisDefinition& ad)
	{
		std::cout << "----------------------------------------------\n";
		std::cout << "Analysis Index: " << ad.analysis_index << "\n"
			<< "Label : " << ad.label << "\n" << std::endl;
	};
	// Get all Types
	AnalysisDefinition* pAd = nullptr;
	uint32_t num_Ad = 0;
	he = GetAllAnalysisDefinitions(num_Ad, pAd);
	if (HawkeyeError::eSuccess != he)
		std::cout << "Failed GetAllAnalysisDefinitions : " << std::string(HawkeyeErrorAsString(he)) << std::endl;

	std::cout << "Num of AnalysisDefinition Found: " << num_Ad << std::endl;
	for (int index = 0; index < (int)num_Ad; index++)
		printAnalysis(pAd[index]);

	// Get Temporary Ad
	AnalysisDefinition* temp_ad = nullptr;
	he = GetTemporaryAnalysisDefinition(temp_ad);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed GetTemporaryAnalysisDefinition:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
	}

	// Set Temporary Ad
	pAd[0].analysis_index = 26;
	he = SetTemporaryAnalysisDefinition(&pAd[0]);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed SetTemporaryAnalysisDefinition: " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	// Get Temporary Cell Type
	AnalysisDefinition* temp_ad1 = nullptr;
	he = GetTemporaryAnalysisDefinition(temp_ad1);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed GetTemporaryAnalysisDefinition:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
	}

	printAnalysis(*temp_ad1);

	// Set Temporary Cell Type from existing cell index
	he = SetTemporaryAnalysisDefinitionFromExisting(0);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed SetTemporaryAnalysisDefinitionFromExisting:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	// Get Temporary Cell Type
	AnalysisDefinition* temp_ad2 = nullptr;
	he = GetTemporaryAnalysisDefinition(temp_ad2);
	if (HawkeyeError::eSuccess != he)
	{
		std::cout << "Failed GetTemporaryAnalysisDefinition:  " << std::string(HawkeyeErrorAsString(he)) << std::endl;
		return;
	}

	printAnalysis(*temp_ad2);
}

void RetrieveAPIsTest::RetrieveSampleDataTest()
{
	uint32_t listsize = 0;
	std::cout << "RetrieveSampleDataTest Record : <Test Start>" << std::endl;
	auto print_wq = [&](auto wqlist, uint32_t list_size)
	{
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < list_size; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "WorkQueue Data[" << index << "]:\n" << "uuid : " << getUUIDStr(wqlist[index].uuid)
				<< "\nUser_id: " << wqlist[index].user_id << "\nTime Stamp: " << wqlist[index].time_stamp << std::endl;

			for (auto i = 0; i < wqlist[index].num_sample_records; i++)
			{
				std::cout << " \tSample record[" << i << "]:" << getUUIDStr(wqlist[index].sample_records[i]) << std::endl;
			}
		}
		std::cout << "\n*********************************************************************************\n";
	};

	auto print_wqi = [=](SampleRecord *p_sr, uint32_t list_size) {
		std::cout << "\n*********************************************************************************\n";
		for (uint32_t index = 0; index < list_size; index++)
		{
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "Sample Record Data[" << index << "]:\n" << "uuid : " << getUUIDStr(p_sr[index].uuid)
				<< "\nUser_id: " << p_sr[index].user_id << "\nTime Stamp: " << p_sr[index].time_stamp
				<< "\nDilution factor: " << p_sr[index].dilution_factor
				<< "\nWashType: " << (int)p_sr[index].wash
				<< "\nComment: " << p_sr[index].comment << std::endl;

			std::cout << "Reagent Information <Start>" << std::endl;
			for (auto i = 0; i < p_sr[index].num_reagent_records; i++)
			{
				std::cout << " ------------REAGENT INFO  ------------" << i << std::endl;
				std::cout << "\tPack number: " << p_sr[index].reagent_info_records[i].pack_number
					<< "\n\tLot Number: " << p_sr[index].reagent_info_records[i].lot_number
					<< "\n\tLabel: " << p_sr[index].reagent_info_records[i].reagent_label
					<< "\n\tExpirationDate: " << p_sr[index].reagent_info_records[i].expiration_date
					<< "\n\tInserviceDate: " << p_sr[index].reagent_info_records[i].in_service_date
					<< "\n\tEffectiveExpirationDate: " << p_sr[index].reagent_info_records[i].effective_expiration_date
					<< std::endl;
			}
			std::cout << "Reagent Information <End>" << std::endl;
			for (auto i = 0; i < p_sr[index].num_image_sets; i++)
			{
				std::cout << "\tImage Sets[" << i << "]:" << getUUIDStr(p_sr[index].image_sets[i]) << std::endl;
			}

			for (auto i = 0; i < p_sr[index].num_result_summary; i++)
			{
				std::cout << "\tResult record[" << i << "]:" << getUUIDStr(p_sr[index].result_summaries[i].uuid) << std::endl;
			}
		}
		std::cout << "\n*********************************************************************************\n";
	};

	auto print_imr = [=](auto p_ir, uint32_t im_no) {
		//std::cout << "\n*********************************************************************************\n";
			std::cout << "\n---------------------------------------------------------------\n";
			std::cout << "Image Record Data[" << im_no << "]:\n" << "uuid : " << getUUIDStr(p_ir->uuid)
				<< "\nUser_id: " << p_ir->user_id << "\nTime Stamp: " << p_ir->time_stamp << std::endl;
		//std::cout << "\n*********************************************************************************\n";
	};

	WorkQueueRecord *p_wqr;
	char * user = "";
	HawkeyeError ec = RetrieveWorkQueues(0, 0, user, p_wqr, listsize);
	if (HawkeyeError::eSuccess != ec)
		std::cout << "Failed to retrieve the work : " << HawkeyeErrorAsString(ec) << std::endl;
	print_wq(p_wqr, listsize);

	//Retrieve the first sample from the first workQueue.

	if (p_wqr->num_sample_records > 0)
	{
		SampleRecord *sr = nullptr;
		HawkeyeError ec = RetrieveSampleRecord(p_wqr->sample_records[0], sr);
		if (HawkeyeError::eSuccess != ec)
			std::cout << "Failed to Retrieve the sample record : " << HawkeyeErrorAsString(ec) << std::endl;
		print_wqi(sr, listsize);

		// Retrieve the Sample image set record
		SampleImageSetRecord* sir_list = nullptr;
		uint32_t retrieved_sir_size = 0;
		ec = RetrieveSampleImageSetRecordList(sr->image_sets, sr->num_image_sets, sir_list, retrieved_sir_size);
		if (HawkeyeError::eSuccess != ec)
			std::cout << "Failed to Retrieve the Sample image set Record : " << HawkeyeErrorAsString(ec) << std::endl;
	//	print_(p_wqr, listsize);

		std::vector<ImageRecord> imrec_list;
		// Retrieve Image record
		if (retrieved_sir_size > 0)
		{
			for (uint32_t index = 0; index < retrieved_sir_size; index++)
			{
				ImageRecord *ir = nullptr;
				ec = RetrieveImageRecord(sir_list[index].brightfield_image, ir);
				if (HawkeyeError::eSuccess != ec)
				{
					std::cout << "Failed to Retrieve the image set Record : " << HawkeyeErrorAsString(ec) << std::endl;
					return;
				}
				print_imr(ir, index);

				ImageWrapper_t *image = nullptr;
				ec = RetrieveImage(ir->uuid, image);
				if (HawkeyeError::eSuccess != ec)
				{
					std::cout << "Failed to Retrieve the image : " << HawkeyeErrorAsString(ec) << std::endl;
					return;
				}
				std::cout << "Retrieved Image : \n" << "Column :" << image->cols << "\n" << "Rows :" << image->rows << std::endl;
				FreeImageWrapper(image, 1);
				FreeListOfImageRecord(ir,1);
			}
		}
		delete[] sir_list;
	
	}
}

void RetrieveAPIsTest::GetCellTypeIndicesTest()
{
	char**userList = nullptr;
	uint32_t num_users = 0;
	auto he = GetUserList(true, userList, num_users);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetUserList Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}
	std::cout << "_________________________________\n";
	for (uint32_t index = 0; index < num_users; index++)
	{
		std::cout << "User " << index << " : " << userList[index] << std::endl;
	}
	std::cout << "_________________________________\n";

	uint32_t num_celltype = 0;
	uint32_t* num_celltype_indices = nullptr;
	NativeDataType dataType;
	he = GetUserCellTypeIndices(userList[0], num_celltype, dataType, num_celltype_indices);
	if (he != HawkeyeError::eSuccess)
	{
		std::cout << "GetUserCellTypeIndices Failed <" << std::string(HawkeyeErrorAsString(he)) << ">\n";
		return;
	}

	std::cout << "_________________________________\n";
	for (uint32_t index = 0; index < num_celltype; index++)
	{
		std::cout << "Cell Type Index " << std::hex << index << " : " << num_celltype_indices[index] << std::endl;
	}
	std::cout << "_________________________________\n";
}

void RetrieveAPIsTest::DeleteResultsDataTest()
{
	auto mainmenu = [=]()-> bool
	{
		std::cout << "Delete Results Data\n Select from the following:\n"
			<< "1  - Delete Single WQIR\n"
			<< "2  - Delete Single WQIR(retain_results_and_first_image)\n"
			<< "3  - Delete ALL WQIR\n"
			<< "4  - Delete ALL WQIR(retain_results_and_first_image)\n"
			<< "5  - Delete Single WQR\n"
			<< "6  - Delete Single WQR(retain_results_and_first_image)\n"
			<< "7  - Delete ALL WQR\n"
			<< "8  - Delete ALL WQR(retain_results_and_first_image)\n"
			<< "9  - Print the results\n"
			<< "10 - Delete result record (retain empty WQI)\n"
			<< "11 - Delete result record (remove empty WQI)\n"
			<< "0 - To Quit Delete Record Test\n";

		int input;
		std::cin.clear();
		std::cin >> input;

		switch (input)
		{
			case 1:
				DeleteParticularWqirTest(false);
				return true;
			case 2:
				DeleteParticularWqirTest(true);
				return true;
			case 3:
				DeleteAllWqirTest(false);
				return true;
			case 4:
				DeleteAllWqirTest(true);
				return true;
			case 5:
				DeleteParticularWqrTest(false);
				return true;
			case 6:
				DeleteParticularWqrTest(true);
				return true;
			case 7:
				DeleteAllWqrTest(false);
				return true;
			case 8:
				DeleteAllWqrTest(true);
				return true;
			case 9:
				PrintResults();
				return true;
			case 10:
				DeleteParticularResultRecordTest(false);
				return true;
			case 11:
				DeleteParticularResultRecordTest(true);
				return true;
			case 0:
				return false;
			default:
			{
				std::cout << "Invalid input try again" << std::endl;
				return true;
			}
		}
	};

	while (mainmenu());
}

void RetrieveAPIsTest::DeleteCompletionCb(HawkeyeError status, uuid__t uuid)
{
	std::string uuid_str = getUUIDStr(uuid);
	if (status == HawkeyeError::eSuccess)
		std::cout << " Successfully deleted the record :  " << uuid_str << std::endl;
	else
		std::cout << " Failed to delete the record :  " << uuid_str  << "  Error : "<< HawkeyeErrorAsString(status) << std::endl;

}

void RetrieveAPIsTest::DeleteParticularWqrTest(bool retain_result_and_first_image)
{
	std::cout << "DeleteParticularWqrTest----------------------<Start>" << std::endl;

	WorkQueueRecord * wqr_list;
	uint32_t list_size_wq = 0;

	auto he = RetrieveWorkQueues(0, 0, NULL, wqr_list, list_size_wq);

	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to Retrieve the work queue records" << std::endl;
		return;
	}

	uint32_t index = 1;
	std::cout << "Select a wqr between |1 to " << list_size_wq << "|" << std::endl;
	std::cin >> index;

	if (index < 1 || index > list_size_wq)
	{
		std::cout << "Invalid Input" << std::endl;
		return;
	}

	he = DeleteWorkQueueRecord(&wqr_list[index - 1].uuid, 1, retain_result_and_first_image, DeleteCompletionCb);
	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to delete the work queue record" << HawkeyeErrorAsString(he) << std::endl;
		return;
	}
	FreeListOfWorkQueueRecord(wqr_list, list_size_wq);

	std::cout << "DeleteParticularWqrTest----------------------<Start>" << std::endl;
}

void RetrieveAPIsTest::DeleteParticularWqirTest(bool retain_result_and_first_image)
{
	std::cout << "DeleteParticularWqirTest----------------------<Start>" << std::endl;

	WorkQueueRecord * wqr_list;
	uint32_t list_size_wq = 0;

	auto he = RetrieveWorkQueues(0, 0, NULL, wqr_list, list_size_wq);

	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to Retrieve the work queue records" << std::endl;
		return;
	}

	uint32_t index = 1;
	std::cout << "Select a wqr between |1 to " << list_size_wq << "|" << std::endl;
	std::cin >> index;

	if (index < 1 || index > list_size_wq)
	{
		std::cout << "Invalid Input" << std::endl;
		return;
	}

	if(wqr_list[index - 1].num_sample_records == 0)
	{
		std::cout << "No samples in the selected WorkQueue" << std::endl;
		return;
	}

	uint32_t wqir_index = 1;
	std::cout << "Select a wqir between |1 to " << wqr_list[index - 1].num_sample_records << "|" << std::endl;
	std::cin >> wqir_index;

	if (wqir_index < 1 || wqir_index > wqr_list[index - 1].num_sample_records)
	{
		std::cout << "Invalid Input" << std::endl;
		return;
	}

	he = DeleteSampleRecord(&wqr_list[index - 1].sample_records[wqir_index -1], 1, retain_result_and_first_image, DeleteCompletionCb);
	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to delete the work queue record" << HawkeyeErrorAsString(he) << std::endl;
		return;
	}
	FreeListOfWorkQueueRecord(wqr_list, list_size_wq);

	std::cout << "DeleteParticularWqirTest----------------------<End>" << std::endl;
}

void RetrieveAPIsTest::DeleteAllWqrTest(bool retain_result_and_first_image)
{
	std::cout << "DeleteAllWqrTest----------------------<Start>" << std::endl;

	WorkQueueRecord * wqr_list;
	uint32_t list_size_wq = 0;

	auto he = RetrieveWorkQueues(0, 0, NULL, wqr_list, list_size_wq);

	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to Retrieve the work queue records" << std::endl;
		return;
	}

	auto wq_uuid_list = new uuid__t[list_size_wq];
	for (uint32_t i = 0; i < list_size_wq; i++)
		wq_uuid_list[i] = wqr_list[i].uuid;

	he = DeleteWorkQueueRecord(wq_uuid_list, list_size_wq, retain_result_and_first_image, DeleteCompletionCb);
	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to delete the work queue record" << HawkeyeErrorAsString(he) << std::endl;
		return;
	}
	FreeListOfWorkQueueRecord(wqr_list, list_size_wq);
	delete[] wq_uuid_list;

	std::cout << "DeleteAllWqrTest----------------------<Start>" << std::endl;
}

void RetrieveAPIsTest::DeleteAllWqirTest(bool retain_result_and_first_image)
{
	std::cout << "DeleteAllWqirTest----------------------<Start>" << std::endl;

	WorkQueueRecord * wqr_list;
	uint32_t list_size_wq = 0;

	auto he = RetrieveWorkQueues(0, 0, NULL, wqr_list, list_size_wq);

	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to Retrieve the work queue records" << std::endl;
		return;
	}

	uint32_t index = 1;
	std::cout << "Select a wqr between |1 to " << list_size_wq << "| to delete all sample records " << std::endl;
	std::cin >> index;

	if (index < 1 || index > list_size_wq)
	{
		std::cout << "Invalid Input" << std::endl;
		return;
	}

	he = DeleteSampleRecord(wqr_list[index - 1].sample_records, wqr_list[index - 1].num_sample_records, retain_result_and_first_image, DeleteCompletionCb);
	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to delete the work queue item record" << HawkeyeErrorAsString(he) << std::endl;
		return;
	}

	FreeListOfWorkQueueRecord(wqr_list, list_size_wq);

	std::cout << "DeleteAllWqirTest----------------------<End>" << std::endl;
}

void RetrieveAPIsTest::DeleteParticularResultRecordTest(bool clean_up_empties)
{
	std::cout << "DeleteParticularResultRecordTest----------------------<Start>" << std::endl;
	/*retrieve list of WQIs
	  print with count of RRs underneath
	  Select WQI index
	  Select RR index
	  */
	SampleRecord * sr_list;
	uint32_t list_size_sr = 0;

	auto he = RetrieveSampleRecords(0, 0, NULL, sr_list, list_size_sr);

	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to Retrieve the sample records" << std::endl;
		return;
	}

	PrintSampleRecordList(sr_list, list_size_sr);

	uint32_t index = 1;
	std::cout << "Select a sample between \"1\" to \"" << list_size_sr << "\" " << std::endl;
	std::cin >> index;

	if (index < 1 || index > list_size_sr)
	{
		std::cout << "Invalid Input" << std::endl;
		return;
	}
	if (sr_list[index - 1].num_result_summary == 0)
	{
		std::cout << "No results found in the selected sample record" << std::endl;
		return;
	}

	uint32_t rr_index = 1;
	std::cout << "Select a result between \"1\" to \"" << sr_list[index - 1].num_result_summary << "\" " << std::endl;
	std::cin >> rr_index;

	if (rr_index < 1 || rr_index > sr_list[index - 1].num_result_summary)
	{
		std::cout << "Invalid Input" << std::endl;
		return;
	}


	he = DeleteResultRecord(&sr_list[index - 1].result_summaries[rr_index - 1].uuid, 1, clean_up_empties, DeleteCompletionCb);
	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to delete the result record" << HawkeyeErrorAsString(he) << std::endl;
		return;
	}
	FreeListOfSampleRecord(sr_list, list_size_sr);

	std::cout << "DeleteParticularResultRecordTest----------------------<End>" << std::endl;
}

void RetrieveAPIsTest::PrintResults()
{
	WorkQueueRecord * wqr_list;
	uint32_t list_size_wq = 0;

	auto he = RetrieveWorkQueues(0, 0, NULL, wqr_list, list_size_wq);

	if (he != HawkeyeError::eSuccess)
	{
		std::cerr << " Failed to Retrieve the work queue records : " << HawkeyeErrorAsString(he) << std::endl;
		return;
	}

	std::cout << "Work Queue Records Data -----------------" << list_size_wq << ": " << std::endl;
	for (uint32_t wq_idx = 0; wq_idx < list_size_wq; wq_idx++)
	{
		std::cout << "Work Queue Record # " << wq_idx + 1 << ". " << getUUIDStr(wqr_list[wq_idx].uuid) << std::endl;

		SampleRecord * wqir_list;
		uint32_t list_size_wqi = 0;
		auto he = RetrieveSampleRecordList(wqr_list[wq_idx].sample_records, wqr_list[wq_idx].num_sample_records, wqir_list, list_size_wqi);
		if (he != HawkeyeError::eSuccess)
		{
			std::cerr << " Failed to Retrieve the Sample records : " << HawkeyeErrorAsString(he) << std::endl;
			continue;
		}

		std::cout << "\tSample Records Data ------------------" << list_size_wqi << ": " << std::endl;

		for (uint32_t sr_idx = 0; sr_idx < list_size_wqi; sr_idx++)
		{
			std::cout << "\tSample Record # " << sr_idx + 1 << ". " << getUUIDStr(wqir_list[sr_idx].uuid) << std::endl;

			std::cout << "\t\tNum Image Set Record found# :  " << wqir_list[sr_idx].num_image_sets << std::endl;
			/*SampleImageSetRecord* imsr_list;
			uint32_t list_size_imsr = 0;
			he = RetrieveSampleImageSetRecordList(wqir_list[sr_idx].image_sets, wqir_list[sr_idx].num_image_sets, imsr_list, list_size_imsr);
			if (he != HawkeyeError::eSuccess)
			{
			std::cerr << " Failed to Retrieve image set records  : " << HawkeyeErrorAsString(he) << std::endl;
			return;
			}
			std::cout << "\tImage Set Records Data ----------------------: " << std::endl;
			for (uint32_t k = 0; k < list_size_imsr; k++)
			{
			std::cout << "\t Image set record # " << k + 1 << ". " << getUUIDStr(imsr_list[k].uuid) << std::endl;
			std::cout << "\t\t" << "BF :" << getUUIDStr(imsr_list[k].brightfield_image) << std::endl;
			}*/

			std::cout << "\t\tResult Records Data ------------------------" << wqir_list[sr_idx].num_result_summary << ": " << std::endl;
			for (uint32_t rr_idx = 0; rr_idx < wqir_list[sr_idx].num_result_summary; rr_idx++)
			{
				std::cout << "\t\t\tResult record # " << rr_idx + 1 << ". " << getUUIDStr(wqir_list[sr_idx].result_summaries[rr_idx].uuid) << std::endl;
			}

			//FreeListOfImageSetRecord(imsr_list, list_size_imsr);
		}
		FreeListOfSampleRecord(wqir_list, list_size_wqi);
	}
	FreeListOfWorkQueueRecord(wqr_list, list_size_wq);
}

std::string RetrieveAPIsTest::getUUIDStr(const uuid__t & uuid)
{
	std::string uuidStr;
	if (!HawkeyeUUID::GetStrFromuuid__t(uuid, uuidStr))
	{
		return "";
		std::cerr << " Failed to UUID string" << std::endl;
	}
	return uuidStr;
}

void RetrieveAPIsTest::ExportCompletionCb(HawkeyeError status, char * archive_filepath)
{
	if (status == HawkeyeError::eSuccess)
	{
		if(archive_filepath)
			std::cout << " Successfully completed the data export" << std::string(archive_filepath) << std::endl;
		else
			std::cout << " Successfully cancelled the data export" << std::endl;
	}
	else
	{
		std::cout << " Failed to export the data - Error : " << HawkeyeErrorAsString(status) << std::endl;
	}

}

void RetrieveAPIsTest::ExportProgressCb(HawkeyeError status, uuid__t uuid)
{
	std::string uuid_str = getUUIDStr(uuid);
	if (status == HawkeyeError::eSuccess)
	{
		std::cout << " Successfully Exported the record :  " << uuid_str << std::endl;
	}
	else
	{
		std::cout << " Failed to export the record :  " << uuid_str << "  Error : " << HawkeyeErrorAsString(status) << std::endl;
	}

}

void RetrieveAPIsTest::ExportDataTest()
{
	uint32_t listsize_sr = 0;
	SampleRecord *p_sr;

	auto retrieveSampleRecord = [](SampleRecord *&p_sr, uint32_t &listsize_sr)->bool
	{
		auto he = RetrieveSampleRecords(0, 0, "", p_sr, listsize_sr);
		if (he != HawkeyeError::eSuccess)
		{
			std::cout << "\n RetrieveSampleRecords---<FAILED> " + std::string(HawkeyeErrorAsString(he)) << std::endl;
			return false;
		}
		return true;
	};


	auto displayAvailableSampleRecords = [&]()
	{
		for (uint32_t index = 0; index < listsize_sr; index++)
		{
			std::cout << index << ". UUID" << getUUIDStr(p_sr[index].uuid) << "\t|Sample ID: " << get_string_from_char_ptr(p_sr[index].sample_identifier) << "\t|User ID: " << get_string_from_char_ptr(p_sr[index].user_id) << std::endl;

			for (auto i = 0; i < p_sr[index].num_result_summary; i++)
			{
				std::cout << "Result record[" << i << "]:" << RetrieveAPIsTest::getUUIDStr(p_sr[index].result_summaries[i].uuid) << std::endl;
			}
		}
	};

	auto selectResultsToExport = [&](std::vector<uuid__t> &rs_uuid)->bool
	{
		uint32_t sample_index_start = {};
		uint32_t sample_index_end = {};

		if (listsize_sr > 0)
		{
			//select a sample a index
			while (true)
			{
				std::string sample_index;
				std::cout << "Select the sample range between : |0 - " << listsize_sr - 1 << "|" << std::endl;
				std::cout << "Enter lower range" << std::endl;
				std::cin >> sample_index;
				sample_index_start = std::stoul(sample_index);
				sample_index.clear();
				std::cout << "Enter upper range" << std::endl;
				std::cin >> sample_index;
				sample_index_end = std::stoul(sample_index);

				if (InBounds(sample_index_start, 0, listsize_sr - 1) && InBounds(sample_index_end, sample_index_start, listsize_sr - 1))
				{
					for (auto start = sample_index_start; start <= sample_index_end; start++)
					{
						if (p_sr[start].num_result_summary > 0)
							for (auto index = 0; index < p_sr[start].num_result_summary; index++)
							{
								rs_uuid.push_back(p_sr[start].result_summaries[index].uuid);
							}
					}
					break;
				}

				std::cout << "Invalid selection, try again" << std::endl;
			}
			return true;
		}
		else
		{
			std::cout << "No Sample records found" << std::endl;
			return false;
		}
	};

	if (!retrieveSampleRecord(p_sr, listsize_sr))
	{
		std::cout << "No Data export" << std::endl;
		return;
	}

	displayAvailableSampleRecords();

	std::vector<uuid__t> data_to_export;

	selectResultsToExport(data_to_export);

	uuid__t *rs_to_export = new uuid__t[data_to_export.size()];
	std::copy(data_to_export.begin(), data_to_export.end(), stdext::checked_array_iterator<uuid__t*>(rs_to_export, data_to_export.size()));
	
	uint16_t nth_image_export = 0;
	eExportImages export_image_option;
	while (true)
	{
		std::string str_input = {};
		std::cout << "Select to export : \n"
			<< "1. All Images\n"
			<< "2. First and Last\n"
			<< "3. N Image\n";

			std::cin >> str_input;

			auto input = std::stoi(str_input);
			if (!InBounds(input, 1, 3))
			{
				std::cout << "Invalid Selection try again\n";
				continue;
			}

			if (input == 3)
			{
				while (true)
				{
					str_input = {};
					std::cout << "Enter nth image to export between |1 to 100|\n";
					std::cin >> str_input;
					auto nth_image_input = std::stoi(str_input);

					if (!InBounds(nth_image_input, 1, 100))
					{
						std::cout << "Invalid Selection try again\n";
						continue;
					}

					nth_image_export = (uint16_t)nth_image_input;
					break;
				}

				export_image_option = eExportImages::eExportNthImage;
			}
			else if(input == 2)
				export_image_option = eExportImages::eFirstAndLastOnly;
			else
				export_image_option = eExportImages::eAll;

			break;
	}

	auto status = ExportInstrumentData(rs_to_export, (uint32_t)data_to_export.size(), "C:\\Instrument\\Export", export_image_option, nth_image_export, ExportCompletionCb, ExportProgressCb);
	if (status == HawkeyeError::eSuccess)
		std::cout << " Export data started" << std::endl;
	else
		std::cout << "  Export data Failed to start - Error : " << HawkeyeErrorAsString(status) << std::endl;
	
}