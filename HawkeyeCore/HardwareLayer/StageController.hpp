#pragma once

#include <stdint.h>
#include <atomic>

#include <boost/optional/optional.hpp>

#include "AsyncCommandHelper.hpp"
#include "DeadlineTimerUtilities.hpp"
#include "StageControllerBase.hpp"

struct RadiusProperties;
struct ProbeProperties;
struct MoveStageParams;

class DLL_CLASS StageController : public StageControllerBase
{

public:
	StageController(std::shared_ptr<CBOService> pCBOService);
	virtual ~StageController();

	// Theta Motor Fields
	typedef struct ThetaMotorProperties
	{
		int32_t thetaPositionTolerance;
		int32_t thetaHomePos;
		int32_t thetaHomePosOffset;
		int32_t thetaCalPos;
		int32_t thetaBacklash;
		int32_t thetaStartTimeout_msec;
		int32_t thetaFullTimeout_msec;
		int32_t maxThetaPos;
	} ThetaProperties;

	// Radius Motor Fields
	typedef struct RadiusMotorProperties
	{
		int32_t radiusOffset;
		int32_t radiusMaxTravel;
		int32_t radiusPositionTolerance;
		int32_t radiusCenterPos;
		int32_t radiusBacklash;
		int32_t radiusStartTimeout_msec;
		int32_t radiusFullTimeout_msec;
	} RadiusProperties;

	// Probe Controller Fields
	typedef struct ProbeControllerProperties
	{
		int32_t probeHomePosOffset;
		int32_t probeHomePos;
		int32_t probePositionTolerance;
		int32_t probeStopPos;
		int32_t probeBusyTimeout_msec;
		int32_t probeStartTimeout_msec;
		int32_t probeMaxTravel;
		int32_t probeRetries;
	} ProbeProperties;

	typedef struct StageControllerProperties
	{
		ThetaProperties thetaProperties;
		RadiusProperties radiusProperties;
		ProbeProperties probeProperties;

		MotorProfileRegisters thetaMotionProfile;
		MotorProfileRegisters radiusMotionProfile;
		MotorProfileRegisters probeMotionProfile;

		bool controllerOk;
		bool initializedOk;
		int retriesCount;

		std::vector<uint32_t> thetaErrorCodes;
		std::vector<uint32_t> radiusErrorCodes;
		std::vector<uint32_t> probeErrorCodes;
	} StageControllerProperties;

	// Async operation Enums - Theta Motor
	typedef enum ThetaMotorOperationSequence : uint16_t
	{
		th_mo_entry_point,
		th_mo_clear_error,
		th_mo_can_do_work,
		th_mo_do_work,
		th_mo_do_work_wait,
		th_mo_cancel,
		th_mo_complete,
		th_mo_timeout,
		th_mo_error,
	} ThetaMotorOperationSequence;

	// Async operation Enums - Radius Motor
	typedef enum RadiusMotorOperationSequence : uint16_t
	{
		ra_mo_entry_point,
		ra_mo_clear_error,
		ra_mo_can_do_work,
		ra_mo_do_work,
		ra_mo_do_work_wait,
		ra_mo_cancel,
		ra_mo_complete,
		ra_mo_timeout,
		ra_mo_error,
	} RadiusMotorOperationSequence;

	// Async operation Enums - Probe Controller
	typedef enum ProbeOperationSequence : uint16_t
	{
		po_entry_point,
		po_clear_error,
		po_can_do_work,
		po_do_work,
		po_wait_for_work,
		po_complete,
		po_timeout,
		po_error,
	} ProbeOperationSequence;

	// Async operation Enums - Carousel Controller
	typedef enum CarouselOperationSequence : uint16_t
	{
		cc_entry_point,
		cc_clear_error,
		cc_do_work,
		cc_wait_for_work,
		cc_cancel,
		cc_complete,
		cc_timeout,
		cc_error,
		cc_no_tube_found,
	} CarouselOperationSequence;

