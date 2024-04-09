#pragma once

#include <functional>
#include <stdint.h>
#include <string>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "BoardStatus.hpp"
#include "Configuration.hpp"
#include "ControllerBoardOperation.hpp"
#include "ErrorStatus.hpp"
#include "Registers.hpp"
#include "SignalStatus.hpp"
#include "StageDefines.hpp"
#include "CBOService.hpp"

#define TIMEOUT_FACTOR  100  // Conversion factor from 10th of second (as provided in motor config info file) to milli second

enum ControllerTypeId : int32_t
{
    ControllerTypeIllegal = -1,
    ControllerTypeUnknown = 0,
    ControllerTypeRadius,
    ControllerTypeTheta,
    ControllerTypeProbe,
    ControllerTypeReagent,
    ControllerTypeFocus,
    ControllerTypeLEDRack,
    ControllerTypeCarousel,
    ControllerTypePlate,
};

enum MotorTypeId : int32_t
{
    MotorTypeIllegal = -1,
    MotorTypeUnknown = 0,
    MotorTypeRadius = MotorIds::RadiusMotorId,
    MotorTypeTheta = MotorIds::ThetaMotorId,
    MotorTypeProbe = MotorIds::ProbeMotorId,
    MotorTypeReagent = MotorIds::ReagentMotorId,
    MotorTypeFocus = MotorIds::FocusMotorId,
    MotorTypeRack1 = MotorIds::Rack1MotorId,
    MotorTypeRack2 = MotorIds::Rack2MotorId,
};

enum MotorTypeRegAddr : uint32_t     // register addresses coresponding to the motor types
{
    MotorTypeRegAddrUnknown = 0,
    MotorTypeRadiusBaseRegAddr = RadiusMotorRegs,
    MotorTypeThetaBaseRegAddr = ThetaMotorRegs,
    MotorTypeProbeBaseRegAddr = ProbeMotorRegs,
    MotorTypeReagentBaseRegAddr = ReagentMotorRegs,
    MotorTypeFocusBaseRegAddr = FocusMotorRegs,
    MotorTypeRack1BaseRegAddr = Rack1MotorRegs,
    MotorTypeRack2BaseRegAddr = Rack2MotorRegs,
    MotorTypeObjectiveBaseRegAddr = ObjectiveMotorRegs,
};

enum MotorRegisterIndex
{
    CommandIdx = 0,
    CommandParamIdx,
    ErrorCodeIdx,    
	PositionIdx,
	HomedStatusIdx,
	ProbeStopPositionIdx,
    HomeDirectionIdx,
    MotorFullStepsPerRevIdx,
    UnitsPerRevIdx,
    GearheadRatioIdx,
    EncoderTicksPerRevIdx,
    DeadbandIdx,
    InvertedDirectionIdx,
    StepSizeIdx,
	ProfileRegsIdx,
	MaxSpeedIdx = ProfileRegsIdx,
    AccelerationIdx,
    DecelerationIdx,
	RunVoltageDivideIdx,
	AccVoltageDivideIdx,
	DecVoltageDivideIdx,
    MinSpeedIdx,    
    OverCurrentIdx,
    StallCurrentIdx,
    HoldVoltageDivideIdx,    
    // raw register addresses
    CONFIG_IDX,
    INT_SPEED_IDX,
    ST_SLP_IDX,
    FN_SLP_ACC_IDX,
    FN_SLP_DEC_IDX,
    //HomingStepSizeIdx,
    DelayAfterMoveIdx,
	MaxMoveRetriesIdx,
    ProbeRegsIdx,
    ProbeSpeed1Idx = ProbeRegsIdx,
    ProbeCurrent1Idx,
    ProbeAbovePositionIdx,
    ProbeSpeed2Idx,
    ProbeCurrent2Idx,
    ProbeRaiseIdx,    
    MaxTravelPosIdx,
};

