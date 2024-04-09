#include "stdafx.h"

#include <functional>
#include <ios>
#include <iostream>
#include <vector>

#include <boost/algorithm/string.hpp>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/foreach.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include "DataReplicator.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeUUID.hpp"


#define POW_2_10 1024.0
#define FILE_ENCRYPTED_KEYWORD    ".e"

std::string GetEncryptedFileName(const std::string& originalFile)
{
	if (originalFile.empty())
		return std::string{};

	auto pos = originalFile.find(".", 0);
	if (pos != std::string::npos)
	{
		auto tmpStr = originalFile;
		return tmpStr.replace(pos, std::string(FILE_ENCRYPTED_KEYWORD).length()-1, FILE_ENCRYPTED_KEYWORD);
	}

	return std::string{};
}

static bool InBounds(uint32_t val, uint32_t lo, uint32_t high)
{
	return  val >= lo && val <= high;
}

static std::string getUUIDStr(const uuid__t & uuid)
{
	std::string uuidStr;
	if (!HawkeyeUUID::GetStrFromuuid__t(uuid, uuidStr))
	{
		return "";
		std::cerr << " Failed to UUID string" << std::endl;
	}
	return uuidStr;
}


std::vector<WorkQueueRecordDLL> wqr_list;
std::vector<SampleRecordDLL> wqir_list;
std::vector<SampleImageSetRecordDLL> imsr_list;
std::vector<ResultSummaryDLL> rs_list;
std::vector<ImageRecordDLL> imr_list;


void getRecordSource(std::vector<WorkQueueRecordDLL>& source)
{
	source = wqr_list;
}

void getRecordSource(std::vector<ImageRecordDLL>& source)
{
	source = imr_list;
}

void getRecordSource(std::vector<SampleImageSetRecordDLL>& source)
{
	source = imsr_list;
}

void getRecordSource(std::vector<SampleRecordDLL>& source)
{
	source = wqir_list;
}

void getRecordSource(std::vector<ResultSummaryDLL>& source)
{
	source = rs_list;
}


template<typename T>
bool retrieveRecord(const uuid__t & uuid, T & record)
{
	record = {};
	std::vector<T> source;
	getRecordSource(source);
	auto found_it = std::find_if(source.begin(), source.end(), [&uuid](const T& rec)->bool
	{
		return DataConversion::AreEqual(rec.uuid, uuid);
	});

	if (found_it == source.end())
		return false;;

	record = *found_it;
	return true;
}

