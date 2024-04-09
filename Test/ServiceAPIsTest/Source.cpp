#include "stdafx.h"

#include <functional>
#include <iostream>
#include <boost/algorithm/string.hpp>
#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/thread.hpp>

#include "CalibrationHistoryDLL.hpp"
#include "HawkeyeError.hpp"
#include "HawkeyeLogic.hpp"
#include "SystemStatusCommon.hpp"
#include "WorkQueueDLL.hpp"

static const char MODULENAME[] = "ServiceAPI'sTest";
static boost::asio::io_context io_svc_;
CalibrationState currentState;

void CallBackFunction (CalibrationState cs)
{
	currentState = cs;
	//std::cout << CalibrationStateAsString (cs) << std::endl;
}

void PlateCalibrationTest ()
{
	char ok;
	// Invoking the IDLe State
	if ( svc_PerformPlateCalibration (CallBackFunction) != HawkeyeError::eSuccess )
	{
		svc_CancelCalibration (CallBackFunction);
		return;
	}

	while ( currentState != CalibrationState::eCompleted )
	{
		if ( currentState == CalibrationState::eWaitingForRadiusPos )
		{
			std::cout << " Mark the plate center by aligning the plate center with probe and Press \"OK\" and Enter to read the Center posintion of radius Motor" << std::endl;
			
			std::cin.clear ();
			std::cin.ignore ();
			std::cin >> ok;

			if ( svc_PerformPlateCalibration (CallBackFunction) != HawkeyeError::eSuccess )
				return;
		}

		if ( currentState == CalibrationState::eWaitingForThetaPos )
		{
			std::cin.clear ();
			std::cin.ignore ();
			std::cout << " Mark the plate center by aligning the plate side line on the fixture, with probe and  Press \"OK\" and Enter  to read the Zero posintion of Theta Motor" << std::endl;
			std::cin >> ok;
			if ( svc_PerformPlateCalibration (CallBackFunction) != HawkeyeError::eSuccess )
				return;

		}

		if ( currentState == CalibrationState::eWaitingForFinish)
		{
			std::cin.clear ();
			std::cin.ignore ();
			std::cout << " Enter \"OK\" and Press to initialize" << std::endl;
			std::cin >> ok;
			if ( svc_PerformPlateCalibration (CallBackFunction) != HawkeyeError::eSuccess )
				return;
		}
	}
}

void CarouselCalibrationTest ()
{
	char ok;
	// Invoking the IDLe State
	if ( svc_PerformCarouselCalibration (CallBackFunction) != HawkeyeError::eSuccess )
	{
		svc_CancelCalibration (CallBackFunction);
		return;
	}

	while ( currentState != CalibrationState::eCompleted )
	{
		if ( currentState == CalibrationState::eWaitingForRadiusThetaPos )
		{
			std::cout << " Bring the tube number one tube in front of  probe and ENTER \"OK\" followed by \"Enter\" key to read the Radius and Theta Motor Position" << std::endl;

			std::cin.clear ();
			std::cin.ignore ();
			std::cin >> ok;

			if ( svc_PerformCarouselCalibration (CallBackFunction) != HawkeyeError::eSuccess )
				return;
		}

		if ( currentState == CalibrationState::eWaitingForFinish )
		{
			std::cin.clear ();
			std::cin.ignore ();
			std::cout << " Enter \"OK\" and Press to initialize" << std::endl;
			std::cin >> ok;
			if ( svc_PerformCarouselCalibration (CallBackFunction) != HawkeyeError::eSuccess )
				return;
		}
	}
}


