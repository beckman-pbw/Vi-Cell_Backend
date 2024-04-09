#pragma once

#include <config.h>
#include <cstdint>
#include "uuid__t.hpp"
#include <DBif_Api.h>

#include "ScheduledExport.hpp"

/// These structures and enums are used as an interface to schedule a data or logfile export
///

struct ScheduledExportDLL
{
	ScheduledExportDLL(ScheduledExport& sch_export) : scheduled_export(sch_export)
	{
	}

	void FromDbStyle(DBApi::DB_SchedulerConfigRecord& dbScheduledExport) const;
	DBApi::DB_SchedulerConfigRecord ToDbStyle() const;
	static void DataFilter_FromDbStyle(DataFilterCriteria& dfc, DBApi::DB_SchedulerConfigRecord& dbScheduledExport);
	void DataFilter_ToDbStyle(DBApi::DB_SchedulerConfigRecord& scr, DataFilterCriteria& dfc) const;
	static void Recurrence_FromDbStyle(RecurrenceFrequency& rf, DBApi::DB_SchedulerConfigRecord& dbScheduledExport);
	void Recurrence_ToDbStyle(DBApi::DB_SchedulerConfigRecord& scr, RecurrenceFrequency& rf) const;

	ScheduledExport& scheduled_export;
};

