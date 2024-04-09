#include "stdafx.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "AppConfig.hpp"
#include "CommandParser.hpp"
#include "ImagePostProcessing.hpp"
#include "ImageProcessing.hpp"
#include "Logger.hpp"

static const char MODULENAME[] = "ImagePostProcessing";
static const char VERSION[] = "0.03";

using namespace boost::filesystem;

typedef struct {
	uint32_t filenum;
	int32_t position;
	int32_t sharpness;
} SharpnessData_t;


//*****************************************************************************
ImagePostProcessing::ImagePostProcessing() {
	inited_ = false;
	imageCnt_ = 0;

	Logger::L().Log (MODULENAME, severity_level::normal, boost::str (boost::format ("ImagePostProcessing v%s") % VERSION));
}

//*****************************************************************************
ImagePostProcessing::~ImagePostProcessing() {	

}

//*****************************************************************************
bool ImagePostProcessing::init (std::string configFilename) {

	Logger::L().Log (MODULENAME, severity_level::debug2, "init: <enter>");

	if (inited_) {
		return true;
	}

	AppConfig appCfg;

	t_opPTree pRoot = appCfg.init ("ImagePostProcessing.info");
	if (!pRoot) {
		return false;
	}

	t_opPTree pConfig = appCfg.findSection (std::string("config"));
	if (!pConfig) {
		return false;
	}

	t_opPTree pAOITree = appCfg.findTagInSection (pConfig, "aoi");
	if (!pAOITree) {
		return false;
	}

	//TODO: these are currently ignored for *focus* testing.
	appCfg.findField<uint32_t> (pAOITree, "left",   false, 0,    config_.aoi.left);
	appCfg.findField<uint32_t> (pAOITree, "right",  false, 2048, config_.aoi.right);
	appCfg.findField<uint32_t> (pAOITree, "top",    false, 0,    config_.aoi.top);
	appCfg.findField<uint32_t> (pAOITree, "bottom", false, 2048, config_.aoi.bottom);

	t_opPTree pWhatTree = appCfg.findTagInSection (pConfig, "what_to_do");
	if (!pWhatTree) {
		return false;
	}
	appCfg.findField<bool>  (pWhatTree, "process_histogram", false, false, config_.processHistogram);
	appCfg.findField<bool>  (pWhatTree, "process_background", false, false, config_.processBackground);
	appCfg.findField<bool>  (pWhatTree, "process_focus", false, false, config_.processFocus);
	appCfg.findField<bool>  (pWhatTree, "process_csv_for_autofocus", false, false, config_.process_csv_for_autofocus);

	inited_ = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "init: <exit>");

	return true;
}

boost::regex regexpression ("(\\d+)");
boost::match_results<std::string::const_iterator> what1, what2;

//*****************************************************************************
uint32_t st2num (const std::string& text) {

	std::stringstream ss(text);
	uint32_t result;
	return ss >> result ? result : 0;
}

//*****************************************************************************
// This sort process is a bit kludgy.
// It sorts filename paths by directories and filenames.
// Assumption: each file path is made up of six parts.
// [0]: ignored, starting part of the path, ".".
// [1]: ignored, no number.
// [2]: contains a number, sort on it.
// [3]: contains a number, sort on it.
// [4]: ignored, no number.
// [5]: contains a number, sort on it.
// 
// Example of breaking up this file path: ./DataFromJulieForHistogramsEtc\1\9\CamBF\CamBF_9.png
// [0]: .
// [1]: DataFromJulieForHistogramsEtc
// [2]: 1
// [3]: 9
// [4]: CamBF
// [5]: CamBF_9.png
//*****************************************************************************
static const int interestingPartOfThePath = 4;
struct ImageProcessingSort {

