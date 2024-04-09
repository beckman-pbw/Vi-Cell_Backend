#pragma once

#include "FocusControllerBase.hpp"

class FocusControllerSim : public FocusControllerBase
{

private:

public:
	FocusControllerSim(std::shared_ptr<CBOService> pCBOService);
	virtual ~FocusControllerSim() override;
	void Quit();

	virtual void Init(
		std::function<void(bool)> callback, t_pPTree cfgTree = NULL,
		bool apply = false, std::string cfgFile = "") override;
	virtual void Reset(void) override;
	virtual bool ControllerOk(void) override;
	virtual void GetMotorStatus( MotorStatus& focusMotorStatus ) override;

	virtual bool IsHome(void) override;										// true if the home sensor is true or the position is at 0;
	virtual bool IsMax(void) override;										// true if the current position is >= focus max travel value

	virtual int32_t Position( void ) override;								// returns the current position of the focus motor
	virtual int32_t GetFocusMax( void ) override;							// returns the max allowable position for focus travel
	virtual int32_t GetFocusStart( void ) override;							// returns the configurable auto-focus sweep start position
	virtual void SetFocusStart( int32_t start_pos ) override;				// returns the configurable auto-focus sweep start position
	virtual uint32_t AdjustCoarseStep( uint32_t stepSize = 0 ) override;	// returns the resulting step size;
																			// returned value may be the same as current if stepValue = 0 (the default parameter),
																			// or if the stepValue is out of the allowable range
	virtual uint32_t AdjustFineStep( uint32_t stepSize = 0 ) override;		// returns the resulting step size;
																			// returned value may be the same as current if stepValue = 0 (the default parameter),
	virtual void ClearErrors(std::function<void(bool)> callback) override;
	virtual void Stop( std::function<void( bool )> cb, bool stopType = false ) override;
	virtual void FocusHome( std::function<void( bool )> cb ) override;		// goto the home position
	virtual void FocusCenter( std::function<void( bool )> cb ) override;	// go to the center of the focus travel (using the defined max travel value)
	virtual void FocusMax( std::function<void( bool )> cb ) override;		// goto the defined max focus travel

	virtual void SetPosition( std::function<void( bool, int32_t endPos )> cb, int32_t new_pos) override;	// go to the specified position (limited by max travel or home)
	virtual void FocusStepUpFast( std::function<void( bool, int32_t endPos )> cb ) override;		// step by 1/2 turn at 3x the normal motor speed
	virtual void FocusStepDnFast( std::function<void( bool, int32_t endPos )> cb ) override;		// step by 1/2 turn at 3x the normal motor speed
	virtual void FocusStepUpCoarse( std::function<void( bool, int32_t endPos )> cb ) override;		// step by 1/10 turn (default step value) at the normal motor speed
	virtual void FocusStepDnCoarse( std::function<void( bool, int32_t endPos )> cb ) override;		// step by 1/10 turn (default step value) at the normal motor speed
	virtual void FocusStepUpFine( std::function<void( bool, int32_t endPos )> cb ) override;		// step by 1/200 turn (default step value, equal to 1.5 microns) at the normal motor speed
	virtual void FocusStepDnFine( std::function<void( bool, int32_t endPos )> cb ) override;		// step by 1/200 turn (default step value, equal to 1.5 microns) at the normal motor speed
};
