#pragma once

#include <iostream>
#include <fstream>

#include <string>

#include "stdafx.h"


class SampleRunList
{
public:
	static const std::string CmdInitialize;
	static const std::string CmdMoveValve;
	static const std::string CmdMoveSyringe;
	static const std::string CmdCapture;
	static const std::string CmdSetExposure;
	static const std::string CmdSetGain;
	static const std::string CmdSetLedPower;
	static const std::string CmdSetLedOnTime;
	static const std::string CmdWaitForKeyPress;
	static const std::string CmdDirectory;
	static const std::string CmdStop;
	static const std::string CmdSetTube;
	static const std::string CmdSetProbe;
	static const std::string CmdSetFocus;
	static const std::string CmdSetReagent;
	static const std::string CmdRemoveCameraLimits;

	SampleRunList::SampleRunList();
	virtual SampleRunList::~SampleRunList();

	bool open (std::string filename);
	bool read (std::string& cmd, std::string& param1, std::string& param2, std::string& param3);
	void getOutputDirectory (std::string& dir);
	std::string& getOutputDirectory();

private:
	std::ifstream cmdFile_;
	std::string outputDirectory_;
};