bool mainMenu()
{
	//Ask the user select a workQueuerecord for creating a duplicate and adding to Disk.
	DataReplicator *pDataSource = new DataReplicator();
	wqr_list.clear();
	wqir_list.clear();
	rs_list.clear();
	imsr_list.clear();
	imr_list.clear();

	std::string inputStr;

	while (true) {
		std::cout << "Enter drive letter (Q to quit): " << std::endl;
		std::cin >> inputStr;
		if (inputStr[0] == 'q' || inputStr[0] == 'Q') {
			exit(0);
		}

		if (inputStr.length() == 1) {
			break;
		}
	}

	boost::to_upper (inputStr);
	HawkeyeDirectory::Instance().setDriveId (inputStr);

	if (!pDataSource->RetrieveDataRecords(wqr_list, wqir_list, rs_list, imsr_list, imr_list))
	{
		std::cout << "Failed to retrieve the Data" << std::endl;
		return false;
	}

	if (wqr_list.empty())
	{
		std::cout << "No Work queue records to generate the disk data" << std::endl;
		return false;
	}

	std::cout << "\nList of available work queues :" << std::endl;

	int index = 0;
	for (auto const& wqr : wqr_list)
	{
		std::cout << index++
			<< ") UUID:  " << getUUIDStr(wqr.uuid)
			<< ", Label: " << (wqr.wqLabel.empty() ? "EMPTY" : wqr.wqLabel)
			<< ", User ID: " << (wqr.username.empty() ? "EMPTY" : wqr.username)
			<< ", DateTime: " << ChronoUtilities::ConvertToString(wqr.timestamp) << std::endl;

		int i = 0;

		std::cout << "\tNum of available samples #" << wqr.sample_records.size() << std::endl;
	}

	uint32_t wqr_index;
	uint32_t numReplications;

	while (true)
	{
		std::cout << "Select the Work queue record between : |0 - " << wqr_list.size() - 1 << "| to generate the disk data" << std::endl;
		std::cout << "Enter \"q\" to exit" << std::endl;
		std::cin >> inputStr;
		if (inputStr[0] == 'q' || inputStr[0] == 'Q') {
			exit(0);
		}

		wqr_index = std::stoul(inputStr);

		if (!InBounds(wqr_index, 0ul, static_cast<uint32_t>(wqr_list.size() - 1))) {
			std::cout << "Invalid selection, try again" << std::endl;
		} 
		else 
		{
			std::cout << "Enter # of replications \"q\" to exit: " << std::endl;
			std::cin >> inputStr;
			if (inputStr[0] == 'q' || inputStr[0] == 'Q') {
				exit(0);
			}

			numReplications = std::stoul(inputStr);
			if (numReplications != 0) {
				break;
			}

			std::cout << "Zero is not valid, try again" << std::endl;
		}
	}

	std::cin.clear();

	// Calculate the memory.
	// Total memory = Memory required for each sample images and result records.
	double total_memory = 0; //In MB
	auto image_path = HawkeyeDirectory::Instance().getImagesBaseDir();

	for (uint32_t cnt = 0; cnt < numReplications; cnt++) {
		for (auto const& sr_uuid : wqr_list[wqr_index].sample_records)
		{
			auto sr_uuid_str = getUUIDStr(sr_uuid);
			auto path = image_path + "\\" + sr_uuid_str;
			auto images_size = FileSystemUtilities::GetSizeoFTheFileorDirectory(path) / (POW_2_10 * POW_2_10);

			total_memory += images_size;

			//Find the Sample Record
			SampleRecordDLL sr;
			if (!retrieveRecord(sr_uuid, sr))
			{
				std::cerr << "Failed to retrieve sample record for : " << sr_uuid_str << std::endl;
				return false;
			}

			for (auto const& rs : sr.result_summaries)
			{
				//Find the Result record
			/*	ResultRecordDLL rr;
				if (!retrieveRecord(rr_uuid, rr))
				{
					std::cerr << "Failed to retrieve result record for : " << getUUIDStr(rr_uuid) << std::endl;
					return false;
				}*/
				// Get the encrypted file name, as the files stored on the disk are encrypted but the path stored ion the meta data is not encrypted.
				std::string en_filename = GetEncryptedFileName(rs.path);

				auto rr_size = FileSystemUtilities::GetSizeoFTheFileorDirectory(en_filename) / (POW_2_10 * POW_2_10);
				total_memory += rr_size;
			}
		}
	}

	boost::filesystem::space_info space_info = FileSystemUtilities::GetSpaceInfoOfDisk(HawkeyeDirectory::Instance().getDriveId());
	std::cout << " Disk space used : " << (space_info.capacity - space_info.free) / (POW_2_10 * POW_2_10 * POW_2_10) << " GB" << std::endl;
	std::cout << " Disk space free : " << space_info.free / (POW_2_10 * POW_2_10 * POW_2_10) << " GB" << std::endl;

	std::cout << "\nTotal memory required to add the Data for the selected work queue is  : " << total_memory/ POW_2_10 << " GB" << std::endl;

	if (!FileSystemUtilities::IsRequiredFreeDiskSpaceInMBAvailable(static_cast<uint64_t>(total_memory)))
	{
		std::cout << "Oops Low Disk space ...!" << std::endl;
		getchar();
		return false;
	}

	std::cout << "Please wait adding the data ...." << std::endl;
	auto st = ChronoUtilities::CurrentTime();
	for (uint32_t cnt = 0; cnt < numReplications; cnt++) {

		// Generate new UUID for Work queue record
		auto new_wqr = wqr_list[wqr_index];
		HawkeyeUUID::Generate().get_uuid__t(new_wqr.uuid = {});

		for (auto const& sr_uuid : new_wqr.sample_records)
		{
			SampleRecordDLL sr = {};
			std::vector<ResultSummaryDLL> rs_list_data;
			std::vector<ImageSetDataRecord_t> image_data_list;
			ImageSetDataRecord_t bg_image_data;

			rs_list_data.clear();
			image_data_list.clear();

			if (!retrieveRecord(sr_uuid, sr))
			{
				std::cerr << "Failed to retrieve sample record for : " << getUUIDStr(sr_uuid) << std::endl;
				return false;
			}

			auto old_sr_uuid_str = getUUIDStr(sr_uuid);
			HawkeyeUUID::Generate().get_uuid__t(sr.uuid = {}); // Generate new UUID for Sample record
			auto new_wqir_uuid_str = getUUIDStr(sr.uuid);

			for (auto const& imsr_uuid : sr.imageSets)
			{
				ImageSetDataRecord_t imagedata = {};
				SampleImageSetRecordDLL imsr = {};
				if (!retrieveRecord(imsr_uuid, imsr))
				{
					std::cerr << "Failed to retrieve image set record for : " << getUUIDStr(imsr_uuid) << std::endl;
					return false;
				}
				ImageRecordDLL bfimr = {};
				if (!retrieveRecord(imsr.brightfield_image, bfimr))
				{
					std::cerr << "Failed to retrieve image record for : " << getUUIDStr(imsr.brightfield_image) << std::endl;
					return false;
				}

				//Change the path of ImageRecord
				//std::cout << "Image path before :\t " << bfimr.path << std::endl;
				bfimr.path.replace(33, 36, new_wqir_uuid_str);
				//std::cout << "Image path After :\t " << bfimr.path << std::endl;

				imagedata.bf_imr = bfimr;

				for (auto const& flimr_pair : imsr.flImagesAndChannelNumlist_)
				{

					ImageRecordDLL flimr = {};
					if (!retrieveRecord(flimr_pair.second, flimr))
					{
						std::cerr << "Failed to retrieve fluorescent image record for : " << getUUIDStr(flimr_pair.second) << std::endl;
						return false;
					}

					//Change the path of ImageRecord
					//std::cout << "Image path before :\t " << flimr.path << std::endl;
					flimr.path.replace(33, 36, new_wqir_uuid_str);
					//std::cout << "Image path After :\t " << flimr.path << std::endl;

					imagedata.fl_imr_list.emplace_back(flimr);
				}

				imagedata.imsr = imsr;
				image_data_list.emplace_back(imagedata);
			}

			//Background image data
			if (!(HawkeyeUUID(sr.imageNormalizationData).isNIL()))
			{
				SampleImageSetRecordDLL bgimsr = {};
				if (!retrieveRecord(sr.imageNormalizationData, bgimsr))
				{
					std::cerr << "Failed to retrieve background image set record for : " << getUUIDStr(sr.imageNormalizationData) << std::endl;
					return false;
				}

				if (!(HawkeyeUUID(bgimsr.brightfield_image).isNIL()))
				{
					ImageRecordDLL bg_bfimr = {};
					if (!retrieveRecord(bgimsr.brightfield_image, bg_bfimr))
					{
						std::cerr << "Failed to retrieve background image record for : " << getUUIDStr(bgimsr.brightfield_image) << std::endl;
						return false;
					}

					//Change the path of ImageRecord
					//std::cout << "Image path before :\t " << bg_bfimr.path << std::endl;
					bg_bfimr.path.replace(33, 36, new_wqir_uuid_str);
					//std::cout << "Image path After :\t " << bg_bfimr.path << std::endl;

					bg_image_data.bf_imr = bg_bfimr;
				}

				
				if (!bgimsr.flImagesAndChannelNumlist_.empty())
				{
					for (auto const& bg_flimr_pair : bgimsr.flImagesAndChannelNumlist_)
					{
						if (!(HawkeyeUUID(bg_flimr_pair.second).isNIL()))
						{
							ImageRecordDLL bg_flimr = {};
							if (!retrieveRecord(bg_flimr_pair.second, bg_flimr))
							{
								std::cerr << "Failed to retrieve background image record for : " << getUUIDStr(bg_flimr_pair.second) << std::endl;
								return false;
							}

							//Change the path of ImageRecord
							//std::cout << "Image path before :\t " << bg_flimr.path << std::endl;
							bg_flimr.path.replace(33, 36, new_wqir_uuid_str);
							//std::cout << "Image path After :\t " << bg_flimr.path << std::endl;

							bg_image_data.fl_imr_list.emplace_back(bg_flimr);
						}
					}
				}

			}

			auto oldImagePath = boost::str(boost::format("%s\\%s") % HawkeyeDirectory::Instance().getImagesBaseDir() % old_sr_uuid_str);
			auto newImagePath = boost::str(boost::format("%s\\%s") % HawkeyeDirectory::Instance().getImagesBaseDir() % new_wqir_uuid_str);

			boost::system::error_code ec;
			boost::filesystem::copy_directory (oldImagePath, newImagePath, ec);

			if (ec)
			{
				std::cerr << "Failed to copy the contents of the folder \n From : " << oldImagePath << "\n To :" << newImagePath << std::endl;
				return false;
			}

			//Create Result record meta data and copy all the result binaries.
			for (auto const& rs : sr.result_summaries)
			{
				ResultSummaryDLL new_rs = rs;
				//Find the Result record
			/*	ResultRecordDLL rr = {};
				if (!retrieveRecord(rr_uuid, rr))
				{
					std::cerr << "Failed to retrieve result record for : " << getUUIDStr(rr_uuid) << std::endl;
					return false;
				}*/

				HawkeyeUUID::Generate().get_uuid__t(new_rs.uuid = {}); // Generate new UUID for result record

                 // Get the encrypted file name
				std::string oldPath = GetEncryptedFileName(new_rs.path);
				// Create the new Result record binary path
				new_rs.path.replace (41, 36, getUUIDStr(new_rs.uuid));
				// Get the new encrypted file name
				std::string newPath = GetEncryptedFileName(new_rs.path);

			//	std::cout << "Result record path before :\t " << oldPath << std::endl;
			//	std::cout << "Result record path After :\t " << newPath << std::endl;

				boost::system::error_code ec;
				boost::filesystem::copy_file (oldPath, newPath, ec);
				if (ec != boost::system::errc::success)
				{
					std::cerr << "Failed to copy the Result record file" << std::endl;
					return false;
				}

				rs_list_data.emplace_back(new_rs);
			}

			//Now add the metadata to Disk
			for (const auto& tmprr : rs_list_data)
			{
				if (!pDataSource->SaveDataRecords(new_wqr,
					sr,
					tmprr,
					image_data_list,
					bg_image_data))
				{
					std::cerr << "Failed to add the MetaData to disk !" << std::endl;
					return false;
				}

				image_data_list.clear();
			}

		}
	}
	auto et = ChronoUtilities::CurrentTime();
	std::cout << "\nSUCCESSFULLY ADDED THE DATA" << std::endl;
	std::cout << "Total Time Taken (sec) : " << std::chrono::duration_cast<std::chrono::seconds>(et - st).count() << std::endl;
	delete pDataSource;
	return true;
}