	// Async operation Enums - Plate Controller
	typedef enum PlateOperationSequence : uint16_t
	{
		pt_entry_point,
		pt_clear_error,
		pt_do_work,
		pt_wait_for_work,
		pt_cancel,
		pt_complete,
		pt_timeout,
		pt_error,
	} PlateOperationSequence;

	// Async operation Enums - Stage Controller
	typedef enum StageControllerOperationSequence : uint16_t
	{
		sc_entry_point,
		sc_move_probe,
		sc_update_values,
		sc_move_radius_chk,				// currently not used
		sc_move_radius,
		sc_check_retry_on_timeout_radius,
		sc_move_theta_chk,				// currently not used
		sc_move_theta,
		sc_check_retry_on_timeout_theta,
		sc_cancel,
		sc_complete,
		sc_timeout,
		sc_error,
	} StageControllerOperationSequence;

	void PauseMotorPolling();
	void ResumeMotorPolling();

private:

	// Common Methods
	void loadConfiguration(std::string port, t_pPTree cfgTree, bool apply, std::string cfgFile);

	int32_t WriteControllerConfig();
	int32_t UpdateControllerConfig (eCarrierType type, t_opPTree& controller_node, t_opPTree& config_node);
	void resetInitParameters();
	void Quit(void);
	bool ChkThetaHome();

	void moveStageToPositionAsync(AsyncCommandHelper::EnumStateParams ex,
								  MoveStageParams thetaParams,
								  MoveStageParams radiusParams,
								  bool eject = false);
    void readRadiusPositionAsync(std::function<void( bool )> posCb);
    void readThetaPositionAsync(std::function<void(bool)> cb);
	void getThetaPosition_DelayAsync(std::function<void(int32_t)> posCb,
									 boost::asio::deadline_timer::duration_type delay = boost::posix_time::milliseconds(200));
	void getRawThetaPosition_DelayAsync(std::function<void(int32_t)> posCb,
										boost::asio::deadline_timer::duration_type delay = boost::posix_time::milliseconds(200));
	void getThetaRadiusPosition_DelayAsync(std::function<void(int32_t, int32_t)> posCb,
										   boost::asio::deadline_timer::duration_type delay = boost::posix_time::milliseconds(200));

	void getRawThetaPosition(int32_t & thetaPos);
	void getThetaRadiusPosition(int32_t & thetaPos, int32_t & radiusPos);

	eCarrierType getCurrentType() const;
	const StageControllerProperties& getCurrentProperties() const;
	const ThetaProperties& getThetaProperties() const;
	const RadiusProperties& getRadiusProperties() const;
	const ProbeProperties& getProbeProperties() const;

	static StageControllerProperties getDefaultProperties (eCarrierType type);
	static ThetaProperties getDefaultThetaProperties (eCarrierType type);
	static RadiusProperties getDefaultRadiusProperties (eCarrierType type);
	static ProbeProperties getDefaultProbeProperties (eCarrierType type);

	void reportErrorsAsync(bool clearAfterReporting = true);
	void reportErrors(bool logDescription, bool clearAfterReporting = true);

	// Common Fields

	t_pPTree parentTree;                  // points at the top-level file tree object
	t_opPTree configNode;                 // points at the (single) "config" node, containing all sub-nodes
	t_opPTree controllersNode;            // points at the "motor_controllers" node that contains all controller type nodes	
	std::string configFile;
	std::string cbiPort;
	bool apply_;

	bool stageCalibrated_;
	bool stageControllerInitBusy_;

	boost::optional<int32_t> opDefaultAbovePosition_;

	std::map<eCarrierType, StageControllerProperties> properties_;
	// reserve copy to use if/when we abort a stage registration event.
	std::map<eCarrierType, StageControllerProperties> cachedProperties_;

	std::atomic<bool> lastCarouselPresentStatus_;
	bool carouselTubeFoundDuringInit_;

	std::shared_ptr<DeadlineTimerUtilities> pStageDetectUpdateTimer;
	bool updateCarouselConfig;
	bool updatePlateConfig;
	bool thetaPosUpdated_;
	bool radiusPosUpdated_;

