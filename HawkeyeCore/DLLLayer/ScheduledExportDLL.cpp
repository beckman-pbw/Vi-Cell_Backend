#include "stdafx.h"

#include "ScheduledExportDLL.hpp"
#include <cstdint>
#include "uuid__t.hpp"
#include <DBif_Api.h>
#include "DataConversion.hpp"
#include "HawkeyeError.hpp"
#include "QualityControlsDLL.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "ScheduledExport";


//*****************************************************************************
void ScheduledExportDLL::DataFilter_FromDbStyle(DataFilterCriteria& dfc, DBApi::DB_SchedulerConfigRecord& scr)
{
	uint8_t vector_index = 0;
	std::string tmpStr;
	bool sampleSetSearch = false;
	dfc.all_cell_types_selected = true;
	for (auto& it : scr.FilterTypesList)
	{
		switch (it)
		{
		case DBApi::eListFilterCriteria::RunDateRangeFilter:
			if (!scr.CompareValsList.empty())
			{
				tmpStr = scr.CompareValsList[vector_index];
				boost::erase_all(tmpStr, "\"");
				boost::erase_all(tmpStr, "\'");
				boost::erase_all(tmpStr, "::timestamp");
				auto sTimeStr = tmpStr.substr(0, tmpStr.find(';'));
				auto eTimeStr = tmpStr.substr(tmpStr.find(';') + 1, tmpStr.size() - tmpStr.find(';'));

				// TODO: Database is removing space between date and time - fix.
				// Add space between start date and time after read from database
				for (int i = 0; i < sTimeStr.length(); i++)
				{
					if ((sTimeStr.length() > (i+4)) && (sTimeStr[i+2] == ':'))
					{
						sTimeStr.insert(i++, " ");
						break;
					}
				}
				// Add space between end date and time after read from database
				for (int i = 0; i < eTimeStr.length(); i++)
				{
					if ((eTimeStr.length() > (i + 4)) && (eTimeStr[i + 2] == ':'))
					{
						eTimeStr.insert(i++, " ");
						break;
					}
				}				
				system_TP tmpTimePnt = ChronoUtilities::ConvertToTimePoint(sTimeStr, "%Y-%m-%d %H:%M:%S");
				dfc.from_date = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(tmpTimePnt);
				tmpTimePnt = ChronoUtilities::ConvertToTimePoint(eTimeStr, "%Y-%m-%d %H:%M:%S");
				dfc.to_date = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(tmpTimePnt);
			}
			vector_index++;
			break;
		case DBApi::eListFilterCriteria::StatusFilter:
			if (scr.CompareValsList.size() > vector_index)
			{
				tmpStr = scr.CompareValsList[vector_index];
				boost::erase_all(tmpStr, "\"");
				boost::erase_all(tmpStr, "\'");
				const int compareVal = static_cast<int>(std::strtol(tmpStr.c_str(), nullptr, 10));
				if (compareVal == static_cast<int>(DBApi::eSampleSetStatus::SampleSetComplete))
				{
					sampleSetSearch = true;
				}
			}
			vector_index++;
			break;
		case DBApi::eListFilterCriteria::ItemNameFilter:
			if (scr.CompareValsList.size() > vector_index)
			{
				tmpStr = scr.CompareValsList[vector_index];
				boost::erase_all(tmpStr, "\"");
				boost::erase_all(tmpStr, "\'");
				if (sampleSetSearch)
					DataConversion::convertToCharPointer(dfc.sample_set_name_search_string, tmpStr);
				else
					DataConversion::convertToCharPointer(dfc.sample_name_search_string, tmpStr);
			}
			vector_index++;
			break;
		case DBApi::eListFilterCriteria::OwnerFilter:
			if (scr.CompareValsList.size() > vector_index)
			{
				tmpStr = scr.CompareValsList[vector_index];
				boost::erase_all(tmpStr, "\"");
				boost::erase_all(tmpStr, "\'");
				std::string username;
				UserList::Instance().GetUsernameByUUID(Uuid::FromStr(tmpStr), username);
				DataConversion::convertToCharPointer(dfc.username_for_data, username);
			}
			vector_index++;
			break;
		case DBApi::eListFilterCriteria::LabelFilter:
			if (scr.CompareValsList.size() > vector_index)
			{
				tmpStr = scr.CompareValsList[vector_index];
				boost::erase_all(tmpStr, "\"");
				boost::erase_all(tmpStr, "\'");
				DataConversion::convertToCharPointer(dfc.sample_tag_search_string, tmpStr);
			}
			vector_index++;
			break;
		case DBApi::eListFilterCriteria::QcFilter:
			dfc.all_cell_types_selected = false;
			if (scr.CompareValsList.size() > vector_index)
			{
				std::string qcUuidStr = scr.CompareValsList[vector_index];
				boost::erase_all(qcUuidStr, "\"");
				boost::erase_all(qcUuidStr, "\'");
				uuid__t qcUuid = Uuid::FromStr(qcUuidStr);
				std::vector<QualityControlDLL>& qcsDLL = QualityControlsDLL::Get();
				for (auto qc : qcsDLL)
				{
					if (memcmp(qcUuid.u, qc.uuid.u, sizeof(qcUuid)) == 0)
					{
						DataConversion::convertToCharPointer(dfc.quality_control_name, qc.qc_name);
						break;
					}
				}
			}
			vector_index++;
			break;
		case DBApi::eListFilterCriteria::CellTypeFilter:
			dfc.all_cell_types_selected = false;
			if (scr.CompareValsList.size() > vector_index)
			{
				tmpStr = scr.CompareValsList[vector_index];
				dfc.cell_type_index = static_cast<int>(std::strtol(tmpStr.c_str(), nullptr, 10));
			}
			vector_index++;
			break;
		}
	}
}

