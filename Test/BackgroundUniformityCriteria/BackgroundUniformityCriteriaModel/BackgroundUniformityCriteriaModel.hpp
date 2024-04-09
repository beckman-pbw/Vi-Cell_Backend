// BackgroundUniformityCriteriaModel.hpp

#pragma once

#include "UnmanagedModel.hpp"

namespace BackgroundUniformityCriteriaModel
{
	public ref struct UnmanagedImageData
	{
		System::UInt16 width;
		System::UInt16 height;
		System::UInt16 type;
		System::UInt32 step;
		System::UIntPtr data;
	};

	public ref struct ImageData
	{
		System::Int32 width;
		System::Int32 height;
		array<System::Byte>^ data;
	};

	public ref struct ResultData
	{
		System::String^ testName;
		double value;
		bool success;
		ImageData^ resultImage;
	};

	public ref struct ResultDataComplete
	{
		array<ResultData^>^ resultList;
	};

	public ref class ImageTestsModel
	{
	public:
		ImageTestsModel();
		~ImageTestsModel();

		bool loadImage(System::String^ imagePath);
		bool execute();
		bool loadDataAndExecute(UnmanagedImageData^ imageData);
		void getResult(
			[System::Runtime::InteropServices::Out]ResultDataComplete^% result);

	private:

		//  Private method used for testing purpose
		void getUnmanagedData(
			System::String^ imagePath, [System::Runtime::InteropServices::Out]UnmanagedImageData^% imageData);

		UnmanagedModel* unmanagedModel_;
	};
}
