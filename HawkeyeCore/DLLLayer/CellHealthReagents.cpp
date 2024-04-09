#include "stdafx.h"

#include <boost/algorithm/string/trim.hpp>

#include "CellHealthReagents.hpp"
#include "CommandParser.hpp"
#include "DBif_Api.h"
#include "HawkeyeDataAccess.h"
#include "HawkeyeError.hpp"
#include "Logger.hpp"
#include "SystemErrorFeeder.hpp"
#include "SystemErrors.hpp"
#include "Workflow.hpp"

static const char MODULENAME[] = "CellHealthReagents";

static std::map<int16_t,DBApi::DB_CellHealthReagentRecord> chrList = {};
static int prevRemainingUses = 0;

//*****************************************************************************
//NOTE: this code has been copied from Workflow.cpp and refactored to
// determine the volume of each reagent required for sampling from the ACup.
// Any changes made to this code "may" also need to be made to the Workflow.cpp code.
//*****************************************************************************

constexpr int MaxNumReagents = 5;
class WorkflowScript
{
public:
	void LoadWorkflow (const std::string& workflowName, const std::string& filename)
	{		
		reagentVolumes.resize(MaxNumReagents);
		loadWorkflow (workflowName, filename);

//debug
		std::string msg = "Reagent usage for " + workflowName + ":";
		for (int i = 0; i < MaxNumReagents; i++)
		{
			msg += boost::str(boost::format("\n\tReagent %d\tVolume: %d") % i % reagentVolumes[i]);
		}
		Logger::L().Log(workflowName, severity_level::debug1, msg);
	}

	std::vector<int>& GetWorkflowReagentVolumes()
	{
		return reagentVolumes;
	}

private:
	int curValve = 0;
	int curVolume = 0;
	std::vector<int> reagentVolumes;

	bool  getNextValidScriptLine (std::istream& stream, std::string& line) const
	{
		// Retrieves the next valid (non-blank, non-comment) line from the file
		// Returns false on error/EOF.

		line.clear();

		while (true)
		{
			if (!getline(stream, line))
			{
				Logger::L().Log (MODULENAME, severity_level::debug2, "getNextValidScriptLine : unable to fetch new line content!");
				return false;
			}

			boost::algorithm::trim(line);

			// Skip empty lines.
			if (line.length() == 0)
				continue;

			// Skip commented out lines.  Comment character is '!'.
			if (line[0] == '!')
				continue;

			// Got a good line.
			break;
		}

		return !line.empty();
	}

