#pragma once

#include <iostream>
#include <map>

#include "boost\property_tree\ptree.hpp"

#include "HawkeyeDirectory.hpp"
#include "HawkeyeError.hpp"

class ImportExportConfiguration
{
public:
	typedef void ImportFn(boost::property_tree::ptree&);
	static const std::vector <std::pair<std::string, boost::property_tree::ptree>> getExportConfigMap();
	static std::vector <std::pair<std::string, ImportFn*>> getImportConfigMap();
	static HawkeyeError getExportConfigurationData (std::string username, boost::property_tree::ptree& pTreeToExport);
	static HawkeyeError setImportConfigurationData (const boost::property_tree::ptree& pImportTree);
	static HawkeyeError ImportConfigurationForOfflineAnalysis (const boost::property_tree::ptree& pImportTree);
	static std::string GetGeneralInformation();
	static bool ImportGeneralInformation (boost::property_tree::ptree& giTree);
};
