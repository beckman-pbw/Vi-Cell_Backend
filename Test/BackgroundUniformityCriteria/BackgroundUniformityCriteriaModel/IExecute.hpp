#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <functional>
#include <opencv2\opencv.hpp>

static void writeMatToFile(cv::Mat& m, const char* filename)
{
	std::ofstream fout(filename);

	if (!fout)
	{
		std::cout << "File Not Opened" << std::endl;  return;
	}

	bool isFloat = m.type() == CV_32FC1;
	for (int i = 0; i<m.rows; i++)
	{
		for (int j = 0; j<m.cols; j++)
		{
			if (isFloat)
			{
				fout << m.at<float>(i, j) << "\t";
			}
			else
			{
				fout << (int)(m.at<uchar>(i, j)) << "\t";
			}
		}
		fout << std::endl;
	}

	fout.close();
}

typedef struct ExecuteResult
{
	bool success;
	double resultValue;
	cv::Mat resultImage;
}ExecuteResult;

typedef struct ExecuteInputParameters
{
	double expectedResult;
}ExecuteParams;

class IExecute
{
public:
	virtual bool execute(std::shared_ptr<cv::Mat> image) = 0;
	virtual void getResult(ExecuteResult& result) = 0;
	virtual std::string testName() = 0;

	virtual ~IExecute()
	{
		result_.reset();
	}

protected:
	bool validateInput(const std::shared_ptr<cv::Mat>& image)
	{
		return image.get() != nullptr && image->empty() == false;
	}

	void performSlidingWindow(
		const cv::Mat& input, cv::Mat& output, const cv::Size& windowSize,
		std::function<double(const cv::Mat& roi)> operation)
	{
		if (input.empty() || operation == nullptr || (windowSize.width * windowSize.height) <= 0)
		{
			output = cv::Mat();
			return;
		}

		int newWidth = input.cols / windowSize.width;
		int newHeight = input.rows / windowSize.height;

		output = cv::Mat(newHeight, newWidth, CV_32FC1);

		for (int i = 0; i < newHeight; i++)
		{
			int rectStartPtY = i *  windowSize.height;
			for (int j = 0; j < newWidth; j++)
			{
				int rectStartPtX = j * windowSize.width;
				auto subImage = input(cv::Rect(rectStartPtX, rectStartPtY, windowSize.width, windowSize.height));

				double data = operation(subImage);

				output.at<float>(i, j) = (float)data;
			}
		}
	}

	void convertTo8UScaled(cv::Mat& in, cv::Mat& out)
	{
		if (in.empty() || in.type() != CV_32FC1)
		{
			out = cv::Mat();
			return;
		}

		//writeMatToFile(in, "Input.csv");

		//// normalize float image to 0.0 - 1.0
		//cv::normalize(in, in, 0.0, 1.0, cv::NORM_MINMAX, CV_32FC1);

		//writeMatToFile(in, "Norm_Input.csv");

		//double minVal, maxVal;
		//cv::minMaxLoc(in, &minVal, &maxVal);

		//if (minVal == maxVal)
		//{
		//	out = cv::Mat();
		//	return;
		//}

		in.convertTo(out, CV_8UC1 /*255.0 * (maxVal - minVal)*/);

		//writeMatToFile(out, "Output.csv");

		double imageMean = cv::mean(out)[0];
		auto newMinVal = imageMean - 25;
		auto newMaxVal = imageMean + 30;
		cv::normalize(out, out, newMinVal, newMaxVal, cv::NORM_MINMAX, out.type());

		//writeMatToFile(out, "Norm_Output.csv");
	}

	std::shared_ptr<ExecuteResult> result_;
};