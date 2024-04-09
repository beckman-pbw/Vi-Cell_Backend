#pragma once

#include <stdint.h>
#include <sstream>

#include "HwConditions.hpp"

// This is copy of the Registers.hpp from the *Controller* repository.
// It was made to eliminate the need to include K70 include files.
//
// !!! WARNING - if changes are made to these definitions Registers.hpp in the *Controller* repository,
// !!! WARNING - this file must also be updated.
//

//================================================================

#define SWVER_REG_SIZE          4
#define ERR_STATUS_REG_SIZE     4
#define HOST_COMM_REG_SIZE      4
#define VOLTMON_BITMAP_SIZE     4
#define VOLT_MON_REG_SIZE       128
#define MOTOR_REG_SIZE          152
#define FAN_REG_SIZE            16
#define TOGGLE_CNTL_REG_SIZE    16      // replaces VALVE_REG_SIZE...
#define CAMERA_REGS_SIZE        32
#define BUBBLE_REGS_SIZE        40
#define CRYPTO_REG_SIZE         100
#define RFREADER_REG_SIZE       8
#define REAGENT_REG_SIZE        1600
#define SYRINGE_REG_SIZE        132
#define PERSISTENT_MEM_REG_SIZE 280
#define FWUPDATE_REG_SIZE       96


#define SIGNALS_REG_SIZE    4
#define UART_REG_SIZE       1048
#define TOTAL_REGS_SIZE     0x2000      // 8192
#define LED_REG_SIZE        48
#define DIODE_REG_SIZE      32

// These IDs for motors used to report to SystemErrors as well, so should be always matching to instrument_error::motion_motor_instances 
enum MotorIds : uint32_t
{
	ProbeMotorId = 1,
	RadiusMotorId,			// sample slide
	ThetaMotorId,			// sample rotate
	FocusMotorId,
	ReagentMotorId,			// reagent pierce
	Rack1MotorId,
	Rack2MotorId,
	ObjectiveMotorId
};


enum MotorDirection
{
	Reverse = 0,
	Forward = 1
};

enum MotorStopType
{
	SoftStop = 0,
	HardStop = 1
};

enum MotorCommands
{
	MotorCmd_GotoHome = 1,
	MotorCmd_MarkPositionAsZero = 2,
	MotorCmd_GotoPosition = 3, // CommandParam: int32 (Config units)
	MotorCmd_ClearErrorCode = 4,
	MotorCmd_Initialize = 5,
	MotorCmd_MoveRelative = 6, // CommandParam: int32 (Config units)
	//MotorCmd_FreeRun = 7, // CommandParam: uint32 (run duration in ms)
	MotorCmd_Enable = 8, // CommandParam: 0/1 = disable/enable, holding current= off/on
	MotorCmd_ProbeDown = 9,
	MotorCmd_GotoTravelLimit = 10,
	//MotorCmd_Reset = 11,
	MotorCmd_Stop = -1
};

enum LEDCommands
{
	LEDCmd_TurnOn = 1,
	LEDCmd_TurnOff = 2,
	LEDCmd_Pulse = 3,
	LEDCmd_Calibrate = 4,
};

enum RS232Commands
{
	RS232Cmd_Transmit = 1,
	RS232Cmd_AckTheRcvBuf = 2,
	RS232Cmd_AckTheErrorCode = 3,
	RS232Cmd_FlushRcvBuf = 4,
};

enum CameraCommands
{
	CameraCmd_TakePictures = 1,
};

enum BubbleDetectorCommands
{
	BubbleDetectorCmd_Enable = 1,
	BubbleDetectorCmd_Disable = 2,
	BubbleDetectorCmd_CalibrateAir = 3,
	BubbleDetectorCmd_CalibrateFluid = 4,
};

enum SyringeCommands
{
	SyringeCmd_Init = 1,
	SyringeCmd_MoveSyringe = 2,
	SyringeCmd_MoveValve = 3,
	SyringeCmd_SendDirect = 4,
};

