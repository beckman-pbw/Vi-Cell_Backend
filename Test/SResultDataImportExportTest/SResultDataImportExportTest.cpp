// SResultDataImportExportTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "SResultDecoder.h"
#include "SaveResults.h"
#include "ICellCounter.h"
#include "CellCounterFactory.h"
#include "MetaData.hpp"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>   
#include "HawkeyeDirectory.hpp"
#include "boost\lexical_cast.hpp"
#include <unordered_map>

using namespace CellCounter;
bool createFile(const std::string& fileName)
{
	boost::filesystem::path fp(fileName);
	boost::filesystem::ofstream fs(fp);
	fs.close();
	return true;
}

enum eMetadataType
{
	eINVALID = 0,
	eWQ,
	eWQ_ITEM,
	eRESULT_RECORD,
	eIMAGE_RECORD,
	eIMAGE,
	eRESULTS_CSV,
};

std::map < std::string, std::pair<eMetadataType, boost::any>> mapMetaData_;

std::unordered_map<std::string, wqMetadata_st> map_AllWqMetaDataList_;
std::unordered_map<std::string, std::unordered_map<std::string, wqiMetadata_st>> map_wqToWqItemMdataList_;
std::unordered_map<std::string, std::unordered_map<std::string, ImageSetRecMetadata_st>> map_WqItemToImageSetRecMdataList_;
std::unordered_map<std::string, std::unordered_map<std::string, ResRecMetadata_st>> map_WqItemToResultRecMdataList_;