void ScheduledExportDLL::DataFilter_ToDbStyle(DBApi::DB_SchedulerConfigRecord& scr, DataFilterCriteria& dfc) const
{
	// Only create a from/to date filter if scheduled for one time only
	if (scr.DayWeekIndicator == 0)
	{
		std::string sTimeStr;
		std::string eTimeStr;
		// if 'from_date' == 'to_date', want to add 24 hrs to 'to_date' (getListTimestampAsStr will do this if 'to_date' == 0) 
		if (dfc.from_date == dfc.to_date)
			dfc.to_date = 0;
		ChronoUtilities::getListTimestampAsStr(dfc.from_date, dfc.to_date, sTimeStr, eTimeStr);
		scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::RunDateRangeFilter);
		scr.CompareOpsList.push_back("=");
		// Because we are storing this filter in the database, the format for date_time strings needs extra surrounding single quotes
		scr.CompareValsList.push_back(boost::str(boost::format("'%s;%s'") % sTimeStr % eTimeStr));
	}

	scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::StatusFilter);
	if (dfc.sample_set_name_search_string != nullptr)
	{ // SampleSet.
		scr.CompareOpsList.push_back("=");
		scr.CompareValsList.push_back(boost::str(boost::format("'%d'") % (int)DBApi::eSampleSetStatus::SampleSetComplete));
	}
	else
	{ // SampleItem.
		scr.CompareOpsList.push_back(">");
		scr.CompareValsList.push_back(boost::str(boost::format("'%d'") % (int)DBApi::eSampleItemStatus::ItemRunning));
	}

	if (dfc.username_for_data != nullptr)
	{
		std::string username;
		DataConversion::convertToStandardString(username, dfc.username_for_data);
		scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::OwnerFilter);
		scr.CompareOpsList.push_back("=");
		uuid__t userUUID = {};
		UserList::Instance().GetUserUUIDByName(username, userUUID);
		std::string temp = boost::str(boost::format("'%s'") % Uuid::ToStr(userUUID));
		scr.CompareValsList.push_back(temp);
	}

	// SampleSet uses SampleSetNameStr and  Sample uses SampleItemNameStr.
	if ((dfc.sample_name_search_string != nullptr) || (dfc.sample_set_name_search_string != nullptr))
	{
		std::string nameSearchString;
		if (dfc.sample_name_search_string != nullptr)
			DataConversion::convertToStandardString(nameSearchString, dfc.sample_name_search_string);
		else
			DataConversion::convertToStandardString(nameSearchString, dfc.sample_set_name_search_string);
		scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::ItemNameFilter);
		scr.CompareOpsList.push_back("~*");	// Substring search.
		std::string temp = boost::str(boost::format("'%s'") % nameSearchString);
		scr.CompareValsList.push_back(temp);
	}

	if (dfc.sample_tag_search_string != nullptr)
	{
		std::string tagSearchString;
		DataConversion::convertToStandardString(tagSearchString, dfc.sample_tag_search_string);
		scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::LabelFilter);
		scr.CompareOpsList.push_back("~*");	// Substring search.
		std::string temp = boost::str(boost::format("'%s'") % tagSearchString);
		scr.CompareValsList.push_back(temp);
	}

	if (dfc.quality_control_name != nullptr)
	{
		std::string qcName;
		DataConversion::convertToStandardString(qcName, dfc.quality_control_name);
		uuid__t qcUuid;
		std::vector<QualityControlDLL>& qcsDLL = QualityControlsDLL::Get();
		for (auto qc : qcsDLL)
		{
			if (qc.qc_name == qcName)
			{
				memcpy(qcUuid.u, qc.uuid.u, sizeof(qcUuid.u));
				scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::QcFilter);
				scr.CompareOpsList.push_back("=");
				scr.CompareValsList.push_back(boost::str(boost::format("'%s'") % Uuid::ToStr(qcUuid)));
				break;
			}
		}
	}
	else if (dfc.all_cell_types_selected == false)
	{
		scr.FilterTypesList.push_back(DBApi::eListFilterCriteria::CellTypeFilter);
		scr.CompareOpsList.push_back("=");
		std::string cellTypeIdxStr;
		// Cell types are stored in the database as signed integers and 
		// user defined cell types are < 0, so if user defined convert like the DB does.
		if (CellTypeDLL::isUserDefined(dfc.cell_type_index))
		{
			cellTypeIdxStr = boost::str(boost::format("%ld") % (int32_t)dfc.cell_type_index);
		}
		else
		{
			cellTypeIdxStr = std::to_string(dfc.cell_type_index);
		}
		scr.CompareValsList.push_back(boost::str(boost::format("'%s'") % cellTypeIdxStr));
	}

}