//================================================================

struct ProbeRegisters
{
	// exclusively for ProbeDown command
	uint32_t  ProbeSpeed1;        // Speed for the initial move
	uint32_t  ProbeCurrent1;      // Current for the initial move
	uint32_t  ProbeAbovePosition; // Probe position where the probe speed changes from ProbeSpeed1 to ProbeSpeed2.
	uint32_t  ProbeSpeed2;        // Speed for the stalling move
	uint32_t  ProbeCurrent2;      // Current for the stalling move
	uint32_t  ProbeRaise;         // Distance to move probe up from the bottom of the sample container.
	//uint32_t  ProbeMaxTravelPosition; // Maximum position of the bottom + some extra (to avoid deacceleration)  (not used)

	ProbeRegisters()
		:	// initialize the internal default parameters to the lowest expected values...
		ProbeSpeed1( 0 ),
		ProbeCurrent1( 0 ),
		ProbeAbovePosition( 0 ),
		ProbeSpeed2( 0 ),
		ProbeCurrent2( 0 ),
		ProbeRaise( 0 )
	{}
};


//================================================================

struct MotorProfileRegisters
{
	uint32_t  MaxSpeed;
	uint32_t  Acceleration;
	uint32_t  Deceleration;
	uint32_t  RunVoltageDivide;   // TVAL_RUN
	uint32_t  AccVoltageDivide;   // TVAL_ACC
	uint32_t  DecVoltageDivide;   // TVAL_DEC

	MotorProfileRegisters()
		:	// initialize the internal default parameters to the lowest expected values...
		MaxSpeed( 50000 ),
		Acceleration( 100000 ),
		Deceleration( 100000 ),
		RunVoltageDivide( 1500000 ),
		AccVoltageDivide( 1500000 ),
		DecVoltageDivide( 1500000 )
	{}

};
typedef struct MotorRegisterValues_type
{
	uint32_t  ErrorCode;
	int32_t   Position;
	uint32_t  HomedStatus;
	uint32_t  ProbeStopPosition;  // The position of the Probe when it was at bottom.
} MotorRegisterValues;

typedef struct MotorRegisters_type
{
	uint32_t  Command; // MotorCommands
	uint32_t  CommandParam;
	uint32_t  ErrorCode;
	int32_t   Position;
	uint32_t  HomedStatus;
	uint32_t  ProbeStopPosition;  // The position of the Probe when it was at bottom.
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
	uint32_t  DelayAfterMove;     // mSec
	uint32_t  MaxMoveRetries;
	ProbeRegisters ProbeRegs;
	uint32_t  MaxTravelPosition;
} MotorRegisters;

//================================================================

typedef struct FanRegisters_type
{
	uint32_t  ErrorCode;
	uint32_t  Speed;
	uint32_t  Power;
	uint32_t  reserved;
} FanRegisters;

//================================================================

typedef struct LEDRegisters_type
{
	uint32_t  Command; // LEDCommands
	uint32_t  ErrorCode;
	uint32_t  SWITCH;         // most recent measurement
	uint32_t  ISENSE;         // most recent measurement
	uint32_t  Power;          // raw ISET DtoA value for the ON state
	uint32_t  ADJ;			  // ???
	uint32_t  SimmerCurrent;  // raw ISET DtoA value for the simmer state
	uint32_t  MAXON;
	uint32_t  MINOFF;
	uint32_t  LTCD;
	uint32_t  CTLD;
	uint32_t  FeedBackPhotodiode;
} LEDRegisters;

//================================================================

typedef struct PhotoDiodeRegisters_type
{
	uint32_t  ErrorCode;
	uint32_t  VREF;
	uint32_t  SamplingPeriod;
	uint32_t  NumMeasurements;
	uint32_t  IntegrationDuration;
	uint32_t  Integration;
	uint32_t  FirstMeasurement;
	uint32_t  FinalMeasurement;
} PhotoDiodeRegisters;

//================================================================

