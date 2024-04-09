#pragma once

#include "Configuration.hpp"

class CommandParser {
public:
	
	bool parse (std::string delimiter, std::string cmd);
	static bool parse (std::string delimiter, std::string cmd, std::vector<std::string>& parsedCommands);
	bool getByIndex (size_t index, std::string& value);
	std::string getByIndex (size_t index);
	size_t size() const { return parsedCmd_.size(); }
	bool hasIndex (size_t index) const;

	std::vector<std::string>::const_iterator begin() { return parsedCmd_.begin(); }
	std::vector<std::string>::const_iterator end() { return parsedCmd_.end(); }

private:
	std::vector<std::string> parsedCmd_;

};
