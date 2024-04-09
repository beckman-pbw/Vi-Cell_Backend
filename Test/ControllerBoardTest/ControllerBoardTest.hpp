#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "ControllerBoardInterface.hpp"

enum eStateMachine
{
	smTop,
	smRdReg_ADDREntry,
	smRdReg_LENEntry,
	smWrReg_ADDREntry,
	smWrReg_VALEntry, // Assume 32-bt uint for now.
};

class ControllerBoardTester
{
public:
	ControllerBoardTester( std::shared_ptr<boost::asio::io_context> ios);

	bool Start(const std::string& sn);
	void Quit();

	void Prompt();


private:
	void HandleInput(const boost::system::error_code& ec);

	void ReadRegister(uint32_t addr, uint32_t len);
	void RdCallback(boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx);

	void WriteRegister(uint32_t addr, uint32_t val);
	void WrCallback(boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx);

	void signal_handler(const boost::system::error_code& ec, int signal_number);

    std::shared_ptr<boost::asio::io_context> io_service;
	std::shared_ptr<boost::asio::io_context::work> ios_work;

	boost::asio::streambuf input_buffer_;

	boost::asio::deadline_timer tmr_input;
	boost::asio::signal_set signals;

	eStateMachine stateMachine_;
	std::string smValueLine_;
	uint32_t smValueEntry_;

	uint32_t smADDRESS;
	uint32_t smLENGTH;
	uint32_t smVALUE;

	ControllerBoardInterface cbi;
};