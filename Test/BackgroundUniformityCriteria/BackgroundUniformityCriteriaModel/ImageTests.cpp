
#include "stdafx.h"
#include "ImageTests.hpp"

TestImageMean::TestImageMean(const ExecuteInputParameters& inParams)
{
	inputParams_ = inParams;
	result_ = std::make_shared<ExecuteResult>();
}

bool TestImageMean::execute(std::shared_ptr<cv::Mat> image)
{
	if (!validateInput(image))
	{
		return false;
	}	

	cv::Size windowSize = cv::Size(128, 128);
	cv::Mat output;

	performSlidingWindow(*image, output, windowSize, [](const cv::Mat& subImage) -> double
	{
		return cv::mean(subImage)[0];
	});

	if (output.empty())
	{
		return false;
	}	

	double minVal, maxVal;
	cv::minMaxLoc(output, &minVal, &maxVal);

	double imageMean = cv::mean(*image)[0];
	double calculatedResult = ((maxVal - minVal) / imageMean) * 100.0;

	result_->resultValue = calculatedResult;
	result_->success = calculatedResult <= inputParams_.expectedResult;
	convertTo8UScaled(output, result_->resultImage);

	return true;
}

void TestImageMean::getResult(ExecuteResult& result)
{
	result = {};
	if (result_.get() == nullptr)
	{
		return;
	}
	result.resultValue = result_->resultValue;
	result.success = result_->success;
	result.resultImage = result_->resultImage.clone();
}

std::string TestImageMean::testName()
{
	return "16 x 16 Image Min-Max Variation";
}

/****************************************************************************************************************/

TestCofficientVar::TestCofficientVar(const ExecuteInputParameters& inParams)
{
	inputParams_ = inParams;
	result_ = std::make_shared<ExecuteResult>();
}

bool TestCofficientVar::execute(std::shared_ptr<cv::Mat> image)
{
	if (!validateInput(image))
	{
		return false;
	}

	cv::Size windowSize = cv::Size(16, 16);
	cv::Mat output;

	performSlidingWindow(*image, output, windowSize, [](const cv::Mat& subImage) -> double
	{
		return cv::mean(subImage)[0];
	});

	if (output.empty())
	{
		return false;
	}	

	cv::Scalar mean;
	cv::Scalar stdDev;
	cv::meanStdDev(output, mean, stdDev);

	double calculatedResult = ((double)stdDev[0] / mean[0]) * 100.0;

	result_->resultValue = calculatedResult;
	result_->success = calculatedResult <= inputParams_.expectedResult;
	convertTo8UScaled(output, result_->resultImage);

	return true;
}

void TestCofficientVar::getResult(ExecuteResult& result)
{
	result = {};
	if (result_.get() == nullptr)
	{
		return;
	}
	result.resultValue = result_->resultValue;
	result.success = result_->success;
	result.resultImage = result_->resultImage.clone();
}

std::string TestCofficientVar::testName()
{
	return "128 x 128 Image Coefficient Variance";
}

/****************************************************************************************************************/

TestCompleteImageAvg::TestCompleteImageAvg(
	const ExecuteInputParameters& inParams, double offset)
{
	inputParams_ = inParams;
	offSet_ = offset;
	result_ = std::make_shared<ExecuteResult>();
}

bool TestCompleteImageAvg::execute(std::shared_ptr<cv::Mat> image)
{
	if (!validateInput(image))
	{
		return false;
	}

	double imageMean = cv::mean(*image)[0];
	bool success = true;
	if (imageMean < inputParams_.expectedResult - offSet_
		|| imageMean > inputParams_.expectedResult + offSet_)
	{
		success = false;
	}

	result_->resultValue = imageMean;
	result_->success = success;
	result_->resultImage = cv::Mat();

	return true;
}

void TestCompleteImageAvg::getResult(ExecuteResult& result)
{
	result = {};
	if (result_.get() == nullptr)
	{
		return;
	}
	result.resultValue = result_->resultValue;
	result.success = result_->success;
	result.resultImage = result_->resultImage.clone();
}

std::string TestCompleteImageAvg::testName()
{
	return "Original Image Intensity Mean";
}
