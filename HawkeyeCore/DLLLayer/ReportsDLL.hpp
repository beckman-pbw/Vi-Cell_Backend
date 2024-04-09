#pragma once

#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "Reports.hpp"

struct audit_log_entryDLL
{
	audit_log_entry ToCStyle()
	{
		audit_log_entry logEntry = {};

		logEntry.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);
		DataConversion::convertToCharPointer(logEntry.active_username, active_username);
		logEntry.event_type = event_type;
		DataConversion::convertToCharPointer(logEntry.event_message, event_message);

		return logEntry;
	}

	int64_t		entry_id_num;
	system_TP	timestamp; // seconds since 1/1/1970 UTC
	std::string active_username; // involved user (logged-in or account triggering audit entry)
	audit_event_type event_type;
	std::string userPermission;
	std::string event_message;
	std::string additionalInfo;
};

//NOTE: not current used.
//struct calibration_consumableDLL
//{
//	std::string consumable_label;
//	std::string consumable_lot_id;
//	system_TP consumable_expiration_date; // days since 1/1/1970
//	double consumable_assay_value;
//};
//
//struct calibration_history_entryDLL
//{
//	system_TP timestamp;
//	std::string username;
//	calibration_type cal_type;
//	std::vector<calibration_consumableDLL> calibConsumableList;
//	double slope;
//	double intercept;
//};

struct error_log_entryDLL
{
	error_log_entry ToCStyle()
	{
		error_log_entry logEntry = {};

		logEntry.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds> (timestamp);
		DataConversion::convertToCharPointer (logEntry.username, username);
		DataConversion::convertToCharPointer (logEntry.message, message);
		logEntry.error_code = error_code;

		return logEntry;
	}

	system_TP timestamp;
	std::string username; // ID of the current user (may be NULL)
	std::string message;
	uint32_t error_code; // definitions TBD.  May be ZERO
};

struct sample_entryDLL
{
	worklist_sample_entry ToCStyle()
	{
		worklist_sample_entry logEntry = {};

		logEntry.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds> (timestamp);
		DataConversion::convertToCharPointer (logEntry.sample_label, sample_label);
		DataConversion::convertToCharPointer (logEntry.celltype_name, celltype_name);
		DataConversion::convertToCharPointer (logEntry.analysis_name, analysis_name);
		logEntry.completion = completion;

		return logEntry;
	}

	system_TP timestamp;
	std::string sample_label;
	std::string celltype_name;
	std::string analysis_name;
	sample_completion_status completion;
};

struct sample_activity_entryDLL
{
	sample_activity_entry ToCStyle ()
	{
		sample_activity_entry logEntry = {};

		logEntry.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds> (timestamp);
		DataConversion::convertToCharPointer (logEntry.username, username);
		DataConversion::convertToCharPointer (logEntry.worklistlabel, worklistLabel);
		DataConversion::convert_vecOfDllType_to_listOfCType (samples, logEntry.samples, logEntry.number_of_samples);

		return logEntry;
	}

	system_TP timestamp; // seconds since 1/1/1970 UTC
	std::string username;
	std::string worklistLabel;
	std::vector<sample_entryDLL> samples;
};
