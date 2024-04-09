#pragma once

#include <iostream>

#include "LedBase.hpp"

bool createLegacyImageDataDirectories (size_t imageCnt, std::string imgDirPath);
bool getLegacyDataPath (HawkeyeConfig::LedType ledType, std::string& dataPath);
bool getLegacyDataType (HawkeyeConfig::LedType ledType, std::string& dataType);