void ScheduledExportDLL::Recurrence_FromDbStyle(RecurrenceFrequency& rf, DBApi::DB_SchedulerConfigRecord& scr)
{
	rf.hour = scr.StartOffset / 60;
	rf.minute = scr.StartOffset % 60;
	if (scr.DayWeekIndicator == DBApi::AllDays)
		rf.repeat_every = Daily;
	else if (scr.DayWeekIndicator & DBApi::WeekBit)
	{
		rf.repeat_every = Weekly;
		rf.weekday = static_cast<eWeekday>(scr.DayWeekIndicator & DBApi::DayIndicatorMask);
	}
	else if (scr.DayWeekIndicator & DBApi::MonthBit) // If monthly bit is set
	{
		rf.repeat_every = Monthly;
		rf.day_of_the_month = scr.MonthlyRunDay;
	}
	else
	{
		rf.repeat_every = Once;
		rf.export_on_date = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(scr.StartDate);
	}
}

void ScheduledExportDLL::Recurrence_ToDbStyle(DBApi::DB_SchedulerConfigRecord& scr, RecurrenceFrequency& rf) const
{
	scr.StartOffset = (rf.hour * 60) + rf.minute;
	switch (rf.repeat_every)
	{
	case Once:
		scr.StartDate = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(rf.export_on_date);
		scr.DayWeekIndicator = 0;
		break;
	case Daily:
		scr.StartDate = ChronoUtilities::CurrentTime();
		scr.DayWeekIndicator = DBApi::AllDays;						// Scheduled every day
		break;
	case Weekly:
		scr.DayWeekIndicator = rf.weekday | DBApi::WeekBit;			// Set Weekly bit and the day of the week bit
		scr.StartDate = ChronoUtilities::CurrentTime();
		break;
	case Monthly:
		scr.DayWeekIndicator = DBApi::MonthBit;						// Set Monthly bit
		scr.MonthlyRunDay = rf.day_of_the_month;
		scr.StartDate = ChronoUtilities::CurrentTime();
		break;
	}
}

