
#include "stdafx.h"
#include "UnmanagedModel.hpp"

UnmanagedModel::UnmanagedModel()
{
	reset();
}

UnmanagedModel::~UnmanagedModel()
{
	reset();
	pImage_.reset();
}

bool UnmanagedModel::loadImage(const std::string& imagePath)
{
	auto inputImage = cv::imread(imagePath, 0);
	return loadImage(inputImage);
}

bool UnmanagedModel::loadImage(const cv::Mat& inputImage)
{
	if (inputImage.empty())
	{
		return false;
	}
	reset();
	initializeTestsToRun();
	pImage_.reset(new cv::Mat(inputImage));
	return true;
}

bool UnmanagedModel::execute()
{
	if (vExecute_.size() <= 0)
	{
		return false;
	}

	for (auto test : vExecute_)
	{
		if (test == nullptr)
		{
			continue;
		}

		ExecuteResult result = {};
		auto name = test->testName();

		auto pair = std::make_pair(name, result);
		vResults_.push_back(pair);

		if (test->execute(pImage_))
		{
			test->getResult(vResults_[vResults_.size() - 1].second);
		}
	}

	return true;
}

void UnmanagedModel::getResults(
	std::vector<std::pair<std::string, ExecuteResult>>& results)
{
	results.clear();
	for (auto item : vResults_)
	{
		ExecuteResult result = {};
		auto pair = std::make_pair(item.first, result);

		pair.second.resultValue = item.second.resultValue;
		pair.second.success = item.second.success;
		if (item.second.resultImage.empty() == false)
		{
			pair.second.resultImage = item.second.resultImage.clone();
		}
		results.push_back(pair);
	}
}

void UnmanagedModel::initializeTestsToRun()
{
	auto testMeanParam = ExecuteParams();
	testMeanParam.expectedResult = 12.0;
	vExecute_.push_back(new TestImageMean(testMeanParam));

	auto testCoffVar = ExecuteParams();
	testCoffVar.expectedResult = 3.0;
	vExecute_.push_back(new TestCofficientVar(testCoffVar));

	auto testOrgImageAvg = ExecuteParams();
	testOrgImageAvg.expectedResult = 135.0;
	vExecute_.push_back(new TestCompleteImageAvg(testOrgImageAvg, 10.0));
}

void UnmanagedModel::reset()
{
	for (int index = 0; index < vExecute_.size(); index++)
	{
		delete vExecute_[index];
	}
	vExecute_.clear();

	for (int index = 0; index < vResults_.size(); index++)
	{
		vResults_[index].second.resultImage.release();
	}
	vResults_.clear();
}