	//*****************************************************************************
	bool loadWorkflow (const std::string& workflowName, const std::string& filename)
	{
		std::stringstream textStream;
		if (!HDA_ReadEncryptedTextFile (filename, textStream))
		{
			Logger::L().Log (workflowName, severity_level::error, "<exit, failed to decrypt the Workflow script: " + filename + ">");
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_readerror,
				instrument_error::instrument_storage_instance::workflow_script,
				instrument_error::severity_level::error));
			return false;
		}

		boost::filesystem::path p(filename);
		const auto fileDirectory = p.parent_path();

		std::string line;
		while (getNextValidScriptLine(textStream, line))
		{
			if (!processLine(workflowName, line, textStream, fileDirectory))
			{
				Logger::L().Log (workflowName, severity_level::error, "failed to process workflow command: " + line);
				return false;
			}

		}

		return true;
	}
	
	//*****************************************************************************
	bool processLine (
		const std::string& workflowName, 
		const std::string& line, 
		std::istream& stream, 
		boost::filesystem::path fileDirectory)
	{
		CommandParser cmdParser;
		cmdParser.parse(",", line);

		std::string cmd = cmdParser.getByIndex(0);
		boost::to_upper(cmd);

		std::stringstream ss;
		ss << "cmd: " << cmd;

		std::vector<std::string> strParams;

		int idx = 1;
		while (cmdParser.hasIndex(idx))
		{
			std::string strTemp = cmdParser.getByIndex(idx);
			boost::algorithm::trim(strTemp);
			strParams.push_back(strTemp);
			ss << ", param" << std::to_string(idx - 1) << ": " << strTemp; // << ", ";
			idx++;
		}

		// Save for debugging: Logger::L().Log (workflowName, severity_level::debug1, ss.str());

		if (cmd == "S" && strParams[0] == "V")
		{
			curValve = std::stoi (strParams[1]);
		}

		if (cmd == "S" && strParams[0] == "M")
		{
			// Save for debugging: Logger::L().Log (workflowName, severity_level::debug1, "curVolume: " + std::to_string(curVolume));

			int volume_being_moved = 0;
			int targetVolume = std::stoi (strParams[1]);
			if (targetVolume > curVolume)
			{
				volume_being_moved = targetVolume - curVolume;
				if (curValve == SyringePumpPort::Reagent_1 ||
					curValve == SyringePumpPort::Cleaner_1 ||
					curValve == SyringePumpPort::Cleaner_2 ||
					curValve == SyringePumpPort::Cleaner_3 ||
					curValve == SyringePumpPort::Diluent)
				{
					reagentVolumes[curValve - SyringePumpPort::Reagent_1] += volume_being_moved;
				}
			}
			else
			{
				volume_being_moved = curVolume - targetVolume;
			}
			
			curVolume = targetVolume;
		}

		// Repeats: Need to pull the next N lines out and process them Y times.
		if (cmd == Workflow::ScriptControlRepeatCmd)
		{
			// Param 1: Lines; param 2: Iterations
			if (strParams.size() != 2)
			{
				Logger::L().Log (workflowName, severity_level::error, "Wrong number of arguments to FlowControlRepeat command");
				return false;
			}
			uint32_t linecount = std::stoul(strParams[0]);
			std::string srepetitionHelper = strParams[1];
			uint32_t repetitionCount = 1;
			if ((srepetitionHelper.at(0) >= '0') && (srepetitionHelper.at(0) <= '9'))
			{
				repetitionCount = stoul(srepetitionHelper);
			}

			// Load next lines.
			std::vector<std::string> nextLines;
			for (uint32_t i = 0; i < linecount; i++)
			{
				std::string line;
				if (!getNextValidScriptLine(stream, line))
				{
					Logger::L().Log (workflowName, severity_level::error, "Unable to collect enough lines to fulfill the loop request in \"" + line + "\"");
					return false;
				}

				nextLines.push_back (line);
			}

			// Process the lines as required.
			for (uint32_t i = 0; i < repetitionCount; i++)
			{
				for (auto l : nextLines)
				{
					if (!processLine(workflowName, l, stream, fileDirectory))
					{
						Logger::L().Log (workflowName, severity_level::error, "Failure while processing script line \"" + l + "\" during Repeat command.");
						return false;
					}
				}
			}
		}

		// RunScript: Load script and insert at this point; then continue.
		if (cmd == Workflow::RunScriptCmd)
		{
			std::string dir = fileDirectory.empty() ? "" : fileDirectory.string() + "\\";
			loadWorkflow (workflowName, dir + strParams[0]);
		}

		return true;
	}	
};

static WorkflowScript workflowScript;

//*****************************************************************************
bool CellHealthReagents::Initialize()
{
	std::vector<DBApi::DB_CellHealthReagentRecord> list = {};
	DBApi::eQueryResult status = DBApi::DbGetCellHealthReagentsList (list);
	//TODO : check return code...

	for (auto& v: list)
	{
		chrList [v.Type] = v;
	}
	
	std::string filePath = HawkeyeDirectory::Instance().getWorkFlowScriptFile (HawkeyeDirectory::WorkFlowScriptType::eSampleACup);
	workflowScript.LoadWorkflow ("Sample", filePath);

	Log();

	return true;
}

//*****************************************************************************
bool CellHealthReagents::Log()
{
	std::vector<int>& requiredReagentVolumes = workflowScript.GetWorkflowReagentVolumes();
	std::stringstream ss;
	
	ss << "CellHealth Reagents\n";
	int idx = 0;
	for (auto& v : chrList)
	{
		ss << boost::str (boost::format ("\tType: %d, Name: %s, Required_uL: %ld, Current_uL: %ld\n")
			% v.second.Type
			% v.second.Name
			% requiredReagentVolumes[idx]
			% v.second.Volume);
		idx++;
	}

	ss << boost::str (boost::format ("\t# of samples remaining: %d")
		% GetRemainingReagentUses());
	
	Logger::L().Log (MODULENAME, severity_level::normal, ss.str());

	return true;
}