void ScheduledExportDLL::FromDbStyle(DBApi::DB_SchedulerConfigRecord& scr) const
{
	DataConversion::convertToCharPointer(scheduled_export.name, scr.Name);
	DataConversion::convertToCharPointer(scheduled_export.comments, scr.Comments);
	DataConversion::convertToCharPointer(scheduled_export.filename_template, scr.FilenameTemplate);
	DataConversion::convertToCharPointer(scheduled_export.destination_folder, scr.DestinationFolder);
	DataConversion::convertToCharPointer(scheduled_export.notification_email, scr.NotificationEmail);
	scheduled_export.uuid = scr.ConfigId;
	scheduled_export.is_enabled = scr.Enabled;
	scheduled_export.is_encrypted = false;
	scheduled_export.export_type = AuditLogs;
	switch (scr.DataType)
	{
	case ExportForOfflineAnalysis:
		scheduled_export.is_encrypted = true;
		scheduled_export.export_type = SampleResults;
		break;
	case ExportCsvAndImages:
		scheduled_export.export_type = SampleResults;
		break;
	case ExportErrorAndAuditLogs:
		scheduled_export.include_audit_log = true;
		scheduled_export.include_error_log = true;
		break;
	case ExportAuditLogs:
		scheduled_export.include_audit_log = true;
		scheduled_export.include_error_log = false;
		break;
	case ExportErrorLogs:
		scheduled_export.include_error_log = true;
		scheduled_export.include_audit_log = false;
		break;
	}
	Recurrence_FromDbStyle(scheduled_export.recurrence_frequency, scr);
	DataFilter_FromDbStyle(scheduled_export.data_filter, scr);
	scheduled_export.last_run_status = static_cast<eScheduledExportLastRunStatus>(scr.LastRunStatus);
	scheduled_export.last_time_run = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(scr.LastRunTime);
	scheduled_export.last_success_time_run = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(scr.LastSuccessRunTime);
}

DBApi::DB_SchedulerConfigRecord ScheduledExportDLL::ToDbStyle() const
{
	DBApi::DB_SchedulerConfigRecord scr = {};
	scr.ConfigId = scheduled_export.uuid;
	DataConversion::convertToStandardString(scr.Name, scheduled_export.name);
	DataConversion::convertToStandardString(scr.Comments, scheduled_export.comments);
	DataConversion::convertToStandardString(scr.FilenameTemplate, scheduled_export.filename_template);
	DataConversion::convertToStandardString(scr.DestinationFolder, scheduled_export.destination_folder);
	DataConversion::convertToStandardString(scr.NotificationEmail, scheduled_export.notification_email);
	scr.Enabled = scheduled_export.is_enabled;
	scr.LastRunStatus = scheduled_export.last_run_status;
	scr.LastRunTime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(scheduled_export.last_time_run);
	scr.LastSuccessRunTime = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(scheduled_export.last_success_time_run);

	if (scheduled_export.is_encrypted)
		scr.DataType = eDbExportType::ExportForOfflineAnalysis;		// DataType = 1
	else if (scheduled_export.export_type == eScheduledExportType::SampleResults)
		scr.DataType = eDbExportType::ExportCsvAndImages;			// DataType = 2
	else if (scheduled_export.include_audit_log && scheduled_export.include_error_log)
		scr.DataType = eDbExportType::ExportErrorAndAuditLogs;		// DataType = 3
	else if (scheduled_export.include_audit_log)
		scr.DataType = eDbExportType::ExportAuditLogs;				// DataType = 4
	else if (scheduled_export.include_error_log)
		scr.DataType = eDbExportType::ExportErrorLogs;				// DataType = 5
	else
		scr.DataType = eDbExportType::ExportNothing;				// DataType = 0

	Recurrence_ToDbStyle(scr, scheduled_export.recurrence_frequency);
	DataFilter_ToDbStyle(scr, scheduled_export.data_filter);
	return scr;
}