enum MotorRegisterAddrOffsets
{
    CommandAddr = 0,
    CommandParamAddr = CommandAddr + 4,
    ErrorCodeAddr = CommandParamAddr + 4,
    PositionAddr = ErrorCodeAddr + 4,
	HomedStatusAddr = PositionAddr + 4,
	ProbeStopPositionAddr = HomedStatusAddr + 4,
    HomeDirectionAddr = ProbeStopPositionAddr + 4,
    MotorFullStepsPerRevAddr = HomeDirectionAddr + 4,
    UnitsPerRevAddr = MotorFullStepsPerRevAddr + 4,
    GearheadRatioAddr = UnitsPerRevAddr + 4,
    EncoderTicksPerRevAddr = GearheadRatioAddr + 4,
    DeadbandAddr = EncoderTicksPerRevAddr + 4,
    InvertedDirectionAddr = DeadbandAddr + 4,
    StepSizeAddr = InvertedDirectionAddr + 4,
	ProfileRegsAddr = StepSizeAddr + 4,
	MaxSpeedAddr = ProfileRegsAddr,
    AccelerationAddr = MaxSpeedAddr + 4,
    DecelerationAddr = AccelerationAddr + 4,
	RunVoltageDivideAddr = DecelerationAddr + 4,
	AccVoltageDivideAddr = RunVoltageDivideAddr + 4,
	DecVoltageDivideAddr = AccVoltageDivideAddr + 4,
    MinSpeedAddr = ProfileRegsAddr + sizeof(MotorProfileRegisters),
    OverCurrentAddr = MinSpeedAddr + 4,
    StallCurrentAddr = OverCurrentAddr + 4,
    HoldVoltageDivideAddr = StallCurrentAddr + 4,    
    // raw register addresses
    CONFIG_ADDR = HoldVoltageDivideAddr + 4,
    INT_SPEED_ADDR = CONFIG_ADDR + 4,
    ST_SLP_ADDR = INT_SPEED_ADDR + 4,
    FN_SLP_ACC_ADDR = ST_SLP_ADDR + 4,
    FN_SLP_DEC_ADDR = FN_SLP_ACC_ADDR + 4,
    //HomingStepSizeAddr = FN_SLP_DEC_ADDR + 4,
    DelayAfterMoveAddr = FN_SLP_DEC_ADDR + 4,
	MaxMoveRetriesAddr = DelayAfterMoveAddr + 4,
    ProbeRegsAddr = MaxMoveRetriesAddr + 4,
    ProbeSpeed1Addr = ProbeRegsAddr,
    ProbeCurrent1Addr = ProbeSpeed1Addr + 4,
    ProbeAbovePositionAddr = ProbeCurrent1Addr + 4,
    ProbeSpeed2Addr = ProbeAbovePositionAddr + 4,
    ProbeCurrent2Addr = ProbeSpeed2Addr + 4,
    ProbeRaiseAddr = ProbeCurrent2Addr + 4,    
    MaxTravelPosAddr = ProbeRegsAddr + sizeof(ProbeRegisters),    
};

struct MotorCfgParams
{
	uint32_t  ProbeStopPosition;
    uint32_t  HomeDirection;      // 0=Reverse, 1=Forward
    uint32_t  MotorFullStepsPerRev;
    uint32_t  UnitsPerRev;
    uint32_t  GearheadRatio;
    int32_t   EncoderTicksPerRev; // Encoder counts/rev (in quadrature)
    uint32_t  Deadband;           // encoder ticks
    uint32_t  InvertedDirection;  // 0=normal, 1=motor circuit is wired in reverse
    uint32_t  StepSize;           // FullStep divisor, 1=full, 2=1/2 .. 128=1/128
	MotorProfileRegisters profileRegs;
    uint32_t  MinSpeed;    
    uint32_t  OverCurrent;
    uint32_t  StallCurrent;
    uint32_t  HoldVoltageDivide;  // TVAL_HOLD   
     // raw registers
    uint32_t  CONFIG;
    uint32_t  INT_SPEED;          // L6470 only
    uint32_t  ST_SLP;             // T_FAST
    uint32_t  FN_SLP_ACC;         // TON_MIN
    uint32_t  FN_SLP_DEC;         // TOFF_MIN
	//uint32_t  HomingStepSize;
    uint32_t  DelayAfterMove;     // mSec
	uint32_t  MaxMoveRetries;
    ProbeRegisters ProbeRegs;
    uint32_t  MaxTravelPosition;    

	MotorCfgParams()
	{
		// Initialize at lowest values expected.
		ProbeStopPosition = 0;
		HomeDirection = 0;
		MotorFullStepsPerRev = 200;
		UnitsPerRev = 101600;
		GearheadRatio = 1;
		EncoderTicksPerRev = 1600;
		Deadband = 45;
		InvertedDirection = 0;
		StepSize = 8;
		profileRegs.MaxSpeed = 50000;
		profileRegs.Acceleration = 100000;
		profileRegs.Deceleration = 100000;
		profileRegs.RunVoltageDivide = 1500000;
		profileRegs.AccVoltageDivide = 1500000;
		profileRegs.DecVoltageDivide = 1500000;
		MinSpeed = 0;
		OverCurrent = 2625000;
		StallCurrent = 0;
		HoldVoltageDivide = 60000;
		CONFIG = 44696;
		INT_SPEED = 0;
		ST_SLP = 25;
		FN_SLP_ACC = 41;
		FN_SLP_DEC = 41;
		DelayAfterMove = 0;
		MaxMoveRetries = 3;
		ProbeRegs.ProbeSpeed1 = 0;
		ProbeRegs.ProbeCurrent1 = 0;
		ProbeRegs.ProbeAbovePosition = 0;
		ProbeRegs.ProbeSpeed2 = 0;
		ProbeRegs.ProbeCurrent2 = 0;
		ProbeRegs.ProbeRaise = 0;
		MaxTravelPosition = 0;
	}
} ;



