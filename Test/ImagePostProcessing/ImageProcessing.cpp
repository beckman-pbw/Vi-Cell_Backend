#include "stdafx.h"

#include <iostream>
#include <fstream>

#include "ErrcHelpers.hpp"
#include "ImageProcessing.hpp"
#include "Logger.hpp"

using namespace boost::filesystem;

static const char MODULENAME[] = "ImageProcessing";


//*****************************************************************************
ImageProcessing::ImageProcessing() {

	pLocalIosvc_.reset(new boost::asio::io_context());
	pLocalWork_.reset(new boost::asio::io_context::work(*pLocalIosvc_));

	// Ugliness necessary because std::bind does not appreciate overloaded function names
	auto T = std::bind(static_cast<std::size_t(boost::asio::io_context::*)(void)>(&boost::asio::io_context::run), pLocalIosvc_.get());

	pIoThread_.reset(new std::thread(T));
}

//*****************************************************************************
ImageProcessing::~ImageProcessing() {

	// Stop the local queue processing with no state retention.
	if (!pLocalIosvc_ || !pIoThread_) {
		return;
	}

	pLocalWork_.reset();	// destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	pLocalIosvc_->stop();	// instruct io_service to stop processing (should exit ::run() and end thread.
	if (pIoThread_->joinable()) {
		pIoThread_->join();	// May take a moment to complete.
	}
	pLocalIosvc_.reset();	// Destroys the queue
	pIoThread_.reset();	// Cleans up the thread.
}

//*****************************************************************************
void ImageProcessing::init (uint32_t width, uint32_t height) {
}

//*****************************************************************************
void ImageProcessing::showImage (const CvArr* image) {

	cvShowImage ("Display window", image);
	cv::waitKey (25); // Wait 25ms.
}

//*****************************************************************************
bool analyzeImage (Mat& image) {

	// queueImageForProcessing()

	// Add imageProcTask

	return true;
}

//*****************************************************************************
bool ImageProcessing::generateHistogramFile (Mat& image, std::string histogramPath) {

	int histSize = 256;

	/// Set the ranges for grayscale.
	float range[] = { 0, 256 };
	const float* histRange = { range };
	bool uniform = true;
	bool accumulate = false;
	Mat histogram;

	calcHist (&image, 1, 0, Mat(), histogram, 1, &histSize, &histRange, uniform, accumulate);

	std::ofstream histFile;
	histFile.open (histogramPath);
	if (!histFile.is_open()) {
		Logger::L().Log (MODULENAME, severity_level::normal, "failed to create histogram file: " + histogramPath);
		return false;
	}
	
	histFile << "Bin, Count" << std::endl;

	for (int i = 0; i < histSize; i++) {
		histFile << i << ", " << histogram.at<float>(i, 0) << std::endl;
	}

	histFile.close();

	return true;
}

//*****************************************************************************
bool ImageProcessing::generateHistogram (Mat& image, std::vector<uint32_t>& histData) {

	int histSize = 256;

	/// Set the ranges for grayscale.
	float range[] = { 0, 256 };
	const float* histRange = { range };
	bool uniform = true;
	bool accumulate = false;
	Mat histogram;

	calcHist (&image, 1, 0, Mat(), histogram, 1, &histSize, &histRange, uniform, accumulate);

	histData.clear();

	for (size_t i = 0; i < (size_t)(histogram.rows * histogram.cols); i++) {
		histData.push_back ((uint32_t)(histogram.at<float>((uint32_t)i, 0)));
	}

	return true;
}

//*****************************************************************************
boost::system::error_code ImageProcessing::Enqueue (pImageTask_t task) {
	
	// Post a task to the private io_service thread.
	// Return an error if the io_service isn't created.

	if (!pLocalIosvc_)
	{
		// Apparently not accepting new work?
		return MAKE_ERRC(boost::system::errc::not_connected);
	}

#if 0
	if ((task->XType == eRegWrite && task->TXBuf && task->TXBuf->size() > 4095) ||
		(task->XType == eRegRead && task->ReadLen > 4095))
	{
		return MAKE_ERRC(boost::system::errc::file_too_large);
	}
#endif

	queueMtx_.lock();
	imageTaskQueue_[taskindex_] = task;
	queueSize_++;

	// Push task off to internal IO Service for later processing.
	pLocalIosvc_->post (std::bind(&ImageProcessing::ProcessTask_, this, taskindex_));

	if (++taskindex_ > 10000) {
		taskindex_ = 0;
	}

	queueMtx_.unlock();

	return MAKE_SUCCESS;
}