void perform(int ip)
{
	switch (ip)
	{
		case 1:
		{
			std::cout << "Execution of \"svc_InitializeCarrier\"\n";
			InitializeCarrier();
		}
		break;
		case 2:
		{
			std::cout << "Execution of \"GetValvePort\"\n";
			char port = 0;
			std::cout << HawkeyeErrorAsString(svc_GetValvePort(port)) << "\n Current port: " << port << std::endl;
		}
			break;
		case 3:
		{
			char inputPort;
			std::cout << "Execution of \"SetValvePort\"\n Enter the input port (A - H):\n";
			std::cin >> inputPort;
			std::cout << "Setting the port : " << inputPort <<"\n" <<HawkeyeErrorAsString(svc_SetValvePort(inputPort)) << "\n Querying the set port..\n";
			perform (2);
		}
			break;
		case 4:
		{
			uint32_t position = 0;
			std::cout << "Execution of \"GetSyringePumpPostion\"\n";
			std::cout << HawkeyeErrorAsString(svc_GetSyringePumpPostion(position)) <<"\n Syringe position: " << position << std::endl;
		}
			break;
		case 5:
		{
			std::cout << "Execution of \"AspirateSample\"\n Enter the volume level 0-1000ul: \n";
			int volume = 0;
			std::cin >> volume;
			std::cout << HawkeyeErrorAsString(svc_AspirateSample(volume)) << std::endl;
		}
			break;
		case 6:
		{
			std::cout << "Execution of \"DispenseSample\"\n Enter the volume level 0-1000ul: \n";
			int volume = 0;
			std::cin >> volume;
			std::cout << HawkeyeErrorAsString(svc_DispenseSample(volume)) << std::endl;
		}
			break;
		case 7:
		{
			std::cout << "Execution of \"GetProbePosition\"\n";
			int32_t pos = 0;
			std::cout << HawkeyeErrorAsString(svc_GetProbePostion(pos)) << "Probe position:" << pos << std::endl;
		}
			break;
		case 8:
		{
			std::cout << "Execution of \"SetProbePosition\"\n";
			std::cout << "Enter 1- to move up and 0-move down\n";
			bool upDown = false;
			uint32_t steps = 0;
			std::cin >> upDown;
			std::cout << "Enter the number of steps to move\n";
			std::cin >> steps;
			std::cout << HawkeyeErrorAsString(svc_SetProbePostion(upDown, steps)) << std::endl;
		}
			break;
		case 9:
		{
			std::cout << "Execution of \"GetSampleWellPosition\"\n";
			SamplePosition pos;
			std::cout << HawkeyeErrorAsString(svc_GetSampleWellPosition(pos)) << std::endl;
			std::cout << "SampleWellPosition :" << pos.row << (int)pos.col << std::endl;
		}
			break;
		case 10:
		{
			std::cout << "Execution of \"SetSampleWellPosition\"\n";
			SamplePosition pos;
			char row;
			uint32_t col;
			std::cout << "Enter the row A-H for Plate and Z for carousel:\n"; std::cin >> row;
			std::cout << "Enter the col# 1 - 11 for Plate and 1 - 24 for carousel:\n"; std::cin >> col;
			std::cout << "Setting  " << row << col << " sample well position" << std::endl;
			pos.row = row;
			pos.col = col;
			std::cout << HawkeyeErrorAsString(svc_SetSampleWellPosition(pos)) << std::endl;
			perform (9);
		}
			break;
		case 11:
		{
			std::cout << "Execution of \"RotateCarousel\"\n";
			SamplePosition tubeNum;
			std::cout << HawkeyeErrorAsString(RotateCarousel(tubeNum)) << std::endl;
			std::cout << "Current Tube Position : " << SamplePositionDLL(tubeNum).getAsStr() << std::endl;
		}
			break;
		case 12:
		{
			std::cout << "Execution of \"EjectSampleStage\"\n";
			std::cout << HawkeyeErrorAsString(EjectSampleStage()) << std::endl;
		}
			break;
		case 13:
		{
			std::cout << "Execution of \"PerformValveRepetition\"\n";
			std::cout << HawkeyeErrorAsString(svc_PerformValveRepetition()) << std::endl;
			perform (2);
		}
		break;
		case 14:
		{
			std::cout << "Execution of \"PerformFocusRepetition\"\n";
			std::cout << HawkeyeErrorAsString(svc_PerformFocusRepetition()) << std::endl;
		}
		break;
		case 15:
		{
			std::cout << "Execution of \"PerformReagentRepetition\"\n";
			std::cout << HawkeyeErrorAsString(svc_PerfomReagentRepetition()) << std::endl;
		}
		break;
		case 16:
		{
			std::cout << "Execution of \"PerfomSyringeRepetition(Note : Select the valid valve)\"\n";
			std::cout << HawkeyeErrorAsString (svc_PerformSyringeRepetition ()) << std::endl;
		}
		break;
		case 17:
		{
			std::cout << "Execution of \"svc_PerformPlateCalibration\"\n";
			CarouselCalibrationTest ();
		}
		break;
		case 18:
		{
			std::cout << "Execution of \"svc_PerformPlateCalibration\"\n";
			PlateCalibrationTest();
		}
		break;
		case 29:
		{
			std::cout << "Execution of \"svc_ MoveProbe\"\n";
			std::cout << "Enter 1- to move up and 0-move down\n";
			bool upDown = 0;
			std::cin >> upDown;
			std::cout << HawkeyeErrorAsString (svc_MoveProbe(upDown)) << std::endl;
		}
		break;
		case 0:
			quick_exit(0);
			break;
		default:
			std::cout << "Invalid Input\n";
			break;
	}
}

