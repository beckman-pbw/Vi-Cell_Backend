#include "stdafx.h"

#include "AuditLog.hpp"
#include "CalibrationHistoryDLL.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_Calibration";

//*****************************************************************************
bool HawkeyeLogicImpl::initializeCalibrationHistory() {

	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeCalibration: <enter>");

	bool status = CalibrationHistoryDLL::Instance().Initialize();

	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeCalibration: <exit>");

	return status;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetConcentrationCalibration (
	calibration_type calType,
	double slope, 
	double intercept,
	uint32_t cal_image_count, 
	uuid__t queue_id, 
	uint16_t num_consumables, 
	calibration_consumable* consumables) {

	Logger::L().Log (MODULENAME, severity_level::debug1, "SetConcentrationCalibration: <enter>");

	// limited to local logins only
	HawkeyeError he = UserList::Instance().CheckConsoleUserPermissionAtLeast( UserPermissionLevel::eService );
	if (he != HawkeyeError::eSuccess)
	{
		if (he == HawkeyeError::eNotPermittedAtThisTime)
		{
			return he;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Set Concentration Calibration"));
		return HawkeyeError::eNotPermittedByUser;
	}

	calibration_history_entry ch = {};

	DataConversion::convertToCharPointer(ch.username, UserList::Instance().GetLoggedInUsername());
	ch.cal_type = calType;
	ch.slope = slope;
	ch.intercept = intercept;
	ch.image_count = cal_image_count;
	ch.uuid = queue_id;
	ch.num_consumables = num_consumables;
	ch.consumable_list = consumables;

	CalibrationDataDLL chDLL{ ch };

	chDLL.timestamp = ChronoUtilities::CurrentTime();

	he = CalibrationHistoryDLL::Instance().SetConcentrationCalibration (chDLL);

	Logger::L().Log (MODULENAME, severity_level::debug1, "SetConcentrationCalibration: <exit>");

	return he;
}

//*****************************************************************************
// This method takes advance of the chronological order of the CalibrationHistory.info file.
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::RetrieveCalibrationActivityLogRange (calibration_type cal_type, 
                                                                    uint64_t starttime, 
                                                                    uint64_t endtime, 
                                                                    uint32_t& num_entries, 
                                                                    calibration_history_entry*& log_entries) {

	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveCalibrationActivityLogRange: <enter>");

	// Protect against UI call the API during initialization.
	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (Logger::L().IsOfInterest(severity_level::debug1))
	{
		std::string output = "Calibration Log Retrieval\n";
		switch (cal_type)
		{
			case calibration_type::cal_All: output += "Type: All\n"; break;
			case calibration_type::cal_Concentration: output += "Type: Concentration\n"; break;
			case calibration_type::cal_Size: output += "Type: Size\n"; break;
			case calibration_type::cal_ACupConcentration: output += "Type: A-Cup\n"; break;
			default: output += "Type: Unknown\n"; break;
		}

		Logger::L().Log (MODULENAME, severity_level::debug1, boost::str(boost::format("%sStart: %d\nEnd: %d") % output % starttime % endtime));
	}

	std::vector<CalibrationDataDLL> validMatchList;

	// If starttime is zero, that means "from beginning of history".
	// If endtime is zero, that means "until end of history".
	
	for (const auto & it : CalibrationHistoryDLL::Instance().Get()) 
	{
		// Not interested in this calibration type?
		if ((cal_type != calibration_type::cal_All) && 
			(it.type != cal_type))
			continue;

		auto timeStamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(it.timestamp);

		// Before requested time?
		if ((starttime != 0) && 
			(timeStamp < starttime))
			continue;

		// After requested time?
		if ((endtime != 0) && 
			(timeStamp > endtime))
			continue;

		validMatchList.push_back (it);
	}

	// Sort the calibration history newest to oldest.
	std::sort (validMatchList.rbegin(), validMatchList.rend(),
		[](const CalibrationDataDLL& a, const CalibrationDataDLL& b)
	{
		return (a.timestamp < b.timestamp);
	});

	int index = 0;
	num_entries = (uint32_t)validMatchList.size();
	log_entries = new calibration_history_entry[num_entries]();
	for (auto logEntry : validMatchList) {
		log_entries[index] = logEntry.ToCStyle();
		index++;
	}
	
	Logger::L().Log (MODULENAME, severity_level::debug1, "RetrieveCalibrationActivityLogRange: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// This clears all entries of the specified *type*, except the *factory defaults*
// and the most recent entry of the specified *type*.
// This method takes advantage of the chronological order of the CalibrationHistory.info file.
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ClearCalibrationActivityLog (calibration_type cal_type, uint64_t clear_prior_to_time, char* verification_password, bool clearAllACupData) {

	Logger::L().Log (MODULENAME, severity_level::debug1, "ClearCalibrationActivityLog: <enter>");

	std::string loggedInUsername = UserList::Instance().GetAttributableUserName();
//TODO:	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	// This must be done by Elevated User.
	// limited to local logins only
	HawkeyeError he = UserList::Instance().CheckConsoleUserPermissionAtLeast( UserPermissionLevel::eElevated );
	if (he != HawkeyeError::eSuccess) {
		if (he == HawkeyeError::eNotPermittedAtThisTime)
		{
			return he;
		}

		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername, 
			audit_event_type::evt_notAuthorized, 
			"Clear Calibration Activity Log"));
		return HawkeyeError::eNotPermittedByUser;
	}

	std::string logmsg;
	switch (cal_type)
	{
		case calibration_type::cal_All: logmsg += "Concentration and Sizing slope logs cleared"; break;
		case calibration_type::cal_Concentration: logmsg += "Concentration slope logs cleared"; break;
		case calibration_type::cal_Size: logmsg += "Sizing slope logs cleared"; break;
		case calibration_type::cal_ACupConcentration: logmsg += "A-Cup Concentration slope logs cleared"; break;
		case calibration_type::cal_UNKNOWN:
		default: logmsg += "Type: Unknown"; break;
	}

	const std::string ptt = ChronoUtilities::ConvertToString (ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(clear_prior_to_time));

	logmsg += " prior to " + ptt + " UTC";

	if (Logger::L().IsOfInterest (severity_level::debug1))
	{
		std::string output = "Calibration Log Clear\n";
		switch (cal_type)
		{
			case calibration_type::cal_All: output += "Type: All\n"; break;
			case calibration_type::cal_Concentration: output += "Type: Concentration\n"; break;
			case calibration_type::cal_Size: output += "Type: Size\n"; break;
			case calibration_type::cal_ACupConcentration: output += "Type: A-Cup Concentration\n"; break;
			default: output += "Type: Unknown\n"; break;
		}
		output.append(boost::str(boost::format("Prior To: %lld") % clear_prior_to_time));
		Logger::L().Log (MODULENAME, severity_level::debug1, output);
	}

	he = ValidateMe (verification_password);
	if (he != HawkeyeError::eSuccess) {
		return he;
	}

	he = CalibrationHistoryDLL::Instance().ClearCalibrationData (cal_type, clear_prior_to_time, clearAllACupData);
	if (he == HawkeyeError::eSuccess)
	{
		AuditLogger::L().Log (generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_clearedcalibrationfactors,
			logmsg));

		if (!CalibrationHistoryDLL::Instance().LoadCurrentCalibration()) {
			he = HawkeyeError::eSoftwareFault;
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug1, "ClearCalibrationActivityLog: <exit>");

	return he;
}

//*****************************************************************************
static void freeCalibrationConsumable (calibration_consumable* cc, uint32_t num_cc)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "freeCalibrationConsumable: <enter>");

	if (cc == nullptr)
	{
		return;
	}

	for (uint32_t index = 0; index < num_cc; index++)
	{
		delete[] cc[index].label;
		delete[] cc[index].lot_id;
	}

	delete[] cc;
	cc = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "freeCalibrationConsumable: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeCalibrationHistoryEntry (calibration_history_entry* entries, uint32_t num_entries)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeCalibrationHistoryEntry: <enter>");

	if (entries == nullptr)
	{
		return;
	}

	for (uint32_t index = 0; index < num_entries; index++)
	{
		delete[] entries[index].username;

		freeCalibrationConsumable (entries[index].consumable_list, entries[index].num_consumables);
	}

	delete[] entries;
	entries = nullptr;

	Logger::L().Log (MODULENAME, severity_level::debug1, "FreeCalibrationHistoryEntry: <exit>");
}

