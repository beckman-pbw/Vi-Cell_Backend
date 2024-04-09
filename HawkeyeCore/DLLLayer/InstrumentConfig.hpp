#pragma once
#include <mutex>
#include "HawkeyeConfig.hpp"
#include "HawkeyeError.hpp"

class InstrumentConfig
{
public:

	static InstrumentConfig& Instance()
	{
		static InstrumentConfig instance;
		return instance;
	}

	bool Initialize();
	DBApi::DB_InstrumentConfigRecord& Get();
	bool Set();
	static void Import (boost::property_tree::ptree& ptConfig);
	static std::pair<std::string, boost::property_tree::ptree> Export(bool disableSecurity = false);
	static std::string GetDLLVersion();
	HawkeyeError setOpticalHardwareConfig(OpticalHardwareConfig type);
	void setLedType(HawkeyeConfig::BrightfieldLedType type);

private:
	bool InitializeDatabase();

	InstrumentConfig() {};
	InstrumentConfig (InstrumentConfig const&);	// Don't Implement
	void operator = (InstrumentConfig const&);	// Don't implement

	bool isInitialized_ = false;
	std::mutex instrumentConfigMutex_;
	DBApi::DB_InstrumentConfigRecord dbInstConfig_ = {};
};