// Display the Available features and get input from USER
void prompt()
{
	std::cin.clear ();
	std::cin.ignore ();
	int input;
	std::cout << "Input :\n" 
				<< " 1. InitializeCarrier\n"
				<< " 2. GetValvePort\n"
				<< " 3. SetValvePort\n" 
				<< " 4. GetSyringePumpPostion\n" 
				<< " 5. AspirateSample\n" 
				<< " 6. DispenseSample\n" 
				<< " 7. GetProbePosition\n" 
				<< " 8. SetProbePosition\n" 
				<< " 9. GetSampleWellPosition\n"
				<< " 10. SetSampleWellPosition\n"
				<< " 11. RotateCarousel\n" 
				<< " 12. EjectSampleStage\n" 
				<< " 13. PerformValveRepetition\n" 
				<< " 14. PerformFocusRepetition\n" 
				<< " 15. PerformReagentRepetition\n" 
				<< " 16. PerfomSyringeRepetition\n"
				<< " 17. PerformCarouselCalibartion\n"
				<< " 18. PerfomPlateCalibration\n"
				<< " 19. Move probe the probe Up/Down\n"
				<< " 0. To exit\n";
	
	std::cin >> input;
	
	perform(input);
}

void waitForHardwareInitialization(const boost::system::error_code&, boost::asio::deadline_timer *tim)
{
	SystemStatusData *sysStat;

	InitializationState initializationState = IsInitializationComplete();
	switch (initializationState)
	{
		case eInitializationInProgress:
			std::cout << "Initialization is in Progress:\n" << std::endl;
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(3));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationFailed:
			GetSystemStatus(sysStat);
			std::cout << "Activer Errors : " << sysStat->active_error_count << std::endl;
			if (sysStat->active_error_count > 0)
			{
				std::stringstream hexstrs;

				for (uint16_t ii = 0; ii < sysStat->active_error_count; ii++)
				{
					std::cout << "Activer Errors : " << sysStat->active_error_codes[ii] << std::endl;
					ClearSystemErrorCode(sysStat->active_error_codes[ii]);
				}
			}
			FreeSystemStatus(sysStat);
			std::cout << "Initialization failed:\n" << std::endl;
			break;

		case eFirmwareUpdateInProgress:
			std::cout << "Firmware update in progress:\n" << std::endl;
			tim->expires_at(tim->expires_at() + boost::posix_time::seconds(3));
			tim->async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, tim));
			return;

		case eInitializationComplete:
			GetSystemStatus(sysStat);
			std::cout << "Activer Errors : " << sysStat->active_error_count << std::endl;
			if (sysStat->active_error_count > 0)
			{
				std::stringstream hexstrs;

				for (uint16_t ii = 0; ii < sysStat->active_error_count; ii++)
					std::cout << "Activer Errors : " << *sysStat->active_error_codes << std::endl;
			}
			FreeSystemStatus(sysStat);
			std::cout << "Initialization is complete:\n Press  Enter to Continue....." << std::endl;
			break;

		default:
			std::cout << "Unknown initialization status:\n" << std::endl;
	}
}

//*****************************************************************************
int main(int argc, char* argv[])
{
	std::shared_ptr<boost::asio::io_context::work> io_svc_work;
	io_svc_work.reset(new boost::asio::io_context::work(io_svc_));

	Initialize(true);

	boost::asio::io_context io;
	boost::asio::deadline_timer t (io, boost::posix_time::seconds(5));

	t.async_wait(boost::bind(waitForHardwareInitialization, boost::asio::placeholders::error, &t));
	io.run();

	LoginUser("bci_service", "106320");

	while (1)
	{
		prompt();
	}
	return 0;
}


