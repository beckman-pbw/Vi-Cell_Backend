// ReSharper disable CppInconsistentNaming
#include "opencv2/opencv.hpp"

#include "AnalysisImagePersistHelper.hpp"

#include "DBif_Structs.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDataAccess.h"
#include "HawkeyeDirectory.hpp"
#include "Logger.hpp"

const std::string AnalysisImagePersistHelper::MODULENAME = "HawkeyeResultsDataManager";  // NOLINT(clang-diagnostic-exit-time-destructors)

void AnalysisImagePersistHelper::writeImage(DBApi::DB_ImageSeqRecord& dbImageSeqRecord,
	DBApi::DB_ImageRecord& dbImageRecord, cv::Mat& imageContent) const
{
	dbImageSeqRecord.ImageList.push_back(dbImageRecord);

	auto imagePath = boost::str(boost::format("%s\\%s") % _dbImageSetRecord.ImageSetPathStr % dbImageSeqRecord.ImageSequenceFolderStr);

	if (!FileSystemUtilities::CreateDirectories(imagePath, false))
	{
		throw std::exception(("<exit, failed to create directory: " + imagePath + ">").c_str());
	}

	imagePath = boost::str(boost::format("%s\\%s") % imagePath % dbImageRecord.ImageFileNameStr);

	if (!HDA_WriteEncryptedImageFile (imageContent, imagePath))
	{
		throw std::exception(("writeImage: <exit: failed to write image :  " + imagePath + ">").c_str());
	}
}

void AnalysisImagePersistHelper::saveBrightfieldImage(DBApi::DB_ImageSeqRecord& dbImageSeqRecord, const size_t namedIndex, cv::Mat &imageContent) const
{
	DBApi::DB_ImageRecord dbImageRecord = {};

	dbImageRecord.ImageChannel = 1;	// Brightfield image is always channel 1.
	dbImageRecord.ImageFileNameStr = boost::str(boost::format("bfimage_%03d.png") % namedIndex);
	dbImageRecord.ImageId = {};		// Filled in by DB when written.

	writeImage(dbImageSeqRecord, dbImageRecord, imageContent);
}

void AnalysisImagePersistHelper::saveFluorescentImage(DBApi::DB_ImageSeqRecord& dbImageSeqRecord, const size_t namedIndex, FLImages_t& images) const
{
	for (auto& vv : images)
	{
		DBApi::DB_ImageRecord dbImageRecord = {};

		dbImageRecord.ImageChannel = static_cast<uint8_t>(vv.first);
		dbImageRecord.ImageFileNameStr = boost::str(boost::format("flimage_%03d_%d.png")
			% namedIndex
			% dbImageRecord.ImageChannel);
		dbImageRecord.ImageId = {};		// Filled in when written.

		writeImage(dbImageSeqRecord, dbImageRecord, vv.second);
	}
}

AnalysisImagePersistHelper::AnalysisImagePersistHelper(const std::string& date_str, const uuid__t& worklist_uuid,
                                                       const uuid__t& sample_data_uuid, const uuid__t& sample_id)
{
	_dbImageSetRecord = {};
	_dbImageSetRecord.ImageSetPathStr = boost::str(boost::format("%s\\%s\\%s\\%s")
		% HawkeyeDirectory::Instance().getImagesBaseDir()
		% date_str
		% Uuid::ToStr(worklist_uuid)
		% Uuid::ToStr(sample_data_uuid));

	_dbImageSetRecord.SampleId = sample_id;
}

void AnalysisImagePersistHelper::storeImage (size_t imageIndex, ImageSet_t imageSet)
{

	DBApi::DB_ImageSeqRecord dbImageSeqRecord = {};
	dbImageSeqRecord.FlChannels = 0;
	dbImageSeqRecord.ImageCount = 1 + dbImageSeqRecord.FlChannels;  // Brightfield plus any FL images.
	dbImageSeqRecord.ImageSequenceFolderStr = boost::str(boost::format("%03d") % imageIndex);
	dbImageSeqRecord.ImageSequenceId = {};
	dbImageSeqRecord.SequenceNum = static_cast<int16_t>(imageIndex);
	if (imageSet.second.empty())
	{
		saveBrightfieldImage(dbImageSeqRecord, imageIndex, imageSet.first);
	}
	else
	{
		saveFluorescentImage(dbImageSeqRecord, imageIndex, imageSet.second);
	}

	_dbImageSetRecord.ImageSequenceList.push_back (dbImageSeqRecord);
}
