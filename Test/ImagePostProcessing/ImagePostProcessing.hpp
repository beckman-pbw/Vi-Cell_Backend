#pragma once

#include <stdint.h>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/thread.hpp>

#include "ImageProcessing.hpp"

class ImagePostProcessing
{
public:
	ImagePostProcessing();
	virtual ~ImagePostProcessing();

	bool init (std::string configFilename);
	bool run (std::string dirPath);
	bool operator() (const std::string & a, const std::string & b);

	void quit();
	uint32_t getNumImages() { return imageCnt_; }
	bool readConfiguration (const std::string& configuration_file, boost::system::error_code& ec);
	bool createDataDirectories (uint32_t imageCnt);
	bool getDataPath (size_t index, std::string& dataPath);
	bool getDataType (size_t index, std::string& dataType);

private:
	void signalHandler (const boost::system::error_code& ec, int signal_number);
	void onCameraTrigger();
	void onCameraTriggerTimeout();
	void processCsvForAutofocus (std::string csvFile);

	bool inited_;
	std::vector<std::string> imageFilepaths_;
	std::shared_ptr<boost::asio::signal_set> pSignals_;
	std::shared_ptr<boost::asio::io_context> pLocalIosvc_;
	std::shared_ptr<boost::asio::io_context::work> pLocalWork_;
	std::shared_ptr<std::thread> pThread_;
	std::string runFilename_;
	uint32_t imageCnt_;
	ImageProcessing imageProcessing_;

	typedef struct {
		uint32_t left;
		uint32_t right;
		uint32_t top;
		uint32_t bottom;
	} AOI_t;

	typedef struct Configuration {
		AOI_t aoi;
		bool processHistogram;
		bool processBackground;
		bool processFocus;
		bool process_csv_for_autofocus;
	} Configuration_t;

	Configuration_t config_;
};
