#include "stdafx.h"

using namespace std;

#include <stdint.h>
#include <conio.h>
#include <functional>
#include <io.h>  
#include <ios>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/thread.hpp>

#include "Logger.hpp"
#include "SysExerciser.hpp"

static const char MODULENAME[] = "SysExerciser";

static std::unique_ptr<HawkeyeThreadPool> pThreadPool_;
static std::shared_ptr<boost::asio::io_context> pLocalIosvc_;
static boost::function<void(bool)> cb_Handler_;
static std::shared_ptr<boost::asio::deadline_timer> inputTimer_;

// For command line input.
#define ESC 0x1B
#define BACKSPACE '\b'
#define CR '\r'
#define NEWLINE '\n'
static std::string smValueLine_;

typedef enum eStateMachine : uint32_t {
	smNOOP,
	smUsageOptions,
	smDecrease,
	smIncrease,
} StateMachine_t;

typedef enum eCommand : char {
	cmdSetCPUUsage = 'C',
	cmdSetRAMUsage = 'R',
	cmdQuit = 'X',
	cmdHelp = '?',
} Command_t;

static std::string menu =
std::string(1, cmdSetCPUUsage) + " | " +
std::string(1, cmdSetRAMUsage) + " | " +
std::string(1, cmdQuit) + " | " +
std::string(1, cmdHelp)
;

static Command_t smCmd_;
static StateMachine_t smState_ = smNOOP;


//*****************************************************************************
SysExerciser::SysExerciser (std::shared_ptr<boost::asio::io_context> pIoSvc)
	: pIoSvc_(pIoSvc)
{
}

//*****************************************************************************
SysExerciser::~SysExerciser()
{
}

//*****************************************************************************
void SysExerciser::prompt() {

	switch (smState_) {
		default:
		case smNOOP:
			std::cout << menu << ": ";
			break;
		case smUsageOptions:
			std::cout << " Decrease(1) | Increase(2): ";
			break;
	}
}

//*****************************************************************************
void SysExerciser::showHelp() {

	std::cout << std::endl << "Usage:\t" << menu << std::endl;
	std::cout << "\t" << (char)cmdSetCPUUsage << " : Change CPU Usage" << std::endl;
	std::cout << "\t" << (char)cmdSetRAMUsage << " : Change RAM Usage" << std::endl;
	std::cout << "\t" << (char)cmdQuit << " : Exit the program" << std::endl;
	std::cout << "\t" << (char)cmdHelp << " : Display this help screen" << std::endl;
	std::cout << std::endl << std::endl;
}

//*****************************************************************************
bool SysExerciser::Start() {

	pRAMThread_ = std::make_unique<HawkeyeThread>("RAMThread");

	std::shared_ptr<SysExerciser::LoadRAM> pLoadRAM = std::make_shared<SysExerciser::LoadRAM>();
	pRAMThread_->Enqueue (std::bind([pLoadRAM]() -> void {
		int xxx = 0;
	}));

	pLoadRAM_ = pLoadRAM;

	inputTimer_ = std::make_shared<boost::asio::deadline_timer>(*pIoSvc_);
	handleInput (boost::system::errc::make_error_code(boost::system::errc::success));

	prompt();

	return true;
}

//*****************************************************************************
void SysExerciser::handleInput (const boost::system::error_code& ec) {

	if (ec) {
		if (ec == boost::asio::error::operation_aborted) {
			Logger::L().Log (MODULENAME, severity_level::debug2, "handleInput:: boost::asio::error::operation_aborted");
		}
		return;
	}

	if (!_kbhit()) {
		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100));
		inputTimer_->async_wait (std::bind(&SysExerciser::handleInput, this, std::placeholders::_1));
		return;
	}

	int C = _getch();
	if (isprint(C)) {
		std::cout << (char)C;

		int CUPPER = toupper (C);
		if (CUPPER == cmdQuit) {
			HWND mHwnd = NULL;
			mHwnd = GetConsoleWindow();
			std::cout << endl << "Detected quit request in input" << std::endl;
			PostMessage (mHwnd, WM_CLOSE, 0, 0);
			return;

		} else if (C == cmdHelp) {
			showHelp();
			smState_ = smNOOP;
			smValueLine_.clear();
		} else {
			smValueLine_ += (char)C;
		}

		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100));
		inputTimer_->async_wait (std::bind(&SysExerciser::handleInput, this, std::placeholders::_1));
		return;

	} else {

		if ((C != ESC) && (C != CR) && (C != NEWLINE)) {
			if (C == BACKSPACE) {
				if (smValueLine_.length() > 0) {
					std::cout << BACKSPACE;
					std::cout << (char)' ';
					std::cout << BACKSPACE;
					smValueLine_.pop_back();
				}
			}

		} else {

			if (C == ESC) {
				smState_ = smNOOP;
				smValueLine_.clear();
				std::cout << std::endl;
				prompt();

			} else if (C == CR || C == NEWLINE) {
				std::cout << std::endl;
				goto PROCESS_STATE;
			}
		}

		inputTimer_->expires_from_now (boost::posix_time::milliseconds(100));
		inputTimer_->async_wait (std::bind(&SysExerciser::handleInput, this, std::placeholders::_1));
		return;
	}

	//	std::cout << std::endl << smValueLine_ << std::endl;

