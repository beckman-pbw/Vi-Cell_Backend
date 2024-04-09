#include "stdafx.h"

#include <memory>
#include <sstream>

#include "ControllerBoardCommand.hpp"
#include "Logger.hpp"
#include "SyringeTest.hpp"

static const char MODULENAME[] = "SyringeTest";

//*****************************************************************************
SyringeTest::SyringeTest() {

	pLocalIosvc_.reset (new boost::asio::io_service());
	pLocalWork_.reset (new boost::asio::io_service::work (*pLocalIosvc_));

	// Ugliness necessary because std::bind does not appreciate overloaded function names (from PMills code).
	auto THREAD = std::bind (static_cast <std::size_t (boost::asio::io_service::*)(void)> (&boost::asio::io_service::run), pLocalIosvc_.get());
	pThread_.reset (new std::thread (THREAD));

	pSignals_.reset (new boost::asio::signal_set(*pLocalIosvc_, SIGINT, SIGTERM));
	pCBI_.reset (new ControllerBoardInterface (*pLocalIosvc_, "BCI_001A", "BCI_001B"));
}

//*****************************************************************************
SyringeTest::~SyringeTest() {
	pCBI_.reset();
	pLocalWork_.reset();   // destruction of work allows io_service::run() to exit (possibly an unnecessary step...)
	pLocalIosvc_->stop();  // instruct io_service to stop processing (should exit ::run() and end thread.
	pLocalIosvc_.reset();  // Destroys the queue
}

//*****************************************************************************
void SyringeTest::quit() {

	pCBI_->CloseSerial();
}

//*****************************************************************************
void SyringeTest::signalHandler (const boost::system::error_code& ec, int signal_number) {
	if (ec) {
		Logger::L().Log (MODULENAME, severity_level::critical, "Signal listener received error \"" + ec.message() + "\"");
	}

	Logger::L().Log (MODULENAME, severity_level::critical, boost::str (boost::format ("Received signal no. %d") % signal_number).c_str());

	// All done listening.
	pSignals_->cancel();

	// Try to get out of here.
	pLocalIosvc_->post (std::bind(&SyringeTest::quit, this));
}

//*****************************************************************************
bool SyringeTest::init() {

	Logger::L().Log (MODULENAME, severity_level::debug1, "SyringeTest::init: <enter>");

	pCBI_->Initialize();

	Logger::L().Log (MODULENAME, severity_level::debug1, "SyringeTest::init: <exit>");

	return true;
}

//*****************************************************************************
void SyringeTest::testErrorDecoding() {

	//******************************
	// Test Hardware Error Decoding.
	//******************************
	// MotorController
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00021FFF).getAsString());	
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00020002).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00020080).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00020100).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00020200).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00020400).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00020800).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00021000).getAsString());

	// SystemCrash
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022002).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022003).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022004).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022005).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022006).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022007).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022008).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00022009).getAsString());

	// SPIController
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00023122).getAsString());

	// RS343Controller
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00024233).getAsString());

	// I2CController
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00025344).getAsString());

	// RFReader Error
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00026055).getAsString());
	
	// RFReader Error - did not turn on.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00026100).getAsString());

	//******************************
	// Test Timeout Error Decoding.
	//******************************
	// Undefined timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00040000).getAsString());
	
	// RFReader timeout
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00041000).getAsString());

	// MotorL6470A timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00042001).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00042002).getAsString());

	// MotorL6470B timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00043005).getAsString());

	// LED Calibration timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00044000).getAsString());

	// SPI Controller timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00045000).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00045001).getAsString());

	// I2C Controller timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00046000).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00046001).getAsString());

	// PhotoDiode timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00047000).getAsString());

	// BubbleDetector timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00048000).getAsString());

	// RS232 timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00049000).getAsString());

	// Host timeouts (CRC, Header, Transmit, RcvAck)
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0004A000).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0004A001).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0004A002).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0004A003).getAsString());

	// Syringe1 timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0004B001).getAsString());

	// Syringe2 timeout.
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0004B002).getAsString());

	//************************************
	// Test Command Failed Error Decoding.
	//************************************
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080000).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080001).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080002).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080003).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080004).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080005).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080006).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080007).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080008).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080009).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0008000A).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0008000B).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0008000C).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0008000D).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0008000E).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x0008000F).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080010).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080011).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080012).getAsString());
	Logger::L().Log (MODULENAME, severity_level::debug1, ErrorCode(0x00080013).getAsString());
}

