#pragma once

#include <mutex>

#pragma warning(push, 0)
#include <boost/asio.hpp>
#pragma warning(pop)

#include "AsyncCommandHelper.hpp"
#include "CBOService.hpp"
#include "MotorBase.hpp"
#include "MotorStatusDLL.hpp"
#include "SamplePositionDLL.hpp"

const uint32_t PosStepPerTube = (MaxThetaPosition / MaxCarouselTubes);
const double_t DegreesPerTube = (360.0 / (double_t)MaxCarouselTubes);

class DLL_CLASS StageControllerBase
{
public:
	StageControllerBase(std::shared_ptr<CBOService> pCBOService);
	virtual ~StageControllerBase();

	void Init (std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile, std::function<void(bool, bool/*tube found*/)> callback);
	void SelectStage(std::function<void(bool)> callback, eCarrierType type = eCarrierType::eCarousel);
	void ClearErrors(std::function<void(bool)> callback);
	void Stop(std::function<void(bool)> callback);
    bool MoveSetup( eCarrierType& type, bool skipCalCheck = false );
    void MoveClear( void );
    void CancelMove( std::function<void( bool )> callback );

	void GetMotorStatus(MotorStatus& thetaMotorStatus,
						MotorStatus& radiusMotorStatus,
						MotorStatus& probeMotorStatus);

	// Probe public Methods
	void ProbeUp(std::function<void(bool)> callback);
	void ProbeStepUp(std::function<void(bool)> callback, int32_t moveStep);
	bool IsProbeUp();

	// Setting "downOnInvalidStagePos = true" parameter will enforce the probe down movement 
	// even if stage position is invalid. This may lead to probe hitting the carousel/plate.
	void ProbeDown(std::function<void(bool)> callback, bool downOnInvalidStagePos = false);
	void ProbeStepDn(std::function<void(bool)> callback, int32_t moveStep, bool downOnInvalidStagePos = false);
	bool IsProbeDown();
	int32_t GetProbePosition();
	
	// Calibration public Methods
	void GetStageBacklashValues(int32_t& radiusBacklash, int32_t& thetaBacklash);
	void CalibrateMotorsHome(std::function<void(bool)> callback);
	void CalibrateStageRadius(std::function<void(bool)> callback);
	void CalibrateStageTheta(std::function<void(bool)> callback);
	void FinalizeCalibrateStage(bool cancelCalibration, std::function<void(bool)> callback);
	bool IsStageCalibrated(eCarrierType sample_carrier);

	// Carousel Specific public Methods
	bool IsTubePresent();
	bool IsCarouselPresent();
	uint32_t GetNearestTubePosition(); // get the nearest carousel tube position is carousel is not properly aligned
	void FindFirstTube(std::function<void(bool)> callback, boost::optional<uint32_t> finalTubePosition = boost::none);
	void FindNextTube(std::function<void(bool)> callback, boost::optional<uint32_t> finalTubePosition = boost::none);
	void GotoNextTube(std::function<void(int32_t)> callback);
	virtual std::function<void(void)> FindTubePosByPos(
		boost::optional<uint32_t> finalTubePosition,
		std::function<void(uint32_t)> stageMoveCallback,
		std::function<void(bool)> completionCallback);
	void SetStageProfile (std::function<void(bool)> onCompletion, bool enable);

	// Plate Specific public Methods
	void IsPlatePresent(std::function<void(bool)> callback);

	// Stage Async Abstract Methods
	void HomeStage(std::function<void(bool)> callback);
	void EjectStage(std::function<void(bool)> callback, int32_t angle = 0);
	void MoveStageToPosition(std::function<void(bool)> callback, SamplePositionDLL position);
	SamplePositionDLL GetStagePosition();

	void getDefaultStagePosition(std::function<void(SamplePositionDLL)> callback);
	void moveToDefaultStagePosition(std::function<void(bool)> callback);

	// Service level public Methods
	void MoveStageToServicePosition(std::function<void(bool)> callback, SamplePositionDLL position);

	virtual void setCarsouselSlots(const std::vector<bool>& slots) {};// Only used in simulation.

	virtual void PauseMotorPolling();
	virtual void ResumeMotorPolling();

protected:
	enum class CalibrationConfigState : uint8_t
	{
		eNone = 0,
		eOnlyPlate,
		eOnlyCarousel,
		ePlateAndCarousel,
	};

	enum class FindTubeMode
	{
		// It is possible that these modes are poorly named.
		// TODO review logic in StageController::FindTube to determine what these
		// are *actually* doing, and document it. 
		// The behavioral difference is that `FIRST` permits the carousel to move
		// counter-clockwise; this mode is used when the *current* position is *not*
		// valid.
		// StageController's docstring for the override-implementation of FindTube notes
		// that `FIRST` "includes the current position in the search for a position
		// holding a tube".
		FIRST,
		NEXT
	};