const uint32_t  ThetaMotorGearTeeth = 16;
const uint32_t  CarouselGearTeeth = 64;
const double_t  DefaultGearRatio = ( CarouselGearTeeth / ThetaMotorGearTeeth );
const double_t  DefaultThetaMotorUnitsPerRev = 900;
const double_t  DefaultDegreesPerTube = 15.0;
const uint32_t  MaxThetaPosition = (uint32_t)( DefaultThetaMotorUnitsPerRev * DefaultGearRatio );
const uint32_t  DefaultThetaUnitsPerRev = MaxThetaPosition;

const int32_t   UnitsChange_0_1_mm = 1000;                      // the user unit value representing 0.1 mm of travel
const int32_t   UnitsChange_0_0_5_mm = 500;                     // the user unit value representing 0.05 mm of travel
const int32_t   UnitsChange_10_0_mm = 100000;                   // the user unit value representing 10.0 mm of travel

const int32_t   ThetaChange1Degree = 10;                        // the user units representing a real 1 degree change
const int32_t   RadiusChange_0_1_mm = UnitsChange_0_1_mm;       // the user unit value representing 0.1 mm of radius plate travel
const int32_t   ProbeChange_0_0_5_mm = UnitsChange_0_0_5_mm;    // the user unit value representing 0.05 mm of probe travel
const int32_t   ProbeChange_10_0_mm = UnitsChange_10_0_mm;      // the user unit value representing 10.0 mm of probe travel
const int32_t   FocusChange_0_4_um = 4;                         // the user units most closely representing 4 microns

const double_t  DefaultPlateThetaCalPos = ( (double) DefaultThetaUnitsPerRev / 4 );     // default theta calibration is at +90 degrees, 1/4 plate rotation...
const int32_t   DefaultThetaPositionTolerance = ThetaChange1Degree;     // 1 degree by default
const int32_t   DefaultRadiusPositionTolerance = RadiusChange_0_1_mm;   // 0.1mm by default
const int32_t   DefaultProbePositionTolerance = ProbeChange_0_0_5_mm;   // 0.05mm by default

const int32_t   DefaultCarouselRadiusPos = 830000;      // a rough offset
const int32_t   DefaultCarouselThetaBacklash = (uint32_t) ( 2.75 * ThetaChange1Degree );      // a compensation when the home offset ws derived by turning 'backwards'
const int32_t   DefaultPlateRadiusBacklash = 10000;
const int32_t   DefaultPlateThetaBacklash = 0;

const uint32_t  CenterPlateY = 315000;                  // position in user units of the center of the plate for the y axis (rows) (31.5mm from plate spec * 10000)
const uint32_t  CenterPlateX = 495000;                  // position in user units of the center of the plate for the x axis (cols) (49.5mm from plate spec * 10000)
const uint32_t  DefaultInterWellSpacing = 90000;        // distance between well centers (from spec) in user units ( 9.0mm from plate spec * 10000 )

const int32_t   ProbeMaxTravel = 470000;                // assume 47mm of movement and the unit scaling in tenth-microns
const int32_t   PlateTopMax = ProbeMaxTravel - 100000;	// approximate location of the lowest plate top surface, for 'above' and piercing calculations
const int32_t   RadiusMaxTravel = 1050000;              // assume 105mm of movement and the unit scaling in tenth-microns without hitting the end-stop
const int32_t   ReagentArmMaxTravel = 1150000;          // assume 115mm of movement and the unit scaling in tenth-microns
const int32_t   ReagentArmPurgePosition = -830000;      // assume that moving up to a position 83mm from the bottom is still in the containers... (relative move)
const int32_t   FocusMaxTravel = 140000;                // assume 14.0mm of movement and the unit scaling in tenth-microns
const int32_t   MaxLEDRackTravel = 500000;              // initial value should be 50 mm as conservative inter-machine/inter-rack max to prevent travel off linear bearing...

// motor wait and timeout intervals are specified in multiples of 100 milliseconds, e.g. 10 = 10 * 100 = 1000 mSec or 1 sec
const int32_t   MotorStartTimeout = 30;                 // allow 3 seconds by default
const int32_t   ThetaFullTimeout = 300;                 // allow 30 seconds by default for longest motor running time
const int32_t   ProbeBusyTimeout = 150;                 // allow 15 seconds by default for full stroke operations
const int32_t   RadiusFullTimeout = 200;                // allow 20 seconds by default for longest motor running time
const int32_t   ReagentBusyTimeout = 150;               // allow 15 seconds by default for longest motor running time
const int32_t   LedRackBusyTimeout = 250;               // allow 25 seconds by default for longest motor running time
const int32_t   FocusBusyTimeout = 1000;                // allow 100 seconds by default for longest motor running time