int main()
{
	/*bool status = false;
	std::vector<int>vecobj = { 12,3,4,5,6,7,89,99 };
	for (auto it : vecobj)
	{
		std::cout << " Vec Data: " << it << std::endl;
		if (it == 5)
		{
			status = true;
			return false;
		}
	}
	std::cout << std::boolalpha << status << std::endl;*/

#if 0
	CellCounterResult::SResult sresultData;
	std::string cumulativefilename = "3-3-scout.csv";
	std::string singleImagefolderPath = "C:\\DEV\\Hawkeye\\ApplicationSource.ycr\\BuildOutput\\x64\\Debug\\Images_and_Data\\ScoutSingleImageAnalysis_3-3-scout\\";
	std::string cumulativeFolder = "C:\\DEV\\Hawkeye\\ApplicationSource.ycr\\BuildOutput\\x64\\Debug\\Images_and_Data\\ScoutExcelResults_3-3-scout\\";
	std::string singleImageFolder = "C:\\DEV\\Hawkeye\\ApplicationSource.ycr\\BuildOutput\\x64\\Debug\\Images_and_Data\\SingleImage\\";
	SResultDecoder decoder;
	decoder.GetImageDataFiles(cumulativeFolder, cumulativefilename, singleImagefolderPath);
	decoder.GetSResult(sresultData);
	SaveResult saveSResult_;
	ICellCounter *pCellCounter;
	pCellCounter = CellCounter::CellCounterFactory::CreateCellCounterInstance();
	saveSResult_.SaveAnalysisInfo(sresultData, "C:\\DEV\\Hawkeye\\ApplicationSource\\BuildOutput\\x64\\Debug\\Images_and_Data\\", "cumulative23", *pCellCounter);
	int NoOfFilesOrImages = decoder.GetSingleImageFilesCount();
	saveSResult_.SaveSingleImageInfo(sresultData, "C:\\DEV\\Hawkeye\\ApplicationSource.ycr\\BuildOutput\\x64\\Debug\\Images_and_Data\\SingelImageData.csv", 2, *pCellCounter);
#endif
#if 0
	for (int count = 1; count < 5; count++)
	{
		stringstream filename;
		filename << singleImageFolder << "singleImage" << count;
		string strfileName;
		filename >> strfileName;
		filename.clear();
		saveSResult_.SaveSingleImageInfo(sresultData, strfileName, count, *pCellCounter);
	}
#endif

	auto pMetaData = make_shared<MetaDataCreator>("BCI");
	// MetaData File creation
#if 1
	for (auto wqIndex = 0; wqIndex < 2; wqIndex++)
	{
		pMetaData->WriteWqMetaDataToDisk();
		for (auto wqItemIndex = 0; wqItemIndex < 2; wqItemIndex++)
		{
			WorkQueueItemDLL tmp;
			std::string label = boost::str(boost::format("%s[%d]") % "Sample"% wqItemIndex);
			tmp.label = label;
			tmp.celltypeIndex = wqItemIndex + 10;
			tmp.analysisIndex = wqItemIndex + 20;
			tmp.bp_qc_name = "QC/BP";
			pMetaData->WriteWqItemMetaDataToDisk(tmp);
			// Create one ResultRecord Directory
			pMetaData->WriteResultRecordMetaDataToDisk();

			for (auto results = 0; results < 5; results++)
			{
				// Create one SampleImageSet Directory
				auto imageNo = boost::str(boost::format("%d") % results);
				pMetaData->WriteImageSetRecordMetaDataToDisk(imageNo);

				std::string singleImageCsvFile = boost::str(boost::format("%s\\%s\\imageNO_%d.csv") % pMetaData->GetResultRecordDir() % HawkeyeDirectory::getSingleCSVFilesDir() % results);
				pMetaData->WriteSingleImgCSVMetaDataToDisk(singleImageCsvFile, imageNo);
				createFile(singleImageCsvFile);
				std::string ImageFile = boost::str(boost::format("%s\\%s\\imageNO_%d.png") % pMetaData->GetImageRecordDir() % HawkeyeDirectory::getRawImagesDir() % results);
				pMetaData->WriteImageMetaDataToDisk(ImageFile, imageNo);
				createFile(ImageFile);
			}
			std::string cumulativeImageCsvFile = boost::str(boost::format("%s\\%s\\cumulative.csv") % pMetaData->GetResultRecordDir() % HawkeyeDirectory::getCumCSVImageDir());
			pMetaData->WriteCumImgCSVMetaDataToDisk(cumulativeImageCsvFile);
			createFile(cumulativeImageCsvFile);
		}
	}
#endif // End of MetaData File Creation.
	auto getUUID = [&](const std::string& uuidStr)->uuid__t
	{
		uuid__t uuidInternal;
		HawkeyeUUID uuid;
		uuid.from_string(uuidStr.c_str());
		uuid.get_uuid__t(uuidInternal);
		return uuidInternal;
	};
	static std::vector<WorkQueueRecordDLL> workQueueRecordResultDLL_;
	static std::vector<ImageRecordDLL> imageRecordDLL_;
	static std::vector<SampleImageSetRecordDLL> sampleImageSetRecordDLL_;
	static std::vector<SampleRecordDLL> sampleRecordDLL_;
	// Read The Meta data File.
	{
		auto wqMdataFile = HawkeyeDirectory::getWqMetaDataFile();
		std::vector<wqMetadata_st> pwqlist_;
		pMetaData->ReadWqMetaDataFromDisk(wqMdataFile, pwqlist_);
		//HawkeyeUUID uuid;
		auto it = pwqlist_.begin();
		std::for_each(pwqlist_.begin(), pwqlist_.end(), [&](const wqMetadata_st& wq)
		{
			std::unordered_map < std::string, wqiMetadata_st> map_wqItemUUIDStrAndMdataInternal;
			map_wqItemUUIDStrAndMdataInternal.clear();

			WorkQueueRecordDLL wqRecordInternal;
			wqRecordInternal.time_stamp = ChronoUtilities::ConvertToTimePoint(wq.wqCreationTimeStamp);
			wqRecordInternal.user_id = wq.wqUserName;
			wqRecordInternal.uuid = getUUID(wq.wqUUIDstr.c_str());

			auto wqItemMdataFilePath = HawkeyeDirectory::getWqItemMetaDataFile();
			std::vector<wqiMetadata_st>pwqItemList_;
			pMetaData->ReadWqItemMetaDataFromDisk(wqItemMdataFilePath, pwqItemList_);
			std::for_each(pwqItemList_.begin(), pwqItemList_.end(), [&](const wqiMetadata_st& wqItem)
			{
				SampleRecordDLL wqItemRecordInternal;
				std::unordered_map<std::string, ResRecMetadata_st> map_resRecUUIDStrAndMdataInternal;
				std::unordered_map<std::string, ImageSetRecMetadata_st> map_imageRecUUIDStrAndMdataInternal;

				map_resRecUUIDStrAndMdataInternal.clear();
				map_imageRecUUIDStrAndMdataInternal.clear();

				uuid__t sampleUuid;
				sampleUuid = getUUID(wqItem.wqiUUIDStr.c_str());
				//Adding the WqItem Record to Wq Record.
				wqRecordInternal.sample_records.push_back(sampleUuid);

				wqItemRecordInternal.time_stamp = ChronoUtilities::ConvertToTimePoint(wqItem.wqiCreationTimeStamp);
				wqItemRecordInternal.user_id = wqItem.wqiUserName;
				wqItemRecordInternal.uuid = sampleUuid;
				wqItemRecordInternal.sample_identifier = wqItem.wqilabel;
				wqItemRecordInternal.bp_qc_identifier = wqItem.QcBPIdentifier;

				// Reading  the  Result Record metaData file.
				auto resultRecordMetaDataFile = HawkeyeDirectory::getResultRecordMetaDataFile();
				std::vector<ResRecMetadata_st> presultRecordList_;
				pMetaData->ReadResultRecordMetaDataFromDisk(resultRecordMetaDataFile, presultRecordList_);

				std::for_each(presultRecordList_.begin(), presultRecordList_.end(), [&](const ResRecMetadata_st& resRec)
				{
					uuid__t resrecUuid;
					resrecUuid = getUUID(resRec.UUIDstr);
					// Adding ResultRecord UUID to WQ Item
					wqItemRecordInternal.result_records.push_back(resrecUuid);

					auto pair_resRecUUIDStrAndMdata = std::make_pair(resRec.UUIDstr, resRec);
					map_resRecUUIDStrAndMdataInternal.insert(pair_resRecUUIDStrAndMdata);
				}); //End of Result record list Loop

				// Reading the image Record metaData file.
				auto imageRecordMetaDataFile = HawkeyeDirectory::getImageSetRecordMetaDataFile();
				std::vector<ImageSetRecMetadata_st> pimageRecordList_;
				pMetaData->ReadImageSetRecordMetaDataFromDisk(imageRecordMetaDataFile, pimageRecordList_);

				std::for_each(pimageRecordList_.begin(), pimageRecordList_.end(), [&](const ImageSetRecMetadata_st& imRec)
				{
					SampleImageSetRecordDLL sampleImageSetRecInternal;

					uuid__t imRecUuid;
					imRecUuid = getUUID(imRec.UUIDstr);
					sampleImageSetRecInternal.uuid = imRecUuid;
					sampleImageSetRecInternal.sequence_number = boost::lexical_cast<uint16_t>(imRec.NoOfEntry);
					sampleImageSetRecInternal.user_id = imRec.UserName;
					sampleImageSetRecInternal.time_stamp = ChronoUtilities::ConvertToTimePoint(imRec.CreationTimeStamp);

					auto imageMetadataFile = boost::str(boost::format("%s\\%s")
														% imRec.fileLocation
														% HawkeyeDirectory::getImageMetaDataFile());

					std::vector<GeneralMetadata_st>pimage;
					pMetaData->ReadImageMetaDataFromDisk(imageMetadataFile, pimage);
					std::for_each(pimage.begin(), pimage.end(), [&](const GeneralMetadata_st& imdata)
					{
						ImageRecordDLL imRecInternal;
						imRecInternal.time_stamp = ChronoUtilities::ConvertToTimePoint(imdata.CreationTimeStamp);
						//imdata.fileLocation;
						// imdata.NoOfEntry;
						imRecInternal.user_id = imdata.UserName;
						uuid__t bf_imageUuid;
						bf_imageUuid = getUUID(imdata.UUIDstr);
						sampleImageSetRecInternal.brightfield_image = bf_imageUuid;
						imRecInternal.uuid = bf_imageUuid;
						imageRecordDLL_.push_back(imRecInternal);
						sampleImageSetRecordDLL_.push_back(sampleImageSetRecInternal);
					});//End of Image For Loop

					auto pair_imageRecordUUIDStrAndMdata = std::make_pair(imRec.UUIDstr, imRec);
					map_imageRecUUIDStrAndMdataInternal.insert(pair_imageRecordUUIDStrAndMdata);
					// Adding ImageRecord UUID to WQ Item
					wqItemRecordInternal.imageSets.push_back(imRecUuid);

				}); //End of Image Set record list Loop

				sampleRecordDLL_.push_back(wqItemRecordInternal);

				auto pair_wqItemUUIDstrAndMdata = std::make_pair(wqItem.wqiUUIDStr, wqItem);
				map_wqItemUUIDStrAndMdataInternal.insert(pair_wqItemUUIDstrAndMdata);

				auto pair_wqItemUUIDstrAndResRecordMap = std::make_pair(wqItem.wqiUUIDStr, map_resRecUUIDStrAndMdataInternal);
				map_WqItemToResultRecMdataList_.insert(pair_wqItemUUIDstrAndResRecordMap);

				auto pair_wqItemUUIDstrAndImageRecordMap = std::make_pair(wqItem.wqiUUIDStr, map_imageRecUUIDStrAndMdataInternal);
				map_WqItemToImageSetRecMdataList_.insert(pair_wqItemUUIDstrAndImageRecordMap);
			}); //End of Wq Item List For Loop

			workQueueRecordResultDLL_.push_back(wqRecordInternal);
			// Update the Main Wq map.
			auto pair_wqUUIDstrAndMdata = std::make_pair(wq.wqUUIDstr, wq);
			map_AllWqMetaDataList_.insert(pair_wqUUIDstrAndMdata);

			auto pair_wqUUIDAndSampleMetadata = std::make_pair(wq.wqUUIDstr, map_wqItemUUIDStrAndMdataInternal);
			map_wqToWqItemMdataList_.insert(pair_wqUUIDAndSampleMetadata);
		}); // End of Wq List For Loop
	}
	std::cout << "===================================: " << std::endl;
	std::cout << "Num of Wq Record: " << workQueueRecordResultDLL_.size() << std::endl;
	std::cout << "Num of Wq/Sample Item Record: " << sampleRecordDLL_.size() << std::endl;
	std::cout << "Num of Sample ImageSet Record: " << sampleImageSetRecordDLL_.size() << std::endl;
	std::cout << "Num of  image Record: " << imageRecordDLL_.size() << std::endl;
	std::cout << "===================================: " << std::endl;

	std::cout << "Size of map_ALLwqMetaDataList_:" << map_AllWqMetaDataList_.size() << std::endl;
	std::cout << "Size of map_wqToWqItemMdataList_:" << map_wqToWqItemMdataList_.size() << std::endl;
	std::cout << "Size of map_WqItemToImageSetRecMdataList_:" << map_WqItemToImageSetRecMdataList_.size() << std::endl;
	std::cout << "Size of map_WqItemToResultRecMdataList_:" << map_WqItemToResultRecMdataList_.size() << std::endl;
	std::cout << "===================================: " << std::endl;
	return 0;
}


//Map For{ wqUUID - Vector(pair("SamUUID's - MetaData"))} --> Single Map to Hold all the Wq Data
//Map for{ SamUUID - vector(pair(imageSetUUID, MetaData))} -- SingleMap to Hold all the Samples Data.
//Map for{ samUUID - vector(pair(resultRecordUUID, MetaData))} -- SingleMap to Hold all the Samples Data.


/* Initialize the Data
1. Read the MetaData File
2. load the MetaData into containers
 Load the Data.
3. Fetch the data from containers to data structures.
*/