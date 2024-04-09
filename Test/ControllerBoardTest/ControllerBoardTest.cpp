// ControllerBoardTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <conio.h>
#include <cstdlib>
#include <iostream>
#include <boost/format.hpp>
#include "ControllerBoardTest.hpp"
#include "Logger.hpp"

ControllerBoardTester::ControllerBoardTester( std::shared_ptr<boost::asio::io_context> ios)
  : io_service(ios)
  , signals(*ios, SIGINT, SIGTERM)
  , tmr_input(*ios)
  , cbi(ios, CNTLR_SN_A_STR, CNTLR_SN_B_STR)
{
		stateMachine_ = smTop;
		smValueEntry_ = 0;

		ios_work.reset(new boost::asio::io_context::work(*io_service));

		signals.async_wait(std::bind(&ControllerBoardTester::signal_handler, this, std::placeholders::_1, std::placeholders::_2));

		tmr_input.expires_from_now(boost::posix_time::seconds(0));
		tmr_input.async_wait(std::bind(&ControllerBoardTester::HandleInput, this, std::placeholders::_1));

}

bool ControllerBoardTester::Start(const std::string& sn)
{
	boost::system::error_code ec = cbi.OpenSerial(sn);

	if (ec)
	{
		std::cout << "Unable to open controller board: " << ec.message() << std::endl;
	}
	else
	{
		Prompt();
	}

	return !ec;
}

void ControllerBoardTester::Quit()
{
	cbi.CloseSerial();
	ios_work.reset();
	io_service->stop();
}

void ControllerBoardTester::signal_handler(const boost::system::error_code& ec, int signal_number)
{
	if (ec)
	{
		std::cout << std::endl << "Signal listener received error \"" << ec.message() << "\"" << std::endl;
	}

	std::cout << boost::str(boost::format("Received signal no. %d") % signal_number).c_str() << std::endl;
	
	// All done listening.
	signals.cancel();

	// Try to get out of here.
	io_service->post(std::bind(&ControllerBoardTester::Quit, this));
}

void ControllerBoardTester::HandleInput(const boost::system::error_code& error)
{
	if (error)
	{
		Quit();
		return;
	}

	tmr_input.expires_from_now(boost::posix_time::milliseconds(50));
	tmr_input.async_wait(std::bind(&ControllerBoardTester::HandleInput, this, std::placeholders::_1));

	if (!_kbhit())
		return;

	int C = _getch();
	std::cout << (char)C;
	// First: process QUIT characters
	if (C == 'q' || C == 'Q')
	{
		std::cout << std::endl << "Detected quit signal in input" << std::endl;
		Quit();
		return;
	}

	
	/*
	* Process multi-character entries
	*/
	switch (stateMachine_)
	{
		case smRdReg_ADDREntry:
		case smRdReg_LENEntry:
		case smWrReg_ADDREntry:
		case smWrReg_VALEntry:
		{
			// Append character to string and wait for next
			// ...unless this is the LF character which lets us continue
			if (C != '\n' && C != '\r')
			{
				smValueLine_ += (char)C;
				return;
			}
			break;
		}
		default:
			break;
	}


	bool SendCommand = false;
	switch (stateMachine_)
	{
		case smTop:
		{
			switch (C)
			{
				case 'r':
				case 'R':
				{
					stateMachine_ = smRdReg_ADDREntry;
					break;
				}
				case 'w':
				case 'W':
				{
					stateMachine_ = smWrReg_ADDREntry;
					break;
				}
				default:
				{
					stateMachine_ = smTop;
					break;
				}
			}
			break;
		}
		case smRdReg_ADDREntry:
		{
			int status = sscanf_s(smValueLine_.c_str(), "%d", &smADDRESS);
			if (status != 1)
			{
				std::cout << std::endl << "Not a legal value: " << smValueLine_.c_str() << std::endl;
				stateMachine_ = smTop;
			}
			else
			{
				stateMachine_ = smRdReg_LENEntry;
			}
			smValueLine_.clear();
			break;
		}
		case smRdReg_LENEntry:
		{
			int status = sscanf_s(smValueLine_.c_str(), "%d", &smLENGTH);
			if (status != 1)
			{
				std::cout << std::endl << "Not a legal value: " << smValueLine_.c_str() << std::endl;
			}
			else
			{
				ReadRegister(smADDRESS, smLENGTH);
			}
			stateMachine_ = smTop;
			smValueLine_.clear();
			break;
		}
		case smWrReg_ADDREntry:
		{
			int status = sscanf_s(smValueLine_.c_str(), "%d", &smADDRESS);
			if (status != 1)
			{
				std::cout << std::endl << "Not a legal value: " << smValueLine_.c_str() << std::endl;
				stateMachine_ = smTop;
			}
			else
			{
				stateMachine_ = smWrReg_VALEntry;
			}
			smValueLine_.clear();
			break;
		}
		case smWrReg_VALEntry:
		{
			int status = sscanf_s(smValueLine_.c_str(), "%d", &smVALUE);
			if (status != 1)
			{
				std::cout << std::endl << "Not a legal value: " << smValueLine_.c_str() << std::endl;
			}
			else
			{
				WriteRegister(smADDRESS, smVALUE);
			}
			stateMachine_ = smTop;
			smValueLine_.clear();
			break;
		}
		default:
		{
			std::cout << std::endl << "Forgot to handle a state machine case!!" << std::endl;
			stateMachine_ = smTop;
		}
	}

	Prompt();
}

