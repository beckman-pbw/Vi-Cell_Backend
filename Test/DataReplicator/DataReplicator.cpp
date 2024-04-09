#include "stdafx.h"

#include "boost\filesystem.hpp"
#include "boost\foreach.hpp"

#include "DataReplicator.hpp"

#define MODULENAME "DataReplicator"

using namespace PtreeConversionHelper;

static void replaceString(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return;
	str.replace(start_pos, from.length(), to);
	return;
}

static bool copyDir(
	boost::filesystem::path const & source,
	boost::filesystem::path const & destination
)
{
	namespace fs = boost::filesystem;
	try
	{
		// Check whether the function call is valid
		if (
			!fs::exists(source) ||
			!fs::is_directory(source)
			)
		{
			std::cerr << "Source directory " << source.string()
				<< " does not exist or is not a directory." << '\n'
				;
			return false;
		}
		if (fs::exists(destination))
		{
			std::cerr << "Destination directory " << destination.string()
				<< " already exists." << '\n'
				;
			return false;
		}
		// Create the destination directory
		if (!fs::create_directory(destination))
		{
			std::cerr << "Unable to create destination directory"
				<< destination.string() << '\n'
				;
			return false;
		}
	}
	catch (fs::filesystem_error const & e)
	{
		std::cerr << e.what() << '\n';
		return false;
	}
	// Iterate through the source directory
	for (
		fs::directory_iterator file(source);
		file != fs::directory_iterator(); ++file
		)
	{
		try
		{
			fs::path current(file->path());
			if (fs::is_directory(current))
			{
				// Found directory: Recursion
				if (
					!copyDir(
						current,
						destination / current.filename()
					)
					)
				{
					return false;
				}
			}
			else
			{
				// Found file: Copy
				fs::copy_file(
					current,
					destination / current.filename()
				);
			}
		}
		catch (fs::filesystem_error const & e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	return true;
}

static boost::property_tree::ptree duplicateWorkQueueItem(
	boost::property_tree::ptree& originalWQI, std::string sampleIdNameExtension, bool saveMinumImageSet)
{
	boost::property_tree::ptree newWQI = originalWQI;
	auto currentTime = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime());
	std::string srcImageFolder;
	std::string dstImageFolder;
	std::string srcResultFile;
	std::string dstResultFile;

	newWQI.put("UUID", HawkeyeUUID::Generate().to_string());
	newWQI.put("TimeStamp", currentTime);

	if (!sampleIdNameExtension.empty())
	{
		auto sampleIdName = newWQI.get<std::string>("SampleId");
		sampleIdName += sampleIdNameExtension;
		newWQI.put("SampleId", sampleIdName);
	}

	auto imageSetList = newWQI.equal_range("ImageSet");
	auto islItr = imageSetList.first;
	while (islItr != imageSetList.second)
	{
		newWQI.erase(islItr->first);
		imageSetList = newWQI.equal_range("ImageSet");
		islItr = imageSetList.first;
	}

	imageSetList = originalWQI.equal_range("ImageSet");
	auto lastEntry = imageSetList.second;
	lastEntry--;
	for (auto it = imageSetList.first; it != imageSetList.second; ++it)
	{
		auto pTree = it->second;
		if (saveMinumImageSet)
		{
			if (it != imageSetList.first && it != lastEntry)
			{
				continue;
			}
		}

		pTree.put("UUID", HawkeyeUUID::Generate().to_string());
		pTree.put("TimeStamp", (currentTime++));

		pTree.put("BFImage.UUID", HawkeyeUUID::Generate().to_string());
		pTree.put("BFImage.TimeStamp", currentTime);

		std::string path = pTree.get<std::string>("BFImage.Path");
		if (srcImageFolder.empty())
		{
			auto uuid = originalWQI.get<std::string>("UUID");
			size_t start_pos = path.find(uuid) + uuid.length();
			if (start_pos != std::string::npos)
			{
				srcImageFolder = path;
				srcImageFolder.replace(start_pos, srcImageFolder.length(), "");
				srcImageFolder = HawkeyeDirectory::Instance().getDriveId() + "/" + srcImageFolder;
			}
		}
		replaceString(
			path,
			originalWQI.get<std::string>("UUID"),
			newWQI.get<std::string>("UUID"));

		if (dstImageFolder.empty())
		{
			auto uuid = newWQI.get<std::string>("UUID");
			size_t start_pos = path.find(uuid) + uuid.length();
			if (start_pos != std::string::npos)
			{
				dstImageFolder = path;
				dstImageFolder.replace(start_pos, dstImageFolder.length(), "");
				dstImageFolder = HawkeyeDirectory::Instance().getDriveId() + "/" + dstImageFolder;
			}
		}
		pTree.put("BFImage.Path", path);

		newWQI.add_child("ImageSet", pTree);
	}

	auto resultList = newWQI.equal_range("Result");
	for (auto it = resultList.first; it != resultList.second; ++it)
	{
		auto orgResultUUID = it->second.get<std::string>("UUID");
		it->second.put("UUID", HawkeyeUUID::Generate().to_string());
		it->second.put("TimeStamp", (currentTime));

		std::string path = it->second.get<std::string>("Path");
		GetEncryptedFileName(HawkeyeDirectory::Instance().getDriveId() + "/" + path, srcResultFile);
		replaceString(
			path,
			orgResultUUID,
			it->second.get<std::string>("UUID"));
		it->second.put("Path", path);
		GetEncryptedFileName(HawkeyeDirectory::Instance().getDriveId() + "/" + path, dstResultFile);
	}

	/*std::cout << "Source Image Folder : " << srcImageFolder << std::endl;
	std::cout << "Destination Image Folder : " << dstImageFolder << std::endl;
	std::cout << std::endl;
	std::cout << "Source Result File : " << srcResultFile << std::endl;
	std::cout << "Destination Result File : " << dstResultFile << std::endl;
	std::cout << std::endl << std::endl;*/

	std::cout << "Processing UUID : " << newWQI.get<std::string>("UUID") << std::endl;

	boost::filesystem::copy(srcResultFile, dstResultFile);

	if (!saveMinumImageSet)
	{
		copyDir(srcImageFolder, dstImageFolder);
	}
	else
	{
		FileSystemUtilities::CreateDirectories(dstImageFolder);
		bool success = copyDir(srcImageFolder + "/1", dstImageFolder + "/1");
		success = copyDir(srcImageFolder + "/100", dstImageFolder + "/100");
		if (!success)
		{
			std::cout << "Failed to save image set : " << (dstImageFolder + "/1") << std::endl;
		}
	}

	return newWQI;
}

static boost::property_tree::ptree duplicateWorkQueue(boost::property_tree::ptree& originalWQ, uint16_t repeatWQICount, bool saveMinumImageSet)
{
	boost::property_tree::ptree newWQ = originalWQ;
	auto currentTime = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime());

	newWQ.put("UUID", HawkeyeUUID::Generate().to_string());
	newWQ.put("TimeStamp", currentTime);

	std::vector<boost::property_tree::ptree> newWQIList;
	BOOST_FOREACH(boost::property_tree::ptree::value_type &it, originalWQ)
	{
		if (it.first == "WorkQueueItem")
		{
			for (uint16_t index = 0; index < repeatWQICount; index++)
			{
				newWQIList.emplace_back(duplicateWorkQueueItem(it.second, repeatWQICount > 1 ? (boost::str(boost::format(".%03d") % (index+1))) : std::string(), saveMinumImageSet));
				currentTime = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(ChronoUtilities::CurrentTime());
				
			}
			newWQ.erase(it.first);
		}
	}

	for (auto& item : newWQIList)
	{
		newWQ.add_child("WorkQueueItem", item);
	}

	return newWQ;
}