	// Theta Motor Methods
	bool ReinitTheta();		// if HW/FW, then make async
	static ThetaProperties ConfigThetaVariables(boost::property_tree::ptree& config,
												eCarrierType type,
												boost::optional<ThetaProperties> defaultValues = boost::none);
	int32_t UpdateThetaControllerConfig (boost::property_tree::ptree& config,
	                                            const ThetaProperties& tp,
	                                            eCarrierType type);
	bool ThetaAtPosition(int32_t pos, int32_t targetPos,
						 int32_t tolerance = -1, bool deadband = false);
	int32_t GetThetaPosition(void) const;
	static void normalizeThetaPosition(int32_t& currentPos, const int32_t& maxThetaPos);

	void initThetaAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries);
	void doHomeThetaAsync(AsyncCommandHelper::EnumStateParams ex, bool failOnTubeDetect, uint8_t numRetries);
	
	void thetaAtPositionAsync(std::function<void(bool)> cb, int32_t targetPos,
							  int32_t tolerance, bool deadband = false);
	void gotoThetaPositionAsync(std::function<void(bool)> cb,
								int32_t tgtPos, int32_t waitMilliSec);
	void moveThetaPositionRelativeAsync(std::function<void(bool)> cb,
										int32_t moveStep, int32_t waitMilliSec);
	void markThetaPositionAsZeroAsync(std::function<void(bool)> cb, int32_t waitMilliSec);

	// Theta Motor Fields
	bool thetaPosInited;
	std::shared_ptr<MotorBase> thetaMotor;


	// Radius Motor Methods
	bool ReinitRadius();
	static RadiusProperties ConfigRadiusVariables (boost::property_tree::ptree& config,
	                                               eCarrierType type,
	                                               boost::optional<RadiusProperties> defaultValues = boost::none);
	int32_t UpdateRadiusControllerConfig (boost::property_tree::ptree& config,
	                                             const RadiusProperties& rp,
	                                             eCarrierType type);
	int32_t GetRadiusPosition(void) const;
	bool RadiusAtPosition(int32_t pos, int32_t targetPos, int32_t tolerance, bool deadband = false) const;
	void StagePositionOk(std::function<void(SamplePositionDLL)> callback);

	void initRadiusAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries);
	void doHomeRadiusAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries);

	void radiusAtPositionAsync(std::function<void(bool)> cb, int32_t targetPos,
							   int32_t tolerance, bool deadband = false);
	void gotoRadiusPositionAsync(std::function<void(bool)> cb,
								 int32_t tgtPos, int32_t waitMilliSec);
	void moveRadiusPositionRelativeAsync(std::function<void(bool)> cb,
										 int32_t moveStep, int32_t waitMilliSec);
	void markRadiusPosAsZeroAsync(std::function<void(bool)> cb, int32_t waitMilliSec);

	// Radius Motor Fields
	bool radiusPosInited;
	std::shared_ptr<MotorBase> radiusMotor;

	// Probe Controller Methods
	void ReinitProbe(void);

	static ProbeProperties ConfigProbeVariables (boost::property_tree::ptree& config,
	                                             eCarrierType type,
	                                             boost::optional<ProbeProperties> defaultValues = boost::none);
	static int32_t UpdateProbeControllerConfig (boost::property_tree::ptree& config,
                                                const ProbeProperties& pp,
	                                            eCarrierType type);

	bool isProbeHome();
	void GetProbeSpeedParams(
		uint32_t& speed1Value, uint32_t& speed1Current,
		uint32_t& speed2Value, uint32_t& speed2Current) const;
	void AdjustProbeSpeeds(
		uint32_t speed1Value, uint32_t speed1Current,
		uint32_t speed2Value, uint32_t speed2Current) const;
	void GetProbeStopPositions(int32_t& abovePosition, int32_t& probeRaise) const;
	void AdjustProbeStop(int32_t abovePosition, int32_t probeRaise) const;

	bool probeAtPosition(int32_t posTgt);
	void setProbeSearchMode(bool enable, std::function<void(bool)> callback);
	
	void initProbeAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries);
	void stepProbeAsync(AsyncCommandHelper::EnumStateParams ex, int32_t moveStep, bool downOnInvalidStagePos);
	void doProbeUpAsync(AsyncCommandHelper::EnumStateParams ex, uint8_t numRetries);
	void doProbeDownAsync(AsyncCommandHelper::EnumStateParams ex, bool downOnInvalidStagePos, uint8_t numRetries);
	void doProbeHomeAsync(AsyncCommandHelper::EnumStateParams ex);
	void doProbeOperationAsync(AsyncCommandHelper::EnumStateParams ex);
	void gotoProbePositionAsync(std::function<void(bool)> cb, int32_t targetPos);
	void markProbePosAsZeroAsync(std::function<void(bool)> cb);

	// Probe Controller Fields
	bool probePosInited;
	std::shared_ptr<MotorBase> probeMotor;
	

	// Carousel Controller Methods
	bool InitCarousel();
	bool InitCarouselDefault();
	void selectCarouselAsync(std::function<void(bool)> callback);
	void setCarouselMotionProfile(std::function<void(bool)> onCompletion);
	StageControllerProperties ConfigCarouselVariables(boost::optional<StageControllerProperties> defaultValues = boost::none);
	int32_t WriteCarouselControllerConfig();
	
	void FindNextCarouselTube(std::function<void(bool)> callback, FindTubeMode mode,
							  boost::optional<uint32_t> finalTubePosition = boost::none);
	void findNextCarouselTubeAsync(AsyncCommandHelper::EnumStateParams ex,
								   uint32_t initialTubeNum, uint32_t finalTubeNumber);
	
	void getCarouselHomePosition(int32_t& thetaPos, int32_t& radiusPos);
	bool getCarouselPositionFromRowCol(uint32_t tgtCol, int32_t& thetaPos) const;
	void doCarouselThetaCW(const int32_t& currentThetaPos, int32_t& thetaPos) const;
	bool adjustCarouselThetaPos(const int32_t& currentThetaPos, int32_t& thetaPos) const;
	bool adjustCarouselRadiusPos(int32_t& radiusPos) const;
	void doCarouselHomeAsync(std::function<void(bool)> cb);
	void doCarouselRadiusHomeAsync(std::function<void(bool)> cb);
	bool canDoRetryForCarouselMovement(uint16_t retriesDone, bool radius, int32_t tgtPos, bool enableTubeCheck = false);
	void moveCarouselToPositionAsync(std::function<void(bool)> cb, SamplePositionDLL pos);
	void getCarouselPosition(SamplePositionDLL& pos);
	void checkTubePresent_DelayedAsync(std::function<void(bool)> cb);
	uint32_t getApproxCarouselTubeNumber(int32_t thetaPos, int32_t homeThetaPos) const;

	enum class ThetaMotionTubeDetectStates : uint8_t
	{
		eEntryPoint = 0,
		eSetThetaMotorSpeed,
		eStartTubeDetectTimer,
		eStartThetaMotion,
		eStopTheta,
		eCompleted,
		eTubeFoundError,
		eThetaMotionError,
	};
	void carouselMoveTheta(bool failonTubeDetect,
	                       ThetaMotionTubeDetectStates currentState,
	                       std::function<void(std::function<void(bool)>)> thetaMoveWork,
	                       std::function<void(bool /*status*/, bool /*tube_found*/)> onWorkComplete,
                           boost::optional<uint32_t> thetaMotorMaxSpeed = boost::none);

	// Carousel Controller Fields
	uint32_t maxTubes;
	double_t degreesPerTube;             // calculated from maxTubes after initialization from the external config file...
	double_t unitsPerTube;
	int32_t maxCarouselPos;
	int32_t thetaDeadband;
	double tubeCalcTolerance;
	double tubePosTolerance;
	t_opPTree carouselControllerNode;
	t_opPTree carouselMotorParamsNode;
	t_opPTree carouselControllerConfigNode;

	// Plate Controller Methods
	bool InitPlate();
	bool InitPlateDefault();
	void selectPlateAsync(std::function<void(bool)> callback);
	void setPlateMotionProfile(std::function<void(bool)> onCompletion);
	StageControllerProperties ConfigPlateVariables(boost::optional<StageControllerProperties> defaultValues = boost::none);
	int32_t WritePlateControllerConfig();
	static uint32_t GetRowOrdinal(TCHAR rowChar);

	bool CalculateRowColPos(uint32_t rowIdx, uint32_t colIdx, int32_t& thetaPos, int32_t& radiusPos) const;
	void getPlateHomePosition(int32_t& thetaPos, int32_t& radiusPos);
	bool getPlatePositionFromRowCol(uint32_t tgtRow, uint32_t tgtCol, int32_t& thetaPos, int32_t& radiusPos) const;
	bool getRowColFromPlatePosition(int32_t tgtThetaPos, int32_t tgtRadiusPos, uint32_t& row, uint32_t& col) const;
	bool adjustPlateRadiusPos(const int32_t& currentRadiusPos, int32_t& radiusPos); // rename
	bool adjustPlateThetaPos(const int32_t& currentThetaPos, int32_t& thetaPos); // rename
	void doPlateHomeAsync(std::function<void(bool)> cb);
	bool canDoRetryForPlateMovement(uint16_t retriesDone);
	void movePlateToPositionAsync(std::function<void(bool)> cb, SamplePositionDLL pos);
	void movePlateToRawPosAsync(std::function<void(bool)> cb, int32_t thetaPos, int32_t radiusPos);
	void getPlatePosition(SamplePositionDLL& pos);

	enum PlateDetectState : uint8_t
	{
		pdsValidityCheck = 0,
		pdsCalculations,
		pdsEnableProbe1,
		pdsProbeUp1,
		pdsOnProbeUp1Complete,
		pdsMovePlateToDetectLocation,
		pdsOnMoveToDetectComplete,
		pdsDisableProbe,
		pdsProbeDown,
		pdsOnProbeDownComplete,
		pdsCheckPlateDetection,
		pdsEnableProbe2,
		pdsProbeUp2,
		pdsOnProbeUp2Complete,
		pdsHomePlate,
		pdsOnHomePlateComplete,
		pdsReport,
		pdsCancelStart,
		pdsCancelComplete,
	};
	void findPlateTop(std::function<void( bool )> cb, PlateDetectState state,
					  bool stateSuccess = true, bool plateFound = false, int32_t stopPos = 0);

	// Plate Controller Fields
	int32_t plateWellSpacing;
	int32_t radiusDeadband;
	int32_t lastRadiusDir;
	int32_t radiusBacklashCompensation;
	bool platePosInited;
	t_opPTree plateControllerNode;
	t_opPTree plateMotorParamsNode;
	t_opPTree plateControllerConfigNode;

	// Async state internal enums
	enum class SelectStageStates: uint8_t
	{
		eEntryPoint = 0,
		eStopStage,
		eSetMotionProfile,
		eInitializeAndHomeMotors,
		eHomeStage,
		eEjectStage,
		eCompleted,
		eError,
	};
	void selectStageInternal(std::function<void(bool)> callback, SelectStageStates state);

	enum class sm_SetMotionProfileInitialized : uint8_t
	{
		smpi_ClearState = 0,
		smpi_SelectStage,
		smpi_SelectStageComplete,
		smpi_SetMotionProfile,
		smpi_SetMotionProfileComplete,
		smpi_Cleanup,
	};
	void setMotionProfileInternalIntializedStage(bool carousel_present, std::function<void(bool)> onComplete,
												 sm_SetMotionProfileInitialized state, bool success = true);

	enum class sm_SetMotionProfileUninitialized : uint8_t
	{
		smpu_ClearState = 0,
		smpu_SelectStage1,
		smpu_SelectStage1Complete,
		smpu_SetMotionProfile1,
		smpu_SetMotionProfile1Complete,
		smpu_SelectStage2,
		smpu_SelectStage2Complete,
		smpu_SetMotionProfile2,
		smpu_SetMotionProfile2Complete,
		smpu_Cleanup,
	};
	void setMotionProfileInternalUnInitializedStage(bool carousel_present, std::function<void(bool)> onComplete,
													sm_SetMotionProfileUninitialized state, bool success = true);

	enum class InitializeAndHomeStates : uint8_t
	{
		eInitializeProbe = 0,
		eIntializeRadius,
		eCarouselRadiusHome,
		eInitializeTheta,
		eCarouselThetaHome,
		eCompleted,
		eError,
	};

	void initializeAndHomeMotorsAsync(std::function<void(bool)> callback, InitializeAndHomeStates state);

	enum class FinalizeStageCalibrationStates : uint8_t
	{
		eProbeHome = 0,
		eRadiusHome,
		eCarouselRadiusHome,
		eThetaHome,
		eHomeStage,
		eCompleted,
		eError,
	};

	void finalizeStageCalibrationAsync(std::function<void(bool)> cb, FinalizeStageCalibrationStates state);

