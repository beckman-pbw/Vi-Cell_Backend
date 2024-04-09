#include "stdafx.h"

#include "GetAsStrFunctions.hpp"
#include "HawkeyeLogic.hpp"
#include "HawkeyeLogicImpl.hpp"

static HawkeyeLogicImpl impl_;

static const char MODULENAME[] = "HawkeyeLogic";


extern "C"
{

//*****************************************************************************
// These APIs are list is the same order as those in *HawkeyeLogicImpl.hpp*
// Please continue to maintain this ordering.  "The Management"
//*****************************************************************************


//*****************************************************************************
//*****************************************************************************
// Miscellaneous APIs.
//*****************************************************************************
//*****************************************************************************
	DLL_CLASS void Initialize(bool with_hardware)
	{
		impl_.Initialize(with_hardware);
	}

	DLL_CLASS InitializationState IsInitializationComplete()
	{
		return impl_.IsInitializationComplete();
	}

	DLL_CLASS void Shutdown()
	{
		impl_.Shutdown();
	}

	DLL_CLASS bool IsShutdownComplete()
	{
		return impl_.IsShutdownComplete();
	}

	DLL_CLASS const char* GetErrorAsStr(HawkeyeError he)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([he]() -> auto
		{
			return getErrorAsStr(he);
		}, __func__);
	}

	DLL_CLASS const char* GetPermissionLevelAsStr(UserPermissionLevel permissions)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([permissions]() -> auto
		{
			return getPermissionLevelAsStr(permissions);
		}, __func__);
	}

	DLL_CLASS const char* GetSensorStatusAsStr(eSensorStatus status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getSensorStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetMotorStatusAsStr(MotorStatus& motorStatus)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([&motorStatus]() -> auto
		{
			return getMotorStatusAsStr(motorStatus);
		}, __func__);
	}

	DLL_CLASS const char * GetReagentDrainStatusAsStr(eDrainReagentPackState status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getReagentPackDrainStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetReagentPackLoadStatusAsStr(ReagentLoadSequence status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getReagentPackLoadStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetReagentPackUnloadStatusAsStr(ReagentUnloadSequence status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getReagentPackUnloadStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetReagentFlushFlowCellStatusAsStr(eFlushFlowCellState status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getReagentFlushFlowCellStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetCleanFluidicsStatusAsStr(eFlushFlowCellState status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getCleanFluidicsStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetReagentDecontaminateFlowCellStatusAsStr(eDecontaminateFlowCellState status)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([status]() -> auto
		{
			return getReagentPackDecontaminateFlowCellStatusAsStr(status);
		}, __func__);
	}

	DLL_CLASS const char* GetSystemStatusAsStr(eSystemStatus sysStatus)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([sysStatus]() -> auto
		{
			return getSystemStatusAsStr(sysStatus);
		}, __func__);
	}

	DLL_CLASS const char* GetSampleStatusAsStr (eSampleStatus sysStatus)
	{
		return impl_.getUiDllInstance().entryPoint<const char*>([sysStatus]() -> auto
		{
			return getSampleStatusAsStr (sysStatus);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SampleTubeDiscardTrayEmptied()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.SampleTubeDiscardTrayEmptied();
		}, __func__);
	}

	DLL_CLASS HawkeyeError EjectSampleStage(const  char* username, const  char* password)
	{
		// NOTE: calls to this API method have NO carrier context, and will act based on the carousel presence sensor
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.EjectSampleStage(username, password, cb);		// defaults for additional parameters specify '0' as the angle, and 'false' for loading (holding current)
		}, __func__);
	}

	// provides an API for automation clients to specifically position the stage carrier angle
	// for plate loading or unloading and allow holding current to be left enabled
	DLL_CLASS HawkeyeError SampleStageLoadUnload(const  char* username, const  char* password, int32_t angle)
	{
		// NOTE: calls to this API method have NO carrier context, and will act on the inferred 'presence' of a plate using the carousel presence sensor.
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, angle](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.EjectSampleStage(username, password, cb, angle, EJECT_HOLDING_CURRENT_ON);
		}, __func__ );
	}

	DLL_CLASS HawkeyeError RotateCarousel(SamplePosition &tubeNum)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&tubeNum](HawkeyeErrorCallback callback) -> auto
		{
			auto wrapCallback = [&tubeNum, callback](HawkeyeError he, SamplePositionDLL pos)
			{
				tubeNum = pos.getSamplePosition();
				callback(he);
			};
			return impl_.RotateCarousel(wrapCallback);
		}, __func__);
	}

	DLL_CLASS void GetSystemStatus(SystemStatusData*& status)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&status]() -> auto
		{
			impl_.GetSystemStatus(status);
		}, __func__);
	}

	DLL_CLASS void FreeSystemStatus(SystemStatusData* status)
	{
		return impl_.getUiDllInstance().entryPoint<void>([status]() -> auto
		{
			impl_.FreeSystemStatus(status);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetSystemSerialNumber(char*& serialNumber)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&serialNumber]() -> auto
		{
			return impl_.GetSystemSerialNumber(serialNumber);
		}, __func__);
	}

	DLL_CLASS char* SystemErrorCodeToString(uint32_t system_error_code)
	{
		return impl_.getUiDllInstance().entryPoint<char*>([system_error_code]() -> auto
		{
			return impl_.SystemErrorCodeToString(system_error_code);
		}, __func__);
	}

	DLL_CLASS void SystemErrorCodeToExpandedStrings(uint32_t system_error_code, char*& severity, char*& system, char*& subsystem, char*& instance, char*& failure_mode, char*& cellHealthErrorCode)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&, system_error_code]() -> auto
		{
			return impl_.SystemErrorCodeToExpandedStrings(system_error_code, severity, system, subsystem, instance, failure_mode, cellHealthErrorCode);
		}, __func__);
	}

	DLL_CLASS void SystemErrorCodeToExpandedResourceStrings(uint32_t system_error_code, char*& severity, char*& system, char*& subsystem, char*& instance, char*& failure_mode, char*& cellHealthErrorCode)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&, system_error_code]() -> auto
		{
			impl_.SystemErrorCodeToExpandedResourceStrings(system_error_code, severity, system, subsystem, instance, failure_mode, cellHealthErrorCode);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ClearSystemErrorCode(uint32_t active_error)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([active_error]() -> auto
		{
			return impl_.ClearSystemErrorCode(active_error);
		}, __func__);
	}

	DLL_CLASS void GetVersionInformation(SystemVersion& version)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&version]() -> auto
		{
			impl_.GetVersionInformation(version);
		}, __func__);
	}
	
	DLL_CLASS HawkeyeError FreeTaggedBuffer(NativeDataType tag, void* ptr)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([tag, ptr]() -> auto
		{
			return impl_.FreeTaggedBuffer(tag, ptr);
		}, __func__);
	}
	
	DLL_CLASS HawkeyeError FreeListOfTaggedBuffers(NativeDataType tag, void* ptr, uint32_t numitems)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([tag, ptr, numitems]() -> auto
		{
			return impl_.FreeListOfTaggedBuffers(tag, ptr, numitems);
		}, __func__);
	}
	
	DLL_CLASS void FreeCharBuffer(char* ptr)
	{
		return impl_.getUiDllInstance().entryPoint<void>([ptr]() -> auto
		{
			impl_.FreeTaggedBuffer(NativeDataType::eChar, ptr);
		}, __func__);
	}
	
	DLL_CLASS void FreeListOfCharBuffers(char** ptr, uint32_t numitems)
	{
		return impl_.getUiDllInstance().entryPoint<void>([ptr, numitems]() -> auto
		{
			impl_.FreeListOfTaggedBuffers(NativeDataType::eChar, ptr, numitems);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetSystemSecurityType(eSECURITYTYPE secType, const char* uname, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([secType, uname, password]() -> auto
		{
			return impl_.SetSystemSecurityType(secType, uname, password);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetSystemSecurityType(eSECURITYTYPE& secType)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&secType]() -> auto
		{
			secType = impl_.GetSystemSecurityType();
			return HawkeyeError::eSuccess;
		}, __func__);
	}

	//*****************************************************************************
	//*****************************************************************************
	// User APIs.
	//*****************************************************************************
	//*****************************************************************************

	DLL_CLASS HawkeyeError GetUserList(bool only_enabled, char**& userList, uint32_t& numUsers)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([only_enabled, &userList, &numUsers]() -> auto
		{
			return impl_.GetUserList(only_enabled, userList, numUsers);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetCurrentUser(char*& name, UserPermissionLevel& permissions)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&name, &permissions]() -> auto
		{
			return impl_.GetCurrentUser(name, permissions);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetUserPermissionLevel(const char* name, UserPermissionLevel& permissions)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, &permissions]() -> auto
		{
			return impl_.GetUserPermissionLevel(name, permissions);
		}, __func__);
	}

	DLL_CLASS HawkeyeError LoginUser(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.LoginConsoleUser(username, password);
		}, __func__);
	}

	DLL_CLASS void LogoutUser()
	{
		return impl_.getUiDllInstance().entryPoint<void>([]() -> auto
		{
			impl_.LogoutConsoleUser();
		}, __func__);
	}

	DLL_CLASS HawkeyeError LoginConsoleUser(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.LoginConsoleUser(username, password);
		}, __func__);
	}

	DLL_CLASS void LogoutConsoleUser()
	{
		return impl_.getUiDllInstance().entryPoint<void>([]() -> auto
		{
			impl_.LogoutConsoleUser();
		}, __func__);
	}

	DLL_CLASS HawkeyeError LoginRemoteUser(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.LoginRemoteUser(username, password);
		}, __func__);
	}

	DLL_CLASS void LogoutRemoteUser(const char* username)
	{
		return impl_.getUiDllInstance().entryPoint<void>([username]() -> auto
		{
			impl_.LogoutRemoteUser(username);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SwapUser(const char* newusername, const char* password, SwapUserRole swapRole)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([newusername, password, swapRole]() -> auto
		{
			return impl_.SwapUser(newusername, password, swapRole);
		}, __func__);
	}


	DLL_CLASS HawkeyeError AdministrativeUserEnable(const char* administrator_account, const char* administrator_password, const char* user_account)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([administrator_account, administrator_password, user_account]() -> auto
		{
			return impl_.AdministrativeUserEnable(administrator_account, administrator_password, user_account);
		}, __func__);
	}

	DLL_CLASS char* GenerateHostPassword(const char* securitykey)
	{
		return impl_.getUiDllInstance().entryPoint<char*>([securitykey]() -> auto
		{
			return impl_.GenerateHostPassword(securitykey);
		}, __func__);
	}

	DLL_CLASS void GetUserInactivityTimeout(uint16_t& minutes)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&minutes]() -> auto
		{
			impl_.GetUserInactivityTimeout(minutes);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserInactivityTimeout(uint16_t minutes)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([minutes]() -> auto
		{
			return impl_.SetUserInactivityTimeout(minutes);
		}, __func__);
	}

	DLL_CLASS void GetUserPasswordExpiration(uint16_t& days)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&days]() -> auto
		{
			impl_.GetUserPasswordExpiration(days);
		}, __func__);
	}

	DLL_CLASS HawkeyeError LogoutUser_Inactivity()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.LogoutUser_Inactivity();
		}, __func__);
	}

	DLL_CLASS HawkeyeError AddUser(const char* name, const char* displayname, const char* password, UserPermissionLevel permissions)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		{
			return impl_.AddUser(name, displayname, password, permissions);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RemoveUser(const char* name)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name]() -> auto
		{
			return impl_.RemoveUser(name);
		}, __func__);
	}

	DLL_CLASS HawkeyeError EnableUser(const char* name, bool enabled)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, enabled]() -> auto
		{
			return impl_.EnableUser(name, enabled);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ChangeUserPassword(const char* name, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, password]() -> auto
		{
			return impl_.ChangeUserPassword(name, password);
		}, __func__);
	}

	DLL_CLASS HawkeyeError IsPasswordExpired(const char* name, bool& expired)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, &expired]() -> auto
		{
			return impl_.IsPasswordExpired(name, expired);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ChangeUserDisplayName(const char* name, const char* displayname)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, displayname]() -> auto
		{
			return impl_.ChangeUserDisplayName(name, displayname);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ChangeUserPermissions(const char* name, UserPermissionLevel permissions)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, permissions]() -> auto
		{
			return impl_.ChangeUserPermissions(name, permissions);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetSampleColumns(const char* username, ColumnSetting*& recs, uint32_t& retrieved_count)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &recs, &retrieved_count]() -> auto
		{
			return impl_.GetSampleColumns(username, recs, retrieved_count);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetSampleColumns(const char* username, ColumnSetting* recs, uint32_t count)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &recs, &count]() -> auto
		{
			return impl_.SetSampleColumns(username, recs, count);
		}, __func__);
	}

	DLL_CLASS void FreeSampleColumns(ColumnSetting* recs, uint32_t count)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &recs, &count]() -> auto
		{
			impl_.FreeSampleColumns(recs, count);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetDisplayDigits(const char* username, uint32_t digits)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &digits]() -> auto
		{
			return impl_.SetDisplayDigits(username, digits);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserFastMode(const char* username, bool enable)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &enable]() -> auto
		{
			return impl_.SetUserFastMode(username, enable);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserEmail(const char* username, const char* email)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &email]() -> auto
		{
			return impl_.SetUserEmail(username, email);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserComment(const char* username, const char* comment)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &comment]() -> auto
		{
			return impl_.SetUserComment(username, comment);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetUserRecord(const char* username, UserRecord*& record)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &record]() -> auto
		{
			return impl_.GetUserRecord(username, record);
		}, __func__);
	}
	DLL_CLASS void FreeUserRecord(UserRecord* record)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &record]() -> auto
		{
			impl_.FreeUserRecord(record);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserCellTypeIndices(const char* name, uint32_t nCells, uint32_t* celltype_indices)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, nCells, celltype_indices]() -> auto
		{
			return impl_.SetAllowedCellTypeIndices(name, nCells, celltype_indices);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserFolder(const char* name, const char* folder)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, folder]() -> auto
		{
			return impl_.SetUserFolder(name, folder);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserPasswordExpiration(uint16_t days)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([days]() -> auto
		{
			return impl_.SetUserPasswordExpiration(days);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetUserAnalysisIndices(const char* name, uint32_t n_ad, uint16_t* analysis_indices)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, n_ad, analysis_indices]() -> auto
		{
			return impl_.SetUserAnalysisIndices(name, n_ad, analysis_indices);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetUserAnalysisIndices(const char* name, uint32_t& n_ad, NativeDataType& tag, uint16_t*& analysis_indices)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([name, &n_ad, &tag, &analysis_indices]() -> auto
		{
			tag = NativeDataType::eInt16;
			return impl_.GetUserAnalysisIndices(name, n_ad, analysis_indices);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ChangeMyPassword(const char* oldpassword, const char* newpassword)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([oldpassword, newpassword]() -> auto
		{
			return impl_.ChangeMyPassword(oldpassword, newpassword);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ValidateMe(const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([password]() -> auto
		{
			return impl_.ValidateMe(password);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ValidateUserCredentials(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.ValidateUserCredentials(username, password);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ValidateLocalAdminAccount(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.ValidateLocalAdminAccount(username, password);
		}, __func__);
	}
	

	//*****************************************************************************
	//*****************************************************************************
	// Reagent data APIs.
	//*****************************************************************************
	//*****************************************************************************
	DLL_CLASS HawkeyeError GetReagentContainerStatus (uint16_t container_num, ReagentContainerState*& status)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([container_num, &status](HawkeyeErrorCallback onComplete) -> void {
			impl_.GetReagentContainerStatus (onComplete, container_num, status);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetReagentContainerStatusAll (uint16_t& num_reagents, ReagentContainerState*& status)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_reagents, &status](HawkeyeErrorCallback onComplete) -> void {
			impl_.GetReagentContainerStatusAll (onComplete, num_reagents, status);
		}, __func__);
	}

	DLL_CLASS void FreeListOfReagentContainerState (ReagentContainerState* list, uint32_t num_reagents)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list, num_reagents]() -> auto
		{
			impl_.FreeListOfReagentContainerState(list, num_reagents);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetReagentDefinitions(uint32_t& num_reagents, ReagentDefinition*& reagents)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_reagents, &reagents](HawkeyeErrorCallback onComplete) -> void {
			impl_.GetReagentDefinitions (onComplete, num_reagents, reagents);
		}, __func__);
	}

	DLL_CLASS void FreeReagentDefinitions(ReagentDefinition*& reagents, uint32_t& num_reagents)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&reagents, &num_reagents]() -> auto
		{
			impl_.FreeReagentDefinitions(reagents, num_reagents);
		}, __func__);
	}

	DLL_CLASS HawkeyeError UnloadReagentPack (ReagentContainerUnloadOption* UnloadActions, uint16_t nContainers, reagent_unload_status_callback onUnloadStatusChange, reagent_unload_complete_callback onUnloadComplete)
	{
		auto wrap_onUnloadStatusChange = impl_.getUiDllInstance().wrapCallback(onUnloadStatusChange);
		auto wrap_onUnloadComplete = impl_.getUiDllInstance().wrapCallback(onUnloadComplete);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=](HawkeyeErrorCallback callback) -> auto
		{
			impl_.UnloadReagentPack(UnloadActions, nContainers, wrap_onUnloadStatusChange, wrap_onUnloadComplete, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError LoadReagentPack (reagent_load_status_callback onLoadStatusChange, reagent_load_complete_callback onLoadComplete)
	{
		auto wrap_onLoadStatusChange = impl_.getUiDllInstance().wrapCallback(onLoadStatusChange);
		auto wrap_onLoadComplete = impl_.getUiDllInstance().wrapCallback(onLoadComplete);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=](HawkeyeErrorCallback callback) -> auto
		{
			impl_.LoadReagentPack(wrap_onLoadStatusChange, wrap_onLoadComplete, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetReagentContainerLocation(ReagentContainerLocation& location)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&location]() -> auto
		{
			return impl_.SetReagentContainerLocation(location);
		}, __func__);
	}

	DLL_CLASS HawkeyeError StartFlushFlowCell(flowcell_flush_status_callback on_status_change)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback(on_status_change);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapcallback](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartFlushFlowCell(wrapcallback, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError StartCleanFluidics(flowcell_flush_status_callback on_status_change)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback(on_status_change);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapcallback](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartCleanFluidics(wrapcallback, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetFlushFlowCellState(eFlushFlowCellState& state)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&state]() -> auto
		{
			return impl_.GetFlushFlowCellState(state);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetCleanFluidicsState(eFlushFlowCellState& state)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&state]() -> auto
		{
			return impl_.GetCleanFluidicsState(state);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelFlushFlowCell()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelFlushFlowCell();
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelCleanFluidics()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelCleanFluidics();
		}, __func__);
	}

	DLL_CLASS HawkeyeError StartDecontaminateFlowCell(flowcell_decontaminate_status_callback on_status_change)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback(on_status_change);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapcallback](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartDecontaminateFlowCell(wrapcallback, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetDecontaminateFlowCellState(eDecontaminateFlowCellState& state)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&state]() -> auto
		{
			return impl_.GetDecontaminateFlowCellState(state);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelDecontaminateFlowCell()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelDecontaminateFlowCell();
		}, __func__);
	}

	DLL_CLASS HawkeyeError StartPrimeReagentLines(prime_reagentlines_callback on_status_change)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback(on_status_change);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapcallback](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartPrimeReagentLines(wrapcallback, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetPrimeReagentLinesState(ePrimeReagentLinesState& state)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&state]() -> auto
		{
			return impl_.GetPrimeReagentLinesState(state);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelPrimeReagentLines()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelPrimeReagentLines();
		}, __func__);
	}

	DLL_CLASS HawkeyeError StartPurgeReagentLines(purge_reagentlines_callback on_status_change)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback(on_status_change);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapcallback](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartPurgeReagentLines(wrapcallback, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelPurgeReagentLines()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelPurgeReagentLines();
		}, __func__);
	}

	DLL_CLASS HawkeyeError StartDrainReagentPack(drain_reagentpack_callback on_status_change, uint8_t valve_position)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback(on_status_change);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapcallback, valve_position](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartDrainReagentPack(wrapcallback, valve_position, 0, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetDrainReagentPackState(eDrainReagentPackState& state)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&state]() -> auto
		{
			return impl_.GetDrainReagentPackState(state);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelDrainReagentPack()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelDrainReagentPack();
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetReagentVolume (CellHealthReagents::FluidType type, int32_t& volume_ul)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([type, &volume_ul]() -> auto
		{
			return impl_.GetReagentVolume (type, volume_ul);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetReagentVolume (CellHealthReagents::FluidType type, int32_t volume_ul)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([type, volume_ul]() -> auto
		{
			return impl_.SetReagentVolume (type, volume_ul);
		}, __func__);
	}

	DLL_CLASS HawkeyeError AddReagentVolume (CellHealthReagents::FluidType type, int32_t volume_ul)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([type, volume_ul]() -> auto
		{
			return impl_.AddReagentVolume (type, volume_ul);
		}, __func__);
	}

	//*****************************************************************************
	//*****************************************************************************
	// Sample Processing APIs.
	//*****************************************************************************
	//*****************************************************************************
	DLL_CLASS HawkeyeError GetCurrentSample (SampleDefinition*& sd)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&sd]() -> auto
		{
			return impl_.GetCurrentSample (sd);
		}, __func__);
	}

	DLL_CLASS void FreeSampleDefinition (SampleDefinition* sampleList, uint32_t numSamples)
	{
		return impl_.getUiDllInstance().entryPoint<void>([sampleList, numSamples]() -> auto
		{
			impl_.FreeSampleDefinition (sampleList, numSamples);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetWorklist (const Worklist& wl)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError> ([wl] (std::function<void (HawkeyeError)>onComplete) -> void {
			impl_.SetWorklist (wl, onComplete);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetImageOutputTypePreference (eImageOutputType image_type)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([image_type](HawkeyeErrorCallback onComplete) -> void {
			impl_.SetImageOutputTypePreference (image_type, onComplete);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SaveSampleSetTemplate (const SampleSet& ss)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ss](std::function<void (HawkeyeError)>onComplete) -> void {
			impl_.SaveSampleSetTemplate (ss, onComplete);
			}, __func__);
	}

	DLL_CLASS HawkeyeError GetSampleSetTemplateList (uint32_t skip, uint32_t take, SampleSet*& sampleSets, uint32_t& numSampleSets, uint32_t& totalSampleSetsAvailable)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>(
			[skip, take, &sampleSets, &numSampleSets, &totalSampleSetsAvailable](std::function<void (HawkeyeError)>onComplete) -> void {
				impl_.GetSampleSetTemplateList (skip, take, sampleSets, numSampleSets, totalSampleSetsAvailable, onComplete);
			}, __func__);
	}

	DLL_CLASS HawkeyeError GetSampleSetTemplateAndSampleList (uuid__t uuid, SampleSet*& sampleSet)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>(
			[uuid, &sampleSet](std::function<void (HawkeyeError)>onComplete) -> void {
				impl_.GetSampleSetTemplateAndSampleList (uuid, sampleSet, onComplete);
			}, __func__);
	}

	DLL_CLASS HawkeyeError GetSampleSetAndSampleList (const uuid__t uuid, SampleSet*& sampleSet)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>(
			[uuid, &sampleSet](std::function<void (HawkeyeError)>onComplete) -> void {
				return impl_.GetSampleSetAndSampleList (uuid, sampleSet, onComplete);
			}, __func__);
	}

	DLL_CLASS HawkeyeError DeleteSampleSetTemplate (uuid__t uuid)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>(
			[uuid](std::function<void (HawkeyeError)>onComplete) -> void {
				impl_.DeleteSampleSetTemplate (uuid, onComplete);
			}, __func__);
	}

	DLL_CLASS HawkeyeError AddSampleSet (const SampleSet& ss)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ss](std::function<void (HawkeyeError)>onComplete) -> void {
			impl_.AddSampleSet (ss, onComplete);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelSampleSet (uint16_t sampleSetIndex)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([sampleSetIndex](std::function<void (HawkeyeError)>onComplete) -> void {
			impl_.CancelSampleSet (sampleSetIndex, onComplete);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetSampleSetList(
		eFilterItem filterItem,
		uint64_t start,
		uint64_t end,
		char* username,
		char* nameSearchString,
		char* tagSearchString,
		char* cellTypeOrQualityControlName,
		uint32_t skip,
		uint32_t take,
		uint32_t& totalSampleSetsAvailable,
		SampleSet*& sampleSets,
		uint32_t& numSampleSets)
	{
		std::string sUsername;
		if (username) sUsername = username;
		std::string sNameSearchString;
		if (nameSearchString) sNameSearchString = nameSearchString;
		std::string sTagSearchString;
		if (tagSearchString) sTagSearchString = tagSearchString;
		std::string sCellTypeOrQualityControlName;
		if (cellTypeOrQualityControlName) sCellTypeOrQualityControlName = cellTypeOrQualityControlName;

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([filterItem, start, end, sUsername, sNameSearchString, sTagSearchString, sCellTypeOrQualityControlName, skip, take, &totalSampleSetsAvailable, &sampleSets, &numSampleSets](std::function<void (HawkeyeError)>onComplete) -> void {
			impl_.GetSampleSetList(
				filterItem,
				start,
				end,
				sUsername,
				sNameSearchString,
				sTagSearchString,
				sCellTypeOrQualityControlName,
				skip,
				take,
				totalSampleSetsAvailable,
				sampleSets,
				numSampleSets,
				onComplete);
			}, __func__);
	}

	DLL_CLASS HawkeyeError StartProcessing (const char* username, const char* password,
		                                    sample_status_callback onSampleStatus,
	                                        sample_image_result_callback onSampleImageProcessed,
	                                        sample_status_callback onSampleComplete,
	                                        worklist_completion_callback onWorklistComplete)
	{
		auto wrap_SampleStatus_delete  = [](SampleDefinition* item) {impl_.FreeSampleDefinition(item, 1); };
		auto wrap_SampleStatus = impl_.getUiDllInstance().wrapCallbackWithDeleter<SampleDefinition*>(onSampleStatus, wrap_SampleStatus_delete);

		auto wrap_SampleImageProcessed_delete = [](SampleDefinition* item, uint16_t, ImageSetWrapper_t* imset, BasicResultAnswers, BasicResultAnswers)
		{
			impl_.FreeSampleDefinition (item, 1);
			impl_.FreeImageSetWrapper (imset, 1);
		};		
		auto wrap_SampleImageProcessed = impl_.getUiDllInstance().wrapCallbackWithDeleter
		<SampleDefinition*, uint16_t, ImageSetWrapper_t*, BasicResultAnswers, BasicResultAnswers>(onSampleImageProcessed, wrap_SampleImageProcessed_delete);

		auto wrap_SampleComplete_delete = [](SampleDefinition* item) {impl_.FreeSampleDefinition (item, 1); };
		auto wrap_SampleComplete = impl_.getUiDllInstance().wrapCallbackWithDeleter<SampleDefinition*>(onSampleComplete, wrap_SampleComplete_delete);

		auto wrap_WorkListComplete = impl_.getUiDllInstance().wrapCallback(onWorklistComplete);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=](HawkeyeErrorCallback onCompleteCallback) -> auto
		{
			impl_.StartProcessing (username, password, wrap_SampleStatus, wrap_SampleImageProcessed, wrap_SampleComplete, wrap_WorkListComplete, onCompleteCallback);
		}, __func__, true);
	}

	DLL_CLASS HawkeyeError GetSampleDefinitionBySampleId (uuid__t sampleDataUuid, SampleDefinition*& sampleDef)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([sampleDataUuid, &sampleDef]() -> auto
		{
			return impl_.GetSampleDefinitionBySampleId (sampleDataUuid, sampleDef);
		}, __func__);
	}

	DLL_CLASS void FreeSampleSet (SampleSet* list, uint32_t num_samplesets)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list, num_samplesets]() -> auto
		{
			impl_.FreeSampleSet (list, num_samplesets);
		}, __func__);
	}

	DLL_CLASS HawkeyeError PauseProcessing(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.PauseProcessing(username, password);
		}, __func__, true);
	}

	DLL_CLASS HawkeyeError ResumeProcessing(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.ResumeProcessing (username, password, callback);
		}, __func__, true);
	}

	DLL_CLASS HawkeyeError StopProcessing(const char* username, const char* password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password]() -> auto
		{
			return impl_.StopProcessing(username, password);
		}, __func__, true);
	}

	//*****************************************************************************
	//*****************************************************************************
	// Cell data APIs.
	//*****************************************************************************
	//*****************************************************************************
	DLL_CLASS HawkeyeError GetAllCellTypes(uint32_t& num_celltypes, CellType*& celltypes)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_celltypes, &celltypes]() -> auto
		{
			return impl_.GetAllCellTypes(num_celltypes, celltypes);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetFactoryCellTypes(uint32_t& num_ct, CellType*& celltypes)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_ct, &celltypes]() -> auto
		{
			return impl_.GetFactoryCellTypes(num_ct, celltypes);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetAllowedCellTypes(const char* username, uint32_t& num_celltypes, CellType*& celltypes)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&username,&num_celltypes, &celltypes]() -> auto
		{
			return impl_.GetAllowedCellTypes(username, num_celltypes, celltypes);
		}, __func__);
	}

	DLL_CLASS HawkeyeError AddCellType(const char* username, const char* password, CellType& celltype, uint32_t& ct_index, const char* retiredName)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, &celltype, &ct_index, retiredName]() -> auto
		{
			return impl_.AddCellType(username, password, celltype, ct_index, retiredName);
		}, __func__);
	}

	// Deprecated: not called by the UI anymore.
	DLL_CLASS HawkeyeError ModifyCellType(const char* username, const char* password, CellType& celltype)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, &celltype]() -> auto
		{
			return impl_.ModifyCellType(username, password, celltype);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RemoveCellType(const char* username, const char* password, uint32_t ct_index)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, ct_index]() -> auto
		{
			return impl_.RemoveCellType(username, password, ct_index);
		}, __func__);
	}

	DLL_CLASS void FreeListOfCellType(CellType* list, uint32_t num_celltypes)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list, num_celltypes]() -> auto
		{
			impl_.FreeListOfCellType(list, num_celltypes);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetTemporaryAnalysisDefinition(AnalysisDefinition*& temp_definition)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&temp_definition]() -> auto
		{
			return impl_.GetTemporaryAnalysisDefinition(temp_definition);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetTemporaryAnalysisDefinition(AnalysisDefinition* temp_definition)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([temp_definition]() -> auto
		{
			return impl_.SetTemporaryAnalysisDefinition(temp_definition);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError SetTemporaryAnalysisDefinitionFromExisting(uint16_t analysis_index)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([analysis_index]() -> auto
		//{
		//	return impl_.SetTemporaryAnalysisDefinitionFromExisting(analysis_index);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError GetTemporaryCellType(CellType*& temp_cell)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&temp_cell]() -> auto
		{
			return impl_.GetTemporaryCellType(temp_cell);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetTemporaryCellType(CellType* temp_cell)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([temp_cell]() -> auto
		{
			return impl_.SetTemporaryCellType(temp_cell);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetTemporaryCellTypeFromExisting(uint32_t ct_index)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ct_index]() -> auto
		{
			return impl_.SetTemporaryCellTypeFromExisting(ct_index);
		}, __func__);
	}

	//*****************************************************************************
	//*****************************************************************************
	// Analysis APIs.
	//*****************************************************************************
	//*****************************************************************************
	DLL_CLASS HawkeyeError GetAllAnalysisDefinitions(uint32_t& num_analyses, AnalysisDefinition*& analyses)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_analyses, &analyses]() -> auto
		{
			return impl_.GetAllAnalysisDefinitions(num_analyses, analyses);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetFactoryAnalysisDefinitions(uint32_t& num_ad, AnalysisDefinition*& analyses)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_ad, &analyses]() -> auto
		{
			return impl_.GetFactoryAnalysisDefinitions(num_ad, analyses);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetUserAnalysisDefinitions(uint32_t& num_ad, AnalysisDefinition*& analyses)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_ad, &analyses]() -> auto
		{
			return impl_.GetUserAnalysisDefinitions(num_ad, analyses);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError AddAnalysisDefinition(AnalysisDefinition ad, uint16_t& ad_index)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ad, &ad_index]() -> auto
		//{
		//	return impl_.AddAnalysisDefinition(ad, ad_index);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ModifyBaseAnalysisDefinition(AnalysisDefinition ad, bool clear_specializations)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ad, clear_specializations]() -> auto
		//{
		//	return impl_.ModifyBaseAnalysisDefinition(ad, clear_specializations);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError RemoveAnalysisDefinition(uint16_t ad_index)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ad_index]() -> auto
		//{
		//	return impl_.RemoveAnalysisDefinition(ad_index);
		//}, __func__);
	}

	DLL_CLASS void FreeAnalysisDefinitions(AnalysisDefinition* list, uint32_t num_analyses)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list, num_analyses]() -> auto
		{
			impl_.FreeAnalysisDefinitions(list, num_analyses);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError GetSupportedAnalysisParameterNames(uint32_t& nparameters, char**& parameters)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&nparameters, &parameters]() -> auto
		//{
		//	return impl_.GetSupportedAnalysisParameterNames(nparameters, parameters);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError GetSupportedAnalysisCharacteristics(uint32_t& ncharacteristics, Hawkeye::Characteristic_t*& characteristics)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&ncharacteristics, &characteristics]() -> auto
		//{
		//	return impl_.GetSupportedAnalysisCharacteristics(ncharacteristics, characteristics);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS char* GetNameForCharacteristic(Hawkeye::Characteristic_t c)
	{
		return nullptr;
		//return impl_.getUiDllInstance().entryPoint<char*>([c]() -> auto
		//{
		//	return impl_.GetNameForCharacteristic(c);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError IsAnalysisSpecializedForCellType(uint16_t ad_index, uint32_t ct_index, bool& is_specialized)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ad_index, ct_index, &is_specialized]() -> auto
		//{
		//	return impl_.IsAnalysisSpecializedForCellType(ad_index, ct_index, is_specialized);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError GetAnalysisForCellType(uint16_t ad_index, uint32_t ct_index, AnalysisDefinition*& ad)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ad_index, ct_index, &ad]() -> auto
		{
			return impl_.GetAnalysisForCellType(ad_index, ct_index, ad);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SpecializeAnalysisForCellType(AnalysisDefinition ad, uint32_t ct_index)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&ad, ct_index]() -> auto
		{
			return impl_.SpecializeAnalysisForCellType(ad, ct_index);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError RemoveAnalysisSpecializationForCellType(uint16_t ad_index, uint32_t ct_index)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([ad_index, ct_index]() -> auto
		//{
		//	return impl_.RemoveAnalysisSpecializationForCellType(ad_index, ct_index);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError ExportInstrumentConfiguration(const  char* username, const  char* password, const char * filename)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, filename]() -> auto
		{
			return impl_.ExportInstrumentConfiguration(username, password, filename);
		}, __func__);
	}
	
	DLL_CLASS HawkeyeError ImportInstrumentConfiguration(const  char* username, const  char* password, const char * filename)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, filename]() -> auto
		{
			return impl_.ImportInstrumentConfiguration(username, password, filename);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ExportInstrumentData(const  char* username, const  char* password, 
		                                         uuid__t * rs_uuid_list,
                                                 uint32_t num_uuid,
                                                 const  char* export_location,
                                                 eExportImages exportImages,
                                                 uint16_t export_nth_image,
                                                 export_data_completion_callback onExportCompletionCb,
                                                 export_data_progress_callback exportProgressCb)
	{
		auto wrapcallback1 = impl_.getUiDllInstance().wrapCallback(onExportCompletionCb);
		auto wrapcallback2 = impl_.getUiDllInstance().wrapCallback(exportProgressCb);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		{
			return impl_.ExportInstrumentData(username, password, rs_uuid_list, num_uuid, export_location, exportImages, export_nth_image, wrapcallback1, wrapcallback2);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelExportData()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelExportData();
		}, __func__);
	}


	DLL_CLASS HawkeyeError Export_Start(
		const char* username,
		const char* password,
		uuid__t* rs_uuid_list,		
		uint32_t num_uuid,
		const  char* outPath,
		eExportImages exportImages,
		uint16_t export_nth_image)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		{
			return impl_.Export_Start(username, password, rs_uuid_list, num_uuid, outPath, exportImages, export_nth_image);
		}, __func__);
	}


	DLL_CLASS HawkeyeError  Export_NextMetaData(uint32_t index, uint32_t delayms)
	{
		return impl_.Export_NextMetaData(index, delayms);
	}


	DLL_CLASS HawkeyeError  Export_IsStorageAvailable()
	{
		return impl_.Export_IsStorageAvailable();
	}

	DLL_CLASS HawkeyeError  Export_ArchiveData(const char* filename, char*& outname)
	{
		return impl_.Export_ArchiveData(filename, outname);
	}

	DLL_CLASS HawkeyeError  Export_Cleanup(bool removeFile)
	{
		return impl_.Export_Cleanup(removeFile);
	}



	//Deprecated
	DLL_CLASS HawkeyeError ImportInstrumentData (const char* import_file_location)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([import_file_location]() -> auto
		//{
		//	return impl_.ImportInstrumentData(import_file_location);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError GetQualityControlList(const char* username, const char* password, bool allFlag, QualityControl_t*& qualitycontrols, uint32_t& num_qcs)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&username, &password, &allFlag, &qualitycontrols, &num_qcs]() -> auto
		{
			return impl_.GetQualityControlList(username, password, allFlag, qualitycontrols, num_qcs);
		}, __func__);
	}

	DLL_CLASS HawkeyeError AddQualityControl(const char* username, const char* password, QualityControl_t qualitycontrol, const char* retiredQcName)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([username, password, qualitycontrol, retiredQcName]() -> auto
		{
			return impl_.AddQualityControl(username, password, qualitycontrol, retiredQcName);
		}, __func__);
	}

	DLL_CLASS void FreeListOfQualityControl(QualityControl_t* list, uint32_t n_items)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list, n_items]() -> auto
		{
			impl_.FreeListOfQualityControl(list, n_items);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError GetConcentrationCalibrationStatus(double& slope, double& intercept, uint32_t& cal_image_count, uint64_t& last_calibration_time, uuid__t& queue_id)
	{
		slope = 0;
		intercept = 0;
		cal_image_count = 0;
		last_calibration_time = 0;
		queue_id = {};
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&]() -> auto
		//{
		//	return impl_.GetConcentrationCalibrationStatus(slope, intercept, cal_image_count, last_calibration_time, queue_id);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError GetSizeCalibrationStatus(double& slope, double& intercept, uint64_t& last_calibration_time, uuid__t& queue_id)
	{
		slope = 0;
		intercept = 0;
		last_calibration_time = 0;
		queue_id = {};
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&]() -> auto
		//{
		//	return impl_.GetSizeCalibrationStatus(slope, intercept, last_calibration_time, queue_id);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError AddSignatureDefinition(DataSignature_t* signature)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([signature]() -> auto
		{
			return impl_.AddSignatureDefinition(signature);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RemoveSignatureDefinition(char* signature_short_text, uint16_t short_text_len)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([signature_short_text, short_text_len]() -> auto
		{
			return impl_.RemoveSignatureDefinition(signature_short_text, short_text_len);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetConcentrationCalibration (calibration_type calType, double slope, double intercept, uint32_t cal_image_count, uuid__t queue_id, uint16_t num_consumables, calibration_consumable* consumables)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		{
			return impl_.SetConcentrationCalibration (calType, slope, intercept, cal_image_count, queue_id, num_consumables, consumables);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError SetSizeCalibration(double slope, double intercept, uuid__t queue_id, uint16_t num_consumables, calibration_consumable* consumables)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		//{
		//	return impl_.SetSizeCalibration(slope, intercept, queue_id, num_consumables, consumables);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError StartBrightfieldDustSubtract(brightfield_dustsubtraction_callback on_status_change)
	{
		auto wrapCallback_delete = [](BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState, ImageWrapper_t* iw, uint16_t num_imset, ImageWrapper_t* imset)
		{
			impl_.FreeImageWrapper(iw, 1);
			impl_.FreeImageWrapper(imset, num_imset);
		};
		
		auto wrapCallback = impl_.getUiDllInstance().wrapCallbackWithDeleter
		<BrightfieldDustSubtractWorkflow::eBrightfieldDustSubtractionState, ImageWrapper_t*, uint16_t, ImageWrapper_t*>(on_status_change, wrapCallback_delete);
		
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.StartBrightfieldDustSubtract(wrapCallback, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError AcceptDustReference(bool accepted)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([accepted](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.AcceptDustReference(accepted, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError CancelBrightfieldDustSubtract()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.CancelBrightfieldDustSubtract();
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveAuditTrailLog(uint32_t& num_entries, audit_log_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.RetrieveAuditTrailLogRange(0, 0, num_entries, log_entries, callback);
		}, __func__);
	}

	DLL_CLASS void WriteToAuditLog (const char* username, audit_event_type type, char* resource)
	{
		return impl_.getUiDllInstance().entryPoint<void>([username, type, resource]() -> auto
		{
			impl_.WriteToAuditLog (username, type, resource);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveAuditTrailLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, audit_log_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([starttime, endtime, &num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.RetrieveAuditTrailLogRange(starttime, endtime, num_entries, log_entries, callback);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ArchiveAuditTrailLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([archive_prior_to_time, verification_password, &archive_location](HawkeyeErrorCallback callback) -> auto
		//{
		//	return impl_.ArchiveAuditTrailLog(archive_prior_to_time, verification_password, archive_location, callback);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ReadArchivedAuditTrailLog(char* archive_location, uint32_t& num_entries, audit_log_entry*& log_entries)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([archive_location, &num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		//{
		//	return impl_.ReadArchivedAuditTrailLog(archive_location, num_entries, log_entries, callback);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveInstrumentErrorLog(uint32_t& num_entries, error_log_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.RetrieveInstrumentErrorLogRange(0, 0, num_entries, log_entries, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveInstrumentErrorLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, error_log_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([starttime, endtime, &num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.RetrieveInstrumentErrorLogRange(starttime, endtime, num_entries, log_entries, callback);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ArchiveInstrumentErrorLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([archive_prior_to_time, verification_password, &archive_location](HawkeyeErrorCallback callback) -> auto
		//{
		//	return impl_.ArchiveInstrumentErrorLog(archive_prior_to_time, verification_password, archive_location, callback);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ReadArchivedInstrumentErrorLog(char* archive_location, uint32_t& num_entries, error_log_entry*& log_entries)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([archive_location, &num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		//{
		//	return impl_.ReadArchivedInstrumentErrorLog(archive_location, num_entries, log_entries, callback);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleActivityLog(uint32_t& num_entries, sample_activity_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.RetrieveSampleActivityLogRange(0, 0, num_entries, log_entries, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleActivityLogRange(uint64_t starttime, uint64_t endtime, uint32_t& num_entries, sample_activity_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([starttime, endtime, &num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.RetrieveSampleActivityLogRange(starttime, endtime, num_entries, log_entries, callback);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ArchiveSampleActivityLog(uint64_t archive_prior_to_time, char* verification_password, char*& archive_location)
	{
		return  HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([archive_prior_to_time, verification_password, &archive_location](HawkeyeErrorCallback callback) -> auto
		//{
		//	return impl_.ArchiveSampleActivityLog(archive_prior_to_time, verification_password, archive_location, callback);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ReadArchivedSampleActivityLog(char* archive_location, uint32_t& num_entries, sample_activity_entry*& log_entries)
	{
		return  HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([archive_location, &num_entries, &log_entries](HawkeyeErrorCallback callback) -> auto
		//{
		//	return impl_.ReadArchivedSampleActivityLog(archive_location, num_entries, log_entries, callback);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveCalibrationActivityLog(calibration_type cal_type, uint32_t& num_entries, calibration_history_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([cal_type, &num_entries, &log_entries]() -> auto
		{
			return impl_.RetrieveCalibrationActivityLogRange(cal_type, 0, 0, num_entries, log_entries);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveCalibrationActivityLogRange(calibration_type cal_type, uint64_t starttime, uint64_t endtime, uint32_t& num_entries, calibration_history_entry*& log_entries)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &num_entries, &log_entries]() -> auto
		{
			return impl_.RetrieveCalibrationActivityLogRange(cal_type, starttime, endtime, num_entries, log_entries);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ClearCalibrationActivityLog(calibration_type cal_type, uint64_t archive_prior_to_time, char* verification_password, bool clearAllACupData)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		{
			return impl_.ClearCalibrationActivityLog(cal_type, archive_prior_to_time, verification_password, clearAllACupData);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError ClearExportDataFolder(char* optional_subfolder, char* confirmation_password)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([optional_subfolder, confirmation_password]() -> auto
		//{
		//	return impl_.ClearExportDataFolder(optional_subfolder, confirmation_password);
		//}, __func__);
	}

	//*****************************************************************************
	//*****************************************************************************
	// Service APIs.
	//*****************************************************************************
	//*****************************************************************************
	DLL_CLASS HawkeyeError svc_SetSystemSerialNumber(char* serial, char* service_password)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([serial, service_password](HawkeyeErrorCallback onComplete) -> auto
		{
			return impl_.svc_SetSystemSerialNumber(serial, service_password, onComplete);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_CameraFocusAdjust(bool direction_up, bool adjustment_fine)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([direction_up, adjustment_fine](HawkeyeErrorCallback onComplete) -> auto
		{
			return impl_.svc_CameraFocusAdjust(onComplete, direction_up, adjustment_fine);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_CameraFocusRestoreToSavedLocation()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([](HawkeyeErrorCallback onComplete) -> auto
		{
			return impl_.svc_CameraFocusRestoreToSavedLocation(onComplete);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_CameraFocusStoreCurrentPosition(char* password)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([password](HawkeyeErrorCallbackonComplete) -> auto
		//{
		//	return impl_.svc_CameraFocusStoreCurrentPosition(onComplete, password);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError svc_CameraAutoFocus(SamplePosition focusbead_location, autofocus_state_callback_t on_status_change, countdown_timer_callback_t on_timer_tick)
	{
		auto on_status_change_delete = [](eAutofocusState, AutofocusResults* afr)
		{
			impl_.FreeAutofocusResults(afr, 1);
		};
		
		auto on_status_change_DLL = impl_.getUiDllInstance().wrapCallbackWithDeleter
		<eAutofocusState, AutofocusResults*>(on_status_change, on_status_change_delete);

		auto on_timer_tick_DLL = impl_.getUiDllInstance().wrapCallback(on_timer_tick);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([focusbead_location, on_status_change_DLL, on_timer_tick_DLL](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_CameraAutoFocus(focusbead_location, on_status_change_DLL, on_timer_tick_DLL, callback);
		}, __func__);
	}
	DLL_CLASS HawkeyeError svc_CameraAutoFocus_FocusAcceptance(eAutofocusCompletion decision)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([decision](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_CameraAutoFocus_FocusAcceptance(decision, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_CameraAutoFocus_ServiceSkipDelay()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_CameraAutoFocus_ServiceSkipDelay();
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_CameraAutoFocus_Cancel()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_CameraAutoFocus_Cancel();
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GetFlowCellDepthSetting(uint16_t& flow_cell_depth)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&flow_cell_depth]() -> auto
		{
			return impl_.svc_GetFlowCellDepthSetting(flow_cell_depth);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_SetFlowCellDepthSetting(uint16_t flow_cell_depth)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([flow_cell_depth]() -> auto
		{
			return impl_.svc_SetFlowCellDepthSetting(flow_cell_depth);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GetValvePort(char & port)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&port]() -> auto
		{
			return impl_.svc_GetValvePort(port);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_SetValvePort(const char port)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&port](HawkeyeErrorCallback onComplete) -> void {
//TODO: add SyringePumpDirection to svc_SetValvePort...
			impl_.svc_SetValvePort (onComplete, port, SyringePumpDirection::Clockwise);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_GetSyringePumpPostion(uint32_t & pos)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&pos]() -> auto
		//{
		//	return impl_.svc_GetSyringePumpPostion (pos);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError svc_AspirateSample(uint32_t volume)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&volume](HawkeyeErrorCallback onComplete) -> void {
			impl_.svc_AspirateSample (onComplete, volume);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_DispenseSample(uint32_t volume)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([volume](HawkeyeErrorCallback onComplete) -> void {
			impl_.svc_DispenseSample (onComplete, volume);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GetProbePostion(int32_t & pos)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&pos]() -> auto
		{
			return impl_.svc_GetProbePostion(pos);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_SetProbePostion(bool upDown, uint32_t stepsToMove)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([upDown, stepsToMove](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_SetProbePostion(upDown, stepsToMove, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GetSampleWellPosition(SamplePosition& pos)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&pos]() -> auto
		{
			return impl_.svc_GetSampleWellPosition(pos);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_SetSampleWellPosition(SamplePosition pos)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([pos](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_SetSampleWellPosition(pos, callback);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_PerformValveRepetition()
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([](HawkeyeErrorCallbackonComplete) -> void {
		//	impl_.svc_PerformValveRepetition (onComplete);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_PerformSyringeRepetition()
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([](HawkeyeErrorCallbackonComplete) -> void {
		//	impl_.svc_PerformSyringeRepetition (onComplete);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_PerformFocusRepetition()
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([](HawkeyeErrorCallbackonComplete) -> auto
		//{
		//	return impl_.svc_PerformFocusRepetition(onComplete);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_PerformReagentRepetition()
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([](HawkeyeErrorCallbackonComplete) -> void {
		//	impl_.svc_PerformReagentRepetition (onComplete);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError svc_MoveReagentArm(bool up)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([up](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.svc_MoveReagentArm(up, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError InitializeCarrier()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.InitializeCarrier(cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_MoveProbe(bool up)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([up](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.svc_MoveProbe(up, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_PerformPlateCalibration(motor_calibration_state_callback onCalibStateChangeCb)
	{
		auto wrapCallback = impl_.getUiDllInstance().wrapCallback(onCalibStateChangeCb);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback]() -> auto
		{
			return impl_.svc_PerformPlateCalibration(wrapCallback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_PerformCarouselCalibration(motor_calibration_state_callback onCalibStateChangeCb)
	{
		auto wrapCallback = impl_.getUiDllInstance().wrapCallback(onCalibStateChangeCb);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback]() -> auto
		{
			return impl_.svc_PerformCarouselCalibration(wrapCallback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_CancelCalibration(motor_calibration_state_callback onCalibStateChangeCb)
	{
		auto wrapCallback = impl_.getUiDllInstance().wrapCallback(onCalibStateChangeCb);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback]() -> auto
		{
			return impl_.svc_CancelCalibration(wrapCallback);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_GetStageCalibrationBacklash(int32_t& thetaBacklash, int32_t& radiusBacklash)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&thetaBacklash, &radiusBacklash]() -> auto
		//{
		//	return impl_.svc_GetStageCalibrationBacklash(thetaBacklash, radiusBacklash);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_SetStageCalibrationBacklash(int32_t thetaBacklash, int32_t radiusBacklash)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([thetaBacklash, radiusBacklash]() -> auto
		//{
		//	return impl_.svc_SetStageCalibrationBacklash(thetaBacklash, radiusBacklash);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GetCameraLampState(bool& lamp_on, float& intensity_0_to_100)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&lamp_on, &intensity_0_to_100](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_GetCameraLampState (lamp_on, intensity_0_to_100, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_SetCameraLampState(bool lamp_on, float intensity_0_to_25)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([lamp_on, intensity_0_to_25](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_SetCameraLampState(lamp_on, intensity_0_to_25, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_SetCameraLampIntensity(float intensity_0_to_100)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([intensity_0_to_100](HawkeyeErrorCallback callback) -> auto
		{
			return impl_.svc_SetCameraLampIntensity(intensity_0_to_100, callback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GenerateSingleShotAnalysis(service_analysis_result_callback callback)
	{
		auto wrapCallback_delete = [](HawkeyeError, BasicResultAnswers, ImageWrapper_t* iw) {impl_.FreeImageWrapper(iw, 1); };
		auto wrapCallback = impl_.getUiDllInstance().wrapCallbackWithDeleter
		<HawkeyeError, BasicResultAnswers, ImageWrapper_t*>(callback, wrapCallback_delete);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback]() -> auto
		{
			return impl_.svc_GenerateSingleShotAnalysis(wrapCallback);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError svc_SetAnalysisImagePreference(eImageOutputType image_type)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([image_type](HawkeyeErrorCallbackonComplete) -> void {
		//	impl_.svc_SetAnalysisImagePreference(image_type, onComplete);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError svc_GenerateContinuousAnalysis(service_analysis_result_callback callback)
	{
		auto wrapCallback_delete = [](HawkeyeError, BasicResultAnswers, ImageWrapper_t* iw) {impl_.FreeImageWrapper(iw, 1); };
		auto wrapCallback = impl_.getUiDllInstance().wrapCallbackWithDeleter
			<HawkeyeError, BasicResultAnswers, ImageWrapper_t*>(callback, wrapCallback_delete);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback]() -> auto
		{
			return impl_.svc_GenerateContinuousAnalysis(wrapCallback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_StopContinuousAnalysis()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_StopContinuousAnalysis();
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_StartLiveImageFeed(service_live_image_callback liveImageCallback)
	{
		auto wrapCallback_delete = [](HawkeyeError, ImageWrapper_t* iw) {
			impl_.FreeImageWrapper(iw, 1);
		};
		
		auto wrapCallback = impl_.getUiDllInstance().wrapCallbackWithDeleter
			<HawkeyeError, ImageWrapper_t*>(liveImageCallback, wrapCallback_delete);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([wrapCallback](HawkeyeErrorCallback functionCallback) -> auto
		{
			return impl_.svc_StartLiveImageFeed (wrapCallback, functionCallback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_StopLiveImageFeed()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_StopLiveImageFeed();
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_ManualSample_Load()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_ManualSample_Load();
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_ManualSample_Nudge()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_ManualSample_Nudge();
		}, __func__);
	}

	DLL_CLASS HawkeyeError svc_ManualSample_Expel()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.svc_ManualSample_Expel();
		}, __func__);
	}
	
	DLL_CLASS void FreeListOfResultSummary(ResultSummary* list_, uint32_t n_items)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list_, n_items]() -> auto
		{
			impl_.FreeListOfResultSummary(list_, n_items);
		}, __func__);
	}
	
	DLL_CLASS void FreeListOfResultRecord(ResultRecord* list_, uint32_t n_items)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list_, n_items]() -> auto
		{
			impl_.FreeListOfResultRecord(list_, n_items);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS void FreeListOfImageRecord(ImageRecord* list_, uint32_t n_items)
	{
		return;
		//return impl_.getUiDllInstance().entryPoint<void>([list_, n_items]() -> auto
		//{
		//	impl_.FreeListOfImageRecord(list_, n_items);
		//}, __func__);
	}
	
	DLL_CLASS void FreeListOfImageSetRecord(SampleImageSetRecord* list_, uint32_t n_items)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list_, n_items]() -> auto
		{
			impl_.FreeListOfImageSetRecord(list_, n_items);
		}, __func__);
	}
	
	DLL_CLASS void FreeListOfSampleRecord (SampleRecord* list_, uint32_t n_items)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list_, n_items]() -> auto
		{
			impl_.FreeListOfSampleRecord(list_, n_items);
		}, __func__);
	}
	
	DLL_CLASS void FreeListOfWorklistRecord (WorklistRecord* list_, uint32_t n_items)
	{
		return impl_.getUiDllInstance().entryPoint<void>([list_, n_items]() -> auto
		{
			impl_.FreeListOfWorklistRecord (list_, n_items);
		}, __func__);
	}

	DLL_CLASS void FreeDetailedResultMeasurement (DetailedResultMeasurements* meas)
	{
		return impl_.getUiDllInstance().entryPoint<void>([meas]() -> auto
		{
			impl_.FreeDetailedResultMeasurement(meas);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveWorklist (uuid__t id, WorklistRecord*& rec)
	{
		return HawkeyeError::eEntryNotFound;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &rec](HawkeyeErrorCallback cb) -> auto
		//{
		//	return impl_.RetrieveWorklist(id, rec, cb);
		//}, __func__);
	}
	DLL_CLASS HawkeyeError RetrieveWorklists (uint64_t start, uint64_t end, char* username, WorklistRecord*& reclist, uint32_t& list_size)
	{
		return HawkeyeError::eEntryNotFound;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &reclist, &list_size](HawkeyeErrorCallback cb) -> auto
		//{
		//	return impl_.RetrieveWorklists(start, end, username, reclist, list_size, cb);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveWorklistList(uuid__t* ids, uint32_t list_size, WorklistRecord*& recs, uint32_t& retrieved_size)
	{
		return HawkeyeError::eEntryNotFound;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_size](HawkeyeErrorCallback cb) -> auto
		//{
		//	return impl_.RetrieveWorklistList(ids, list_size, recs, retrieved_size, cb);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleRecord(uuid__t id, SampleRecord*& rec)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &rec](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleRecord(id, rec, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleRecords(uint64_t start, uint64_t end, char* username, SampleRecord*& reclist, uint32_t& list_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &reclist, &list_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleRecords(start, end, username, reclist, list_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleRecordList(uuid__t* ids, uint32_t list_size, SampleRecord*& recs, uint32_t& retrieved_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleRecordList(ids, list_size, recs, retrieved_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleImageSetRecord(uuid__t id, SampleImageSetRecord*& rec)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &rec](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleImageSetRecord(id, rec,cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleImageSetRecords(uint64_t start, uint64_t end, char* username, SampleImageSetRecord*& reclist, uint32_t& list_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &reclist, &list_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleImageSetRecords(start, end, username, reclist, list_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleImageSetRecordList(uuid__t* ids, uint32_t list_size, SampleImageSetRecord*& recs, uint32_t& retrieved_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleImageSetRecordList(ids, list_size, recs, retrieved_size, cb);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError RetrieveImageRecord(uuid__t id, ImageRecord*& rec)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &rec](HawkeyeErrorCallback cb) -> auto
		//{
		//	return impl_.RetrieveImageRecord(id, rec, cb);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError RetrieveImageRecords(uint64_t start, uint64_t end, char* username, ImageRecord*& reclist, uint32_t& list_size)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &reclist, &list_size](HawkeyeErrorCallback cb) -> auto
		//{
		//	return impl_.RetrieveImageRecords(start, end, username, reclist, list_size,cb);
		//}, __func__);
	}

	//Deprecated
	DLL_CLASS HawkeyeError RetrieveImageRecordList(uuid__t* ids, uint32_t list_size, ImageRecord*& recs, uint32_t& retrieved_size)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_size](HawkeyeErrorCallback cb) -> auto
		//{
		//	return impl_.RetrieveImageRecordList(ids, list_size, recs, retrieved_size,cb);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveResultRecord(uuid__t id, ResultRecord*& rec)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &rec](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveResultRecord(id, rec, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveResultRecords(uint64_t start, uint64_t end, char* username, ResultRecord*& reclist, uint32_t& list_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &reclist, &list_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveResultRecords(start, end, username, reclist, list_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveResultRecordList(uuid__t* ids, uint32_t list_size, ResultRecord*& recs, uint32_t& retrieved_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveResultRecordList(ids, list_size, recs, retrieved_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveResultSummaryRecord (uuid__t id, ResultSummary*& rec)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &rec](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveResultSummaryRecord (id, rec, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveResultSummaryRecordList (uuid__t* ids, uint32_t list_size, ResultSummary*& recs, uint32_t& retrieved_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveResultSummaryRecordList (ids, list_size, recs, retrieved_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveDetailedMeasurementsForResultRecord(uuid__t id, DetailedResultMeasurements*& measurements)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &measurements](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveDetailedMeasurementsForResultRecord(id, measurements, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveImage(uuid__t id, ImageWrapper_t*& img)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([id, &img](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveImage(id, img, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveAnnotatedImage(uuid__t result_id, uuid__t image_id, ImageWrapper_t*& img)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([result_id, image_id, &img](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveAnnotatedImage(result_id, image_id, img, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveBWImage (uuid__t image_id, ImageWrapper_t*& img)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([image_id, &img](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveBWImage (image_id, img, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError ReanalyzeSample(uuid__t sample_id, uint32_t celltype_index, uint16_t analysis_index, bool from_images, sample_analysis_callback onSampleComplete)
	{
		auto wrapCallback_delete = [](HawkeyeError, uuid__t, ResultRecord* rec) {impl_.FreeListOfResultRecord(rec, 1); };
		auto wrapCallback = impl_.getUiDllInstance().wrapCallbackWithDeleter
		<HawkeyeError, uuid__t, ResultRecord*>(onSampleComplete, wrapCallback_delete);

		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		{
			return impl_.ReanalyzeSample(sample_id, celltype_index, analysis_index, from_images, wrapCallback);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveHistogramForResultRecord(uuid__t id, bool POI, Hawkeye::Characteristic_t measurement, uint8_t& bin_count, histogrambin_t*& bins)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &bin_count, &bins](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveHistogramForResultRecord(id, POI, measurement, bin_count, bins, cb);
		}, __func__);
	}
	
	DLL_CLASS HawkeyeError FreeHistogramBins(histogrambin_t* list)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([list]() -> auto
		{
			return impl_.FreeHistogramBins(list);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSampleRecordsForQualityControl(const char* QC_name, SampleRecord*& reclist, uint32_t& list_size)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([QC_name, &reclist, &list_size](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.RetrieveSampleRecordsForBPandQC(QC_name, reclist, list_size, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError RetrieveSignatureDefinitions(DataSignature_t*& signatures, uint16_t& num_signatures)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&signatures, &num_signatures]() -> auto
		{
			return impl_.RetrieveSignatureDefinitions(signatures, num_signatures);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SignResultRecord(uuid__t record_id, char* signature_short_text, uint16_t short_text_len)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([record_id, signature_short_text, short_text_len](HawkeyeErrorCallback cb) -> auto
		{
			return impl_.SignResultRecord(record_id, signature_short_text, short_text_len, cb);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetRootFolderForExportData(char*& folder_location)
	{
		return HawkeyeError::eDeprecated;
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&folder_location]() -> auto
		//{
		//	return impl_.GetRootFolderForExportData(folder_location);
		//}, __func__);
	}

	DLL_CLASS void FreeImageSetWrapper(ImageSetWrapper_t*& image_set_wrapper, uint16_t num_image_set_wrapper)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&image_set_wrapper, num_image_set_wrapper]() -> auto
		{
			impl_.FreeImageSetWrapper(image_set_wrapper, num_image_set_wrapper);
		}, __func__);
	}

	DLL_CLASS void FreeImageWrapper(ImageWrapper_t*& image_wrapper, uint16_t num_image_wrapper)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&image_wrapper, num_image_wrapper]() -> auto
		{
			impl_.FreeImageWrapper(image_wrapper, num_image_wrapper);
		}, __func__);
	}

	DLL_CLASS void FreeAutofocusResults(AutofocusResults*& results, uint8_t num_result)
	{
		return impl_.getUiDllInstance().entryPoint<void>([&results, num_result]() -> auto
		{
			impl_.FreeAutofocusResults(results, num_result);
		}, __func__);
	}

	DLL_CLASS void FreeAuditLogEntry(audit_log_entry* entries, uint32_t num_entries)
	{
		return impl_.getUiDllInstance().entryPoint<void>([entries, num_entries]() -> auto
		{
			impl_.FreeAuditLogEntry(entries, num_entries);
		}, __func__);
	}

	DLL_CLASS void FreeSampleWorklistEntry (worklist_sample_entry* entries, uint32_t num_entries)
	{
		return impl_.getUiDllInstance().entryPoint<void>([entries, num_entries]() -> auto
		{
			impl_.freeSampleWorklistEntry (entries, num_entries);
		}, __func__);
	}

	DLL_CLASS void FreeSampleActivityEntry(sample_activity_entry* entries, uint32_t num_entries)
	{
		return impl_.getUiDllInstance().entryPoint<void>([entries, num_entries]() -> auto
		{
			impl_.FreeSampleActivityEntry(entries, num_entries);
		}, __func__);
	}

	DLL_CLASS void FreeCalibrationHistoryEntry(calibration_history_entry* entries, uint32_t num_entries)
	{
		return impl_.getUiDllInstance().entryPoint<void>([entries, num_entries]() -> auto
		{
			impl_.FreeCalibrationHistoryEntry(entries, num_entries);
		}, __func__);
	}

	DLL_CLASS void FreeErrorLogEntry(error_log_entry* entries, uint32_t num_entries)
	{
		return impl_.getUiDllInstance().entryPoint<void>([entries, num_entries]() -> auto
		{
			impl_.FreeErrorLogEntry(entries, num_entries);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS void FreeCalibrationConsumable(calibration_consumable* cc, uint32_t num_cc)
	{
		return;
		//return impl_.getUiDllInstance().entryPoint<void>([cc, num_cc]() -> auto
		//{
		//	impl_.FreeCalibrationConsumable(cc, num_cc);
		//}, __func__);
	}

	DLL_CLASS void FreeDataSignature(DataSignature_t* signatures, uint16_t num_signatures)
	{
		return impl_.getUiDllInstance().entryPoint<void>([signatures, num_signatures]() -> auto
		{
			impl_.FreeDataSignature(signatures, num_signatures);
		}, __func__);
	}

	//Deprecated
	DLL_CLASS void FreeDataSignatureInstance(DataSignatureInstance_t* signatures, uint16_t num_signatures)
	{
		return;
		//return impl_.getUiDllInstance().entryPoint<void>([signatures, num_signatures]() -> auto
		//{
		//	return impl_.FreeDataSignatureInstance(signatures, num_signatures);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError DeleteSampleRecord(const char* username, const char* password, uuid__t * wqi_uuidlist, uint32_t num_uuid, bool delete_images_and_results, delete_results_callback onDeleteCompletion)
	{
		auto wrapcallback = impl_.getUiDllInstance().wrapCallback (onDeleteCompletion);
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &username, &password](HawkeyeErrorCallback onCompleteCallback) -> auto
		{
			impl_.DeleteSampleRecord (username, password, wqi_uuidlist, num_uuid, delete_images_and_results, wrapcallback, onCompleteCallback);
		}, __func__, true);
	}


	//Deprecated
	DLL_CLASS HawkeyeError DeleteResultRecord(uuid__t * rr_uuidlist, uint32_t num_uuid, bool cleanup_if_deletinglastresult, delete_results_callback onDeleteCompletion)
	{
		return HawkeyeError::eDeprecated;
		//auto wrapcallback = impl_.getUiDllInstance().wrapCallback(onDeleteCompletion);
		//return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=]() -> auto
		//{
		//	return impl_.DeleteResultRecord(rr_uuidlist, num_uuid, cleanup_if_deletinglastresult, wrapcallback);
		//}, __func__);
	}

	DLL_CLASS HawkeyeError ValidateActiveDirConfig (ActiveDirectoryConfig cfg, const char* adminGroup, const char* uName, const char* password, bool& valid)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg, &adminGroup, &uName, &password, &valid]() -> auto
		{
			return impl_.ValidateActiveDirConfig (cfg, adminGroup, uName, password, valid);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetActiveDirConfig (ActiveDirectoryConfig cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			impl_.SetActiveDirConfig (cfg);
			return HawkeyeError::eSuccess;
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetActiveDirConfig (ActiveDirectoryConfig*& cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			impl_.GetActiveDirConfig (cfg);
			return HawkeyeError::eSuccess;
		}, __func__);
	}

	DLL_CLASS void FreeActiveDirConfig(ActiveDirectoryConfig* cfg)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &cfg]() -> auto
		{
			impl_.FreeActiveDirConfig (cfg);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetActiveDirectoryGroupMaps (ActiveDirectoryGroup*& recs, uint32_t& retrieved_count)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &retrieved_count]() -> auto
		{
			impl_.GetActiveDirectoryGroupMaps (recs, retrieved_count);
			return HawkeyeError::eSuccess;
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetActiveDirectoryGroupMaps (ActiveDirectoryGroup* recs, uint32_t count)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &recs, &count]() -> auto
		{
			impl_.SetActiveDirectoryGroupMaps (recs, count);
			return HawkeyeError::eSuccess;
		}, __func__);
	}

	DLL_CLASS void FreeActiveDirGroupMaps(ActiveDirectoryGroup* recs, uint32_t count)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &recs, &count]() -> auto
		{
			impl_.FreeActiveDirGroupMaps(recs, count);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetDBConfig(DBConfig cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			return impl_.SetDBConfig (cfg);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetDBConfig(DBConfig*& cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			return impl_.GetDBConfig (cfg);
		}, __func__);
	}

	DLL_CLASS void FreeDBConfig(DBConfig* cfg)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &cfg]() -> auto
		{
			impl_.FreeDBConfig (cfg);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetDbBackupUserPassword( const char* password )
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>( [password]() -> auto
		{
			return impl_.SetDbBackupUserPassword( password );
		}, __func__ );
	}

	DLL_CLASS HawkeyeError SetSMTPConfig (SMTPConfig cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			return impl_.SetSMTPConfig (cfg);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetSMTPConfig(SMTPConfig*& cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			return impl_.GetSMTPConfig(cfg);
		}, __func__);
	}

	DLL_CLASS void FreeSMTPConfig (SMTPConfig* cfg)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &cfg]() -> auto
		{
			impl_.FreeSMTPConfig(cfg);
		}, __func__);
	}

	DLL_CLASS HawkeyeError SetAutomationSettings (AutomationConfig cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			return impl_.SetAutomationSettings (cfg);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetAutomationSettings (AutomationConfig*& cfg)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &cfg]() -> auto
		{
			return impl_.GetAutomationSettings (cfg);
		}, __func__);
	}

	DLL_CLASS void FreeAutomationSettings(AutomationConfig* cfg)
	{
		return impl_.getUiDllInstance().entryPoint<void>([=, &cfg]() -> auto
		{
			impl_.FreeAutomationSettings(cfg);
		}, __func__);
	}

	DLL_CLASS bool SystemHasData()
	{
		return impl_.getUiDllInstance().entryPoint<bool>([]() -> auto
		{
			return impl_.SystemHasData();
		}, __func__);
	}

	DLL_CLASS HawkeyeError AddScheduledExport(ScheduledExport scheduled_export, uuid__t *uuid)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &scheduled_export]() -> auto
		{
			return impl_.AddScheduledExport(scheduled_export, uuid);
		}, __func__);
	}

	DLL_CLASS HawkeyeError EditScheduledExport(ScheduledExport scheduled_export)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &scheduled_export]() -> auto
		{
			return impl_.EditScheduledExport(scheduled_export);
		}, __func__);
	}

	DLL_CLASS HawkeyeError DeleteScheduledExport(uuid__t uuid)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &uuid]() -> auto
		{
			return impl_.DeleteScheduledExport(uuid);
		}, __func__);
	}

	DLL_CLASS HawkeyeError GetScheduledExports(eScheduledExportType export_type, ScheduledExport*& scheduled_exports, uint32_t& count)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &scheduled_exports, &count]() -> auto
		{
			return impl_.GetScheduledExports(export_type, scheduled_exports, count);
		}, __func__);
	}

	DLL_CLASS HawkeyeError FreeListOfScheduledExports(ScheduledExport* scheduled_exports, uint32_t count)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([=, &scheduled_exports, &count]() -> auto
		{
			return impl_.FreeListOfScheduledExports(scheduled_exports, count);
		}, __func__);
	}

	DLL_CLASS void UseCarouselSimulation (bool state)
	{
		HawkeyeConfig::Instance().get().isSimulatorCarousel = state;
	}

	DLL_CLASS void ShutdownOrReboot (ShutdownOrRebootEnum operation)
	{
		return impl_.getUiDllInstance().entryPoint<void>([operation]() -> auto
		{
			impl_.ShutdownOrReboot(operation);
		}, __func__);
	}

	DLL_CLASS HawkeyeError DeleteCampaignData()
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([]() -> auto
		{
			return impl_.DeleteCampaignData();
		}, __func__);
	}	

	DLL_CLASS HawkeyeError SetOpticalHardwareConfig(OpticalHardwareConfig type)
	{
		return impl_.getUiDllInstance().entryPoint<HawkeyeError>([&type]() -> auto
		{
			return impl_.SetOpticalHardwareConfig(type);
		}, __func__);
	}

	DLL_CLASS OpticalHardwareConfig GetOpticalHardwareConfig()
	{
		return impl_.getUiDllInstance().entryPoint<OpticalHardwareConfig>([]() -> auto
		{
			return impl_.GetOpticalHardwareConfig();
		}, __func__);
	}
	
} // End "extern "C""