DataReplicator::DataReplicator()
{
	isExistingMetaDataIsRead_ = false;
}

bool DataReplicator::ReadMetaDataFromDisk()
{
	//Logger::L().Log (MODULENAME, severity_level::debug1, "ReadMetaDataFromDisk: <Enter>");

	if (isExistingMetaDataIsRead_)
		return true;

	try
	{
		std::string mdata_filename = HawkeyeDirectory::Instance().getMetaDataFile();
		if (!EncryptedFileExists(mdata_filename))
			return true;

		if (!ReadPtreeDataFromEncryptedFile(mdata_filename, mainMetaDataTree_))
		{
			if (!RestoreEncryptedFileFromBackup(mdata_filename))
			{
				//Logger::L().Log (MODULENAME, severity_level::error, "ReadMetaDataFromDisk: <Exit : Failed to restore the encrypted Metadata XML file from backup>");
				ClearMetaDataPtrees();
				return false;
			}

			if (!ReadPtreeDataFromEncryptedFile(mdata_filename, mainMetaDataTree_))
			{
				//Logger::L().Log (MODULENAME, severity_level::error, "ReadMetaDataFromDisk: <Exit : Failed to read the encrypted Metadata XML file>");
				ClearMetaDataPtrees();
				return false;
			}
		}

		isExistingMetaDataIsRead_ = true;
	}

	catch (boost::property_tree::xml_parser_error er)
	{
		//Logger::L().Log (MODULENAME, severity_level::error, "ReadMetaDataFromDisk: <Exit Exception occurred while reading Metadata XML file : exception " + std::string(er.what()) + " >");
		ClearMetaDataPtrees();
		return false;
	}

	//Logger::L().Log (MODULENAME, severity_level::debug1, "ReadMetaDataFromDisk: <Exit>");
	return true;
}

