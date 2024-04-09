#pragma once

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "MotorBase.hpp"

#define ESC 0x1b

enum eStateMachine
{
	smTop,
    smRdReg_MOTORADDREntry,
    smRdReg_GLOBALADDREntry,
    smRdReg_LENEntry,
    smRdReg_MOTORIDEntry,
    smRdReg_MOTORStatus,
    smRdReg_ALLStatus,
    smWrReg_MOTORInit,
    smWrReg_MOTORADDREntry,
    smWrReg_GLOBALADDREntry,
    smWrReg_PARAMADDREntry,
    smWrReg_VALEntry,       // Assume 32-bt uint for now.
    smWrReg_CMDEntry,
    smWrReg_POSEntry,
    smWrReg_STOPEntry,
    smWrReg_ENABLEEntry,
    smWrReg_CMDExecute,
    smWrReg_ERRORClear,
};



class MotorControlTester
{
public:
    MotorControlTester( std::shared_ptr<boost::asio::io_context> ios, std::shared_ptr<ControllerBoardInterface> pcbi );
    virtual ~MotorControlTester();

    bool Init();
    bool Start(std::string config_file = "");

    enum InitTypes
    {
        NoInit = 0,
        InitStatus = 1,
        InitTypeParamsLocal = 2,
        InitTypeParams = 3,
        InitAllParamsLocal = 4,
        InitAllParams = 5,
        ReadAllParams = 6,
    };

private:

    std::shared_ptr<boost::asio::io_context> pLocalIosvc;
    std::shared_ptr<boost::asio::io_context::work> pLocalWork_;
    std::shared_ptr<std::thread> pThread_;
    std::shared_ptr<boost::asio::deadline_timer> pTimer;
    std::shared_ptr<ControllerBoardInterface> pCbi_;
    std::shared_ptr<boost::asio::signal_set> pSignals_;

    boost::system::error_code       timerError;
    boost::asio::signal_set *       pSignalSet;

    t_pPTree            ptfilecfg_;
    t_opPTree           ptconfig;
    t_opPTree           controllers_node;
    t_opPTree           motorcfg_node;
    std::string         configFile;

    MotorBase           probeMotor;
    MotorBase           radiusMotor;
    MotorBase           thetaMotor;
    MotorBase           focusMotor;
    MotorBase           reagentMotor;
    MotorBase           rack1Motor;
    MotorBase           rack2Motor;

    bool                pcbiOk;
    bool                quitDone;
    std::string         smValueLine;
    std::string         cbiPort;
    uint32_t            smState;
    uint32_t            prevState;
    int32_t             smMOTORADDR;
    int32_t             smMOTORBASEADDR;
    int32_t             smMOTOROFFSETADDR;
    int32_t             smGLOBALADDR;
    uint32_t            smVALUE;
    uint32_t            smLENGTH;
    int32_t             smMOTORID;
    int32_t             smCMDPARAM;
    BoardStatus         smSTATUS;
    uint32_t            rdValue;
    uint32_t            motorCmd;

    MotorRegisters *    pMotorRegs_;
    MotorRegisters      motorRegs_;
    MotorRegisters      thetaMotorRegs_;
    MotorCfgParams      dfltThetaParams_;
    MotorRegisters      radiusMotorRegs_;
    MotorCfgParams      dfltRadiusParams_;
    MotorRegisters      probeMotorRegs_;
    MotorCfgParams      dfltProbeParams_;
    MotorRegisters      reagentMotorRegs_;
    MotorCfgParams      dfltReagentParams_;
    MotorRegisters      focusMotorRegs_;
    MotorCfgParams      dfltFocusParams_;
    MotorRegisters      rack1MotorRegs_;
    MotorCfgParams      dfltRack1Params_;
    MotorRegisters      rack2MotorRegs_;
    MotorCfgParams      dfltRack2Params_;
    MotorRegisters      objectiveMotorRegs_;
    MotorCfgParams      dfltObjectiveParams_;


    void Quit();
    void AppQuit();
    bool OpenCbi(std::string port);
    void InitInfoNode(std::string controllerNodeName, t_opPTree & cntlNode);
    void signal_handler(const boost::system::error_code& ec, int signal_number);

    void CloseAllMotors(void);
    void GetAllStatus(bool initStatus, bool show);
    void InitAllConfig(void);
    void DoAllMotors(bool close, bool stop, InitTypes type = InitTypes::NoInit, bool show = false);
    void InitMotorParams(int32_t motorId, InitTypes type, t_opPTree & motorCfg);
    void ReadMotorParamsInfo(MotorCfgParams * pMotorCfg, t_opPTree paramNode);
    void UpdateRegParams(MotorRegisters * pReg, MotorCfgParams * pMotorCfg);
    void UpdateParamsFromRegs(MotorCfgParams * pCfg, int32_t motorId);
    void UpdateMotorDefaultParams(MotorCfgParams * pCfg, int32_t motorId);        // send the default params to the motor controller...
    void WriteMotorParams(MotorRegisters * pReg, int32_t motorId);
    void InitDefaultParams(int32_t motorId, t_opPTree configTree);
    void ShowMotorRegister(MotorBase * pMotor, MotorRegisters * pReg);
    void Prompt();
    void ShowHelp(void);
    void HandleInput(const boost::system::error_code& ec);
    void HandleTopState(int C);
    bool ReadMotorOffsetAddress(std::string & inputStr);
    bool UpdateMotorRegFromOffsetAddr( uint32_t addr_offset, uint32_t newVal );
    bool ReadGlobalAddress(std::string & inputStr);

    void DoMotorCmd(uint32_t cmd);
    void GetMotorObjForId(int32_t motorId, MotorBase * & pMotor);
    void GetMotorRegsForId(int32_t motorId, MotorRegisters * & pMotorRegs);
    void GetMotorObjAndRegsForId(int32_t motorId, MotorBase * & pMotor, MotorRegisters * & pMotorRegs);
    uint32_t GetMotorRegsAddrForId(int32_t motorId);

    void RdCallback(boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx);
    void WrtCallback(boost::system::error_code ec, uint16_t status, ControllerBoardInterface::t_ptxrxbuf tx, ControllerBoardInterface::t_ptxrxbuf rx);

};