//*****************************************************************************
void ImageProcessing::ProcessTask_ (uint16_t taskID) {

	/*
	* Called by the private io_service.run() thread.
	*
	* Locate task within map.
	* If task exists, remove it from the map
	*  and execute the task.
	*
	*  Splitting the execution out lets us release
	*  the queue earlier.
	*/
	queueMtx_.lock();

	if (imageTaskQueue_.find(taskID) == imageTaskQueue_.end()) {
		queueMtx_.unlock();
		return;
	}

	pImageTask_t task = imageTaskQueue_[taskID];
	imageTaskQueue_.erase(taskID);
	queueMtx_.unlock();

	if (task) {
		boost::system::error_code ec;

		ProcessTask (ec, task);

		// only work the callback if we're not taking down the system.
//		if (ec != boost::system::errc::operation_canceled)
//			_io_service.post(std::bind(task->Callback, ec, boardStatus, task->TXBuf, task->RXBuf));

		queueMtx_.lock();
		queueSize_--;
		queueMtx_.unlock();

	}

}

//*****************************************************************************
void ImageProcessing::ProcessTask (boost::system::error_code& ec, pImageTask_t task) {
	assert(task);
	ec = MAKE_SUCCESS;


	Logger::L().Log (MODULENAME, severity_level::debug1, "Processing iamge...");


}

//*****************************************************************************
struct ImageVarianceData {
	
	double lowerPercent;
	double upperPercent;
	uint8_t blue;
	uint8_t green;
	uint8_t red;
};

/*
Red (245, 15, 0) (f50f00) : >12.5%
Orange (255, 141, 0) (ff8d00) : 7.5% to 12.5%
Yellow (245, 245, 0) (f5f500) : 2.5% to 7.5%
Bright green (1, 254, 1) (01fe01) : +/ -2.5% of average
Light blue (01, 224, 254) (01e0fe) : -2.5% to - 7.5%
Blue (0, 110, 255) (006eff) : -7.5% to - 12.5%
Dark blue (1, 1, 171) (0101ab) : < -12.5%
*/
#define MAX_VARIANCE_RANGES 7
static ImageVarianceData imageVariance[MAX_VARIANCE_RANGES] =
{
	{   12.5, 100.0, 255,  15,   0 },
	{    7.5,  12.5, 255, 241,   0 },
	{    2.5,   7.5, 245, 245,   0 },
	{   -2.5,   2.5,   1, 254,   1 },
	{   -7.5,  -2.5,   1, 224, 254 },
	{  -12.5,  -7.5,   0, 110, 255 },
	{ -100.0, -12.5,   1,   1, 171 }
};

//*****************************************************************************
#define PERCENT_VARIANCE_STEP 0.01
bool ImageProcessing::processBackground (std::string filename, Mat& grayImage) {


	// Compute the mean pixel value for all of the pixels.
	uint32_t totalPixelValue = 0;
	uint32_t totalNumPixels = (uint32_t)(grayImage.rows * grayImage.cols);
	for (int row = 0; row < grayImage.rows; row++) {
		for (int col = 0; col < grayImage.cols; col++) {
			UINT8 value = grayImage.at<uint8_t>(row, col);
			totalPixelValue += value;
		}
	}

	uint32_t mean = totalPixelValue / totalNumPixels;
	Logger::L().Log (MODULENAME, severity_level::debug1, "Grayscale Mean: " + std::to_string(mean));

	Mat colorImage;
	cv::cvtColor (grayImage, colorImage, cv::COLOR_GRAY2BGR);

#if 1
	for (int row = 0; row < grayImage.rows; row++) {
		for (int col = 0; col < grayImage.cols; col++) {

			uint8_t grayPixel = grayImage.at<uint8_t>(row, col);
			Vec3b color = colorImage.at<Vec3b>(row, col); //(Point(col, row));

			for (int i = 0; i < 7; i++) {
				int16_t lowerVarianceValue = (int16_t) round ((double)mean * (imageVariance[i].lowerPercent / 100.0));
				int16_t upperVarianceValue = (int16_t) round ((double)mean * (imageVariance[i].upperPercent / 100.0));

#if 0
				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("graypixel: %d") % (int)grayPixel));
				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("mean - %f%% of mean: %d") % imageVariance[i].lowerPercent % lowerVarianceValue));
				Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("mean + %f%% of mean: %d") % imageVariance[i].upperPercent % upperVarianceValue));