	bool operator() (const std::string& a, const std::string& b) {
		std::vector<std::string> a_parts;
		std::vector<std::string> b_parts;

//		Logger::L().Log (MODULENAME, severity_level::debug1, "*************************************************");

		// Break up the filename paths.
		for (auto& p1 : path(a)) {
			a_parts.push_back (p1.string());
//			Logger::L().Log (MODULENAME, severity_level::debug1, p1.string());
		}
		for (auto& p2 : path(b)) {
			b_parts.push_back (p2.string());
//			Logger::L().Log (MODULENAME, severity_level::debug1, p2.string());
		}

		int idx = (int)a_parts.size() - interestingPartOfThePath;
		if (idx < 0) {
			// Image files are in a single directory, use the last entry in a_parts/b_parts.
			idx = (int)a_parts.size() - 1;
			boost::regex_search (a_parts[2].cbegin(), a_parts[idx].cend(), what1, regexpression, boost::match_default);
			boost::regex_search (b_parts[2].cbegin(), b_parts[idx].cend(), what2, regexpression, boost::match_default);

			bool state = st2num (what1[1]) < st2num (what2[1]);
			return state;

		} else {

			boost::regex_search (a_parts[idx].cbegin(), a_parts[idx].cend(), what1, regexpression, boost::match_default);
			boost::regex_search (b_parts[idx].cbegin(), b_parts[idx].cend(), what2, regexpression, boost::match_default);

			if (st2num (what1[1]) >= st2num (what2[1])) {
			
				boost::regex_search (a_parts[idx + 1].cbegin(), a_parts[idx+1].cend(), what1, regexpression, boost::match_default);
				boost::regex_search (b_parts[idx + 1].cbegin(), b_parts[idx + 1].cend(), what2, regexpression, boost::match_default);
			
				if (st2num (what1[1]) >= st2num (what2[1])) {
					boost::regex_search (a_parts[idx + 2].cbegin(), a_parts[idx + 2].cend(), what1, regexpression, boost::match_default);
					boost::regex_search (b_parts[idx + 2].cbegin(), b_parts[idx + 2].cend(), what2, regexpression, boost::match_default);

					bool state = st2num (what1[1]) < st2num (what2[1]);

	//				Logger::L().Log (MODULENAME, severity_level::debug1, "state: " + (state == true ? std::string("ordered") : std::string("not ordered")));

					return state;
				}
			}

		}

		return true;
	}
};

//*****************************************************************************
void ImagePostProcessing::processCsvForAutofocus (std::string csvFile) {

	std::ifstream csvHandle;
	csvHandle.open (csvFile);
	if (!csvHandle.is_open()) {
		Logger::L().Log (MODULENAME, severity_level::error, "Unable to open " + csvFile);
		return;
	}

	std::vector<SharpnessData_t> sharpnessData;
	std::string line;
	CommandParser cmdParser;
	uint32_t count = 0;
	while (!csvHandle.eof()) {
		getline (csvHandle, line);
		
		count++;
		if (count == 1) {
			// Skip the first line since it is the CSV file header.
			continue;
		}

		boost::algorithm::trim (line);

		// Skip empty lines.
		if (line.length() == 0) {
			continue;
		}

		std::cout << line << std::endl;

		cmdParser.parse (",", line);

		SharpnessData_t temp;
		temp.filenum = std::stoi (cmdParser.getByIndex(0));
		if (temp.filenum < 50) {
			// The coarse focus data is at the beginning of the CSV file, skip it.
			continue;
		}
		temp.position = std::stoi (cmdParser.getByIndex(1));
		temp.sharpness = std::stoi (cmdParser.getByIndex(2));

		sharpnessData.push_back (temp);
	}


	// Find the max sharpness value for the fine focus.
	SharpnessData_t maxFocusData = { 0, 0, 0 };
	uint32_t maxIdx = 0;
	uint32_t idx = 0;
	for (auto it : sharpnessData) {
		Logger::L().Log  (MODULENAME, severity_level::debug1,
			boost::str  (boost::format  ("sharpnessData:: filenum: %d, position: %d, sharpness: %d")
				% it.filenum
				% it.position
				% it.sharpness));
			if (it.sharpness > maxFocusData.sharpness) {
			maxFocusData = it;
			maxIdx = idx;
		}
		idx++;
	}

	Logger::L().Log  (MODULENAME, severity_level::debug1,
		boost::str  (boost::format  ("maxFocusData: filenum: %d, position: %d, sharpness: %d")
			% maxFocusData.filenum
			% maxFocusData.position
			% maxFocusData.sharpness));

	std::vector<SharpnessData_t> M;
	M.resize (sharpnessData.size());

	uint32_t W0_half = maxFocusData.sharpness / 2;

	uint32_t i = 0;
	for (auto it : sharpnessData) {
		M[i].sharpness = it.sharpness - W0_half;
		M[i].position = it.position;
		M[i].filenum = it.filenum;
		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]: %d, %d") % i % M[i].sharpness % M[i].position));
		i++;
	}

	// Starting from the beginning of M, find the position where the sharpness is 1/2 of W0.
	uint32_t idx1 = 0;
	while (M[idx1].sharpness < 0 && idx1 < (M.size() / 2)) {
		idx1++;
	}
	Logger::L().Log  (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % (uint32_t)(idx1 - 1) % M[idx1 - 1].position % M[idx1 - 1].sharpness));
	Logger::L().Log  (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % idx1 % M[idx1].position % M[idx1].sharpness));


	// Interpolate to find Cmin.
	uint32_t Cmin = (sharpnessData[idx1].position * M[idx1 + 1].sharpness - sharpnessData[idx1 + 1].position * M[idx1].sharpness)
		/ (M[idx1 + 1].sharpness - M[idx1].sharpness);


	// Starting from the maximum of M, find the position where the sharpness is 1/2 of W0.
	uint32_t idx2 = maxIdx;
	while (M[idx2].sharpness > 0 && idx2 < M.size()) {
		idx2++;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % (uint32_t)(idx2 - 1) % M[idx2 - 1].position % M[idx2 - 1].sharpness));
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("M[%d]:: p: %d, s: %d") % idx2 % M[idx2].position % M[idx2].sharpness));


	// Interpolate to find Cmax.
	uint32_t Cmax = (sharpnessData[idx2].position * M[idx2 + 1].sharpness - sharpnessData[idx2 + 1].position * M[idx2].sharpness)
		/ (M[idx2 + 1].sharpness - M[idx2].sharpness);

	uint32_t Cref = (uint32_t)round ((Cmin + Cmax) / 2);
	Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Optimal focus position: %d") % Cref));
}

