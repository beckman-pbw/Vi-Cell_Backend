#include <boost/dll/runtime_symbol_info.hpp>

#include "HawkeyeConfig.hpp"
#include "HawkeyeDirectory.hpp"

HawkeyeDirectory::HawkeyeDirectory() {

	boost::filesystem::path path = boost::dll::program_location();
	driveId = path.root_name().string();
	instrumentDir = "Instrument";
	curInstrumentDir = instrumentDir;
}

// Returns the complete file path.
std::string HawkeyeDirectory::GetWorkFlowScriptFile(WorkFlowScriptType scriptType)
{
	if (HawkeyeConfig::Instance().get().instrumentType == HawkeyeConfig::CellHealth_ScienceModule)
	{
		return boost::str(boost::format("%s\\%s\\%s") % boost::filesystem::current_path().string() % CellHealthResourcesDir % getWorkFlowScriptFilename(scriptType));
	}

	return boost::str(boost::format("%s\\%s\\%s") % boost::filesystem::current_path().string() % ViCellResourcesDir % getWorkFlowScriptFilename(scriptType));
}
