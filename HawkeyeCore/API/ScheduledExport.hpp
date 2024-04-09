#pragma once

#include <cstdint>


/// These structures and enums are used as an interface to schedule a data or logfile export
///

enum eScheduledExportType : uint16_t
{
	SampleResults = 0,
	AuditLogs
};

enum eDbExportType : uint16_t
{
	ExportNothing = 0,
	ExportForOfflineAnalysis = 1,
	ExportCsvAndImages = 2,
	ExportErrorAndAuditLogs = 3,
	ExportAuditLogs = 4,
	ExportErrorLogs = 5
};


enum eRecurrenceFrequency : uint16_t
{
	// We aren't using all these values for the scheduled exports
	Once = 0,
	//Secondly = 1,
	//Minutely = 2,
	//Hourly = 3,
	Daily = 4,
	Weekly = 5,
	Monthly = 6,
	//Yearly = 7
};

enum eWeekday : uint16_t
{
	Sunday = 0,
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday
};

enum eScheduledExportLastRunStatus : uint16_t
{
	NotRun = 0,
	Running,
	RunSuccess,
	RunError,
	RunPaused,
};

struct DataFilterCriteria
{
	// from_date and to_date are for sample retrieval
	uint64_t from_date;
	uint64_t to_date;
	char* username_for_data;
	char* sample_tag_search_string;
	char* sample_name_search_string;
	char* sample_set_name_search_string;
	bool all_cell_types_selected;
	uint32_t cell_type_index;
	char* quality_control_name;
	bool since_last_export; // not used on GUI
	// start_date and end_date are for scheduled export execution
	uint64_t start_date; // not used on GUI
	uint64_t end_date; // not used on GUI
};

struct RecurrenceFrequency
{
	eRecurrenceFrequency repeat_every;
	uint64_t export_on_date;
	uint16_t hour;
	uint16_t minute;
	eWeekday weekday;
	uint16_t day_of_the_month;
	uint64_t start_on; // not used on GUI
};

struct ScheduledExport
{
	uuid__t uuid;
	char* name;
	char* comments;
	char* filename_template;
	char* destination_folder;
	bool is_enabled;
	bool is_encrypted;	// Only used with SampleResults Scheduled Exports
	char* notification_email;
	RecurrenceFrequency recurrence_frequency;
	DataFilterCriteria data_filter;

	uint64_t last_time_run; // not used on GUI
	uint64_t last_success_time_run; // not used on GUI
	eScheduledExportLastRunStatus last_run_status; // not used on GUI

	bool include_audit_log;	// Only used with Audit Log Scheduled Exports
	bool include_error_log;	// Only used with Error Log Scheduled Exports

	eScheduledExportType export_type;
};