//*****************************************************************************
HawkeyeError CellHealthReagents::GetVolume (CellHealthReagents::FluidType type, int& volume_ul)
{
	DBApi::DB_CellHealthReagentRecord chr;
	const DBApi::eQueryResult dbStatus = DBApi::DbGetCellHealthReagentByType(chr, static_cast<int16_t>(type));
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		volume_ul = 0;

		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("GetVolume: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));

		return HawkeyeError::eDatabaseError;
	}

	chrList[static_cast<int16_t>(type)] = chr;
	
	volume_ul = chr.Volume;

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError CellHealthReagents::SetVolume (CellHealthReagents::FluidType type, int volume_ul)
{
	DBApi::DB_CellHealthReagentRecord chr;
	DBApi::eQueryResult dbStatus = DBApi::DbGetCellHealthReagentByType (chr, static_cast<int16_t>(type));
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("AddVolume: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));
		return HawkeyeError::eDatabaseError;
	}

	chr.Volume = volume_ul;
	chrList[static_cast<int16_t>(type)] = chr;

	dbStatus = DBApi::DbModifyCellHealthReagent (chr);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("SetVolume: <exit, DB modify failed, status: %ld>") % (int32_t)dbStatus));

		return HawkeyeError::eDatabaseError;
	}

	Logger::L().Log (MODULENAME, severity_level::normal,
	boost::str (boost::format ("\tSetVolume:: Type: %d, Name: %s, Volume: %ld")
		% chr.Type
		% chr.Name
		% chr.Volume));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError CellHealthReagents::AddVolume (CellHealthReagents::FluidType type, int volume_ul)
{
	DBApi::DB_CellHealthReagentRecord chr;
	DBApi::eQueryResult dbStatus = DBApi::DbGetCellHealthReagentByType (chr, static_cast<int16_t>(type));
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("AddVolume: <exit, DB read failed, status: %ld>") % (int32_t)dbStatus));
		return HawkeyeError::eDatabaseError;
	}

	chr.Volume += volume_ul;
	chrList[static_cast<int16_t>(type)] = chr;

	dbStatus = DBApi::DbModifyCellHealthReagent (chr);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("AddVolume: <exit, DB modify failed, status: %ld>") % (int32_t)dbStatus));
		return HawkeyeError::eDatabaseError;
	}

	Logger::L().Log (MODULENAME, severity_level::normal,
		boost::str (boost::format ("\tAddVolume:: Type: %d, Name: %s, Volume: %ld")
			% chr.Type
			% chr.Name
			% chr.Volume));

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError CellHealthReagents::AspirateVolume (CellHealthReagents::FluidType type, int volume_ul)
{
	if ((type != FluidType::TrypanBlue &&
		 type != FluidType::Cleaner &&
		 type != FluidType::ConditioningSolution &&
		 type != FluidType::Buffer &&
		 type != FluidType::Diluent) || 
		volume_ul < 0)
	{
		return HawkeyeError::eInvalidArgs;
	}
	
	DBApi::DB_CellHealthReagentRecord& chr = chrList[static_cast<int16_t>(type)];

	const auto orgVolume = chr.Volume;
	chr.Volume -= volume_ul;
	if (chr.Volume < 0)
	{
		chr.Volume = 0;
	}

	std::stringstream ss;
	ss << "AspirateVolume: ";
	ss << boost::str (boost::format ("\tType: %d, Name: %s, Org Volume: %ld, Change: %ld, New Volume: %ld")
		% chr.Type
		% chr.Name
		% orgVolume
		% volume_ul
		% chr.Volume);

	Logger::L().Log (MODULENAME, severity_level::normal, ss.str());
	
	DBApi::eQueryResult dbStatus = DBApi::DbModifyCellHealthReagent (chr);
	if (dbStatus != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("AspirateVolume: <exit, DB modify failed, status: %ld>") % (int32_t)dbStatus));

		return HawkeyeError::eDatabaseError;
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
uint16_t CellHealthReagents::GetRemainingReagentUses()
{
	std::vector<int> reagentUses (MaxNumReagents, -1);
	std::vector<int>& requiredReagentVolumes = workflowScript.GetWorkflowReagentVolumes();

	for (int fluidType = static_cast<int>(FluidType::TrypanBlue);
	         fluidType <= static_cast<int>(FluidType::Diluent);
	         fluidType++)
	{
		int idx = fluidType - 1;
		
		// Some reagents may not be used in a script.
		// For example, Conditioning Solution is not used in the current ACup sample workflow script.		
		if (requiredReagentVolumes[idx] > 0)
		{
			GetVolume (static_cast<FluidType>(fluidType), reagentUses[idx]);
			reagentUses[idx] /= requiredReagentVolumes[idx];
		}
	}
	
	uint16_t remainingUses = std::numeric_limits<uint16_t>::max();

	for (const auto& uses : reagentUses)
	{		
		if (uses >= 0) // Do not use unused reagents in the final calculation.
		{
			// Uses remaining is the minimum remaining events of the reagents.
			remainingUses = std::min (static_cast<uint16_t>(uses), remainingUses);
		}
	}

	if (remainingUses != prevRemainingUses)
	{
		Logger::L().Log (MODULENAME, severity_level::normal,
			boost::str (boost::format ("# of samples remaining: %d")
				% remainingUses));
	}

	prevRemainingUses = remainingUses;

	return remainingUses;
}
