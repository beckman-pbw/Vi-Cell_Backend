#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <boost/optional/optional.hpp>

#include "CameraBase.hpp"
#include "CBOService.hpp"
#include "ControllerBoardOperation.hpp"
#include "EEPROMController.hpp"
#include "FirmwareDownload.hpp"
#include "FocusController.hpp"
#include "InitializationState.hpp"
#include "LedBase.hpp"
#include "ReagentControllerBase.hpp"
#include "StageControllerBase.hpp"
#include "SyringePumpBase.hpp"
#include "SyringePump.hpp"
#include "SystemErrors.hpp"
#include "SystemStatus.hpp"
#include "VoltageMeasurement.hpp"


#define	EJECT_HOLDING_CURRENT_ON	true
#define	EJECT_HOLDING_CURRENT_OFF	false

const int32_t DfltAutomationEjectAngle = 90;
const int32_t NoEjectAngle = 0;
const int32_t ShutdownEjectAngle = -10800;		// indicator also modulos to '0'
const int32_t AutoSelectEjectAngle = 10800;		// indicator also modulos to '0'


class Hardware
{
public:
	enum class InitializationSequence : uint8_t
	{ // Order of initialization.
		Start,
		FirmwareUpdate,
		Probe,
		Stage,
		Camera,
		EEPROM,
		FocusController,
		BrightfieldLed,
		ReagentController,
		Syringe,
		VoltageMeasurement,
		Complete,
		Error,
	};

	static Hardware& Instance()
	{
		static Hardware instance;
		return instance;
	}

	Hardware();
	void initialize (
		std::shared_ptr<boost::asio::io_context> pUpstream, 
		InitializationState initState,
		std::function<void (bool)> callback);

	virtual ~Hardware();

	//void initialize (std::function<void(bool)> callback, bool withHardware);
	void StopHardwareOperations (void);
	bool createHardwareModules();
	eCarrierType getCarrier (void);
	SamplePositionDLL getCarrierPosition() const;
	void getDefaultCarrierPosition(std::function<void(SamplePositionDLL)> positionCb);
	bool isFirmwareUpdated() const;
	bool isFirmwareUpdateFailed() const;
	bool isConfiguredHWPresent();
	bool detectHardware();

	static std::shared_ptr<LedBase> createBrightFieldLed(int16_t led, std::shared_ptr<CBOService>& cbo);
	static std::shared_ptr<iCamera> createCamera(int16_t camera);

	inline bool isInited()
	{
		return inited_;
	};
	HawkeyeError SaveFocusPositionToConfig(const int32_t& pos);
	boost::optional<InitializationState> getInitializationState();
	enum sm_RestoreFocusFromConfig
	{
		rffc_Start,
		rffc_Home,
		rffc_SetPosition,
	};
	void RestoreFocusPositionFromConfig(std::function<void(HawkeyeError, int32_t)> onComplete, bool moveMotor = false, sm_RestoreFocusFromConfig state = rffc_Start);
	virtual std::shared_ptr<CameraBase>& getCamera();
	virtual std::shared_ptr<FocusControllerBase>& getFocusController();
	virtual std::shared_ptr<LedBase> getLed (HawkeyeConfig::LedType ledType);
	virtual std::shared_ptr<ReagentControllerBase>& getReagentController();
	virtual std::shared_ptr<SyringePumpBase>& getSyringePump();
	virtual std::shared_ptr<StageControllerBase>& getStageController();
	virtual std::shared_ptr<EEPROMController>& getEEPROMController();
	virtual std::shared_ptr<FirmwareDownload>& gefirmwareDownloader();
	std::map<HawkeyeConfig::LedType, std::shared_ptr<LedBase>>& getLedObjectsMap();

	virtual bool IsControllerBoardFaulted();


private:
	bool getFirmware(std::string & binFileFullPath, std::string & hashFileFullPath, std::string & binFileName, std::string & hashFileName) const;
	void archiveFirmware(const std::string & binFileFullPath, const std::string& hashFileFullPath, const std::string& binFileName, const std::string& hashFileName) const;
	void setInitialSystemStatus();
	void UpdateSystemStatus (const boost::system::error_code& ec);
	void updateFirmware(std::function<void(bool)> fwUpdateComplteCallback, const std::string& binFileFullPath, const std::string& hashFileFullPath);

	void internalInitializationProcess (std::function<void(bool)> callback, InitializationSequence sequence);
	void setNextInitializationSequence (std::function<void(bool)> callback, InitializationSequence nextSequence);
	std::shared_ptr<LedBase> Hardware::getLedSharedPtr (HawkeyeConfig::LedType ledType);

	std::shared_ptr<FirmwareDownload> pFirmwareDownload_;
	std::shared_ptr<CameraBase> pCamera_;
	std::shared_ptr<FocusControllerBase> pFocusController_;
	std::map<HawkeyeConfig::LedType, std::shared_ptr<LedBase>> ledObjects_; 
	std::shared_ptr<ReagentControllerBase> pReagentController_;
	std::shared_ptr<SyringePumpBase> pSyringePump_;
	std::shared_ptr<VoltageMeasurementBase> pVoltageMeasurement_;
	std::shared_ptr<StageControllerBase> pStageController_;
	std::shared_ptr<EEPROMController> pEEPROMController_;

	bool inited_;
	eCarrierType carrierType_;
	bool stopSystemStatusUpdateTimer_;
	bool isFirmwareUpdated_;
	bool needFirmwareUpdate_;
	bool isFirmwareUpdateFailed_;
	boost::optional<InitializationState> initializationState_;

	std::shared_ptr<CBOService> pCBOService_;
	std::shared_ptr<boost::asio::deadline_timer> systemStatusUpdateTimer_;
};
