#pragma once

#include "FocusControllerBase.hpp"

const int32_t DefaultFocusStartPos = 30000;             // specify a starting position to reduce the sweep times...
const int32_t DefaultFocusPositionTolerance = 10;       // allow 0.001 mm of inaccuracy?
const int32_t DefaultFocusCoarseStep = 300;
const int32_t DefaultFocusFineStep = 15;
const int32_t DefaultFocusDeadband = 4;

class DLL_CLASS FocusController : public FocusControllerBase
{
public:
	FocusController(std::shared_ptr<CBOService> pCBOService);
	virtual ~FocusController() override;
	void Quit(void);

private:

	enum HomeMoveState
	{
		homeStartPolling,
		homeDoMove,
		homeMoveComplete,
		homePausePolling,
	};

	enum SetCenterState
	{
		setCenterStartPolling,
		setCenterDoHome,
		setCenterHomeComplete,
		setCenterSetPosition,
		setCenterSetComplete,
		setCenterPausePolling,
	};

	enum SetPosState
	{
		setPosStartPolling,
		setPosDoHome,
		setPosHomeComplete,
		setPosFirstMove,
		setPosFirstMoveComplete,
		setPosTgtMove,
		setPosTgtMoveComplete,
		setPosPausePolling,
	};

	enum MoveRelState
	{
		moveRelStartPolling,
		moveRelDoMove,
		moveRelComplete,
		moveRelPausePolling,
	};

	std::shared_ptr<MotorBase> focusMotor;
	bool        closing;
	bool        controllerOk;
	bool        updateConfig;
	t_pPTree    parentTree;
	t_opPTree   configNode;
	t_opPTree   controllersNode;
	t_opPTree   thisControllerNode;
	t_opPTree   motorParamsNode;
	t_opPTree   controllerConfigNode;
	std::string configFile;

	bool        holdingEnabled;
	int32_t     PositionTolerance;              // to allow external adjustment
	int32_t     focusMaxPos;
	int32_t     focusStartPos;
	uint32_t    focusCoarseStepValue;           // the amount to move for each coarse step adjustment
	uint32_t    focusFineStepValue;             // the amount to move for each fine step adjustment
	int32_t     focusMaxTravel;
	int32_t     focusStartTimeout_msec;
	int32_t     focusBusyTimeout_msec;
	uint32_t    focusDeadband;

	bool        FocusAtPosition( int32_t posTgt );
	bool        FocusAtHome( void );

	void        ConfigControllerVariables( void );
	int32_t     WriteControllerConfig();
	int32_t     UpdateControllerConfig( t_opPTree & controller_node, t_opPTree & config_node );
	void        DoHome( std::function<void( bool )> cb, HomeMoveState opState, bool stateSuccess );
	void        DoSetCenter( std::function<void( bool )> cb, SetCenterState opState, bool stateSuccess );
	void        DoSetPosition( std::function<void( bool )> cb, SetPosState opState, bool stateSuccess, int32_t position);
	void        DoMoveRelativePosition( std::function<void( bool )> cb, MoveRelState opState, bool stateSuccess, int32_t moveStep);
	void        DoEnable( std::function<void( bool )> cb, bool enable);

public:
    
	// Status Query
	virtual void        Init(std::function<void(bool)> callback, t_pPTree cfgTree, bool apply, std::string cfgFile) override;
	virtual bool        ControllerOk(void) override; 
	virtual void        GetMotorStatus(MotorStatus& focusMotorStatus) override;
	virtual bool        IsHome(void) override;                        // true if the home sensor is true or the position is at 0;
	virtual bool        IsMax(void) override;                         // true if the current position is >= focus max travel value
	virtual int32_t     Position(void) override;                      // returns the current position of the focus motor
	virtual int32_t     GetFocusMax(void) override;                   // returns the max allowable position for focus travel
	virtual int32_t     GetFocusStart(void) override;                 // returns the configurable auto-focus sweep start position
	virtual void        SetFocusStart(int32_t start_pos) override;    // returns the configurable auto-focus sweep start position

	// Immediate Settings Change
	virtual uint32_t    AdjustCoarseStep(uint32_t stepSize) override;   // returns the resulting step size;
																		// returned value may be the same as current if stepValue = 0 (the default parameter),
																		// or if the stepValue is out of the allowable range
	virtual uint32_t    AdjustFineStep(uint32_t stepSize) override;     // returns the resulting step size;
																		// returned value may be the same as current if stepValue = 0 (the default parameter),
																		// or if the stepValue is out of the allowable range
	void                AdjustSpeed( uint32_t maxSpeed, uint32_t speedCurrent,
									 uint32_t accel, uint32_t accelCurrent,
									 uint32_t decel, uint32_t decelCurrent );

	// Request action / non-zero-time operation
	virtual void        Reset(void) override;
	virtual void        ClearErrors(std::function<void(bool)> callback) override;
	void                SetHoldingCurrent( std::function<void( bool )> cb, bool enabled );

	virtual void        Stop( std::function<void( bool )> cb, bool stopType = false ) override;
	virtual void		FocusHome( std::function<void( bool )> cb ) override;		// goto the home position
	virtual void        FocusCenter( std::function<void( bool )> cb ) override;		// go to the center of the focus travel (using the defined max travel value)
	virtual void        FocusMax( std::function<void( bool )> cb ) override;		// goto the defined max focus travel

	virtual void		SetPosition( std::function<void( bool, int32_t endPos )> cb, int32_t new_pos ) override;	// go to the specified position (limited by max travel or home)
	virtual void		FocusStepUpFast( std::function<void( bool, int32_t endPos )> cb ) override;					// step by 1/2 turn at 3x the normal motor speed
	virtual void		FocusStepDnFast( std::function<void( bool, int32_t endPos )> cb ) override;					// step by 1/2 turn at 3x the normal motor speed
	virtual void		FocusStepUpCoarse( std::function<void( bool, int32_t endPos )> cb ) override;				// step by 1/10 turn (default step value) at the normal motor speed
	virtual void		FocusStepDnCoarse( std::function<void( bool, int32_t endPos )> cb ) override;				// step by 1/10 turn (default step value) at the normal motor speed
	virtual void		FocusStepUpFine( std::function<void( bool, int32_t endPos )> cb ) override;					// step by 1/200 turn (default step value, equal to 1.5 microns) at the normal motor speed
	virtual void		FocusStepDnFine( std::function<void( bool, int32_t endPos )> cb ) override;					// step by 1/200 turn (default step value, equal to 1.5 microns) at the normal motor speed

};