const int64_t   MotorChkSleepInterval = 25;             // timer millisecond delay during sleep while waiting for completion
const int32_t   MotorRegUpdateStdInterval = 5000;       // update the complete register set every 5 seconds (5000 milliseconds) during non-motor-busy periods
const int32_t   MotorStatusUpdateBusyInterval = 100;    // update the status every 100 milliseconds while doing motor waits
const int32_t   MotorWaitTimerStdInterval = 100;        // default to 100 milliseconds for normal motor-only waits
const int32_t   MotorWaitTimerTestInterval = 50;        // default to 50 milliseconds when performing other signal tests

const int32_t   DefaultPosUpdateStdInterval = 250;      // update the motor position every 250 milliseconds during non-motor-busy periods
const int32_t   ProbePosUpdateInterval = 250;           // motor position update interval for non-movement condition updates
const int32_t   ProbeStopPosUpdateInterval = 250;       // motor position update interval for non-movement condition updates
const int32_t   ThetaPosUpdateInterval = 250;           // motor position update interval for non-movement condition updates
const int32_t   RadiusPosUpdateInterval = 250;          // motor position update interval for non-movement condition updates
const int32_t   ReagentPosUpdateInterval = 2000;        // motor position update interval for non-movement condition updates
const int32_t   FocusPosUpdateInterval = 2000;          // motor position update interval for non-movement condition updates
const int32_t   LedRackPosUpdateInterval = 2000;        // motor position update interval for non-movement condition updates

const int32_t   PositionUpdateDelay = 2 * DefaultPosUpdateStdInterval;  // interval to sleep to ensure positions are reported correctly
const int32_t   CalibrationPosUpdateDelay = 2 * PositionUpdateDelay;    // interval to sleep after a calibration step to ensure positions are reported correctly

const uint32_t  MinEffectiveMotorCurrent = 32000;       // minmum motor current value guaranteed to be effective


#define REG_ADDR_VALID(addr)            ( ( (addr) >= RegisterIds::SwVersion ) && ( (addr) <= RegisterIds::LastRegs ) )
#define MASK_BIT(x)                     (1 << (x))

const uint16_t BoardStatusMotorBits =   ( MASK_BIT( BoardStatus::HostCommError )    |
                                          MASK_BIT( BoardStatus::ProbeMotorBusy )   |
                                          MASK_BIT( BoardStatus::RadiusMotorBusy )  |
                                          MASK_BIT( BoardStatus::ThetaMotorBusy )   |
                                          MASK_BIT( BoardStatus::FocusMotorBusy )   |
                                          MASK_BIT( BoardStatus::ReagentMotorBusy ) |
                                          MASK_BIT( BoardStatus::Rack1MotorBusy )   |
                                          MASK_BIT( BoardStatus::Rack2MotorBusy )   |
                                          MASK_BIT( BoardStatus::Error ) );

const uint16_t BoardStatusBusyBits =    ( MASK_BIT( BoardStatus::ProbeMotorBusy )   |
                                          MASK_BIT( BoardStatus::RadiusMotorBusy )  |
                                          MASK_BIT( BoardStatus::ThetaMotorBusy )   |
                                          MASK_BIT( BoardStatus::FocusMotorBusy )   |
                                          MASK_BIT( BoardStatus::ReagentMotorBusy ) |
                                          MASK_BIT( BoardStatus::Rack1MotorBusy )   |
                                          MASK_BIT( BoardStatus::Rack2MotorBusy )   |
                                          MASK_BIT( BoardStatus::ObjectiveMotorBusy ) );


const uint32_t  ReagentSignalMask =     ( MASK_BIT( SignalStatus::ReagentMotorHome )  |
                                          MASK_BIT( SignalStatus::ReagentMotorLimit ) |
                                          MASK_BIT( SignalStatus::ReagentDoorClosed ) |
                                          MASK_BIT( SignalStatus::ReagentPackInstalled ) );

const uint32_t  ReagentArmMask =        ( MASK_BIT( SignalStatus::ReagentMotorHome ) |
                                          MASK_BIT( SignalStatus::ReagentMotorLimit ) );


class DLL_CLASS MotorBase
{

public:
    MotorBase(std::shared_ptr<CBOService> pCBOService, MotorTypeId mtype = MotorTypeUnknown );
    ~MotorBase();

private:

    std::shared_ptr<CBOService> pCBOService_;

    boost::asio::deadline_timer                     regUpdTimer;
    boost::asio::deadline_timer                     posUpdTimer;
    
	// Configuration
    bool                updateConfig;
    t_pPTree            configTree;
    t_opPTree           paramNode;
    t_opPTree           motorNode;
    std::string         motorNodeName;
    std::string         configFile;
    std::string         cbiPort;

	// Initialization State
    bool                initComplete;
    bool                closing;

	// Current internal state records
    int32_t             positionTolerance;
    int32_t             probeStopPos;


	// Configuration Parameters
    int32_t             motorStartTimeout_msec;
    int32_t             motorBusyTimeout_msec;
	uint32_t            homeDirectionParam_;
	boost::optional<int32_t> maxPos;

    uint32_t                    regUpdateInterval_ms;
    boost::optional<uint32_t>   posUpdateInterval_ms;

