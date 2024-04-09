#include "stdafx.h"

#include <memory>
#include <sstream>

#include "CameraTest.hpp"
#include "BoardStatus.hpp"
#include "ControllerBoardCommand.hpp"
#include "ErrorStatus.hpp"
#include "LedControllerBoardCommand.hpp"
#include "Logger.hpp"


static const char MODULENAME[] = "CameraTest";


//*****************************************************************************
CameraTest::CameraTest() {

	pLocalIosvc_.reset (new boost::asio::io_service());
	pLocalWork_.reset (new boost::asio::io_service::work (*pLocalIosvc_));

	// Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
	auto THREAD = std::bind (static_cast <std::size_t (boost::asio::io_service::*)(void)> (&boost::asio::io_service::run), pLocalIosvc_.get());
	pThread_.reset (new std::thread (THREAD));

	pSignals_.reset (new boost::asio::signal_set(*pLocalIosvc_, SIGINT, SIGTERM));
	pCBI_.reset (new ControllerBoardInterface (*pLocalIosvc_, "BCI_001A", "BCI_001B"));
	pLCBI_.reset (new LedControllerBoardInterface (*pLocalIosvc_, "LEDMOD"));
	pCBCamera_.reset (new Camera (*pLocalIosvc_, pCBI_));
}

//*****************************************************************************
CameraTest::~CameraTest() {
	pCBCamera_.reset();
	pLCBI_.reset();
	pCBI_.reset();
	pLocalWork_.reset();   // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	pLocalIosvc_->stop();  // instruct io_service to stop processing (should exit ::run() and end thread.
	pLocalIosvc_.reset();  // Destroys the queue
}

//*****************************************************************************
void CameraTest::quit() {

	pCBI_->CloseSerial();
}

//*****************************************************************************
void CameraTest::signalHandler (const boost::system::error_code& ec, int signal_number)
{
	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"");
	}

	Logger::L().Log (MODULENAME, severity_level::critical, boost::str (boost::format ("Received signal no. %d") % signal_number).c_str());

	// All done listening.
	pSignals_->cancel();

	// Try to get out of here.
	pLocalIosvc_->post (std::bind(&CameraTest::quit, this));
}

//*****************************************************************************
bool CameraTest::init() {

	Logger::L().Log (MODULENAME, severity_level::debug1, "CameraTest::init: <enter>");

	pCBI_->Initialize();
	
	std::string port = "LEDMOD";
	boost::system::error_code ec2 = pLCBI_->OpenSerial (port);
	if (ec2) {
		Logger::L().Log (MODULENAME, severity_level::normal, "Unable to open LED controller board " + port + ": " + ec2.message());
		return false;
	}

	pCamera_ = std::make_shared<Camera_Basler>(*pLocalIosvc_);
	if (!pCamera_) {
		return false;
	}

	if (pCamera_->init (std::bind(&CameraTest::onCameraTrigger,this))) {
		pCamera_->setGain (0.50);

		//TODO: This needs to send a command to the ControllerBoard to set the exposure time.
		// Actually this probably just needs to go away as the exposure time is not a function used in the ControllerBoard.
		pCamera_->setExposure (500);    // exposureTime is in usecs.  
		
	} else {
		Logger::L().Log (MODULENAME, severity_level::critical, "init() failed");
		return false;
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "CameraTest::init: <exit>");

	return true;
}

//*****************************************************************************
void CameraTest::trigger() {

	pCBCamera_->trigger();
}


//*****************************************************************************
void CameraTest::armTrigger() {
	unsigned int triggerTimeout = 5000;
	
	pCamera_->armTrigger (triggerTimeout);
}

//*****************************************************************************
void CameraTest::waitForImage() {
	unsigned int timeout = 1000;

	pCamera_->waitForImage (timeout);
}

//*****************************************************************************
void CameraTest::onCameraTrigger() {

	Logger::L().Log (MODULENAME, severity_level::normal, "CameraTest::onComplete called...");

	Mat image;
	pCamera_->getImage (image);

	// Must use the C language API as the C++ API causes the application to crash
	// when building for debug.
	// Refer to: http://answers.opencv.org/question/93302/save-png-with-imwrite-read-access-violation/
	cvSaveImage ("testimage.png", &IplImage(image));

	// This C++ API crashes in debug mode.
//	imwrite ("testimage.png", image);
}

//*****************************************************************************
void CameraTest::onCameraTriggerTimeout() {

	Logger::L().Log (MODULENAME, severity_level::normal, "CameraTest::onCameraTriggerTimeout called...");
}

