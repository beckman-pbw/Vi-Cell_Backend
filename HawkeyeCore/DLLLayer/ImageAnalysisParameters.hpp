#pragma once

#include "DBif_Api.h"

class ImageAnalysisParameters
{
public:
	static ImageAnalysisParameters& Instance()
	{
		static ImageAnalysisParameters instance;
		return instance;
	}

	bool Initialize();
	DBApi::DB_ImageAnalysisParamRecord& Get();
	bool Set();

private:
	ImageAnalysisParameters() {};
	ImageAnalysisParameters (ImageAnalysisParameters const&);	// Don't Implement
	void operator = (ImageAnalysisParameters const&);			// Don't implement

	DBApi::DB_ImageAnalysisParamRecord imageAnalysisParams_ = {};
};