    MotorTypeId         motorTypeId_;
    MotorTypeRegAddr    motorRegsAddr_;
    MotorRegisters      motorRegs_;             // this object is updated asynchronously by the update timer; don't use for writing to the controller...
    MotorRegisters      dfltMotorRegs_;
    
	MotorRegisterValues motorRegisterValuesCache_;

    MotorCfgParams      dfltParams_;
    

	std::function<void(bool, BoardStatus)> runMotorCmdCb_;
	std::function<void(uint32_t)> errorReportingCb_;
	
	
private:

    void                FormatMotorRegister( MotorRegisters * pRegs, std::string & regStr ) const;

	// Configuration input/output
	static void         ReadMotorParamsFromInfo( MotorCfgParams & pMotorCfg, t_opPTree paramNode );
    int32_t             WriteMotorConfig( void );
    int32_t             UpdateMotorConfig( MotorCfgParams * pMotorCfg, t_opPTree & motor_node ) const;
	
	// Internal state maintenance
	static void         UpdateParamsFromRegs( MotorCfgParams * pMotorCfg, MotorRegisters * pRegs );
	static void         UpdateRegsFromParams( MotorRegisters * pRegs, MotorCfgParams * pMotorCfg );
    void                InitDefaultParams(std::function<void(bool)> cb, MotorTypeId motorId, t_pPTree cfgTree, t_opPTree cfgNode, bool apply );

	// Register Mapping
	static uint32_t     GetMotorAddrForId( uint32_t motorId );
	static uint32_t     GetMotorIdForAddr( uint32_t motorAddr );
	static bool         MotorIdValid( uint32_t motorId );
	static bool         MotorRegAddrValid( MotorTypeId type, uint32_t regAddr );


	// Motor Position Polling
	void                UpdateMotorPositionStatus(boost::system::error_code error);
	void                SetMotorPositionUpdateInterval(boost::optional<uint32_t> interval);
    
	// Motor Register (full set) polling
    void                UpdateMotorRegisters(boost::system::error_code error );
    
	// Explicit Commands (non-polling)
	void				RdMotorPositionStatus( std::function<void( bool )> cb, const boost::system::error_code error );
	void                RdMotorRegisters();

	void				updateHost( std::function<void( bool )> cb, bool status ) const;

	// Common Error Reporting/handling
	bool                HandleCallbackStatus(ControllerBoardOperation::CallbackData_t cbData, bool regWrite = false );
	void                HandleRdCallbackError(ControllerBoardOperation::CallbackData_t cbData);
	void                HandleConnectOrAddressError( uint32_t addr, bool isMotorAddr, bool isAddrNeeded, void * pData, std::string & logStr );