PROCESS_STATE:
	switch (smState_) {
		case smNOOP:
		{
			boost::to_upper (smValueLine_);
			smCmd_ = (Command_t)smValueLine_.c_str()[0];
			switch (smCmd_) {
				case cmdSetCPUUsage:
					smState_ = smUsageOptions;
					prompt();
					break;
				case cmdSetRAMUsage:
					smState_ = smUsageOptions;
					prompt();
					break;
				default:
					std::cout << "Invalid command..." << std::endl;
					break;

			} // End "switch (smCmd_)"

			break;
		}

		case smUsageOptions:
		{
			switch (atoi (smValueLine_.c_str())) {
				case 0:
					smState_ = smNOOP;
					break;
				case 1:
				{
//std::cout << "got here 1" << std::endl;
					if (smCmd_ == cmdSetCPUUsage) {
						if (pLoadCPU_.size()) {
//std::cout << "got here 2" << std::endl;
							pLoadCPU_.back()->Stop();
//std::cout << "got here 3" << std::endl;
							//pLoadCPU_.pop_back();
//std::cout << "got here 4" << std::endl;
//pCPUThreads_.back()->Pause();
							pCPUThreads_.pop_back();
//std::cout << "got here 5" << std::endl;

						} else {
							std::cout << "Nothing running..." << std::endl;
						}
					} else if (smCmd_ == cmdSetRAMUsage) {

						pLoadRAM_->Decrease();

					} else {
						// For future commands.
					}
					break;
				}
				case 2:
				{
					if (smCmd_ == cmdSetCPUUsage) {
						std::shared_ptr<SysExerciser::LoadCPU> pLoadCPU = std::make_shared<SysExerciser::LoadCPU>();

						pLoadCPU_.push_back (pLoadCPU);

						auto size = pLoadCPU_.size();

						pCPUThreads_.push_back (std::make_unique<HawkeyeThread>(std::to_string(size)));

						pCPUThreads_.back()->Enqueue (std::bind([pLoadCPU, size]() -> void {
							pLoadCPU->Start (size);
						}));
					} else if (smCmd_ == cmdSetRAMUsage) {

						pLoadRAM_->Increase();

					} else {
						// For future commands.
					}

					break;
				}
				default:
					smState_ = smNOOP;
					std::cout << "Invalid entry..." << std::endl;
					break;

			} // End "switch (atoi (smValueLine_.c_str()))"

			break;
		}

		smState_ = smNOOP;
		break;

	} // End "switch (smState_)"

	smValueLine_.clear();

	if (smState_ == smNOOP) {
		prompt();
	}

	inputTimer_->expires_from_now(boost::posix_time::milliseconds(100));
	inputTimer_->async_wait (std::bind(&SysExerciser::handleInput, this, std::placeholders::_1));
}

////*****************************************************************************
//void SysExerciser::setCPUUsage() {
//
//	while (true);
//}
//
////*****************************************************************************
//void SysExerciser::setRAMUsage() {
//
//
//}

//*****************************************************************************
//TODO:
// Use code from ScoutTest.cpp to get commands from the command line.
// Change RAM Usage Command: R
//   u: for increase (up).
//   d: for decrease (down).
// Change CPU Usage (C)
//   u: for increase (up).
//   d: for decrease (down).
//*****************************************************************************


//*****************************************************************************
int main (int argc, char* argv[]) {

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "SysExerciser.info", "logger");

	pThreadPool_ = std::make_unique<HawkeyeThreadPool>(true/*auto start*/, 1/*num thread*/, "Main_Thread");
	pLocalIosvc_ = pThreadPool_->GetIoService();

	SysExerciser sysExerciser (pLocalIosvc_);
	sysExerciser.Start();

	pLocalIosvc_->run();

	return 0;
}
