#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include "AppConfig.hpp"
#include "DataConversion.hpp"
#include "SignatureDLL.hpp"

class SignaturesDLL
{
public:
	static bool Initialize();

	static HawkeyeError Add (DataSignatureDLL& signature);
	static HawkeyeError Remove (const std::string& shortText);
	static std::string SignText (const std::string& inputText);
	static std::vector<DataSignatureDLL> Get();
	static void Import (boost::property_tree::ptree& ptConfig);
	static std::pair<std::string, boost::property_tree::ptree> Export();

private:
};