#endif

				if (grayPixel > (mean + lowerVarianceValue) && grayPixel <= (mean + upperVarianceValue)) {
					color.val[0] = imageVariance[i].blue;
					color.val[1] = imageVariance[i].green;
					color.val[2] = imageVariance[i].red;
				}
			}

//			colorImage.at<Vec3b>(Point(row, col)) = color;
			colorImage.at<Vec3b>(row, col) = color;

		} // End "for (int col = 0; col < grayImage.cols; col++)"

	} // End "for (int row = 0; row < grayImage.rows; row++)"
#endif


#if 0
	for (int row = 0; row < grayImage.rows; row++) {
		for (int col = 0; col < grayImage.cols; col++) {

			uint8_t value = grayImage.at<uint8_t>(row, col);
			Vec3b color = colorImage.at<Vec3b>(Point(row, col));

			// From 0 to 15 percent...
			uint8_t step;
			bool isSet = false;
#if 1
			for (step = 0; step < 15; step++) {
				double percentVariance = (double)step * PERCENT_VARIANCE_STEP;
				uint16_t variance = (uint16_t)round((double)mean * percentVariance);

#if 0
				Logger::L().Log (MODULENAME, severity_level::debug1, "value: " + std::to_string(value));
				Logger::L().Log (MODULENAME, severity_level::debug1, "variance: " + std::to_string(variance));
				Logger::L().Log (MODULENAME, severity_level::debug1, "Mean + 5% of mean: " + std::to_string(mean + variance));
				Logger::L().Log (MODULENAME, severity_level::debug1, "Mean - 5% of mean: " + std::to_string(mean - variance));
#endif

				if (step < 5) {
					
					// Do the +-5% band in green.				
					if (value >= (mean - variance) && value <= (mean + variance)) {
						color.val[0] = 0;		// blue
						color.val[1] = 255 - (20 * step); // green
						color.val[2] = 0;		// red
						isSet = true;
						break;
					}
				
				} else {

					// Do the 5% -> 15% band in read.				
					if (value >= (mean - variance) && value <= (mean + variance)) {
						color.val[0] = 0;
						color.val[1] = 0;
//						color.val[2] = 255 - (20 * (step - 5));
						color.val[2] = 255 - (20 * (15 - step));
						isSet = true;
						break;
					}
				}

			} // End "for (uint8_t step = 0; step < 15; step++)"
#endif

			if (step < 5) {
				if (!isSet) {
					color.val[0] = 0;
					color.val[1] = 255 - (20 * step);
					color.val[2] = 0;
				}

			} else {
				if (!isSet) {
					color.val[0] = 0;
					color.val[1] = 0;
//					color.val[2] = 255 - (20 * (step - 5));
					color.val[2] = 255;
				}
			}

			colorImage.at<Vec3b>(Point(row, col)) = color;

		} // End "for (int col = 0; col < grayImage.cols; col++)"
	
	} // End "for (int row = 0; row < grayImage.rows; row++)"
#endif

	path p(filename);
	std::string colorImageFilename = p.parent_path().string() + std::string("/") + p.stem().string() + std::string("_c") + p.extension().string();
	cvSaveImage (colorImageFilename.c_str(), &IplImage(colorImage));

	return true;
}