void mainMenuAlternate()
{
	std::string inputStr;

	while (true)
	{
		std::cout << "Enter drive letter (Q to quit): " << std::endl;
		std::cin >> inputStr;
		if (inputStr[0] == 'q' || inputStr[0] == 'Q')
		{
			exit(0);
		}

		if (inputStr.length() == 1)
		{
			break;
		}
	}

	boost::to_upper(inputStr);
	HawkeyeDirectory::Instance().setDriveId(inputStr);

	DataReplicator::duplicateData();
}

int main(int argc, char *argv[])
{
	if (argc > 1 && (std::string(argv[1]) == "-a" || std::string(argv[1]) == "-A"))
	{
		std::cout << "Launching Alternate Mode" << std::endl;
		mainMenuAlternate();
	}
	else
	{
		while (mainMenu());
	}

	getchar();
	return 0;
}


//Make a copy of image data
//Make a copy of result record data.

// Get the FSDATASource Class into this project
// Include FileSysytem Utilities and resolve the errors

//LOOP - start
//Create the path of images and the result record
//Calculate the memory occupied
//LOOP - end

//Check the total memory of the sample data and verify against the total memory

// Create the tree
// Work Queue Record -->Generate new UUID
//		List of Sample Records
//				List of Image Set Records -->Generate new UUID
//				List of Result Records --> Generate new UUID
//Change the Image path 
//Replace 36 characters starting from 33-68
//C:\Instrument\ResultsData\Images\6a883f2b-161d-465f-956a-897c1f27e4c3\3\CamBF\CamBF_3.png
//C:\Instrument\ResultsData\Images\########-####-####-####-############\3\CamBF\CamBF_3.png

// Add the MetaData for this new List of Hybrid data.

// Save the Actual Data into disk now.
//1. Copy the Image data and paste it into the newly created Sample_UUID folder.
//2. Copy the Result data and paste it into the newly created Result_UUID file.