bool DataReplicator::WriteMetaDataToDisk()
{
	//Logger::L().Log (MODULENAME, severity_level::debug1, "WriteMetaDataToDisk: <Enter>");
	try
	{
		std::string mdata_filename = HawkeyeDirectory::Instance().getMetaDataFile();
		std::string rdata_dir = HawkeyeDirectory::Instance().getResultsDataBaseDir();
		if (mdata_filename.empty() || rdata_dir.empty())
		{
			//Logger::L().Log (MODULENAME, severity_level::error, "WriteMetaDataToDisk: <Exit No metadata to write/ empty metadata file string/empty directory string >");
			return false;
		}

		if (!FileSystemUtilities::FileExists(rdata_dir) && !FileSystemUtilities::CreateDirectories(rdata_dir))
		{
			//Logger::L().Log (MODULENAME, severity_level::error, "WriteMetaDataToDisk: <Exit : Failed to create the Results data directory: " + rdata_dir + " >");
			return false;
		}

		//boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
		//boost::property_tree::write_xml(mdata_filename, mainMetaDataTree_, std::locale(), settings);

		if (!WritePtreeDataToEncryptedFile(mdata_filename, mainMetaDataTree_))
		{
			//Logger::L().Log (MODULENAME, severity_level::error, "WriteMetaDataToDisk: <Exit : Failed to write the Metadata XML file >");
			return false;
		}
		
		if (!SaveEncryptedFileToBackup(mdata_filename))
		{
			//Logger::L().Log (MODULENAME, severity_level::error, "WriteMetaDataToDisk: <Failed to backup the Metadata XML file >");
		}
	}

	catch (boost::property_tree::xml_parser_error er)
	{
		//Logger::L().Log (MODULENAME, severity_level::error, "WriteMetaDataToDisk: <Exit Exception occurred while writing Metadata XML file : exception " + std::string(er.what()) + " >");
		return false;
	}

	//Logger::L().Log (MODULENAME, severity_level::debug1, "WriteMetaDataToDisk: <exit>");
	return true;
}

