#pragma once

#include <iostream>
#include  <string.h>

#include "HawkeyeError.hpp"
#include "HawkeyeLogic.hpp"
#include "HawkeyeUUID.hpp"


class RetrieveAPIsTest
{
public:
	static void SampleRecordTest(); // Made static to invoke from the reanalyzeCallback
	static void reanalyzeCallback(HawkeyeError he, uuid__t sample_uuid, ResultRecord * rr);
	void Reanalyzetest(bool from_images);
	static void DeleteCompletionCb(HawkeyeError status, uuid__t uuid);
	static void PrintResults();
	static std::string getUUIDStr(const uuid__t& uuid);

	static void ExportCompletionCb(HawkeyeError status, char* archive_filepath);
	static void ExportProgressCb(HawkeyeError status, uuid__t uuid);
	void ExportDataTest();

	void WorkqueueRecordTest();
	void SampleImageSetRecordTest();
	void ImageRecordTest();
	void ResultrecordTest(bool print_results);
	void SampleRecordForBPQCTest(std::string BPQC);
	void BiprocessTest();
	void QualityControlTest();
	void CellTypeTest();
	void AnalysisTest();
	void TempCellTypeTest();
	void TempAnalysisTest();
	void RetrieveSampleDataTest();
	void GetCellTypeIndicesTest();
	void DeleteParticularWqrTest(bool retain_result_and_first_image);
	void DeleteAllWqirTest(bool retain_result_and_first_image);
	void DeleteResultsDataTest();
	void DeleteParticularWqirTest(bool retain_result_and_first_image);
	void DeleteAllWqrTest(bool retain_result_and_first_image);
	void DeleteParticularResultRecordTest(bool clean_up_empties);

	void RetrieveImageTest();
	std::vector<uuid__t> imageRecUUIDList;
};