void ControllerBoardTester::Prompt()
{
	std::cout << std::endl;
	switch (stateMachine_)
	{
		case smTop:
			std::cout << "R/W/Q: ";
			break;
		case smRdReg_ADDREntry:
		case smWrReg_ADDREntry:
			std::cout << "Address: ";
			break;
		case smRdReg_LENEntry:
			std::cout << "Length (bytes): ";
			break;
		case smWrReg_VALEntry:
			std::cout << "Value (uint32_t): ";
			break;
	}
}


void ControllerBoardTester::ReadRegister(uint32_t addr, uint32_t len)
{
	cbi.ReadRegister(addr, len, ControllerBoardInterface::t_ptxrxbuf(),
		std::bind(&ControllerBoardTester::RdCallback, this,
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void ControllerBoardTester::RdCallback(boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx)
{
	if (ec)
	{
		std::cout << std::endl << "Read Callback reports an error: " << ec.message() << std::endl;
		return;
	}
	if (!rx)
	{
		std::cout << std::endl << "Read Callback received no buffer!" << std::endl;
		return;
	}

	uint32_t* p = (uint32_t*)rx->data();
	if (!p)
	{
		std::cout << "Read Callback received no data!  Status: " << status << std::endl;
	}
	else
	{
		std::string data = boost::str(boost::format("Read Callback\n\tStatus: %d\n\tValue: %d\n") % status % (*p));
		std::cout << std::endl << data.c_str() << std::endl;
	}
}

void ControllerBoardTester::WriteRegister(uint32_t addr, uint32_t val)
{
	ControllerBoardInterface::t_ptxrxbuf txb = std::make_shared<ControllerBoardInterface::t_txrxbuf>();
	txb->resize(sizeof(uint32_t));
	uint32_t* p = (uint32_t*)txb->data();
	*p = val;

	cbi.WriteRegister(addr, txb, 
		std::bind(&ControllerBoardTester::WrCallback, this, 
			std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void ControllerBoardTester::WrCallback(boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx)
{
	if (ec)
	{
		std::cout << std::endl << "Write Callback reports an error: " << ec.message() << std::endl;
		return;
	}
	
	std::string data = boost::str(boost::format("Write Callback\n\tStatus: %d\n") % status );
	std::cout << std::endl << data.c_str() << std::endl;
}



int main()
{
	boost::system::error_code ec;
	Logger::L().Initialize (ec, "ControllerBoardTest.info");

    std::shared_ptr<boost::asio::io_context> ios;

    ios.reset( new boost::asio::io_context() );

	DWORD ndevs;
	FT_STATUS fts = FT_CreateDeviceInfoList(&ndevs);
	std::cout << ndevs << " ftdi devices attached" << std::endl;

	
	
	for (std::size_t i = 0; i < ndevs; i++)
	{
		DWORD Flags;
		DWORD ID;
		DWORD Type;
		DWORD LocId;
		char SerialNumber[16];
		char Description[64];
		FT_HANDLE ftHandleTemp;

		// get information for device 0
		fts = FT_GetDeviceInfoDetail((DWORD)i, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp);

		printf("Dev %zd:\n", i);
		printf(" Flags=0x%x\n", Flags);
		printf(" Type=0x%x\n", Type);
		printf(" LocId=0x%x\n", LocId);
		printf(" SerialNumber=%s\n", SerialNumber);
		printf(" Description=%s\n", Description);
	}

	ControllerBoardTester cbt(ios);

	if (!cbt.Start(CNTLR_SN_A_STR))
	{
		std::cout << "Unable to open controller board ";
		return -1;
	}


	ios->run();
    return 0;
}