//*****************************************************************************
int main() {

	boost::asio::io_service io_svc;
	std::shared_ptr<boost::asio::io_service::work> io_svc_work;

	boost::system::error_code ec;
	Logger::L().Initialize (ec, "SyringeTest.info");
	
	Logger::L().Log (MODULENAME, severity_level::normal, "Starting SyringeTest");

	io_svc_work.reset (new boost::asio::io_service::work(io_svc));

	SyringeTest syringeTest;
	syringeTest.init();

	ControllerBoardCommand cbc(syringeTest.getCBI());

#if 0
	// Test decoding of errors.
    Logger::L().Log( MODULENAME, severity_level::debug1, "**************************************************************************" );
    Logger::L().Log( MODULENAME, severity_level::debug1, "                      Test Decoding Error Codes" );
    syringeTest.testErrorDecoding();
    Logger::L().Log( MODULENAME, severity_level::debug1, "**************************************************************************" );
#endif

	BoardStatus boardStatus;
	uint32_t swVersion=0;

	boardStatus = cbc.send (ControllerBoardMessage::Read, RegisterIds::SwVersion, &swVersion);
	Logger::L().Log (MODULENAME, severity_level::debug1, "Controller Board: v" + std::to_string(swVersion));
	Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	// Currently, only one syringe pump.

	//NOTE: this works...
	// Displays this on debug port: Syringe 2 PNs(pump,OEM,valve,syringe): 36013, 0 9, 28008	
	uint32_t command = 1;	// Init...

//	while (true) {
		Logger::L().Log(MODULENAME, severity_level::debug1, "Syringe Init <before>...");
		boardStatus = cbc.send(ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, &command, sizeof(uint32_t), BoardStatus::Syringe1Busy, 20000);
		Logger::L().Log(MODULENAME, severity_level::debug1, "Syringe Init <after>...");
//	}



#if 1
	Logger::L().Log(MODULENAME, severity_level::debug1, "Syringe1ErrorCode: " + boost::str(boost::format("%08X") % cbc.getStatus().readErrorCode(Syringe1ErrorCode).get()));

	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	SyringeRegisters SyringeCmd;

	Logger::L().Log (MODULENAME, severity_level::debug1, "Syringe set valve parameters to Port #1, rotate CW...");
	memset ((void*)&SyringeCmd, 0, sizeof(SyringeRegisters));
	SyringeCmd.Command = 0;
	SyringeCmd.CommandParam = 1;
	SyringeCmd.CommandParam2 = 0;
	SyringeCmd.ErrorCode = 0;
    boardStatus = cbc.send( ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, (uint32_t*)&SyringeCmd, BoardStatus::Syringe1Busy, 1000 );
	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Syringe move valve to Port #1...");
	command = 3;	// Move valve.
    boardStatus = cbc.send( ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, &command, sizeof( uint32_t ), BoardStatus::Syringe1Busy, 2000 );
	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Syringe set pump parameters to aspirate 1/4 ml (3000)...");
	memset ((void*)&SyringeCmd, 0, sizeof(SyringeRegisters));
	SyringeCmd.Command = 0;
	SyringeCmd.CommandParam = 3000;
	SyringeCmd.CommandParam2 = 3000;
	SyringeCmd.ErrorCode = 0;
    boardStatus = cbc.send( ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, (uint32_t*)&SyringeCmd, BoardStatus::Syringe1Busy, 1000 );
    if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Syringe aspirate 1/4 ml from Port #1...");
	command = 2;	// Aspirate.
    boardStatus = cbc.send( ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, &command, sizeof( uint32_t ), BoardStatus::Syringe1Busy, 20000 );
	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Syringe set valve parameters to Port #2, rotate CW...");
	memset ((void*)&SyringeCmd, 0, sizeof(SyringeRegisters));
	SyringeCmd.Command = 0;
	SyringeCmd.CommandParam = 2;
	SyringeCmd.CommandParam2 = 0;
	SyringeCmd.ErrorCode = 0;
    boardStatus = cbc.send( ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, (uint32_t*)&SyringeCmd, BoardStatus::Syringe1Busy, 20000 );
	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Syringe set move valve to Port #2...");
	command = 3;	// Move valve.
    boardStatus = cbc.send( ControllerBoardMessage::Write, RegisterIds::Syringe1Regs, &command, sizeof( uint32_t ), BoardStatus::Syringe1Busy, 20000 );
	if (boardStatus.isSet(BoardStatus::Error)) {
		Logger::L().Log (MODULENAME, severity_level::debug1, "ControllerBoardStatus: " + cbc.getStatus().getAsString());
	}

	cbc.clearErrorCode (Syringe1ErrorCode);
	cbc.clearHostCommError();

	Logger::L().Log (MODULENAME, severity_level::debug1, "Clear HostCommError...");
	cbc.clearErrorCode (HostCommErrorCode);
#endif

	Logger::L().Log (MODULENAME, severity_level::normal, "Before io_svc.run() in *main*");

	Logger::L().Flush();

	io_svc.run();

    return 0;	// Never get here normally.
}
