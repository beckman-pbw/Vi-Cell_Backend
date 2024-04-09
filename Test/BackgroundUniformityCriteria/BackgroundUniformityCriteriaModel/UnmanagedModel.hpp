#pragma once

#include "ImageTests.hpp"

class UnmanagedModel
{
public:
	UnmanagedModel();
	~UnmanagedModel();

	bool loadImage(const std::string& imagePath);
	bool loadImage(const cv::Mat& inputImage);
	bool execute();
	void getResults(std::vector<std::pair<std::string, ExecuteResult>>& results);

private:

	void initializeTestsToRun();
	void reset();

	std::vector<IExecute*> vExecute_;
	std::vector<std::pair<std::string, ExecuteResult>> vResults_;
	std::shared_ptr<cv::Mat> pImage_;
};