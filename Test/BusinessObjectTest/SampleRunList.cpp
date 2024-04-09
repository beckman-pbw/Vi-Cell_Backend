#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include <sstream>

#include "CommandParser.hpp"
#include "Logger.hpp"
#include "SampleRunList.hpp"

static const char MODULENAME[] = "SampleRunList";

const std::string SampleRunList::InitializeCmd					= "I";
const std::string SampleRunList::WaitForKeyPressCmd				= "K";
const std::string SampleRunList::CameraCmd						= "C";
const std::string SampleRunList::CameraParamCapture				= "C";
const std::string SampleRunList::CameraParamTakePicture         = "T";
const std::string SampleRunList::CameraParamSetExposure			= "E";
const std::string SampleRunList::CameraParamSetGain				= "G";
const std::string SampleRunList::CameraParamRemoveGainLimits	= "RGL";
const std::string SampleRunList::CameraAdjustBackgroundIntensity = "ABI";
const std::string SampleRunList::CarrierSelectCmd				= "CS";
const std::string SampleRunList::CarrierSelectParamCarousel		= "C";
const std::string SampleRunList::CarrierSelectParam96WellPlate	= "P";
const std::string SampleRunList::FocusCmd						= "F";
const std::string SampleRunList::FocusParamMoveHome				= "H";
const std::string SampleRunList::FocusParamMoveCenter			= "C";
const std::string SampleRunList::FocusParamMoveMax				= "M";
const std::string SampleRunList::FocusParamMoveUpFast			= "UQ";
const std::string SampleRunList::FocusParamMoveDownFast			= "DQ";
const std::string SampleRunList::FocusParamMoveUpCoarse			= "UC";
const std::string SampleRunList::FocusParamMoveDownCoarse		= "DC";
const std::string SampleRunList::FocusParamMoveUpFine			= "UF";
const std::string SampleRunList::FocusParamMoveDownFine			= "DF";
//const std::string SampleRunList::FocusParamAdjust = "???";
const std::string SampleRunList::FocusParamSetCoarseStep		= "CS";
const std::string SampleRunList::FocusParamSetFineStep			= "FS";
const std::string SampleRunList::FocusParamSetPosition          = "S";
const std::string SampleRunList::FocusParamRun					= "R";
const std::string SampleRunList::FocusParamRunAnalysis          = "RA";
const std::string SampleRunList::DirectoryCmd					= "D";
const std::string SampleRunList::LedCmd							= "L";
//const std::string SampleRunList::LedParamSetLedToUse            = "L";
const std::string SampleRunList::LedParamSetPower				= "P";
const std::string SampleRunList::LedParamSetOnTime				= "O";
const std::string SampleRunList::RunScriptCmd					= "N";
const std::string SampleRunList::RunScriptReturnCmd				= "NRET";
const std::string SampleRunList::ProbeCmd						= "P";
const std::string SampleRunList::ProbeParamUp					= "U";
const std::string SampleRunList::ProbeParamDown					= "D";
const std::string SampleRunList::ReagentCmd						= "R";
const std::string SampleRunList::ReagentParamUp					= "U";
const std::string SampleRunList::ReagentParamDown				= "D";
const std::string SampleRunList::ReagentParamPurge				= "P";
const std::string SampleRunList::ReagentParamDoor				= "L";
const std::string SampleRunList::RfidCmd						= "RF";
const std::string SampleRunList::RfidParamScan					= "S";
const std::string SampleRunList::RfidParamRead					= "R";
const std::string SampleRunList::SyringeCmd						= "S";
const std::string SampleRunList::SyringeParamInitialize			= "I";
const std::string SampleRunList::SyringeParamMove				= "M";
const std::string SampleRunList::SyringeParamValve				= "V";
const std::string SampleRunList::TubeCmd						= "T";
//const std::string SampleRunList::TubeParamSetHomePosition       = "S";
const std::string SampleRunList::StopCmd						= "X";
const std::string SampleRunList::WaitCmd						= "W";

