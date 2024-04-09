#include "stdafx.h"

#include "CommandParser.hpp"
#include "Logger.hpp"

static const char MODULENAME[]="CommandParser";

//*****************************************************************************
bool CommandParser::parse (std::string delimiter, const std::string cmd) 
{
	return parse(delimiter, cmd, parsedCmd_);
}

bool CommandParser::parse(std::string delimiter, std::string cmd, std::vector<std::string>& parsedCommands)
{
	if (cmd.empty()) {
		Logger::L().Log(MODULENAME, severity_level::error, "Nothing to parse, command is empty.");
		return false;
	}

	size_t pos = 0;
	size_t delimPos = 0;
	std::string token;

	parsedCommands.clear();

	while (true) {
		if ((delimPos = cmd.find(delimiter, pos)) != std::string::npos) {
			token = cmd.substr(pos, delimPos - pos);
			pos = delimPos + delimiter.length();
			parsedCommands.push_back(token);
		}
		else {
			token = cmd.substr(pos);
			parsedCommands.push_back(token);
			break;
		}
	}

	return true;
}

//*****************************************************************************
bool CommandParser::hasIndex (size_t index) const
{

	if (parsedCmd_.size() >= index+1) {
		return true;
	}
	
	return false;
}

//*****************************************************************************
bool CommandParser::getByIndex (size_t index, std::string& value) {

   if (index >= parsedCmd_.size()) {
      value = "";
      return false;
   }

   value = parsedCmd_[index];

   return true;
}

//*****************************************************************************
std::string CommandParser::getByIndex (size_t index) {

	if (index >= parsedCmd_.size()) {
		return "";
	}

	return parsedCmd_[index];
}
