#pragma once

#include <stdint.h>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "stdafx.h"

#include "ControllerBoardInterface.hpp"
#include "MotorBase.hpp"
#include "PlateController.hpp"
#include "PlateMoveList.hpp"


class PlateMoveTest
{
public:
	PlateMoveTest();
	virtual ~PlateMoveTest();

	bool init();
	bool runMoveList (std::string runFile);
	void quit();
	std::shared_ptr<ControllerBoardInterface>& PlateMoveTest::getControllerBoardInterface() { return pCbi_; }
//	bool readConfiguration (const std::string& configuration_file, boost::system::error_code& ec);

private:

    const int32_t DefaultPositionTolerance = 1000;      // allow 0.1 mm of inaccuracy?

    void signalHandler (const boost::system::error_code& ec, int signal_number);

    bool inited_;
	std::shared_ptr<boost::asio::signal_set> pSignals_;
	std::shared_ptr<boost::asio::io_service> pLocalIosvc_;
	std::shared_ptr<boost::asio::io_service::work> pLocalWork_;
	std::shared_ptr<std::thread> pThread_;
	std::shared_ptr<ControllerBoardInterface> pCbi_;
    std::shared_ptr<PlateController> pPlateController;
	PlateMoveList moveList_;
	std::string runFileName_;

    t_pPTree    ptfilecfg_;
    t_opPTree   ptconfig;
    t_opPTree   controllerConfigNode;
    std::string configFile;

    int32_t     platePositionTolerance;     // to allow external adjustment
    int32_t     plateThetaHomePos;          // the base position value for the Theta home position
    int32_t     plateThetaHomePosOffset;    // the position offset correction value associated with the Theta home position calibration
    int32_t     plateThetaA1Pos;            // the base position value for the Theta A-1 well position
    int32_t     plateThetaA1PosOffset;      // the position offset correction value for the Theta A-1 well position calibration
    int32_t     plateThetaH12Pos;           // the base position value for the Theta H-12 well position
    int32_t     plateThetaH12PosOffset;     // the position offset correction value for the Theta H-12 well position calibration
    int32_t     plateThetaExtentPos;        // the base position value corresponding to the Theta maximum extent well position
    int32_t     plateThetaExtentPosOffset;  // the position offset correction value associated with the Theta maximum extent position calibration
    int32_t     thetaStartTimeout;
    int32_t     thetaFullTimeout;
    int32_t     plateRadiusHomePos;         // the base position value for the Radius home position
    int32_t     plateRadiusHomePosOffset;   // the position offset correction value associated with the Radius home position calibration
    int32_t     plateRadiusMaxTravel;       // the maximum allowable travel for the Radius motor
    int32_t     plateRadiusOutPos;          // target position for fully extended (eject) position
    int32_t     plateRadiusOutPosOffset;    // calibration offset for fully extended position
    int32_t     plateRadiusA1Pos;           // the base position value for the Radius A-1 well position
    int32_t     plateRadiusA1PosOffset;     // the position offset correction for the Radius A-1 well position calibration
    int32_t     plateRadiusH12Pos;          // the base position value for the Radius H-12 well position
    int32_t     plateRadiusH12PosOffset;    // the offset value associated with the Radius H-12 well position calibration
    int32_t     plateRadiusExtentPos;       // the base position value corresponding to the Radius maximum extent well position
    int32_t     plateRadiusExtentPosOffset; // the position offset correstion value associated with the Radius maximum extent position calibration
    int32_t     radiusStartTimeout;
    int32_t     radiusFullTimeout;

    MotorBase * pTheta;
    MotorBase * pRadius;

    bool        InitInfoFile( void );
    void        ConfigPlateVariables( void );

};
