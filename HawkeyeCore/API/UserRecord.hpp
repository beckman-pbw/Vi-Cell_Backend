#pragma once
#include <cstdint>
#include <list>
#include <map>
#include <regex>
#include "stdafx.h"
#include "ChronoUtilities.hpp"
#include "CellType.hpp"
#include "QualityControl.hpp"

struct UserRecord
{
	char* username = "";
	char* displayName = "";
	char* comments = "";
	char* email = "";
	char* exportFolder = "";
	char* csvFolder = "";
	char* langCode = "";
	char* defaultResultFileNameStr = "";
	char* defaultSampleNameStr = "";

	int16_t	defaultImageSaveN = 1;
	int16_t defaultWashType = 0;
	int16_t defaultDilution = 1;
	uint32_t defaultCellTypeIndex;
	bool exportPdfEnabled = false;

	uint32_t displayDigits = 2;
	bool allowFastMode = false;

	UserPermissionLevel permissionLevel = UserPermissionLevel::eNormal;

};