typedef struct ToggleRegisters_type
{
	uint32_t  ErrorCode;
	uint32_t  State;      // 0=off,  1=on,  2=fault
	uint32_t  reserved1;
	uint32_t  reserved2;
} ToggleRegisters;

//================================================================

typedef struct BubbleDetectorRegisters_type
{
	uint32_t  Command;
	uint32_t  ErrorCode;
	uint32_t  State;
	uint32_t  NumEdges;
	uint32_t  BubbleDurationMin;
	uint32_t  BubbleDurationMax;
	uint32_t  TIA_OFFSET;
	uint32_t  EMITTER_CURRENT;
	uint32_t  TIA_OUT;
	uint32_t  reserved;
} BubbleDetectorRegisters;

//================================================================

typedef struct CameraRegisters_type
{
	uint32_t  Command;
	uint32_t  ErrorCode;
	uint32_t  LEDs;
	uint32_t  CET;
	uint32_t  FPS;
	uint32_t  TNF;
	uint32_t  reserved1;
	uint32_t  reserved2;
} CameraRegisters;

//================================================================

typedef struct CryptoRegisters_type
{
	uint32_t  Command;
	uint32_t  ErrorCode;
	uint32_t  Counter1;
	uint32_t  Counter2;
	uint32_t  Counter3;
	uint32_t  Counter4;
	uint8_t   RootKey1[8];
	uint8_t   RootKey2[8];
	uint8_t   RootKey3[8];
	uint8_t   RootKey4[8];
	uint8_t   AuthenticationSecret[16];
	uint8_t   SN[16];
	uint32_t  reserved1;
	uint32_t  reserved2;
	uint32_t  reserved3;
} CryptoRegisters;

//================================================================

typedef union TagUID_Type
{
	struct
	{
		uint8_t   TagConst;
		uint8_t   ManufCode;
		uint8_t   ICODE_Type;
		uint8_t   TagSN[5];
	};
	uint8_t    UID[8];
} TagUID;

//===============================================================

typedef struct TagStatus_Type
{
	uint8_t   TagSN[5];
	uint8_t   AuthStatus;
	uint8_t   ValidationStatus;
	uint8_t   ProgramStatus;
} TagStatus;

// =================================================================

typedef struct RfTagAppdataRegisters_type
{
	TagStatus Status;
	uint8_t   ParamMap[252u];   // 252 bytes of EEPROM user space available in RFID tag
} RfAppdataRegisters;

//================================================================

static const uint8_t MaxRfTags = 6u;
typedef struct ReagentRegisters_type
{
	uint32_t  Command;
	uint32_t  CommandParam;
	uint32_t  ErrorCode;
	uint32_t  ReaderSN;
	uint32_t  ReaderFWVersion;
	uint32_t  ReaderHWVersion;
	uint32_t  ReaderID;
	uint32_t  NumOfTags;
	uint32_t	Reserved[2];
	RfAppdataRegisters Tags[MaxRfTags];
} ReagentRegisters;

//================================================================
typedef struct SyringeRegisters_type
{
	uint32_t  Command;
	uint32_t  CommandParam;
	uint32_t  CommandParam2;
	uint32_t  ErrorCode;
	uint32_t  SyringeVolume;
	uint32_t  MaxPosition;
	uint32_t  SyringePosition;
	uint32_t  ValvePosition;
	char      PumpFirmwareVersion[24u];
	uint32_t  Reserved;
	char      Text[72u];
} SyringeRegisters;

//================================================================
typedef struct PersistentMemoryRegisters_type
{
	uint32_t  Command;
	uint32_t  Address;
	uint32_t  Length;
	uint32_t  ErrorCode;
	uint32_t  ChipSize;
	uint32_t  SectorSize;
	char      Data[256u];
} PersistentMemoryRegisters;

enum PersistentMemoryRegisterOffsets
{
	pmr_Command = 0,
	pmr_Address = 4,
	pmr_Length = 8,
	pmr_ErrorCode = 12,
	pmr_ChipSize = 16,
	pmr_SectorSize = 20,
	pmr_Data = 24
};