void DataReplicator::ClearMetaDataPtrees()
{
	mainMetaDataTree_.clear();
	isExistingMetaDataIsRead_ = false;
}

DataReplicator::~DataReplicator()
{
}

bool DataReplicator::SaveDataRecords(const WorkQueueRecordDLL & wqr,
									 const SampleRecordDLL & wqir,
									 const ResultSummaryDLL & rs,
									 const std::vector<ImageSetDataRecord_t>& image_data_list,
									 const ImageSetDataRecord_t& background_img_data)
{
	try
	{
		std::lock_guard<std::mutex>guard(mtx_MetaDataHandler);
		//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Enter>");

		if (!ReadMetaDataFromDisk())
		{
			//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Exit Failed to read the existing Metadata>");
			return false;
		}

		// Note : path is there only in "ResultRecordDll" and "ImageRecordDLL" structures.
		auto strip_drive_id = [](std::string& path) 
		{
			if (path.length() > 2 && path[1] == ':')
				path = std::string(&path[2]);
		};

		auto strip_drive_id_from_imagedata = [strip_drive_id](ImageSetDataRecord_t& im_data)
		{
			
			strip_drive_id(im_data.bf_imr.path); // Stripping the drive ID from BF image record path

			for (ImageRecordDLL& fl_imr : im_data.fl_imr_list)
			{
				strip_drive_id(fl_imr.path); // Stripping the drive ID from FL image record path
			}
		};

		std::string ip_wqr_uuid_str = {};
		if (!HawkeyeUUID::GetStrFromuuid__t(wqr.uuid, ip_wqr_uuid_str))
		{
			//Logger::L().Log (MODULENAME, severity_level::error, "SaveDataRecords: <Exit, Failed to create uuid string for wq>");
			return false;
		}

		// Find whether the mdata for given wq is available 
		auto wqr_range = mainMetaDataTree_.equal_range(WORKQUEUE_NODE);
		auto wqr_it = wqr_range.first;
		for (wqr_it; wqr_it != wqr_range.second; wqr_it++)
		{
			std::string uuid_str = wqr_it->second.get<std::string>(UUID_NODE);

			if (ip_wqr_uuid_str == uuid_str)
				break;
		}

		if (wqr_it == wqr_range.second) // New Wq entry
		{
			//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Creating new wq entry : Enter>");
			pt::ptree wqr_node_tree = {};
			if (!ConvertToPtree(wqr, wqr_node_tree))
				return false;

			pt::ptree wqir_node_tree = {};
			if (!ConvertToPtree(wqir, wqir_node_tree))
				return false;

			for (auto im_data : image_data_list)
			{
				strip_drive_id_from_imagedata(im_data);
				
				const auto& imsr = im_data.imsr;
				auto bf_imr = im_data.bf_imr;
				auto fl_imr_list = im_data.fl_imr_list;

				pt::ptree imsr_node_tree = {};
				if (!ConvertToPtree(imsr, bf_imr, fl_imr_list, imsr_node_tree))
					return false;
				wqir_node_tree.add_child(IMAGE_SET_NODE, imsr_node_tree);
			}
			// Normalization data
			HawkeyeUUID ndu(background_img_data.imsr.uuid);
			if (!ndu.isNIL())
			{
				auto im_data = background_img_data;
				strip_drive_id_from_imagedata(im_data);

				const auto& imsr = im_data.imsr;
				auto bf_imr = im_data.bf_imr;
				auto fl_imr_list = im_data.fl_imr_list;

				pt::ptree imsr_node_tree = {};
				if (!ConvertToPtree(imsr, bf_imr, fl_imr_list, imsr_node_tree))
					return false;
				wqir_node_tree.add_child(NORMALIZATION_IMAGE_SET_NODE, imsr_node_tree);
			}

			pt::ptree rr_node_tree = {};
			auto tmp_rs = rs;
			strip_drive_id(tmp_rs.path); // Stripping the drive ID from result record path
			if (!ConvertToPtree(tmp_rs, rr_node_tree))
				return false;

			wqir_node_tree.add_child(RESULT_NODE, rr_node_tree);
			wqr_node_tree.add_child(WORKQUEUE_ITEM_NODE, wqir_node_tree);

			mainMetaDataTree_.add_child(WORKQUEUE_NODE, wqr_node_tree);

//			Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Creating new wq entry : Exit>");
		}
		else
		{
			// Check whether it is a new sample data or reanalysis data
			std::string ip_wqir_uuid_str = {};
			if (!HawkeyeUUID::GetStrFromuuid__t(wqir.uuid, ip_wqir_uuid_str))
			{
//				Logger::L().Log (MODULENAME, severity_level::error, "SaveDataRecords: <Exit Failed to create uuid string for wqir>");
				return false;
			}

			auto wqir_range = wqr_it->second.equal_range(WORKQUEUE_ITEM_NODE);
			auto wqir_it = wqir_range.first;
			for (wqir_it; wqir_it != wqir_range.second; wqir_it++)
			{
				std::string uuid_str = wqir_it->second.get<std::string>(UUID_NODE);
				if (ip_wqir_uuid_str == uuid_str)
					break;
			}

			if (wqir_it == wqir_range.second) // New sample data
			{
				//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Creating new wqi entry : Enter>");
				pt::ptree wqir_node_tree = {};
				if (!ConvertToPtree(wqir, wqir_node_tree))
					return false;

				for (auto im_data : image_data_list)
				{
					strip_drive_id_from_imagedata(im_data);

					const auto& imsr = im_data.imsr;
					auto bf_imr = im_data.bf_imr;
					auto fl_imr_list = im_data.fl_imr_list;

					pt::ptree imsr_node_tree = {};
					if (!ConvertToPtree(imsr, bf_imr, fl_imr_list, imsr_node_tree))
						return false;
					wqir_node_tree.add_child(IMAGE_SET_NODE, imsr_node_tree);
				}
				HawkeyeUUID ndu(wqir.imageNormalizationData);
				if (!ndu.isNIL())
				{
					auto im_data = background_img_data;
					strip_drive_id_from_imagedata(im_data);

					const auto& imsr = im_data.imsr;
					auto bf_imr = im_data.bf_imr;
					auto fl_imr_list = im_data.fl_imr_list;

					pt::ptree imsr_node_tree = {};
					if (!ConvertToPtree(imsr, bf_imr, fl_imr_list, imsr_node_tree))
						return false;
					wqir_node_tree.add_child(NORMALIZATION_IMAGE_SET_NODE, imsr_node_tree);
				}

				pt::ptree rr_node_tree = {};
				auto tmp_rs = rs;
				strip_drive_id(tmp_rs.path); // Stripping the drive ID from result record path
				if (!ConvertToPtree(tmp_rs, rr_node_tree))
					return false;
				wqir_node_tree.add_child(RESULT_NODE, rr_node_tree);

				wqr_it->second.add_child(WORKQUEUE_ITEM_NODE, wqir_node_tree);
				//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Creating new wqi entry : Exit>");
			}
			else // Reanalysis
			{
				//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Creating new Reanlysis entry : Entry>");
				pt::ptree rr_node_tree = {};
				auto tmp_rs = rs;
				strip_drive_id(tmp_rs.path); // Striping the drive ID from result record path
				if (!ConvertToPtree(tmp_rs, rr_node_tree))
					return false;
				wqir_it->second.add_child(RESULT_NODE, rr_node_tree);
				//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Creating new Reanlysis entry : Exit>");
			}
		}
	}
	catch (const pt::ptree_error& er)
	{
		//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Exception Occured: " + std::string(er.what()) + " <Exit>");
		return false;
	}

	// Write the Metadata to disk
	if (!WriteMetaDataToDisk())
	{
		//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Failed to Write the meta data <Exit>");
		return false;
	}
	//Logger::L().Log (MODULENAME, severity_level::debug1, "SaveDataRecords: <Exit>");

	return true;
}

