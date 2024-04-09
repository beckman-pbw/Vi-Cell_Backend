#pragma once

#include "AuditEventType.hpp"
#include "ReportsCommon.hpp"

typedef struct audit_log_entry
{
	uint64_t timestamp; // seconds since 1/1/1970 UTC
	char* active_username; // involved user (logged-in or account triggering audit entry)
	audit_event_type event_type;
	char* event_message;
} audit_log_entry;

typedef struct error_log_entry
{
	uint64_t timestamp;
	char* username; // ID of the current user (may be NULL)
	char* message;
	uint32_t error_code; // definitions TBD.  May be ZERO
} error_log_entry;

typedef struct worklist_sample_entry
{
	uint64_t timestamp;
	char* sample_label;
	char* celltype_name;
	char* analysis_name;
///TODO : etc - whatever else we need to report about this sample.  Need to refer to ViCell output log.
	sample_completion_status completion;
} worklist_sample_entry;

typedef struct sample_activity_entry
{
	uint64_t timestamp; // seconds since 1/1/1970 UTC
	char* username;
	char* worklistlabel;
	uint32_t number_of_samples;
	worklist_sample_entry* samples;
} sample_activity_entry;