	static std::string ToString(FindTubeMode mode)
	{
		switch (mode)
		{
			case FindTubeMode::FIRST:
				return "first";
			case FindTubeMode::NEXT:
				return "next";
		}

		return std::string();
	}

	virtual void initAsync(std::function<void(bool)> callback, std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile) = 0;
	virtual void selectStageAsync(std::function<void(bool)> callback, eCarrierType type) = 0;
	virtual void stopAsync(std::function<void(bool)> callback) = 0;
	virtual void clearErrorsAsync(std::function<void(bool)> callback) = 0;
	virtual bool carouselTubeFoundDuringInit() = 0;
	virtual void Reinit() = 0;
	virtual bool ControllerOk() = 0;

	// Probe Async Abstract Methods
	virtual void probeUpAsync(std::function<void(bool)> callback) = 0;
	// Setting "downOnInvalidStagePos = true" parameter will enforce the probe down movement 
	// even if stage position is invalid. This may lead to probe hitting the carousel/plate.
	virtual void probeDownAsync(std::function<void(bool)> callback, bool downOnInvalidStagePos) = 0;
	virtual void probeStepUpAsync(std::function<void(bool)> callback, int32_t moveStep) = 0;
	// Setting "downOnInvalidStagePos = true" parameter will enforce the probe down movement 
	// even if stage position is invalid. This may lead to probe hitting the carousel/plate.
	virtual void probeStepDnAsync(std::function<void(bool)> callback, int32_t moveStep, bool downOnInvalidStagePos) = 0;
	virtual bool isProbeUp() = 0;
	virtual bool isProbeDown() = 0;
	virtual int32_t getProbePosition() = 0;

	// Calibration Async Abstract Methods
	virtual int32_t getRadiusBacklash() = 0;
	virtual int32_t getThetaBacklash() = 0;
	virtual void calibrateMotorsHomeAsync(std::function<void(bool)> callback) = 0;
	virtual void calibrateStageRadiusAsync(std::function<void(bool)> callback) = 0;
	virtual void calibrateStageThetaAsync(std::function<void(bool)> callback) = 0;
	virtual void finalizeCalibrateStageAsync(std::function<void(bool)> callback, bool cancelCalibration) = 0;
	virtual CalibrationConfigState isStageCalibrated() = 0;
	
	// Carousel Specific Async Abstract Methods
	virtual bool checkTubePresent() = 0;
	virtual bool checkCarouselPresent() = 0;
	// get the nearest carousel tube position is carousel is not properly aligned
	virtual uint32_t getNearestTubePosition() = 0;
	virtual void findTubeAsync(
		std::function<void(bool)> callback, FindTubeMode mode, boost::optional<uint32_t> finalTubePosition) = 0;
	virtual void goToNextTubeAsync(std::function<void(boost::optional<uint32_t>)> callback) = 0;
	virtual void enableStageHoldingCurrent(std::function<void(bool)> onCompletion, bool enable) = 0;
	virtual void setStageProfile(std::function<void(bool)> onCompletion, bool enable) = 0;

	// Plate Specific Async Abstract Methods
	virtual void isPlatePresentAsync(std::function<void(bool)> callback) = 0;

	// Stage Async Abstract Methods
	virtual void homeStageAsync(std::function<void(bool)> callback) = 0;
	virtual void ejectStageAsync(std::function<void(bool)> callback, int32_t angle) = 0;
	virtual void moveStageToPosition(std::function<void(bool)> callback, SamplePositionDLL position) = 0;
	virtual void getStagePosition(SamplePositionDLL& pos) = 0;
	virtual bool canSkipMotorCalibrationCheck();

	bool stageControllerInitialized_;
	std::atomic<bool> cancelMove_;
	std::atomic<bool> cancelThetaMove_;
	std::atomic<bool> cancelRadiusMove_;

	std::shared_ptr<CBOService> pCBOService_;
	std::shared_ptr<AsyncCommandHelper> asyncCommandHelper_;
	MotorStatusDLL thetaMotorStatus_;
	MotorStatusDLL radiusMotorStatus_;
	MotorStatusDLL probeMotorStatus_;
	eCarrierType carrierType_;	

	std::vector<bool> carouselSlotsForSimulation_;
	std::mutex carouselSlotsForSimulationMutex_;

private:
	template<typename ... Args>
	std::function<void(Args...)> wrapCallback(std::function<void(Args...)> callback)
	{
		return [this, callback](Args... args)
		{
			pCBOService_->enqueueExternal (std::bind(callback, args...));
		};
	}

	void StageControllerBase::findTubeInternal(
		std::shared_ptr<bool> pSearchNext,
		uint32_t currentTubePosition,
		boost::optional<uint32_t> finalTubePosition,
		std::function<void(uint32_t)> stageMoveCallback,
		std::function<void(bool)> completionCallback);

	void setCanSkipMotorCalibrationCheck(bool canSkip);
	std::atomic<bool> canSkipMotorCalibrationCheck_;
};