bool DataReplicator::RetrieveDataRecords(std::vector<WorkQueueRecordDLL>& wqr_list,
											std::vector<SampleRecordDLL>& wqir_list,
											std::vector<ResultSummaryDLL>& rs_list,
											std::vector<SampleImageSetRecordDLL>& imsr_list,
											std::vector<ImageRecordDLL>& imr_list)
{
	try
	{
		std::lock_guard<std::mutex>guard(mtx_MetaDataHandler);
		//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecords: <Enter>");
		wqr_list.clear(); wqir_list.clear(); rs_list.clear(); imsr_list.clear(); imr_list.clear();

		if (!ReadMetaDataFromDisk())
		{
			//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecords: <Exit failed to read the Existing data>");
			return false;
		}

		auto add_drive_id = [](std::string& path) 
		{
			path = HawkeyeDirectory::Instance().getDriveId() + path;
		};

		std::pair<pt_assoc_it, pt_assoc_it> wqr_range = mainMetaDataTree_.equal_range(WORKQUEUE_NODE);
		for (auto wqr_it = wqr_range.first; wqr_it != wqr_range.second; wqr_it++)
		{
			WorkQueueRecordDLL tmp_wqr = {};
			if (!ConvertToStruct(wqr_it, tmp_wqr))
				return false;

			std::pair<pt_assoc_it, pt_assoc_it> wqir_range = wqr_it->second.equal_range(WORKQUEUE_ITEM_NODE);
			for (auto wqir_it = wqir_range.first; wqir_it != wqir_range.second; wqir_it++)
			{
				SampleRecordDLL tmp_wqir = {};
				if (!ConvertToStruct(wqir_it, tmp_wqir))
					return false;

				std::pair<pt_assoc_it, pt_assoc_it> rs_range = wqir_it->second.equal_range(RESULT_NODE);
				for (auto rs_it = rs_range.first; rs_it != rs_range.second; rs_it++)
				{
					ResultSummaryDLL tmp_rs = {};

					if (!ConvertToStruct(rs_it, tmp_rs))
						return false;
					add_drive_id(tmp_rs.path); // Add the drive id to result record path

					//Update the result record list
					rs_list.push_back(tmp_rs);
					tmp_wqir.result_summaries.push_back(tmp_rs);
				}
				// END of Result record 

				std::pair<pt_assoc_it, pt_assoc_it> imsr_range = wqir_it->second.equal_range(IMAGE_SET_NODE);
				for (auto imsr_it = imsr_range.first; imsr_it != imsr_range.second; imsr_it++)
				{
					SampleImageSetRecordDLL tmp_imsr = {};

					if (!ConvertToStruct(imsr_it, tmp_imsr))
						return false;

					// Only one Bright field image should be there.
					auto bfi_it = imsr_it->second.find(BF_IMAGE_NODE);
					if (imsr_it->second.not_found() != bfi_it)
					{
						ImageRecordDLL tmp_imr = {};
						if (!ConvertToStruct(bfi_it, tmp_imr))
							return false;
						add_drive_id(tmp_imr.path); // Add the drive id to image record path
						//Add bright field image to image set record
						tmp_imsr.brightfield_image = tmp_imr.uuid;
						//Update the main image record list
						imr_list.push_back(tmp_imr);
					}//END of Bright field image

					 // TODO Flourscent channel.

					 //Add image set record to sample record.
					tmp_wqir.imageSets.push_back(tmp_imsr.uuid);
					//Update the main image set record list
					imsr_list.push_back(tmp_imsr);
				}//END of Sample image set record

				 // Add Wqi to Wq
				tmp_wqr.sample_records.push_back(tmp_wqir.uuid);
				// Update the main sample record list
				wqir_list.push_back(tmp_wqir);
			}// END of Sample record.

			 //Update the main work queue record list
			wqr_list.push_back(tmp_wqr);
		}
	}
	catch (const pt::ptree_error& er)
	{
		//Logger::L().Log (MODULENAME, severity_level::error, "RetrieveDataRecords: <Exception Occurred: " + std::string(er.what()) + " Exit>");
		return false;
	}
	//Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveDataRecords: <Exit>");

	return true;
}

