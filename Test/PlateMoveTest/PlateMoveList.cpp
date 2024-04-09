#include "stdafx.h"

#include <boost/algorithm/string.hpp>

#include "CommandParser.hpp"
#include "Logger.hpp"
#include "PlateMoveList.hpp"

static const char MODULENAME[] = "PlateMoveList";

const std::string PlateMoveList::CmdInitialize      = "I";      // Initialize.
const std::string PlateMoveList::CmdHomeTheta       = "Ht";     // Home the Theta motor
const std::string PlateMoveList::CmdZeroTheta       = "Zt";     // Set the current position as the Theta zero point
const std::string PlateMoveList::CmdHomeRadius      = "Hr";     // Home the Radius motor
const std::string PlateMoveList::CmdZeroRadius      = "Zr";     // Set the current position as the Radius zero point
const std::string PlateMoveList::CmdA1              = "Pa";     // move the Theta and Radius motors to the A1 location
const std::string PlateMoveList::CmdH12             = "Ph";     // move the Theta and Radius motors to the H12 location
const std::string PlateMoveList::CmdT               = "Mt";     // move the Theta motor to the specified location
const std::string PlateMoveList::CmdR               = "Mr";     // move the Radius motor to the specified location
const std::string PlateMoveList::CmdCal1Theta       = "C1t";    // move the Theta motor to the cal1 position
const std::string PlateMoveList::CmdCal1Radius      = "Cr1";    // move the Radius motor to the cal1 position
const std::string PlateMoveList::CmdCal2Theta       = "C2t";    // move the Theta motor to the cal2 position
const std::string PlateMoveList::CmdCal2Radius      = "C2r";    // move the Radius motor to the cal2 position
const std::string PlateMoveList::CmdMovePlate       = "Ax";     // Move plate to absolute radius, theta position
const std::string PlateMoveList::CmdMoveRelative    = "Rx";     // Move plate to relative radius, theta position change values
const std::string PlateMoveList::CmdWaitForKeyPress = "K";      // Wait for a keypress.
const std::string PlateMoveList::CmdSleep           = "S";      // insert a delay into the command stream processing
const std::string PlateMoveList::CmdExit            = "E";      // Stop processing the run list and exit the program.

//*****************************************************************************
PlateMoveList::PlateMoveList()
{
}

//*****************************************************************************
PlateMoveList::~PlateMoveList()
{
}

//*****************************************************************************
bool PlateMoveList::open( std::string filename )
{
    cmdFile_.open( filename );
    boost::filesystem::path p( filename );

    Logger::L().Log( MODULENAME, severity_level::normal, "opening move run list: " + p.filename().generic_string() );
    if ( cmdFile_.is_open() )
    {
        return true;
    }
    else
    {
        Logger::L().Log( MODULENAME, severity_level::normal, "failed to open move run list: " + filename );
        return false;
    }
}

//*****************************************************************************
bool PlateMoveList::read( std::string& cmd, uint32_t& param1, uint32_t& param2, uint32_t& param3 )
{
    std::string line;
    CommandParser cmdParser;

READLINE:
    if ( getline( cmdFile_, line ) )
    {

        boost::algorithm::trim( line );

        // Skip empty lines.
        if ( line.length() == 0 )
        {
            goto READLINE;
        }

        // Skip commented out lines.  Comment character is '!'.
        if ( line[0] == '!' )
        {
            goto READLINE;
        }

        cmdParser.parse( ",", line );

        cmd = cmdParser.getByIndex( 0 );
        cmd[0] = std::toupper( cmd[0] );    // multi-character commands are only uppercase in the first character

        if ( cmdParser.hasIndex( 1 ) )
        {
            param1 = std::stoul( cmdParser.getByIndex( 1 ) );
        }
        if ( cmdParser.hasIndex( 2 ) )
        {
            param2 = std::stoul( cmdParser.getByIndex( 2 ) );
        }
        if ( cmdParser.hasIndex( 3 ) )
        {
            param3 = std::stoul( cmdParser.getByIndex( 3 ) );
        }

        Logger::L().Log( MODULENAME, severity_level::debug1, boost::str( boost::format( "cmd: %s, param1: %d, param2: %d, param2: %d" ) % cmd % param1 % param2 % param3 ) );

        return true;
    }
    else
    {
        cmdFile_.close();
        return false;
    }
}