enum PeristentMemoryCommands
{
	ReadEEPROM = 1,
	WriteEEPROM = 2,
	EraseEEPROM = 3,
};

//================================================================
enum BootloaderCommands
{
	Reboot = 1,
	EraseFlash = 2,
	DownloadFirmware = 3,
	LaunchApplication = 4,
	CalculateCRC = 5
};

typedef struct FwUpdateRegisters_type
{
	uint32_t  Command;
	uint32_t  ErrorCode;
	uint32_t  AppImageCRC;
	uint32_t  ParamLen;
	char      CmdParam[80u];
} FwUpdateRegisters;

//================================================================
#define XmtBufLen   512     // these are the default buffer lengths, but the macro allows allocating a variable buffer length
#define RcvBufLen   512

#define RS232_REGS(id, rcvBufLen, xmtBufLen) typedef volatile struct RS232_REGS ## id ## _type { \
  uint32_t  Command; /* 1=transmit the TransmitBuffer, 2=ack the ReceiveBuffer*/ \
  uint32_t  CommandParam; \
  uint32_t  ErrorCode; \
  uint32_t  ReceiveBufferLen; \
  uint32_t  TransmitBufferLen; \
  uint8_t   TransmitBuffer [ xmtBufLen ]; \
  uint32_t  TerminationChar; \
  uint8_t   ReceiveBuffer [ rcvBufLen ];  } RS232_REGS ## id;

//================================================================

enum RegisterIds : uint32_t
{
	SwVersion = 0, // 1..n
	ErrorStatus1 = SwVersion + SWVER_REG_SIZE,						// bitmask identifying all non-zero ErrorCodes
	ErrorStatus2 = ErrorStatus1 + ERR_STATUS_REG_SIZE,				// bitmask identifying all non-zero ErrorCodes
	HostCommError = ErrorStatus2 + ERR_STATUS_REG_SIZE,				// System monitor voltages [0..31]
	Signals = HostCommError + HOST_COMM_REG_SIZE,
	MonitorVoltageBitMap = Signals + SIGNALS_REG_SIZE,				// bitmask of measurements that are out of limits

	MonitorVoltageMeasurements = MonitorVoltageBitMap + VOLTMON_BITMAP_SIZE,    // System monitor voltages [0..31]

	Motor1Regs = MonitorVoltageMeasurements + VOLT_MON_REG_SIZE,	// MotorRegisters
	Motor2Regs = Motor1Regs + MOTOR_REG_SIZE,						// MotorRegisters
	Motor3Regs = Motor2Regs + MOTOR_REG_SIZE,						// MotorRegisters
	Motor4Regs = Motor3Regs + MOTOR_REG_SIZE,						// MotorRegisters
	Motor5Regs = Motor4Regs + MOTOR_REG_SIZE,						// MotorRegisters
	Motor6Regs = Motor5Regs + MOTOR_REG_SIZE,						// MotorRegisters
	Motor7Regs = Motor6Regs + MOTOR_REG_SIZE,						// MotorRegisters
	Motor8Regs = Motor7Regs + MOTOR_REG_SIZE,						// MotorRegisters
	ProbeMotorRegs = Motor1Regs,									// MotorRegisters
	RadiusMotorRegs = Motor2Regs,									// MotorRegisters
	ThetaMotorRegs = Motor3Regs,									// MotorRegisters
	FocusMotorRegs = Motor4Regs,									// MotorRegisters
	ReagentMotorRegs = Motor5Regs,									// MotorRegisters
	Rack1MotorRegs = Motor6Regs,									// MotorRegisters
	Rack2MotorRegs = Motor7Regs,									// MotorRegisters
	ObjectiveMotorRegs = Motor8Regs,								// MotorRegisters
	LastMotorRegs = Motor8Regs,										// MotorRegisters

