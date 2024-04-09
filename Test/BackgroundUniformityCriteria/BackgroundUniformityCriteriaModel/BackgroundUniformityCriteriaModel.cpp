// This is the main DLL file.

#include "stdafx.h"


#include "BackgroundUniformityCriteriaModel.hpp"
#include <msclr\marshal_cppstd.h>

using namespace System::Runtime::InteropServices;

std::string convertToStandardString(System::String^ input)
{
	msclr::interop::marshal_context context;
	return context.marshal_as<std::string>(input);
}

System::String^ convertToSystemString(std::string& input)
{
	return gcnew System::String(input.c_str());
}

BackgroundUniformityCriteriaModel::ImageData^ convertToImageData(cv::Mat& image)
{
	int length = 0;
	BackgroundUniformityCriteriaModel::ImageData^ output
		= gcnew BackgroundUniformityCriteriaModel::ImageData();
	if (image.empty())
	{
		output->width = 0;
		output->height = 0;
		output->data = nullptr;
		return output;
	}

	output->width = image.cols;
	output->height = image.rows;
	length = image.rows * image.cols;
	output->data = gcnew array<System::Byte>(length);

	Marshal::Copy((System::IntPtr)image.data, output->data, 0, length);

	return output;
}

void convertBufferToMatImageWithoutCopy(
	BackgroundUniformityCriteriaModel::UnmanagedImageData^ inputUnmanagedData, cv::Mat& outputImage)
{
	if (inputUnmanagedData == nullptr)
	{
		outputImage = cv::Mat();
		return;
	}

	if (inputUnmanagedData->width == 0 || inputUnmanagedData->height == 0
		|| inputUnmanagedData->data == System::UIntPtr::Zero)
	{
		outputImage = cv::Mat();
		return;
	}

	outputImage = cv::Mat(
		inputUnmanagedData->height, inputUnmanagedData->width,
		inputUnmanagedData->type, inputUnmanagedData->data.ToPointer(),
		inputUnmanagedData->step);
}

BackgroundUniformityCriteriaModel::ImageTestsModel::ImageTestsModel()
{
	unmanagedModel_ = new UnmanagedModel();
}

BackgroundUniformityCriteriaModel::ImageTestsModel::~ImageTestsModel()
{
	if (unmanagedModel_ != nullptr)
	{
		delete unmanagedModel_;
		unmanagedModel_ = nullptr;
	}
}

bool BackgroundUniformityCriteriaModel::ImageTestsModel::loadImage(System::String^ imagePath)
{
	return unmanagedModel_->loadImage(convertToStandardString(imagePath));
}

bool BackgroundUniformityCriteriaModel::ImageTestsModel::execute()
{
	return unmanagedModel_->execute();
}

bool BackgroundUniformityCriteriaModel::ImageTestsModel::loadDataAndExecute(UnmanagedImageData^ imageData)
{
	try
	{
		cv::Mat inputImage;
		convertBufferToMatImageWithoutCopy(imageData, inputImage);
		if (!unmanagedModel_->loadImage(inputImage))
		{
			return false;
		}
		return execute();
	}
	catch(...)
	{
		throw gcnew System::Exception("Unknown error occurred when performing BackgroundUniformityCriteriaModel::loadDataAndExecute");
	}
}

void BackgroundUniformityCriteriaModel::ImageTestsModel::getResult(
	[System::Runtime::InteropServices::Out]ResultDataComplete^% result)
{
	std::vector<std::pair<std::string, ExecuteResult>> results;
	unmanagedModel_->getResults(results);

	int nSize = (int)results.size();
	if (nSize <= 0)
	{
		return;
	}

	result = gcnew ResultDataComplete();
	result->resultList = gcnew array<ResultData^>(nSize);

	for (int index = 0; index < nSize; index++)
	{
		result->resultList[index] = gcnew ResultData();

		result->resultList[index]->testName =
			convertToSystemString(results[index].first);
		result->resultList[index]->success = results[index].second.success;
		result->resultList[index]->value = results[index].second.resultValue;

		result->resultList[index]->resultImage = convertToImageData(
			results[index].second.resultImage);
	}

	for (int index = 0; index < nSize; index++)
	{
		results[index].second.resultImage.release();
	}
}

//  Private method used for testing purpose
void BackgroundUniformityCriteriaModel::ImageTestsModel::getUnmanagedData(
	System::String ^ imagePath, UnmanagedImageData ^% imageData)
{
	static cv::Mat inputImage = cv::imread(convertToStandardString(imagePath));

	imageData = gcnew BackgroundUniformityCriteriaModel::UnmanagedImageData();
	if (inputImage.empty())
	{
		imageData->width = 0;
		imageData->height = 0;
		imageData->type = 0;
		imageData->step = 0;
		imageData->data = System::UIntPtr::Zero;
		return;
	}

	int length = inputImage.rows * inputImage.cols;

	imageData->width = inputImage.cols;
	imageData->height = inputImage.rows;
	imageData->type = inputImage.type();
	imageData->step = static_cast<System::UInt32>(inputImage.step);
	imageData->data = System::UIntPtr((void*)inputImage.data);
}