//*****************************************************************************
void CameraTest::getLedControllerBoardStatus (LedControllerBoardCommand& lcbc) {
	std::vector<std::string> strVec;


	// Get information about the LED controller.
	std::string response = lcbc.send (LedControllerBoardCommand::GetSerialNumberCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetSerialNumberHeadCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetSpecInfoCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetMaxPowerCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetWorkingHoursCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetMeasureDiodePowerCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetMeasureTempDiodeCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetMeasureTempAmbientCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetActualStatusCmd);
	lcbc.ParseResponse (response, strVec);
	lcbc.dumpActualStatus (strVec[0]);

	response = lcbc.send (LedControllerBoardCommand::GetFailureByteCmd);
	lcbc.ParseResponse (response, strVec);
	lcbc.dumpErrorStatus (strVec[0]);

	response = lcbc.send (LedControllerBoardCommand::GetLatchedFailureCmd);
	lcbc.ParseResponse (response, strVec);
	lcbc.dumpErrorStatus (strVec[0]);

	response = lcbc.send (LedControllerBoardCommand::GetLevelPowerCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetPercentPowerCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::TemporaryPowerCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetOperatingModeCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetICGFreqCmd);
	lcbc.ParseResponse (response, strVec);

	response = lcbc.send (LedControllerBoardCommand::GetICGDutyCycleCmd);
	lcbc.ParseResponse (response, strVec);
}

//*****************************************************************************
int main() {

	boost::asio::io_service io_svc;
	std::shared_ptr<boost::asio::io_service::work> io_svc_work;

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "CameraTest.info");
	
	Logger::L().Log (MODULENAME, severity_level::normal, "Starting CameraTest");

	io_svc_work.reset (new boost::asio::io_service::work(io_svc));

	CameraTest cameraTest;
	cameraTest.init();

	ControllerBoardCommand cbc(cameraTest.getCBI());

	cbc.clearAllErrorCodes();

	cbc.readErrorStatus();

	LedControllerBoardCommand lcbc(cameraTest.getLCBI());

	// Get LED controller firmware info and set the response delimiter to '|'.
	std::string ledCtlrCmd (LedControllerBoardCommand::GetFirmwareCmd);
	ledCtlrCmd += "|";	// Command response delimiter.
	std::string response = lcbc.send (ledCtlrCmd);
	std::vector<std::string> strVec;
	lcbc.ParseResponse (response, strVec);

	cameraTest.getLedControllerBoardStatus (lcbc);

	// Set up the LED controller to turn off Auto-Startup.
	// Otherwise, the LED comes on when power is applied.

	// Get the LED Operating Mode.
	response = lcbc.send (LedControllerBoardCommand::GetOperatingModeCmd);
	lcbc.ParseResponse (response, strVec);

	// The following bits are turned on.  The *reserved* bits are left on as that was their state out of the box.
	// AutoPowerup, AutoStartup, Analog Input Impedance, Digital Input Impedance, Bit 10 Reserved, Bit 6 Reserved,
	// Digital Input Release, Operating Level Release, Bias Level Release.
	uint16_t operatingMode = 0xDC78;
	lcbc.dumpOperatingMode (operatingMode);

	std::stringstream ss;
	ss << hex << setfill('0') << setw(4) << operatingMode;
	ledCtlrCmd = LedControllerBoardCommand::SetOperatingModeCmd;
	ledCtlrCmd += ss.str();
	response = lcbc.send (ledCtlrCmd);
	lcbc.ParseResponse (response, strVec);

	Logger::L().Log (MODULENAME, severity_level::debug1, "Setting up LED 1...");

#ifdef REV1_HW
	// Configure LED 1 registers.
	LEDRegisters ledCmd;
	memset ((void*)&ledCmd, 0, sizeof(LEDRegisters));
	ledCmd.Command = 1;		// On...
	ledCmd.ErrorCode = 0;
	ledCmd.OnTime = 200;
	ledCmd.Power = 2;
	BoardStatus boardStatus = cbc.send (ControllerBoardMessage::Write, RegisterIds::LED1Regs, (uint32_t*)&ledCmd);
	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	// The LED percentage power level.
	ledCtlrCmd = LedControllerBoardCommand::TemporaryPowerCmd;
	ledCtlrCmd += "15.0";	// gives a grey image.
	response = lcbc.send (ledCtlrCmd);
	lcbc.ParseResponse (response, strVec);
#endif

#if 0	//NOTE: not needed in the Digital Release mode.
	// Turn on LED.
	response = lcbc.send (LedControllerBoardCommand::SetLEDOnCmd);
	lcbc.ParseResponse (response, strVec);
#endif

	while (true) {

		try {
			cameraTest.armTrigger();

			Logger::L().Log (MODULENAME, severity_level::debug1, "Triggering Camera 1...");

			cameraTest.trigger();

			cameraTest.waitForImage();

#if 0	//NOTE: not needed in the Digital Release mode.
		// Turn off LED.
			response = lcbc.send (LedControllerBoardCommand::SetLEDOffCmd);
			lcbc.ParseResponse (response, strVec);
#endif

		} catch (exception& e) {
			Logger::L().Log (MODULENAME, severity_level::critical, "exception: " + std::string(e.what()));
		}
	}

	Logger::L().Log (MODULENAME, severity_level::normal, "Before io_svc.run() in *main*");

	Logger::L().Flush();

	io_svc.run();

    return 0;	// Never get here normally.
}
