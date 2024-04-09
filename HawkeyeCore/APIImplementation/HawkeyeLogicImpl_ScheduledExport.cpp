#include "stdafx.h"

#include <functional>

#include "AuditLog.hpp"
#include "BlobLabel.h"
#include "CellTypesDLL.hpp"
#include "ChronoUtilities.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "ImageAnalysisUtilities.hpp"
#include "Logger.hpp"
#include "SampleDefinitionDLL.hpp"
#include "SystemErrors.hpp"
#include "ScheduledExportDLL.hpp"
#include "DBif_Structs.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_ScheduledExport";

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::AddScheduledExport(ScheduledExport scheduled_export, uuid__t *uuid)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "AddScheduledExport: <enter>");

	if (!InitializationComplete())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "AddScheduledExport: <exit, hardware initialization not complete>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
	ScheduledExportDLL scheduled_export_dll(scheduled_export);
	DBApi::DB_SchedulerConfigRecord scr = scheduled_export_dll.ToDbStyle();
	qResult = DBApi::DbAddSchedulerConfig(scr);
	if (qResult != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "AddScheduledExport: <exit, failed to add scheduled export database record>");
		return HawkeyeError::eInvalidArgs;
	}
	*uuid = scr.ConfigId;
	Logger::L ().Log (MODULENAME, severity_level::debug1, "AddScheduledExport: <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::EditScheduledExport(ScheduledExport scheduled_export)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "EditScheduledExport: <enter>");

	if (!InitializationComplete())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "EditScheduledExport: <exit, hardware initialization not complete>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
	ScheduledExportDLL scheduled_export_dll(scheduled_export);
	DBApi::DB_SchedulerConfigRecord scr = scheduled_export_dll.ToDbStyle();
	qResult = DBApi::DbModifySchedulerConfig(scr);
	if (qResult != DBApi::eQueryResult::QueryOk)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "EditScheduledExport: <exit, failed to edit scheduled export database record>");
		return HawkeyeError::eInvalidArgs;
	}
	Logger::L().Log(MODULENAME, severity_level::debug1, "EditScheduledExport: <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::DeleteScheduledExport(uuid__t uuid)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "DeleteScheduledExport: <enter>");

	if (!InitializationComplete())
	{
		Logger::L().Log(MODULENAME, severity_level::error, "DeleteScheduledExport: <exit, hardware initialization not complete>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
	qResult = DBApi::DbRemoveSchedulerByUuid(uuid);
	if (qResult != DBApi::eQueryResult::QueryOk && qResult != DBApi::eQueryResult::NoResults)
	{
		Logger::L().Log(MODULENAME, severity_level::error, "DeleteScheduledExport: <exit, failed to delete scheduled export database record>");
		return HawkeyeError::eInvalidArgs;
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "DeleteScheduledExport: <exit>");

	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::GetScheduledExports(eScheduledExportType export_type, ScheduledExport*& scheduled_exports, uint32_t& count)
{
	if (!InitializationComplete())
	{
		Logger::L().Log(MODULENAME, severity_level::warning, "GetScheduledExports: <exit, not initialized>");
		return HawkeyeError::eNotPermittedAtThisTime;
	}
	ScheduledExport one_scheduled_export{};
	ScheduledExportDLL scheduled_export_dll(one_scheduled_export);
	std::vector<ScheduledExport> scheduled_exps{};
	DBApi::eQueryResult qResult = DBApi::eQueryResult::QueryFailed;
	std::vector<DBApi::DB_SchedulerConfigRecord> scheduler_list;
	qResult = DBApi::DbGetSchedulerConfigList(scheduler_list);
	if ((qResult != DBApi::eQueryResult::QueryOk) && (qResult != DBApi::eQueryResult::NoResults))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "GetScheduledExports: <exit, invalid args>");
		return HawkeyeError::eInvalidArgs;
	}
	for (auto& db_scr : scheduler_list)
	{
		scheduled_export_dll.scheduled_export = {};
		scheduled_export_dll.FromDbStyle(db_scr);
		if (scheduled_export_dll.scheduled_export.export_type == export_type)
			scheduled_exps.push_back(scheduled_export_dll.scheduled_export);	
	}
	DataConversion::create_Clist_from_vector(scheduled_exps, scheduled_exports, count);
	return HawkeyeError::eSuccess;
}

HawkeyeError HawkeyeLogicImpl::FreeListOfScheduledExports(ScheduledExport* scheduled_exports, uint32_t count)
{
	std::vector<ScheduledExport> scheduled_exps = DataConversion::create_vector_from_Clist(scheduled_exports, count);

	for (auto& oneExport : scheduled_exps)
	{
		delete oneExport.comments;
		delete oneExport.destination_folder;
		delete oneExport.filename_template;
		delete oneExport.name;
		delete oneExport.notification_email;
		delete oneExport.data_filter.quality_control_name;
		delete oneExport.data_filter.sample_set_name_search_string;
		delete oneExport.data_filter.sample_name_search_string;
		delete oneExport.data_filter.sample_tag_search_string;
		delete oneExport.data_filter.username_for_data;
	}
	scheduled_exports = nullptr;
	return HawkeyeError::eSuccess;
}

