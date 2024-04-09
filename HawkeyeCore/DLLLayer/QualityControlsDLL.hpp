#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include <CellCountingOutputParams.h>
#include "QualityControlDLL.hpp"
#include "ResultDefinition.hpp"
#include "SampleDefinitionDLL.hpp"

QcStatus IsSampleWithinQCLimits (const std::string& bpQcName, const uuid__t& qcUuid, const CellCounterResult::SResult& sresult);

class QualityControlsDLL
{
public:
	static bool Initialize();

	static HawkeyeError Add (std::string username, QualityControlDLL& qcDLL);
	static std::vector<QualityControlDLL>& Get();
	static void Import (boost::property_tree::ptree& ptConfig);
	static std::pair<std::string, boost::property_tree::ptree> Export();
};