//*****************************************************************************
bool ImagePostProcessing::run (std::string dirPath) {

	path p(dirPath);

	if (!config_.process_csv_for_autofocus) {
		for (recursive_directory_iterator i(dirPath), end; i != end; ++i) {
			if (!is_directory(i->path()) && i->path().extension().compare(std::string(".png")) == 0) {
				Logger::L().Log (MODULENAME, severity_level::normal, i->path().string());
				imageFilepaths_.push_back (i->path().string());
			}
		}

		std::sort (imageFilepaths_.begin(), imageFilepaths_.end(), ImageProcessingSort());
	}

	if (config_.processBackground) {

		for (auto& iter = imageFilepaths_.begin(); iter != imageFilepaths_.end(); iter++) {
			Logger::L().Log (MODULENAME, severity_level::normal, "Loading " + *iter);

			IplImage* iplImage = cvLoadImage (iter->c_str(), CV_LOAD_IMAGE_GRAYSCALE);
			Mat image(iplImage, true);
			cvReleaseImage(&iplImage);
		
			ImageProcessing::processBackground (*iter, image /* percent_that_is_green */);
		}

		return true;

	} else if (config_.processHistogram) {

		std::ofstream histCSVFile;
		std::string histoDataPath = dirPath + "/histogram_data.csv";
		histCSVFile.open (histoDataPath);
		if (!histCSVFile.is_open()) {
			Logger::L().Log (MODULENAME, severity_level::normal, "failed to create histogram file: " + histoDataPath);
			return false;
		}

		for (auto& iter = imageFilepaths_.begin(); iter != imageFilepaths_.end(); iter++) {
			Logger::L().Log (MODULENAME, severity_level::normal, "Loading " + *iter);

			IplImage* iplImage = cvLoadImage (iter->c_str(), CV_LOAD_IMAGE_GRAYSCALE);
			Mat image(iplImage, true);
			cvReleaseImage(&iplImage);


			Logger::L().Log (MODULENAME, severity_level::normal,
				boost::str (boost::format ("Using AOI:: left: %d, right: %d, top: %d, bottom: %d")
					% config_.aoi.left
					% config_.aoi.right
					% config_.aoi.top
					% config_.aoi.bottom));

			Rect rect (
				config_.aoi.left,						// x
				config_.aoi.top,						// y
				config_.aoi.right - config_.aoi.left,	// width
				config_.aoi.bottom - config_.aoi.top);	// height
			Mat histoImage = image (rect);

			std::vector<uint32_t> histData;
			ImageProcessing::generateHistogram (histoImage, histData);

			histCSVFile << *iter << ", ";

			for (auto& iter : histData) {
				histCSVFile << iter << ", ";
			}
			histCSVFile << std::endl;
		}

		histCSVFile.close();

	} else if (config_.processFocus) {

		//std::ofstream csvFile;
		//std::string csvPath = dirPath + "/focus_data.csv";
		//csvFile.open (csvPath);
		//if (!csvFile.is_open()) {
		//	Logger::L().Log (MODULENAME, severity_level::normal, "failed to create focus data file: " + csvPath);
		//	return false;
		//}

		for (auto& iter = imageFilepaths_.begin(); iter != imageFilepaths_.end(); iter++) {
			Logger::L().Log (MODULENAME, severity_level::normal, "Loading " + *iter);

			IplImage* iplImage = cvLoadImage (iter->c_str(), CV_LOAD_IMAGE_GRAYSCALE);
			Mat image(iplImage, true);
			cvReleaseImage(&iplImage);

			// Any primitive type from the list can be defined by an identifier in the form CV_<bit-depth>{U|S|F}C(<number_of_channels>).
			Mat imageX (image.rows, image.cols, CV_8U); //, Scalar(0, 0, 0));
			Mat imageY (image.rows, image.cols, CV_8U); //, Scalar(0,0,0));
			Mat imageL (image.rows, image.cols, CV_8U); //, Scalar(0,0,0));

			//Mat imageX (image.rows, image.cols, CV_8U, 1, 0, 3, 1, 0, BORDER_DEFAULT);
			//Mat imageY (image.rows, image.cols, CV_8U, Scalar(0, 0, 0));

			//Mat imageX;
			//Mat imageY;

			//int aperture_size = 3;
			//Sobel (image, imageX, 1, 0, aperture_size);
			//Sobel (image, imageY, 0, 1, aperture_size);

// void Sobel (InputArray src, OutputArray dst, int ddepth, int dx, int dy, int ksize = 3, double scale = 1, double delta = 0, int borderType = BORDER_DEFAULT);
			Sobel (image, imageX, CV_8U, 1, 0, 3, 1, 0, BORDER_DEFAULT);
			Sobel (image, imageY, CV_8U, 0, 1, 3, 1, 0, BORDER_DEFAULT);

// void Laplacian (InputArray src, OutputArray dst, int ddepth, int ksize = 1, double scale = 1, double delta = 0, int borderType = BORDER_DEFAULT);
			Laplacian (image, imageL, CV_8U, 3, 1, 0, BORDER_DEFAULT);
//			Laplacian (image, imageL, CV_16S);

			Scalar meanX;
			Scalar meanY;
			Scalar meanL;
			Scalar stddevX;
			Scalar stddevY;
			Scalar stddevL;

			meanStdDev (imageX, meanX, stddevX);
			meanStdDev (imageY, meanY, stddevY);
			meanStdDev (imageL, meanL, stddevL);

			std::cout << "meanX: " << meanX[0] << ", stddevX: " << stddevX[0] << ", varianceX: " << stddevX[0] * stddevX[0] << std::endl;
			std::cout << "meanY: " << meanY[0] << ", stddevY: " << stddevY[0] << ", varianceY: " << stddevY[0] * stddevY[0] << std::endl;
			std::cout << "meanL: " << meanL[0] << ", stddevL: " << stddevL[0] << ", varianceL: " << stddevL[0] * stddevL[0] << std::endl;

			path filePath (*iter);
			//std::cout << "stem: " << filePath.stem() << std::endl;
			//std::cout << "filename: " << filePath.filename() << std::endl;
			//std::cout << "extension: " << filePath.extension() << std::endl;
			//std::cout << "root_name: " << filePath.root_name() << std::endl;
			//std::cout << "root_directory: " << filePath.root_directory() << std::endl;
			//std::cout << "root_path: " << filePath.root_path() << std::endl;
			//std::cout << "parent_path: " << filePath.parent_path() << std::endl;

			std::ofstream csvFile;
			std::string imagePath = dirPath + "/" + filePath.stem().string() + "_x.png";
			cvSaveImage (imagePath.c_str(), &IplImage(imageX));
			imagePath = dirPath + "/" + filePath.stem().string() + "_y.png";
			cvSaveImage (imagePath.c_str(), &IplImage(imageY));

			imagePath = dirPath + "/" + filePath.stem().string() + "_L.png";
			cvSaveImage (imagePath.c_str(), &IplImage(imageL));

//			convert


			//Logger::L().Log (MODULENAME, severity_level::normal,
			//	boost::str (boost::format ("Using AOI:: left: %d, right: %d, top: %d, bottom: %d")
			//		% config_.aoi.left
			//		% config_.aoi.right
			//		% config_.aoi.top
			//		% config_.aoi.bottom));

			//Rect rect (
			//	config_.aoi.left,						// x
			//	config_.aoi.top,						// y
			//	config_.aoi.right - config_.aoi.left,	// width
			//	config_.aoi.bottom - config_.aoi.top);	// height
			//Mat histoImage = image (rect);

			//std::vector<uint32_t> histData;
			//ImageProcessing::generateHistogram (histoImage, histData);

//			histCSVFile << *iter << ", ";

			//for (auto& iter : histData) {
			//	histCSVFile << iter << ", ";
			//}
			//histCSVFile << std::endl;
		
		}

	} else if (config_.process_csv_for_autofocus) {
		std::string csvFile = dirPath;
		processCsvForAutofocus (csvFile);
//TODO: read the *resutlts.csv* file and feed the values to the aotofocus calculation...

	} 




	return true;
}


//*****************************************************************************
int main (int argc, char *argv[]) {

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "ImagePostProcessing.info");
	
	Logger::L().Log (MODULENAME, severity_level::normal, "Starting ImagePostProcessing");

	if (argc < 2) {
		Logger::L().Log (MODULENAME, severity_level::normal, "No image directory specified.");
		exit (0);
	}

	ImagePostProcessing imagePostProc;
	if (imagePostProc.init("ImagePostProcessing.info")) {
		imagePostProc.run (argv[1]);
	}

	Logger::L().Flush();


    return 0;	// Never get here normally.
}
