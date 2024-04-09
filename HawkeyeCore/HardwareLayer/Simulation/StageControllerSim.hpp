#pragma once

#include "StageControllerBase.hpp"

class DLL_CLASS StageControllerSim : public StageControllerBase
{
public:

	StageControllerSim(std::shared_ptr<CBOService> pCBOService);
	~StageControllerSim();
	
	virtual void setCarsouselSlots(const std::vector<bool>& slots) override;

protected:
	void initAsync(std::function<void(bool)> callback, std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile) override;
	void selectStageAsync(std::function<void(bool)> callback, eCarrierType type) override;
	void clearErrorsAsync(std::function<void(bool)> callback) override;
	void stopAsync(std::function<void(bool)> callback) override;
	bool carouselTubeFoundDuringInit() override;
	void Reinit() override;
	bool ControllerOk() override;

	// Probe Async Abstract Methods
	void probeUpAsync(std::function<void(bool)> callback) override;
	void probeDownAsync(std::function<void(bool)> callback, bool downOnInvalidStagePos) override;
	void probeStepUpAsync(std::function<void(bool)> callback, int32_t moveStep) override;
	void probeStepDnAsync(std::function<void(bool)> callback, int32_t moveStep, bool downOnInvalidStagePos) override;
	bool isProbeUp() override;
	bool isProbeDown() override;
	int32_t getProbePosition() override;

	// Calibration Async Abstract Methods
	int32_t getRadiusBacklash() override;
	int32_t getThetaBacklash() override;
	void calibrateMotorsHomeAsync(std::function<void(bool)> callback) override;
	void calibrateStageRadiusAsync(std::function<void(bool)> callback) override;
	void calibrateStageThetaAsync(std::function<void(bool)> callback) override;
	void finalizeCalibrateStageAsync(std::function<void(bool)> callback, bool cancelCalibration) override;

	// Carousel Specific Async Abstract Methods
	bool checkTubePresent() override;
	bool checkCarouselPresent() override;
	uint32_t getNearestTubePosition() override; // get the nearest carousel tube position is carousel is not properly aligned
	void findTubeAsync(
		std::function<void(bool)> callback, FindTubeMode mode, boost::optional<uint32_t> finalTubePosition) override;
	void goToNextTubeAsync(std::function<void(boost::optional<uint32_t>)> callback) override;
	void enableStageHoldingCurrent(std::function<void(bool)> onCompletion, bool enable) override;
	void setStageProfile(std::function<void(bool)> onCompletion, bool enable) override;

	// Plate Specific Async Abstract Methods
	void isPlatePresentAsync(std::function<void(bool)> callback) override;

	// Stage Async Abstract Methods
	void homeStageAsync(std::function<void(bool)> callback) override;
	void ejectStageAsync(std::function<void(bool)> callback, int32_t angle = 0) override;
	void moveStageToPosition(std::function<void(bool)> callback, SamplePositionDLL position) override;
	void getStagePosition(SamplePositionDLL& position) override;
	virtual CalibrationConfigState isStageCalibrated() override;

private:
	void updateRadiusThetaPos();
	static bool getCarouselPostionFromRowCol(uint32_t tgtCol, int32_t& thetaPos, int32_t& radiusPos);
	bool getPlatePostionFromRowCol(uint32_t tgtRow, uint32_t tgtCol, int32_t& thetaPos, int32_t& radiusPos) const;
	void updateCarouselStatus(const int32_t& thetaPos, const int32_t& radiusPos);
	void updatePlateStatus(const int32_t& thetaPos, const int32_t& radiusPos);
	void Quit(void);

	SamplePositionDLL currentPos_;
	eCarrierType currentType_;
	bool probeUp_;
	int32_t probePos_;
	int32_t thetaPos_;
	int32_t radiusPos_;
	int32_t radiusBacklash_;
	int32_t thetaBacklash_;
	bool isStageCalibrated_;
};