	Fan1Regs = LastMotorRegs + MOTOR_REG_SIZE,						// FanRegisters
	Fan2Regs = Fan1Regs + FAN_REG_SIZE,								// FanRegisters
	LED1Regs = Fan2Regs + FAN_REG_SIZE,								// LEDRegisters (BrightField)
	LED2Regs = LED1Regs + LED_REG_SIZE,								// LEDRegisters (Blue)
	LED3Regs = LED2Regs + LED_REG_SIZE,								// LEDRegisters (UV)
	LED4Regs = LED3Regs + LED_REG_SIZE,								// LEDRegisters (Red)
	LED5Regs = LED4Regs + LED_REG_SIZE,								// LEDRegisters (Green)
	LED6Regs = LED5Regs + LED_REG_SIZE,								// LEDRegisters (undefined)
	PhotoDiode1Regs = LED6Regs + LED_REG_SIZE,						// PhotoDiodeRegisters
	PhotoDiode2Regs = PhotoDiode1Regs + DIODE_REG_SIZE,				// PhotoDiodeRegisters
	PhotoDiode3Regs = PhotoDiode2Regs + DIODE_REG_SIZE,				// PhotoDiodeRegisters
	PhotoDiode4Regs = PhotoDiode3Regs + DIODE_REG_SIZE,				// PhotoDiodeRegisters
	PhotoDiode5Regs = PhotoDiode4Regs + DIODE_REG_SIZE,				// PhotoDiodeRegisters
	PhotoDiode6Regs = PhotoDiode5Regs + DIODE_REG_SIZE,				// PhotoDiodeRegisters
	DoorLatchRegs = PhotoDiode6Regs + DIODE_REG_SIZE,				// Toggle control hardware; on/off control
	Toggle2Regs = DoorLatchRegs + TOGGLE_CNTL_REG_SIZE,				// Toggle control hardware; on/off control
	Toggle3Regs = Toggle2Regs + TOGGLE_CNTL_REG_SIZE,				// Toggle control hardware; on/off control
	Toggle4Regs = Toggle3Regs + TOGGLE_CNTL_REG_SIZE,				// Toggle control hardware; on/off control
	UART0Regs = Toggle4Regs + TOGGLE_CNTL_REG_SIZE,					// RS232Registers;
	UART3Regs = UART0Regs + UART_REG_SIZE,							// RS232Registers
	UART5Regs = UART3Regs + UART_REG_SIZE,							// RS232Registers
	Camera1Regs = UART5Regs + UART_REG_SIZE,						// CameraRegisters
	Camera2Regs = Camera1Regs + CAMERA_REGS_SIZE,					// CameraRegisters
	Camera3Regs = Camera2Regs + CAMERA_REGS_SIZE,					// CameraRegisters
	BubbleDetector1Regs = Camera3Regs + CAMERA_REGS_SIZE,			// BubbleDetectorRegisters
	BubbleDetector2Regs = BubbleDetector1Regs + BUBBLE_REGS_SIZE,	// BubbleDetectorRegisters
	ReagentRegs = BubbleDetector2Regs + BUBBLE_REGS_SIZE,			// ReagentRegisters
	SyringeRegs = ReagentRegs + REAGENT_REG_SIZE,					// SyringeRegisters
	PersistentMemoryRegs = SyringeRegs + ( SYRINGE_REG_SIZE * 2u ),	// PersistentMemoryRegisters
	FwUpdateRegs = PersistentMemoryRegs + PERSISTENT_MEM_REG_SIZE,	// FwUpdateRegiters
	LastRegs = FwUpdateRegs + FWUPDATE_REG_SIZE,

	RegistersSize = TOTAL_REGS_SIZE
};

