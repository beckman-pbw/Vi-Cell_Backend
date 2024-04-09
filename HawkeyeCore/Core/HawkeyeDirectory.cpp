#include <boost/dll/runtime_symbol_info.hpp>

#include "HawkeyeDirectory.hpp"

HawkeyeDirectory::HawkeyeDirectory() {

	boost::filesystem::path path = boost::dll::program_location();
	driveId = path.root_name().string();
	instrumentDir = "Instrument";
	curInstrumentDir = instrumentDir;
}
