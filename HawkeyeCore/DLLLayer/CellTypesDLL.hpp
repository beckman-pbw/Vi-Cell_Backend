#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include "AnalysisDefinitionDLL.hpp"
#include "AppConfig.hpp"
#include "CellTypeDLL.hpp"
#include "DataConversion.hpp"

#define TEMP_CELLTYPE_INDEX USHRT_MAX
#define DELETED_CELLTYPE_INDEX 4294967295

/*
 * CellType describes the characteristics used to identify and select
 * a cell within the brightfield image.
 */
class CellTypesDLL
{
public:
	static bool Initialize();
	static void Import (boost::property_tree::ptree& ptConfig);
	static std::pair<std::string, boost::property_tree::ptree>Export();
	static std::vector<CellTypeDLL>& Get();
	static CellTypeDLL& Add (CellTypeDLL& cellType, bool isImported=false);
	static void Replace (const CellTypeDLL& cellTypeDLL);
	static bool isCellTypeIndexValid (uint32_t index);
	static uint32_t NumBCICellTypes();
	static uint32_t NumUserCellTypes();
	static bool getCellTypeByIndex (uint32_t index, CellTypeDLL& cellType);
	static CellTypeDLL getCellTypeByIndex (uint32_t index);
	static std::string getCellTypeName (uint32_t index);
	static CellTypeDLL getCellTypeByUUID (const uuid__t& uuid);
	static CellTypeDLL getCellTypeByName (std::string name);
	static uint32_t GetNextUserCellTypeIndex();

private:
	static bool loadCellType (boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& assoc_it, CellTypeDLL& ct);
};
