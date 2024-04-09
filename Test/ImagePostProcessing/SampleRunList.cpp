#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include "CommandParser.hpp"
#include "Logger.hpp"
#include "SampleRunList.hpp"

static const char MODULENAME[] = "SampleRunList";

const std::string SampleRunList::CmdInitialize  = "I";	// Initialize.
const std::string SampleRunList::CmdMoveValve   = "V";	// Move valve A to position B.
const std::string SampleRunList::CmdMoveSyringe = "S";	// Aspirate A volume at B speed (rate).
const std::string SampleRunList::CmdCapture     = "C";	// Capture image.
const std::string SampleRunList::CmdSetExposure = "E";	// Set camera exposure.
const std::string SampleRunList::CmdSetGain     = "G";	// Set camera gain.
const std::string SampleRunList::CmdSetLedPower = "L";	// Set LED power.
const std::string SampleRunList::CmdSetLedOnTime = "O";	// Set time LED is on.
const std::string SampleRunList::CmdWaitForKeyPress = "K";	// Wait for a keypress.
const std::string SampleRunList::CmdDirectory   = "D";	// Directory to write the image files to.
const std::string SampleRunList::CmdStop        = "X";	// Stop processing the run list.
const std::string SampleRunList::CmdSetTube		= "T";	// Go to the indicated tube.
const std::string SampleRunList::CmdSetProbe	= "P";	// Go to specified probe position.
const std::string SampleRunList::CmdSetFocus	= "F";	// Go to specified focus position.
const std::string SampleRunList::CmdSetReagent	= "R";	// Go to specified reagent position.
const std::string SampleRunList::CmdRemoveCameraLimits = "Z"; // Temporary test case...


//*****************************************************************************
SampleRunList::SampleRunList() {
}

//*****************************************************************************
SampleRunList::~SampleRunList() {
}

//*****************************************************************************
bool SampleRunList::open (std::string filename) {


	cmdFile_.open (filename);
	boost::filesystem::path p(filename);

	Logger::L().Log (MODULENAME, severity_level::normal, "opening sample run list: " + p.filename().generic_string());
	if (cmdFile_.is_open()) {
		return true;
	} else {
		Logger::L().Log (MODULENAME, severity_level::normal, "failed to open sample run list: " + filename);
		return false;
	}
}

//*****************************************************************************
//bool SampleRunList::read (std::string& cmd, uint32_t& param1, uint32_t& param2, uint32_t& param3) {
bool SampleRunList::read (std::string& cmd, std::string& sParam1, std::string& sParam2, std::string& sParam3) {
	std::string line;
	CommandParser cmdParser;


	sParam1.clear();
	sParam2.clear();
	sParam3.clear();

READLINE:
	if (getline (cmdFile_, line)) {
		
		boost::algorithm::trim (line);

		// Skip empty lines.
		if (line.length() == 0) {
			goto READLINE;
		}

		// Skip commented out lines.  Comment character is '!'.
		if (line[0] == '!') {
			goto READLINE;
		}

		cmdParser.parse (",", line);

		cmd = cmdParser.getByIndex(0);
		boost::to_upper (cmd);

		if (cmd == SampleRunList::CmdDirectory) {
			outputDirectory_ = cmdParser.getByIndex(1);
			boost::algorithm::trim (outputDirectory_);

			// Make sure that there is a "/" at the end of the string.
			if (outputDirectory_[outputDirectory_.length() - 1] != '/') {
				outputDirectory_ += '/';
			}

			// Create the directory path.
			boost::filesystem::path p(outputDirectory_);
			boost::system::error_code ec;
			boost::filesystem::create_directories (p, ec);
			if (ec == boost::system::errc::success) {
				Logger::L().Log (MODULENAME, severity_level::normal, "created directory: " + p.generic_string());
			} else {
				Logger::L().Log (MODULENAME, severity_level::error, "failed to create directory: " + p.generic_string() + " : " + ec.message());
				return false;
			}

		} else {
			if (cmdParser.hasIndex(1)) {
				sParam1 = cmdParser.getByIndex(1);
				boost::algorithm::trim (sParam1);
			}
			if (cmdParser.hasIndex(2)) {
				sParam2 = cmdParser.getByIndex(2);
				boost::algorithm::trim (sParam2);
			}
			if (cmdParser.hasIndex(3)) {
				sParam3 = cmdParser.getByIndex(3);
				boost::algorithm::trim (sParam3);
			}
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format("cmd: %s, param1: %s, param2: %s, param3: %s") % cmd % sParam1 % sParam2 % sParam3));

		return true;

	} else {
		cmdFile_.close();
		return false;
	}

}

//*****************************************************************************
void SampleRunList::getOutputDirectory (std::string& dir) {

	dir = outputDirectory_;
}

//*****************************************************************************
std::string& SampleRunList::getOutputDirectory() {

	return outputDirectory_;
}