void DataReplicator::duplicateData()
{
	uint16_t wqRepeatFactor = 1;
	uint16_t wqiRepeatFactor = 1;
	uint32_t selectedIndex = 0;
	bool saveMinimumImageSet = false;

	std::shared_ptr<DataReplicator> obj;
	obj.reset(new DataReplicator());
	obj->ReadMetaDataFromDisk();

	std::string availableInfo;
	uint32_t totalWQAvailable = 0;
	BOOST_FOREACH(boost::property_tree::ptree::value_type &wq, obj->mainMetaDataTree_)
	{
		availableInfo.append(boost::str(boost::format("[%d] : Work Queue Id {%s}\n") % totalWQAvailable % wq.second.get<std::string>("UUID")));
		totalWQAvailable++;
	}

	availableInfo = "Total Work Queue Available : " + std::to_string(totalWQAvailable) + "\n" + availableInfo;
	std::cout << availableInfo << std::endl;

	std::string inputStr;
	while (true)
	{
		std::cout << "Select the Work queue record between : |0 - " << totalWQAvailable - 1 << "| to generate the disk data" << std::endl;
		std::cout << "Enter \"q\" to exit" << std::endl;
		std::cin >> inputStr;
		if (inputStr[0] == 'q' || inputStr[0] == 'Q')
		{
			exit(0);
		}

		selectedIndex = std::stoul(inputStr);

		if (selectedIndex < 0 || selectedIndex > (totalWQAvailable - 1))
		{
			std::cout << "Invalid selection, try again" << std::endl;
		}
		else
		{
			std::cout << "Enter # of work queue repeat count \"q\" to exit: " << std::endl;
			std::cin >> inputStr;
			if (inputStr[0] == 'q' || inputStr[0] == 'Q')
			{
				exit(0);
			}

			wqRepeatFactor = (uint16_t)std::stoul(inputStr);
			if (wqRepeatFactor != 0)
			{
				std::cout << "Enter # of work queue item repeat count {Default is 1} \"i\" to ignore: " << std::endl;
				std::cin >> inputStr;
				if (inputStr[0] == 'i' || inputStr[0] == 'I')
				{
					break;
				}

				wqiRepeatFactor = (uint16_t)std::stoul(inputStr);

				std::cout << "Press \"y\" to save minimum images {Any key to ignore} " << std::endl;
				std::cin >> inputStr;
				if (inputStr[0] == 'y' || inputStr[0] == 'Y')
				{
					saveMinimumImageSet = true;
					break;
				}
				break;
			}

			std::cout << "invalid work queue repeat count, try again" << std::endl;
		}
	}

	auto st = ChronoUtilities::CurrentTime();

	auto copyMainTree = obj->mainMetaDataTree_;
	BOOST_FOREACH(boost::property_tree::ptree::value_type &wq, copyMainTree)
	{
		if (wq.first == "WorkQueue" && selectedIndex == 0)
		{
			for (uint16_t index = 0; index < wqRepeatFactor; index++)
			{
				auto item = duplicateWorkQueue(wq.second, wqiRepeatFactor, saveMinimumImageSet);
				obj->mainMetaDataTree_.add_child("WorkQueue", item);
				obj->WriteMetaDataToDisk();
			}
		}
		selectedIndex--;
	}

	auto et = ChronoUtilities::CurrentTime();
	std::cout << "Total Time Taken (sec) : " << std::chrono::duration_cast<std::chrono::seconds>(et - st).count() << std::endl;
}

