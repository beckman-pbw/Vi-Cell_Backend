#pragma once

#include <stdint.h>

#include <atomic>
#include <ftd2xx.h>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include <opencv2\core\core.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

class DLL_CLASS ImageProcessing
{
public:
	ImageProcessing();
	virtual ~ImageProcessing();

	void init (uint32_t width, uint32_t height);
	void showImage (const CvArr* image);
	bool analyzeImage (Mat& image);
	uint32_t queueSize();

	static bool generateHistogramFile (Mat& image, std::string histogramPath);
	static bool generateHistogram (Mat& image, std::vector<uint32_t>& histData);
	static bool processBackground (std::string filename, Mat& image);

private:
	struct ImageTask {
		Mat image;
		//		t_xferCallback Callback;
	};
	typedef std::shared_ptr<ImageTask> pImageTask_t;
	typedef std::map< int, pImageTask_t > ImageTaskQueue_t;

	void ProcessTask_ (uint16_t taskID);
	boost::system::error_code Enqueue (pImageTask_t task);
	void ProcessTask (boost::system::error_code& ec, pImageTask_t task);

	std::shared_ptr<boost::asio::io_context> pLocalIosvc_;
	std::shared_ptr<boost::asio::io_context::work> pLocalWork_;
	std::shared_ptr<std::thread> pIoThread_;

	uint32_t queueSize_;
	uint16_t taskindex_;
	ImageTaskQueue_t imageTaskQueue_;
	std::mutex queueMtx_;

};