//================================================================
enum ErrorCodeRegister : uint32_t
{
	HostCommErrorCode = HostCommError,
	ProbeMotorErrorCode = ProbeMotorRegs + 8,
	Motor1ErrorCode = ProbeMotorErrorCode,
	RadiusMotorErrorCode = RadiusMotorRegs + 8,
	Motor2ErrorCode = RadiusMotorErrorCode,
	ThetaMotorErrorCode = ThetaMotorRegs + 8,
	Motor3ErrorCode = ThetaMotorErrorCode,
	FocusMotorErrorCode = FocusMotorRegs + 8,
	Motor4ErrorCode = FocusMotorErrorCode,
	ReagentMotorErrorCode = ReagentMotorRegs + 8,
	Motor5ErrorCode = ReagentMotorErrorCode,
	Rack1MotorErrorCode = Rack1MotorRegs + 8,
	Motor6ErrorCode = Rack1MotorErrorCode,
	Rack2MotorErrorCode = Rack2MotorRegs + 8,
	Motor7ErrorCode = Rack2MotorErrorCode,
	ObjectiveMotorErrorCode = ObjectiveMotorRegs + 8,
	Motor8ErrorCode = ObjectiveMotorErrorCode,
	Fan1ErrorCode = Fan1Regs,
	Fan2ErrorCode = Fan2Regs,
	LED1ErrorCode = LED1Regs + 4,
	LED2ErrorCode = LED2Regs + 4,
	LED3ErrorCode = LED3Regs + 4,
	LED4ErrorCode = LED4Regs + 4,
	LED5ErrorCode = LED5Regs + 4,
	LED6ErrorCode = LED6Regs + 4,
	PhotoDiode1ErrorCode = PhotoDiode1Regs,
	PhotoDiode2ErrorCode = PhotoDiode2Regs,
	PhotoDiode3ErrorCode = PhotoDiode3Regs,
	PhotoDiode4ErrorCode = PhotoDiode4Regs,
	PhotoDiode5ErrorCode = PhotoDiode5Regs,
	PhotoDiode6ErrorCode = PhotoDiode6Regs,
	DoorLatchErrorCode = DoorLatchRegs,
	Toggle2ErrorCode = Toggle2Regs,
	Toggle3ErrorCode = Toggle3Regs,
	Toggle4ErrorCode = Toggle4Regs,
	//TODO: do we need to check these?
	//	UART0Regs = 1444,
	//	UART3Regs = 1980,
	//	UART5Regs = 2516,
	Camera1ErrorCode = Camera1Regs + 4,
	Camera2ErrorCode = Camera2Regs + 4,
	Camera3ErrorCode = Camera3Regs + 4,
	BubbleDetector1ErrorCode = BubbleDetector1Regs + 4,
	BubbleDetector2ErrorCode = BubbleDetector2Regs + 4,
	ReagentErrorCode = ReagentRegs + 8,
	SyringeErrorCode = SyringeRegs + 12,
	PersistentMemoryErrorCode = PersistentMemoryRegs + 12,
	FwUpdateErrorCode = FwUpdateRegs + 4,

};

enum ReagentRegisterOffsets : uint32_t
{
	ReagentCommand = 0,
	ReagentCommandParam = 4,
	ReagentRFErrorCode = 8,
	ReagentRFReaderSN = 12,
	ReagentRFReaderFWV = 16,
	ReagentRFReaderHWV = 20,
	ReagentRFReaderId = 24,
	ReagentRFTagsTotalNum = 28,
	ReagentRFTagsAppData = 40   //Array [1..NumOfTags] of RFID tag/card application data structure
};

enum ReagentCommandRegisterCodes : uint16_t
{
	ReagentCommandRFScanTagMode = 0x01, //See NumOfTags register to find the result
	ReagentCommandSetTime = 0x02,       //Set Time counter internally with the parameter provided
	ReagentCommandMapTagValue = 0x03,   //Map valve positions to the RFID tag for decrementing usage count, repeat this command for each reagent/consumable.
	ReagentCommandRFReaderReset = 0x04, //Reset RFID reader module
	ReagentCommandRFReaderSetID = 0x05,
	ReagentCommandProgTag = 0xFF        //Write parameter map to user data & set configuration memory. Does read back confirm internally.
};