protected:

	void initAsync(std::function<void(bool)> callback,
				   std::string port, t_pPTree cfgTree,
				   bool apply, std::string cfgFile) override;
	void selectStageAsync(std::function<void(bool)> callback, eCarrierType type) override;
	void clearErrorsAsync(std::function<void(bool)> callback) override;
	void stopAsync(std::function<void(bool)> callback) override;
	bool carouselTubeFoundDuringInit() override;
	void Reinit() override;
	bool ControllerOk() override;

	// Probe Async Abstract Methods
	void probeUpAsync(std::function<void(bool)> callback)  override;
	// Setting "downOnInvalidStagePos = true" parameter will enforce the probe down movement 
	// even if stage position is invalid. This may lead to probe hitting the carousel/plate.
	void probeDownAsync(std::function<void(bool)> callback, bool downOnInvalidStagePos)  override;
	void probeStepUpAsync(std::function<void(bool)> callback, int32_t moveStep)  override;
	// Setting "downOnInvalidStagePos = true" parameter will enforce the probe down movement 
	// even if stage position is invalid. This may lead to probe hitting the carousel/plate.
	void probeStepDnAsync(std::function<void(bool)> callback,
						  int32_t moveStep,
						  bool downOnInvalidStagePos)  override;
	bool isProbeUp()  override;
	bool isProbeDown()  override;
	int32_t getProbePosition() override;

	int32_t getRadiusBacklash() override;
	int32_t getThetaBacklash() override;
	void calibrateMotorsHomeAsync(std::function<void(bool)> callback) override;
	void calibrateStageRadiusAsync(std::function<void(bool)> callback) override;
	void calibrateStageThetaAsync(std::function<void(bool)> callback) override;
	void finalizeCalibrateStageAsync(std::function<void(bool)> callback,
									 bool cancelCalibration) override;

	bool checkTubePresent() override;
	bool checkCarouselPresent() override;
	uint32_t getNearestTubePosition() override;
	void findTubeAsync(std::function<void(bool)> callback,
					   FindTubeMode mode,
					   boost::optional<uint32_t> finalTubePosition) override;
	void goToNextTubeAsync(std::function<void(boost::optional<uint32_t>)> callback) override;
	void enableStageHoldingCurrent(std::function<void(bool)> onCompletion, bool enable) override;
	void setStageProfile(std::function<void(bool)> onCompletion, bool enable) override;

	void isPlatePresentAsync(std::function<void(bool)> callback) override;
	
	void homeStageAsync(std::function<void(bool)> callback) override;
	void ejectStageAsync(std::function<void(bool)> callback, int32_t angle = 0) override;
	void moveStageToPosition(std::function<void(bool)> callback, SamplePositionDLL position) override;
	void getStagePosition(SamplePositionDLL& position) override;
	CalibrationConfigState isStageCalibrated() override;

	int32_t radiusCurrentMaxTravel()
	{
		return (getRadiusProperties().radiusMaxTravel - getRadiusProperties().radiusOffset - getRadiusProperties().radiusCenterPos);
	}
};