	// motor register data
	void                WrtDataValue( uint32_t addr, uint32_t value, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	void                WrtDataBlock( std::function<void( bool )> cb, uint32_t addr, void * dataAddr, uint32_t blklen );

	// motor status
	bool                HomeLimitSwitch( void );
	bool                DownLimitSwitch( void );

	// CBO Write Operation
	boost::optional<uint16_t> executeWriteOperation(const ControllerBoardOperation::Operation& op, uint32_t timeout_secs, std::function<void(ControllerBoardOperation::CallbackData_t)> onCompleteCallback);

	// State Interpretation (static)
	static bool         MotorBusy( BoardStatus brdstatus, uint32_t mtype );

	static bool         MotorStatusError( ErrorStatus estatus, uint32_t mtype );
	static bool         MotorHomeSwitch( SignalStatus estatus, uint32_t mtype );
	static bool         MotorLimitSwitch( SignalStatus estatus, uint32_t mtype );
	static void         GetMotorTypeAsString(const MotorTypeId mType, std::string & typeStr );
	static void         GetMotorNameAsString(const MotorTypeId mtype, std::string & nameStr);
	static bool         PosInTolerance (int32_t currentPos, int32_t tgtPos, int32_t tolerance);
    
public:
	
	void                Init(std::function<void(bool)> callback, t_pPTree cfgTree, t_opPTree cfgNode, bool apply = false);
	void                Quit( void );
	bool                PosAtTgt (int32_t currentPos, int32_t tgtPos, int32_t tolerance = 0, bool useDeadband = false) const;
	void                SetPosTolerance( int32_t tolerance );
	void                SetMotorTimeouts( int32_t startTimeout_msec, int32_t busyTimeout_msec );      // set the externally configurable motor startup timeout value

	// Motor Position Polling
	void                ResumePositionUpdates( uint32_t interval_ms ){ SetMotorPositionUpdateInterval( interval_ms ); };
	void                PausePositionUpdates(){ SetMotorPositionUpdateInterval( boost::none ); };

	// Error Reporting/hadling
	void                registerToErrorReportCb( std::function<void( uint32_t )> cb );
	void                triggerErrorReportCb( uint32_t errorCode );

	// motor configuration
	static bool         CfgTreeValid( t_pPTree cfgtree, std::string controllerName );
	MotorTypeId         SetMotorType(MotorTypeId mType );
	MotorTypeId         GetMotorType( void ) const;
	std::string &       GetMotorTypeAsString(const MotorTypeId mType ) const;
	std::string &       GetMotorNameAsString(const MotorTypeId mtype) const;
	std::string         GetModuleName() const;
	
	// motor status
	bool                IsHome( void );
	bool                IsDown( void );
	bool                IsBusy( void ) const;
	bool                GetMotorRegs( MotorRegisters & motorRegs ) const;				// full motor register set
	bool                ReadMotorRegs( MotorRegisters & motorRegs );
	

	// TODO : Make these private
	// motor commands
	void                ClearErrors(std::function<void(bool)> callback, bool clr_host_comm = false );

	
	// motor properties/parameters
	void                SetAllParams( MotorCfgParams * pMotorCfg = nullptr, MotorRegisters * pRegs = nullptr );
	void                ApplyAllParams( void );
	void                UpdateDefaultParams( MotorCfgParams * pMotorCfg );
	void                ApplyDefaultParams( void );
	uint32_t            SetHomeDirection( uint32_t homeDir, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);		// only valid for carousel (theta) motor
	uint32_t            GetHomeDirection( void ) const;																										// only valid for carousel (theta) motor
	uint32_t            SetStepsPerRev( uint32_t count, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetStepsPerRev( void ) const;
	uint32_t            SetUnitsPerRev( uint32_t count, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetUnitsPerRev( void ) const;
	uint32_t            SetGearRatio( uint32_t ratio, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetGearRatio( void ) const;
	uint32_t            SetTicksPerRev( uint32_t count, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetTicksPerRev( void ) const;
	uint32_t            SetDeadband( uint32_t units, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetDeadband( void ) const;
	uint32_t            SetInvertedDirection( uint32_t reversed, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetInvertedDirection( void ) const;
	uint32_t            SetStepSize( uint32_t size, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetStepSize( void ) const;
	uint32_t            SetAccel( uint32_t accel, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetAccel( void ) const;
	uint32_t            SetDecel( uint32_t decel, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetDecel( void ) const;
	uint32_t            SetMinSpeed( uint32_t minSpeed, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetMinSpeed( void ) const;
	uint32_t            SetMaxSpeed(uint32_t maxSpeed, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetMaxSpeed( void ) const;
	uint32_t            SetOverCurrent( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetOverCurrent( void ) const;
	uint32_t            SetStallCurrent( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetStallCurrent( void ) const;
	uint32_t            SetHoldVoltDivide( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetHoldVoltageDivide( void ) const;
	uint32_t            SetRunVoltDivide( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetRunVoltageDivide( void ) const;
	uint32_t            SetAccelVoltDivide( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetAccelVoltageDivide( void ) const;
	uint32_t            SetDecelVoltDivide( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetDecelVoltageDivide( void ) const;
	uint32_t            SetConfig( uint32_t cconfig, bool apply = false );
	uint32_t            GetConfig( void ) const;
	uint32_t            SetIntSpd( uint32_t cconfig, bool apply = false );
	uint32_t            GetIntSpd( void ) const;
	uint32_t            SetStSlp( uint32_t slpCfg, bool apply = false );
	uint32_t            GetStSlp( void ) const;
	uint32_t            SetFnSlpAccel( uint32_t cconfig, bool apply = false );
	uint32_t            GetFnSlpAccel( void ) const;
	uint32_t            SetFnSlpDecel( uint32_t cconfig, bool apply = false );
	uint32_t            GetFnSlpdecel( void ) const;    
	uint32_t            SetDelayAfterMove( uint32_t delay, bool apply = false );
	uint32_t            GetDelayAfterMove( void ) const;
	void                SetMotorProfileRegisters( std::function<void( bool )> cb, MotorProfileRegisters profileRegisters, bool apply = false );
	void                GetMotorProfileRegisters( MotorProfileRegisters& profileRegisters ) const;

	// write-only properties
	uint32_t            AdjustProbeSpeed1( uint32_t speed, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            AdjustProbeSpeed1Current( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            AdjustProbeSpeed2( uint32_t speed, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            AdjustProbeSpeed2Current( uint32_t current, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	int32_t             AdjustProbeAbovePosition( int32_t pos, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	int32_t             GetProbeAbovePosition( void ) const;                                  // this value is used to monitor for the speed to be used in manual movement mode
	int32_t             AdjustProbeRaise( int32_t distance, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	int32_t             GetProbeRaise( void ) const;                                          // this value is used to check if the probe is at its down position...
	int32_t             SetDefaultProbeStop( int32_t stopPos );
	int32_t             AdjustMaxTravel( int32_t maxPos, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	int32_t             GetMaxTravel( void ) const;                                           // this value is used to set the limit of travel for probe and reagent?
	uint32_t            AdjustMaxRetries( uint32_t retries, bool apply = false, boost::optional<std::function<void(bool)>> opCallback = boost::none);
	uint32_t            GetMaxRetries( void ) const;

	// read-only properties
	uint32_t            GetErrorCode(void) const;
	int32_t             GetPosition( void ) const;
	void                ReadPosition (std::function<void (bool)> cb, const boost::system::error_code error);
	int32_t             GetProbeStop( void ) const;	
	bool                GetHomeStatus(void) const;

	// motor commands - asynchronous
	void                SetMaxMotorPositionValue(boost::optional<int32_t> maxPos);
	void				InitializeMotor( std::function<void( bool )> cb, uint32_t timeout_Sec, MotorRegisters srcRegs );
	void				UpdateMotorParams(std::function<void(bool)> cb);					// used to apply motor register parameter changes that have been sent to the controller board to allow updates to non-motion parameters
	void				UpdateMotorParamsFromDefaultRegs(std::function<void(bool)> cb);		// used to and and update all motor register parameters to allow updates to non-motion parameters
	void				ReinitializeMotorDefaults(std::function<void(bool)> cb, MotorRegisters srcRegs );
	void                Home( std::function<void( bool )> cb, uint32_t timeout_Sec );
	void                Stop (std::function<void (bool)> cb, bool hardStop, uint32_t timeout_Sec);
	void                MarkPosAsZero(std::function<void(bool)> cb, uint32_t timeout_Sec);
	void                SetPosition(std::function<void(bool)> cb, int32_t pos, uint32_t timeout_Sec);
	void                MovePosRelative(std::function<void(bool)> cb, int32_t pos, uint32_t timeout_Sec);
	void                ProbeDown(std::function<void(bool)> cb, uint32_t timeout_Sec);
	void                ProbeUp(std::function<void(bool)> cb, uint32_t timeout_Sec);
	void                Enable(std::function<void(bool)> cb, bool currentOn, uint32_t timeout_Sec);
	void                GotoTravelLimit(std::function<void(bool)> cb, uint32_t timeout_Sec);

};

class MotorOperation : public ControllerBoardOperation::Operation
{
public:
	enum eMotorCommand
	{
		//SEE `MotorCommands` in Registers.hpp
	};

	MotorOperation (RegisterIds regForOperation)
	{
		Operation::Initialize(&motorRegs_);	// the data source for write operations
		// initialize the controller board register address for the operations
		regAddr_ = regForOperation;
	}

protected:
	MotorRegisters motorRegs_;
};

class MotorHomeOperation : public MotorOperation
{
public:
	MotorHomeOperation( RegisterIds regForOperation ) : MotorOperation( regForOperation )
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_GotoHome);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class MotorMarkAsZeroOperation : public MotorOperation
{
public:
	MotorMarkAsZeroOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_MarkPositionAsZero);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class MotorGoToPositionOperation : public MotorOperation
{
public:
	MotorGoToPositionOperation( RegisterIds regForOperation, int32_t position) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_GotoPosition);
		motorRegs_.CommandParam = static_cast<uint32_t>(position);
		lengthInBytes_ = 2 * sizeof(uint32_t);
	}
};

class MotorRelativeMoveOperation : public MotorOperation
{
public:
	MotorRelativeMoveOperation (RegisterIds regForOperation, int32_t position) : MotorOperation (regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_MoveRelative);
		motorRegs_.CommandParam = static_cast<uint32_t>(position);
		lengthInBytes_ = 2 * sizeof (uint32_t);
	}
};

class MotorClearErrorOperation : public MotorOperation
{
public:
	MotorClearErrorOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_ClearErrorCode);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class MotorInitializeOperation : public MotorOperation
{
public:
	MotorInitializeOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_Initialize);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class MotorInitializeFullRegisterOperation : public MotorOperation
{
public:
	MotorInitializeFullRegisterOperation(RegisterIds regForOperation, MotorRegisters newRegs) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		// copy the data for the write operation to the expected source location
//		motorRegs_ = newRegs;
		memcpy (&motorRegs_, &newRegs, sizeof(MotorRegisters));

		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_Initialize);
		motorRegs_.CommandParam = 0;
		motorRegs_.ErrorCode = 0;
		motorRegs_.Position = 0;
		motorRegs_.ProbeStopPosition = 0;
		lengthInBytes_ = sizeof(MotorRegisters);
	}
};

class MotorMoveRelativeOperation : public MotorOperation
{
public:
	MotorMoveRelativeOperation( RegisterIds regForOperation, int32_t distance) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_MoveRelative);
		motorRegs_.CommandParam = static_cast<uint32_t>(distance);
		lengthInBytes_ = 2 * sizeof(uint32_t);
	}
};

#if(0)		// currently not used
class MotorFreeRunOperation : public MotorOperation
{
public:
	MotorFreeRunOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_FreeRun);
		lengthInBytes_ = sizeof(uint32_t);
	}
};
#endif

class MotorEnableOperation : public MotorOperation
{
public:
	MotorEnableOperation( RegisterIds regForOperation, bool enable) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_Enable);
		motorRegs_.CommandParam = enable ? 1 : 0;
		lengthInBytes_ = 2 * sizeof(uint32_t);
	}
};

class MotorProbeDownOperation : public MotorOperation
{
public:
	MotorProbeDownOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_ProbeDown);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

class MotorGoToTravelLimitOperation : public MotorOperation
{
public:
	MotorGoToTravelLimitOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_GotoTravelLimit);
		lengthInBytes_ = sizeof(uint32_t);
	}
};

#if(0)		// currently not used
class MotorResetOperation : public MotorOperation
{
public:
	MotorResetOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = WriteMode;
		regData_.Command = static_cast<uint32_t>(MotorCmd_Reset);
		lengthInBytes_ = sizeof(uint32_t);
	}
};
#endif

class MotorStopOperation : public MotorOperation
{
public:
	MotorStopOperation( RegisterIds regForOperation, bool hardStop) : MotorOperation(regForOperation)
	{
		mode_ = OverridingWriteMode;
		motorRegs_.Command = static_cast<uint32_t>(MotorCmd_Stop);
		motorRegs_.CommandParam = hardStop ? 1 : 0;
		lengthInBytes_ = 2 * sizeof(uint32_t);
	}
};

class MotorReadFullRegisterOperation : public MotorOperation
{
public:
	MotorReadFullRegisterOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = ReadMode;
		lengthInBytes_ = sizeof(MotorRegisters);
	}
};

// this writes the registers as a data block, excludintg the command and parameter registers...
class MotorWriteFullRegisterOperation : public MotorOperation
{
public:
	MotorWriteFullRegisterOperation( RegisterIds regForOperation, MotorRegisters srcRegs ) : MotorOperation( regForOperation )
	{
		mode_ = WriteMode;
		// copy the data for the write operation to the expected source location
//		motorRegs_ = srcRegs;
		memcpy( &motorRegs_, &srcRegs, sizeof( MotorRegisters ) );
		// clear unused registers
		motorRegs_.Command = 0;
		motorRegs_.CommandParam = 0;
		motorRegs_.ErrorCode = 0;
		motorRegs_.Position = 0;
		motorRegs_.ProbeStopPosition = 0;
		// calculate the data source address
		regData_ = (void *)(&motorRegs_ + ( 2 * sizeof( uint32_t ) ) );
		// adjust destination register block start address to point beyond the command and parameter registers;
		// necessary to avoid busy-bit lock on multiple motor write operations
		regAddr_ += 2 * sizeof( uint32_t );
		lengthInBytes_ = sizeof( MotorRegisters ) - ( 2 * sizeof( uint32_t ) );
	}
};

class MotorWriteRegisterValueOperation : public MotorOperation
{
public:
	// NOTE: for data block operations that are not aligned with the base address of a motor,
	// the destination address for the write operation must be calculated before the operation
	// object is instantiated, and passed to the object at the time of instantiation 
	MotorWriteRegisterValueOperation( RegisterIds regForOperation, uint32_t regValue ) : MotorOperation( regForOperation )
	{
		mode_ = WriteMode;
		// Using motorRegs as a container to ensure that the value to be written is not lost
		// due to going out of scope.
		motorRegs_.CommandParam = regValue;
		regData_ = &motorRegs_.CommandParam;
		lengthInBytes_ = sizeof( uint32_t );
	}
};

class MotorWriteRegisterBlockOperation : public MotorOperation
{
public:
	MotorWriteRegisterBlockOperation( RegisterIds regForOperation, void * dataAddr, uint32_t blklen ) : MotorOperation( regForOperation )
	{
		mode_ = WriteMode;
		regData_ = dataAddr;
		lengthInBytes_ = blklen;
	}
};

class MotorWriteProfileRegistersOperation : public MotorOperation
{
public:
	MotorWriteProfileRegistersOperation( RegisterIds regForOperation, void * dataAddr ) : MotorOperation( regForOperation )
	{
		mode_ = WriteMode;
		regData_ = dataAddr;
		lengthInBytes_ = sizeof(MotorProfileRegisters);
	}
};

#if(0)		// currently not used
class MotorReadProfileRegistersOperation : public MotorOperation
{
public:
	MotorReadProfileRegistersOperation( RegisterIds regForOperation, void * dataAddr ) : MotorOperation( regForOperation )
	{
		mode_ = WriteMode;
		this->regAddr_ += ProfileRegsAddr;
		lengthInBytes_ = sizeof( MotorProfileRegisters );
	}
};
#endif

class MotorReadStatusOperation : public MotorOperation
{
public:
	MotorReadStatusOperation( RegisterIds regForOperation) : MotorOperation(regForOperation)
	{
		mode_ = ReadMode;
		this->regAddr_ += ErrorCodeAddr;
		lengthInBytes_ = sizeof(MotorRegisterValues);
	}
};