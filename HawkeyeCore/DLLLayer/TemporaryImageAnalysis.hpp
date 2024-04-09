#pragma once

#include <atomic>
#include <functional>
#include <iostream>
#include <memory>

#include "CellTypesDLL.hpp"
#include "HawkeyeError.hpp"
#include "ImageProcessingUtilities.hpp"
#include "ImageWrapperDLL.hpp"
#include "LedBase.hpp"

class TemporaryImageAnalysis
{
public:
	typedef std::function<void (HawkeyeError, BasicResultAnswers, ImageWrapperDLL)> temp_analysis_result_callback_DLL;

	TemporaryImageAnalysis (std::shared_ptr<boost::asio::io_context> pIoService);
	~TemporaryImageAnalysis();

	void generateResultsContinuous(
		eImageOutputType image_preference,
		std::shared_ptr<AnalysisDefinitionDLL> tempAnalysisDefinition,
		std::shared_ptr<CellTypeDLL> tempCellTypeDefinition,
		temp_analysis_result_callback_DLL cb);
	void stopContinuousExecution();
	bool isBusy() const;

	void setCameraParams(uint32_t exposureTime_usec, HawkeyeConfig::LedType ledType);

private:

	void generateResultsInternal();
	void generateImage (std::function<void()> image_cb);
	void onCameraTrigger (cv::Mat capturedImage, std::function<void()> trigger_cb);
	void onAnalysisComplete (eImageOutputType image_preference,
		int imageNum, 
		CellCounterResult::SResult oSCellCountingResult,
		temp_analysis_result_callback_DLL cb);
	void updateHost (temp_analysis_result_callback_DLL cb, HawkeyeError he, BasicResultAnswers result, cv::Mat image);

	std::shared_ptr<boost::asio::io_context> pIoService_;
	std::atomic<bool> canContinueExecution_;
	std::atomic<bool> isBusy_;
	uint32_t exposureTime_usec_;
	HawkeyeConfig::LedType ledType_;
	ImageCollection_t singleImage_;
};
