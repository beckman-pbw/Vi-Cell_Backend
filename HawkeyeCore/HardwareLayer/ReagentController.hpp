#pragma once

#include "AsyncCommandHelper.hpp"
#include "ReagentControllerBase.hpp"

class DLL_CLASS ReagentController : public ReagentControllerBase
{
public:
	enum class OperationSequence {
		UnlatchDoor,
		ArmUp,
		ArmDown,
		ArmPurge,
		Enable,
		Disable,
	};

    ReagentController(std::shared_ptr<CBOService> pCBOService);
    ~ReagentController();

	virtual void    Initialize (std::function<void(bool)> callback, std::string cfgFile, bool apply = false) override;
	virtual void    Reset() override;
	virtual void    ClearErrors(std::function<void(bool)> callback) override;
	virtual bool    ControllerOk() override;
	virtual void    Stop() override;
	virtual bool    IsDoorClosed() override;
	virtual bool    IsPackInstalled() override;
	virtual bool    IsHome() override;
	virtual bool    IsUp() override;
	virtual bool    IsDown() override;
	//virtual int32_t ArmStepUp(int32_t moveStep) override;
	//virtual int32_t ArmStepDn(int32_t moveStep) override;
	//virtual int32_t ArmMove(int32_t target) override;
	//virtual void    AdjustSpeed (uint32_t maxSpeed, uint32_t speedCurrent,
	//                             uint32_t accel, uint32_t accelCurrent,
	//                             uint32_t decel, uint32_t decelCurrent);
	virtual void    GetMotorStatus (MotorStatus& reagentMotorStatus);
	virtual void    ArmPurge(std::function<void(bool)> callback);
	virtual void    ArmHome(std::function<void(bool)> callback);
	virtual void    ArmUp(std::function<void(bool)> callback);
	virtual void    ArmDown(std::function<void(bool)> callback);
	virtual void    UnlatchDoor (std::function<void(bool)> callback);
	void MoveToPosition (std::function<void(bool)> callback, int32_t tgtPos);	// , uint32_t waitMilliSec);
	void MoveReagentPositionRelative (std::function<void(bool)> callback, int32_t moveStep);	// , uint32_t waitMilliSec);


private:
	void processAsync (std::function<void(bool)> callback, OperationSequence sequence);
	uint32_t getTimeoutInMs() {
		return static_cast<uint32_t>(reagentBusyTimeout_msec);
	}	
	std::shared_ptr<boost::asio::deadline_timer> doorLatchTimer_;
	void        ConfigControllerVariables();
	void        WriteControllerConfig();
	bool        UpdateControllerConfig();
	int32_t     GetReagentArmPosition();
	void        Quit();
	void        reportError (uint32_t errorCode);

    std::shared_ptr<MotorBase> reagentMotor_;
    bool        closing_;
    bool        controllerOk_;
	std::string configFile_;
	t_opPTree   configNode_;
	t_opPTree   controllersNode_;
	t_opPTree   controllerNode_;
	t_opPTree   controllerConfigNode_;
	t_opPTree   motorParamsNode_;

    bool        updateConfig_;
	int32_t     positionTolerance_;
	int32_t     armHomePos_;         // the position value corresponding to home
    int32_t     armHomeOffset_;
    int32_t     armDownPos_;         // the reagent arm down location updated after each ArmDown command
    int32_t     armDownOffset_;
    int32_t     armMaxTravel_;
    int32_t     armPurgePosition_;
    int32_t     reagentStartTimeout_msec;
    int32_t     reagentBusyTimeout_msec;

	// Periodic update of controller board timestamp
	// Intended to adapt to leap days and DST switches
	std::shared_ptr<boost::asio::deadline_timer> boardTimestampTimer_;
	void SetControllerTimestamp(boost::system::error_code ec);
};

class DoorOperation : public ControllerBoardOperation::Operation
{
public:
	enum eDoorCommand
	{
		LatchDoor = 0,
		UnlatchDoor = 1,
	};

	DoorOperation()
	{
		Operation::Initialize (&regData_);
		regAddr_ = DoorLatchRegs;
	}

protected:
	ToggleRegisters regData_ = { };
};

class LatchDoorOperation : public DoorOperation
{
public:
	LatchDoorOperation() : DoorOperation()
	{
		mode_ = WriteMode;
		regData_.State = static_cast<uint32_t>(DoorOperation::LatchDoor);
		lengthInBytes_ = sizeof(uint32_t) * 2;	// ToggleRegisters);
	}
};

class UnlatchDoorOperation : public DoorOperation
{
public:
	UnlatchDoorOperation() : DoorOperation()
	{
		mode_ = WriteMode;
		regData_.State = static_cast<uint32_t>(DoorOperation::UnlatchDoor);
		lengthInBytes_ = sizeof(uint32_t) * 2;	// ToggleRegisters);
	}
};