const std::string SampleRunList::ImageAnalysisCmd				= "IA";
const std::string SampleRunList::ImageAnalysisParamEnable		= "E";
const std::string SampleRunList::ImageAnalysisParamInstrumentConfigFile = "ICF";
const std::string SampleRunList::ImageAnalysisParamCellAnalysisConfigFile = "CACF";
const std::string SampleRunList::ImageAnalysisParamRun			= "R";

const std::string SampleRunList::DownloadK70					= "DLK70";


//*****************************************************************************
SampleRunList::SampleRunList() {
}

//*****************************************************************************
SampleRunList::~SampleRunList() {
}

//*****************************************************************************
bool SampleRunList::open (std::string filename) {

	cmdFileHandle_.open (filename);
	boost::filesystem::path p(filename);

	Logger::L().Log (MODULENAME, severity_level::normal, "opening workflow file: " + p.filename().generic_string());
	if (cmdFileHandle_.is_open()) {
		cmdFilename_ = filename;
		curLineNum_ = 0;
		return true;

	} else {
		Logger::L().Log (MODULENAME, severity_level::normal, "failed to open workflow file: " + filename);
		return false;
	}
}

//*****************************************************************************
bool SampleRunList::read (std::string& cmd, std::vector<std::string>& strParams) {
    std::stringstream ss;
    std::string line;
	CommandParser cmdParser;

    strParams.clear();

READLINE:
	if (getline (cmdFileHandle_, line)) {
		
		curLineNum_++;

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

        ss << "cmd: " << cmd;

        if (cmd == SampleRunList::RunScriptCmd) {
			Logger::L().Log (MODULENAME, severity_level::debug1, boost::str (boost::format ("Caching \"%s\" @ line #: %d") % cmdFilename_ % curLineNum_));

			// Save the position in the current file, push the filename to list of files and its last line number.
			CmdFileEntry_t cmdFileEntry;
			cmdFileEntry.cmdFilename = cmdFilename_;
			cmdFileEntry.lineNum = curLineNum_;
			cmdFileList_.push_back (cmdFileEntry);

			cmdFileHandle_.close();

			std::string includeFile = cmdParser.getByIndex(1);
			boost::algorithm::trim (includeFile);

			if (!open (includeFile)) {
				return false;
			}

		} else {
            int idx = 1;
            while (true) {
                if (cmdParser.hasIndex(idx)) {
                    std::string strTemp = cmdParser.getByIndex (idx);
                    boost::algorithm::trim (strTemp);
                    strParams.push_back (strTemp);
                    ss << ", param" << std::to_string(idx-1) << ": " << strTemp << ", ";
                    idx++;

                } else {
                    break;
                }
            }
		}

        Logger::L().Log (MODULENAME, severity_level::debug1, ss.str());

		return true;

	} else {
NEXTFILE:
		cmdFileHandle_.close();

		if (cmdFileList_.size() == 0) {
			return false;

		} else {

			CmdFileEntry_t cmdFileEntry = cmdFileList_.back();
			cmdFilename_ = cmdFileEntry.cmdFilename;

			open (cmdFilename_);

			curLineNum_ = cmdFileEntry.lineNum + 1;

			Logger::L().Log (MODULENAME, severity_level::normal, "Returning to script: " + cmdFilename_ + ", line " + std::to_string(curLineNum_));

			// Position to the correct line number in the previous file.
			for (uint32_t i=0; i < curLineNum_; i++) {
				if (!getline (cmdFileHandle_, line)) {
					cmdFileList_.pop_back();
					cmd = RunScriptReturnCmd;
					goto NEXTFILE;
				}
			}

			cmdParser.parse (",", line);

			cmd = cmdParser.getByIndex (0);
			boost::to_upper (cmd);

            int idx = 1;
            while ( true ) {
                if ( cmdParser.hasIndex(idx)) {
                    std::string strTemp = cmdParser.getByIndex (idx);
                    boost::algorithm::trim (strTemp);
                    strParams.push_back (strTemp);
                    ss << "param" << std::to_string( idx ) << strTemp << ", ";
					idx++;

                } else {
                    break;
                }
            }

			cmdFileList_.pop_back();
		}

		return true;
	}

}
