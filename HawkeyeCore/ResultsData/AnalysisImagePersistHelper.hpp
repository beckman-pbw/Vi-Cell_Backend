// ReSharper disable CppInconsistentNaming
#pragma once
#include <xstring>
#include <opencv2/core/core.hpp>

#include "DBif_Structs.hpp"
#include "ImageCollection.hpp"
#include "uuid__t.hpp"

/**
 * This helper class is used by the HawkeyeResultsDataManager::saveAnalysisImages() method to both write the encrypted
 * image files as well as constructing the C++ structure used to store them in the database. It builds up the ImageSetRecord
 * used to write the selected image data to the database.
 */
class AnalysisImagePersistHelper
{
	private:
		DBApi::DB_ImageSetRecord _dbImageSetRecord;
		void writeImage(DBApi::DB_ImageSeqRecord& dbImageSeqRecord, DBApi::DB_ImageRecord& dbImageRecord, cv::Mat& imageContent) const;
		void saveBrightfieldImage(DBApi::DB_ImageSeqRecord& dbImageSeqRecord, const size_t namedIndex, cv::Mat& imageContent) const;
		void saveFluorescentImage(DBApi::DB_ImageSeqRecord& dbImageSeqRecord, const size_t namedIndex, FLImages_t& image) const;

	public:
		static const std::string MODULENAME;

		/**
		 * \brief Constructor that builds the path for images and saves references to images
		 * \param date_str is the date of the image and used to construct the path to store images.
		 * \param worklist_uuid is the date of the image and used to construct the path to store images.
		 * \param sample_data_uuid is the date of the image and used to construct the path to store images.
		 * \param sample_id is set in every DB_ImageSeqRecord created for persisting images in the database.
		 * \param images is the list of images this helper acts on when saving and creating DB_ImageSeqRecord(s). The images are not yet annotated.
		 */
		AnalysisImagePersistHelper(const std::string& date_str, const uuid__t& worklist_uuid, const uuid__t& sample_data_uuid, const uuid__t& sample_id);

		/**
		 * \brief Write an encrypted image file as well as constructing the DB_ImageSeqRecord used to store them in the database
		 * \param imageIndex is the integer index into the _images data member.
		 */
		void storeImage (size_t imageIndex, ImageSet_t imageSet);

		/**
		 * \brief Accessor to the ImageSet record that this helper has been building. Usually the last method called.
		 * \return The embedded ImageSetRecord that the helper class has been building.
		 */
		DBApi::DB_ImageSetRecord GetImageSetRecord(void) const { return _dbImageSetRecord; }
};

