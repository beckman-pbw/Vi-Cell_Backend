#pragma once

#include <iostream>
#include <fstream>

#include <string>

#include "stdafx.h"


class PlateMoveList
{
public:
    static const std::string CmdInitialize;         // Initialize.
    static const std::string CmdHomeTheta;          // Home the Theta motor
    static const std::string CmdZeroTheta;          // Set the current position as the Theta zero point
    static const std::string CmdHomeRadius;         // Home the Radius motor
    static const std::string CmdZeroRadius;         // Set the current position as the Radius zero point
    static const std::string CmdA1;                 // move the Theta and Radius motors to the A1 location
    static const std::string CmdH12;                // move the Theta and Radius motors to the H12 location
    static const std::string CmdT;                  // move the Theta motor to the specified location
    static const std::string CmdR;                  // move the Radius motor to the specified location
    static const std::string CmdCal1Theta;          // move the Theta motor to the cal1 position
    static const std::string CmdCal1Radius;         // move the Radius motor to the cal1 position
    static const std::string CmdCal2Theta;          // move the Theta motor to the cal2 position
    static const std::string CmdCal2Radius;         // move the Radius motor to the cal2 position
    static const std::string CmdMovePlate;          // Move plate to absolute radius, theta position
    static const std::string CmdMoveRelative;       // Move plate to relative radius, theta position change values
    static const std::string CmdWaitForKeyPress;    // Wait for a keypress.
    static const std::string CmdSleep;              // insert a delay into the command stream processing
    static const std::string CmdExit;               // Stop processing the run list and exit the program.



	PlateMoveList::PlateMoveList();
	virtual PlateMoveList::~PlateMoveList();

	bool open (std::string filename);
	bool read (std::string& cmd, uint32_t& param1, uint32_t& param2, uint32_t& param3);

private:
	std::ifstream cmdFile_;
};