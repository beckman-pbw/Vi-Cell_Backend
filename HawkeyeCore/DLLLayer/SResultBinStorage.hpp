#pragma once
#include <string>

#include "CellCountingOutputParams.h"
#include "InputConfigurationParams.h"


class SResultBinStorage
{
public:
	static bool SerializeSResult(const CellCounterResult::SResult& sr, std::string filename);
	static bool DeSerializeSResult(CellCounterResult::SResult& dsr, std::string filename);
};