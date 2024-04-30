#include "stdafx.h"

#include "AnalysisDefinitionsDLL.hpp"
#include "AuditLog.hpp"
#include "CellTypesDLL.hpp"
#include "DataConversion.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "Logger.hpp"
#include "UserList.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_CellType";

std::shared_ptr<CellTypeDLL> pTempCellType_;

//*****************************************************************************
bool HawkeyeLogicImpl::initializeCellType() {

	Logger::L ().Log (MODULENAME, severity_level::debug2, "initializeCellType: <enter>");

	bool retStatus = CellTypesDLL::Initialize();

	Logger::L ().Log (MODULENAME, severity_level::debug2, "initializeCellType: <exit>");

	return retStatus;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfCellType (CellType* list, uint32_t num_ct)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "FreeListOfCellType: <enter>");

	if (list == nullptr)
		return;

	for (uint32_t i = 0; i < num_ct; i++)
	{
		CellTypeDLL::freeCelltypeInternal (list[i]);
	}

	delete[] list;

	Logger::L ().Log (MODULENAME, severity_level::debug3, "FreeListOfCellType: <exit>");
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetAllCellTypes (uint32_t& num_ct, CellType*& celltypes)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAllCellTypes: <enter>");

	num_ct = 0;
	celltypes = nullptr;

	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();

	// Do not return the "retired" CellTypes.
	std::vector<CellTypeDLL> tCellTypesDLL = {};
	for (auto v : cellTypesDLL)
	{
		if (v.retired)
		{
			continue;
		}

		tCellTypesDLL.push_back (v);
	}

	DataConversion::convert_vecOfDllType_to_listOfCType (tCellTypesDLL, celltypes, num_ct);
	if (!cellTypesDLL.size ())
	{
		Logger::L ().Log (MODULENAME, severity_level::debug1, "GetAllCellTypes: <exit, none found>");
		return HawkeyeError::eNoneFound;
	}

	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAllCellTypes: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetAllowedCellTypes(const char* username, uint32_t& num_ct, CellType*& celltypes)
{
	Logger::L().Log(MODULENAME, severity_level::debug3, "GetAllowedCellTypes: <enter>");
	num_ct = 0;
	celltypes = nullptr;
	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();
	std::vector<uint32_t> index_list = {};
	UserList::Instance().GetAllowedCellTypeIndices(username, index_list);
	std::vector<CellTypeDLL> tCellTypesDLL = {};
	for (auto v : cellTypesDLL)
	{
		if (v.retired)
		{
			continue;
		}
		if (std::find(index_list.begin(), index_list.end(), v.celltype_index) != index_list.end()) {
			tCellTypesDLL.push_back(v);
		}
	}
	DataConversion::convert_vecOfDllType_to_listOfCType(tCellTypesDLL, celltypes, num_ct);
	if (!cellTypesDLL.size())
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "GetAllowedCellTypes: <exit, none found>");
		return HawkeyeError::eNoneFound;
	}
	Logger::L().Log(MODULENAME, severity_level::debug3, "GetAllowedCellTypes: <exit>");
	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetFactoryCellTypes (uint32_t& num_ct, CellType*& celltypes)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetFactoryCellTypes: <enter>");

	num_ct = 0;
	celltypes = nullptr;

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();

	if (!cellTypesDLL.size()) {
		Logger::L ().Log (MODULENAME, severity_level::debug1, "GetFactoryCellTypes: <exit, none found>");
		return HawkeyeError::eNoneFound;
	}

	celltypes = new CellType[CellTypesDLL::NumBCICellTypes()];

	for (auto& ct : cellTypesDLL) {
		if (!CellTypeDLL::isUserDefined (ct.celltype_index)) {
			celltypes[num_ct++] = ct.ToCStyle();
		}
	}

	Logger::L().Log (MODULENAME, severity_level::debug3, "GetFactoryCellTypes: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
// AddCellType was originally designed to add default and user-defined CellTypes.
// Adding default CellTypes has never been used or tested.
// AddCellType has been updated and 

// If the CellType name to add exists in the database...
// 1) the existing CellType in the database is marked as "retired".
// 2) a new user-defined CellType is created.
//
// Else
// 1) a new user-defined CellType is created.
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::AddCellType (
	const char* username,
	const char* password, 
	CellType& ct, 
	uint32_t& ct_index, 
	const char* retiredName)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "AddCellType: <enter> - user: " + std::string(username));

	ct_index = 0;

	// This must be done by an Elevated or higher.
	if (!UserList::Instance().IsUserPermissionAtLeast(username, UserPermissionLevel::eElevated))
	{
		AuditLogger::L().Log(generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized,
			"User not allowed to add cell type"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (ct.concentration_adjustment_factor > 20.0 || ct.concentration_adjustment_factor < -20.0)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "AddCellType: <exit, concentration adjustment factor out of range (+-20)>");
		return HawkeyeError::eInvalidArgs;
	}

	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();

	// When the retired CellType name is empty the specified the CellType is either a new CellType <or>
	// a copy of an existing CellType.
	// Otherwise, the CellType is an edit of an existing CellType.
	bool modifyingExistingType = false;
	if (strlen(retiredName) == 0)
	{
		// Check for duplicate name.
		for (auto& ect : cellTypesDLL)
		{
			if (ect.label == ct.label) {
				Logger::L ().Log (MODULENAME, severity_level::debug1, "AddCellType: <exit, already exists>");
				return HawkeyeError::eAlreadyExists;
			}
		}
	}
	else
	{		
		// If the existing CellType (by name) exists, mark it as "retired".
		for (auto& ect : cellTypesDLL)
		{
			if (ect.label == ct.label)
			{
				modifyingExistingType = true;

				if (!CellTypeDLL::isUserDefined(ect.celltype_index)) // LH6531-5945 - Somebody found a loophole for editing factory cell types.
				{
					Logger::L().Log(MODULENAME, severity_level::debug1, "AddCellType: <exit, invalid args - cannot modify factory cell type>");
					return HawkeyeError::eInvalidArgs;
				}
				ect.retired = true;
				ect.label = retiredName;
				
				// Build the update string for the modification.  
				CellTypeDLL updatedCellType = {ct};
				std::string ctinfo_str = CellTypeDLL::UpdateCellType (updatedCellType, ect);
				if (!ctinfo_str.empty ())
				{
					AuditLogger::L().Log(generateAuditWriteData(
						username,
						audit_event_type::evt_celltypemodify,
						ctinfo_str));
				}

				break;
			}
		}
	}

	CellTypeDLL ctDLL{ ct };
	ctDLL.celltype_index = 0;		// User defined cell type will be assigned an index in CellTypesDLL::Add()

	CellTypeDLL& cellTypeDLL = CellTypesDLL::Add(ctDLL);
	ct_index = ctDLL.celltype_index;

	// Must call the UserList AFTER adding to CellTypesDLL - UserList uses CellTypesDLL to validate cell type indices
	auto status = UserList::Instance().AddCellTypeIndex(ct_index);
	if (status == HawkeyeError::eSuccess)
	{
		// Suppress the audit trail entry for the "retired" cell type.
		if (!modifyingExistingType)
		{
			std::string ctinfo_str = CellTypeDLL::CellTypeAsString(ctDLL);
			AuditLogger::L().Log(generateAuditWriteData(
				username,
				audit_event_type::evt_celltypecreate,
				ctinfo_str));
		}
	}

	return status;
}

//*****************************************************************************
// Deprecated
//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::ModifyCellType (const char* username, const char* password, CellType& cellType)
{
	return HawkeyeError::eDeprecated;
	
	Logger::L ().Log (MODULENAME, severity_level::debug3, "ModifyCellType: <enter> - user: " + std::string(username));

	// This must be done by an Elevated or higher.
	if (!UserList::Instance().IsUserPermissionAtLeast(username, UserPermissionLevel::eElevated))
	{
		AuditLogger::L().Log(generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized,
			"User not allowed to modify cell type"));
		return HawkeyeError::eNotPermittedByUser;
	}

	CellTypeDLL inputCellType{ cellType };

	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();

	// Validation: 
	// 1) Make sure the user has not duplicated some other cell type label
	for (auto ect : cellTypesDLL)
	{
		if (ect.label == cellType.label)
		{
			if (ect.celltype_index != cellType.celltype_index)
			{
				Logger::L().Log(MODULENAME, severity_level::debug1, "ModifyCellType: <exit, already exists>");
				return HawkeyeError::eAlreadyExists;
			}
			else if (!CellTypeDLL::isUserDefined(ect.celltype_index)) // LH6531-5945 - Somebody found a loophole for editing factory cell types.
			{
				Logger::L().Log(MODULENAME, severity_level::debug1, "ModifyCellType: <exit, invalid args - cannot modify factory cell type>");
				return HawkeyeError::eInvalidArgs;
			}
		}
	}

	//Validation:
	// 2) No modifying factory-supplied cell types, thank you!
	if (!CellTypeDLL::isUserDefined (cellType.celltype_index))
	{
		Logger::L ().Log (MODULENAME, severity_level::debug1, "ModifyCellType: <exit, invalid args - cannot modify factory cell type>");
		return HawkeyeError::eInvalidArgs;
	}

	if (cellType.concentration_adjustment_factor > 20.0 || cellType.concentration_adjustment_factor < -20.0)
	{
		Logger::L().Log(MODULENAME, severity_level::debug1, "ModifyCellType: <exit, concentration adjustment factor out of range (+-20)>");
		return HawkeyeError::eInvalidArgs;
	}

	for (auto it = cellTypesDLL.begin (); it != cellTypesDLL.end (); ++it) {
		if (cellType.celltype_index == it->celltype_index)
		{
			std::string ctinfo_str = CellTypeDLL::UpdateCellType (inputCellType, *it);
			if (!ctinfo_str.empty ())
			{
				AuditLogger::L().Log(generateAuditWriteData(
					username,
					audit_event_type::evt_celltypemodify,
					ctinfo_str));
			}

			Logger::L ().Log (MODULENAME, severity_level::debug3, "ModifyCellType: <exit>");

			return HawkeyeError::eSuccess;
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "ModifyCellType: <exit, not found>");

	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::RemoveCellType (const char* username, const char* password, uint32_t ct_index)
{
	Logger::L().Log(MODULENAME, severity_level::debug3, "RemoveCellType: <enter> - user: " + std::string(username));

	// This must be done by an Elevated or higher.
	if (!UserList::Instance().IsUserPermissionAtLeast(username, UserPermissionLevel::eElevated))
	{
		AuditLogger::L().Log(generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized,
			"User not allowed to remove cell type"));
		return HawkeyeError::eNotPermittedByUser;
	}

	//Validation:
	// 1) No modifying factory-supplied cell types, thank you!
	if (!CellTypeDLL::isUserDefined (ct_index))
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "RemoveCellType: <exit - Can't modify factory supplied- Inavlid args>");
		return HawkeyeError::eInvalidArgs;
	}

	std::vector<CellTypeDLL>& cellTypesDLL = CellTypesDLL::Get();

	auto ct_it = find_if (cellTypesDLL.begin (), cellTypesDLL.end (), [ct_index](const CellTypeDLL& ct)->bool
		{
			return ct.celltype_index == ct_index;
		});

	if (ct_it == cellTypesDLL.end ()) {
		return HawkeyeError::eNoneFound;
	}

	DBApi::eQueryResult dbStatus = DBApi::DbRemoveCellTypeByUuid (ct_it->uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log(MODULENAME, severity_level::error,
			boost::str(boost::format("RemoveCellType: <exit, unable to remove celltype, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError(BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::celltype,
			instrument_error::severity_level::warning));
		return HawkeyeError::eSuccess;
	}

	// "DbRemoveCellTypeByUuid" (above) marks the user celltype as "retired".
	// Mark the celltype in the collection as retired since the celltypes are not reloaded.
	ct_it->retired = true;

	AuditLogger::L().Log(generateAuditWriteData(
		username,
		audit_event_type::evt_celltypedelete, 
		CellTypeDLL::CellTypeAsString (*ct_it)));

	// Must call the UserList AFTER removing from CellTypesDLL - UserList uses CellTypesDLL to validate cell type indices
	UserList::Instance().RemoveCellTypeIndex(ct_index);

	Logger::L ().Log (MODULENAME, severity_level::debug3, "RemoveCellType: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetAnalysisForCellType (uint16_t ad_index, uint32_t ct_index, AnalysisDefinition*& ad)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <enter>");

	/// 1) Make sure ad_index is valid.
	/// 2) Make sure ct_index is valid.
	/// 3) Look up ct_index - does it have a specialization of ad_index?
	/// 3.1) Yes: return the specialization definition.
	/// 3.2) No: return the master list definition

	ad = nullptr;

	if (!InitializationComplete())
	{
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	// Check for temporary cell type index and return analysis specialization for temporary cell type
	if ((ct_index == TEMP_CELLTYPE_INDEX) && pTempCellType_)
	{
		for (auto& item : pTempCellType_->analysis_specializations)
		{
			if (item.analysis_index != ad_index)
			{
				continue;
			}

			if (ad != nullptr)
			{
				FreeAnalysisDefinitions (ad, 1);
			}
			DataConversion::convert_dllType_to_cType (item, ad);

			Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit Temp cell type success>");
			return HawkeyeError::eSuccess;
		}
	}

	std::vector<CellTypeDLL> cellTypesDLL = CellTypesDLL::Get ();

	for (auto& ct : cellTypesDLL) {
		if (ct.celltype_index == ct_index) {
			for (auto& as : ct.analysis_specializations) {
				if (as.analysis_index == ad_index)
				{
					if (ad != nullptr) {
						FreeAnalysisDefinitions (ad, 1);
					}

					DataConversion::convert_dllType_to_cType (as, ad);

					Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit 1 success>");

					return HawkeyeError::eSuccess;
				}
			}

			// No specialization found...  check the factory analyses.
			std::vector<AnalysisDefinitionDLL>& analyses = AnalysisDefinitionsDLL::Get();
			for (auto adDLL : analyses) {
				if (adDLL.analysis_index == ad_index)
				{
					if (ad != nullptr) {
						FreeAnalysisDefinitions (ad, 1);
					}

					DataConversion::convert_dllType_to_cType (adDLL, ad);

					Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit 2 success>");
					return HawkeyeError::eSuccess;
				}
			}
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit, not found>");

	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
HawkeyeError GetAnalysisForCellTypeDLL(uint16_t ad_index, uint32_t ct_index, AnalysisDefinitionDLL& ad)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <enter>");

	/// 1) Make sure ad_index is valid.
	/// 2) Make sure ct_index is valid.
	/// 3) Look up ct_index - does it have a specialization of ad_index?
	/// 3.1) Yes: return the specialization definition.
	/// 3.2) No: return the master list definition

	// Check for temporary cell type index and return analysis specialization for temporary cell type
	if ((ct_index == TEMP_CELLTYPE_INDEX) && pTempCellType_)
	{
		for (auto& item : pTempCellType_->analysis_specializations)
		{
			if (item.analysis_index != ad_index)
			{
				continue;
			}

			ad = item;

			Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit Temp cell type success>");
			return HawkeyeError::eSuccess;
		}
	}

	std::vector<CellTypeDLL> cellTypesDLL = CellTypesDLL::Get ();

	for (auto& ct : cellTypesDLL) {
		if (ct.celltype_index == ct_index) {
			for (auto& as : ct.analysis_specializations) {
				if (as.analysis_index == ad_index)
				{
					ad = as;

					Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit 1 success>");

					return HawkeyeError::eSuccess;
				}
			}

			// No specialization found...  check the factory analyses.
			std::vector<AnalysisDefinitionDLL>& analyses = AnalysisDefinitionsDLL::Get();
			for (auto adDLL : analyses) {
				if (adDLL.analysis_index == ad_index)
				{
					ad = adDLL;

					Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit 2 success>");
					return HawkeyeError::eSuccess;
				}
			}
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug3, "GetAnalysisForCellType: <exit, not found>");

	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SpecializeAnalysisForCellType (AnalysisDefinition& ad, uint32_t ct_index)
{
	Logger::L ().Log (MODULENAME, severity_level::debug3, "SpecializeAnalysisForCellType: <enter>");

	std::string loggedInUsername = UserList::Instance().GetAttributableUserName();
//TODO: 	std::string loggedInUsername = UserList::Instance().GetLoggedInUsername();

	//TODO: need to test...
		/// 0) Check user permissions (admin or higher)
		/// 1) Make sure ad.analysis_index is valid.
		/// 2) Make sure ct_index is valid.
		/// 3) Look up ct_index - does it have a specialization of ad_index?
		/// 3.01) Are CT and AD both FACTORY? 
		/// 3.01.1) Yes: then don't modify!
		/// 3.1) Yes: replace that with ad.
		/// 3.2) No: add ad as an override.

		// This must be done by an Elevated or higher.
	auto status =UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		// No logged in user
		if (status == HawkeyeError::eNotPermittedAtThisTime)
			return status;

		AuditLogger::L().Log(generateAuditWriteData(
			loggedInUsername,
			audit_event_type::evt_notAuthorized,
			"Logged in user is not allowed to specialize analysis for CellType"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// Cannot add a specialization for the temporary analysis
	if (ad.analysis_index == TEMP_ANALYSIS_INDEX)
	{
		Logger::L ().Log (MODULENAME, severity_level::error, "SpecializeAnalysisForCellType: cannot specialize the temporary analysis");
		return HawkeyeError::eInvalidArgs;
	}

	// Check the analysis definitions for ad.analysis_index.
	std::vector<AnalysisDefinitionDLL>& analyses = AnalysisDefinitionsDLL::Get ();
	for (const auto& adDLL : analyses)
	{
		if (ad.analysis_index != adDLL.analysis_index)
			continue;

		//TODO: need to lock access to CellType collection...   use shared ptr...

		std::vector<CellTypeDLL> cellTypesDLL = CellTypesDLL::Get ();

		bool found_ad = false;
		std::string modified_ad_str = {};
		// Find the CellType that matches ct_index.
		for (auto& ct : cellTypesDLL)
		{
			if (ct.celltype_index != ct_index)
				continue;
			// Check if there is a CellType specialization for ad.analysis_index.
			// If so, delete the existing specialization and replace it with the new one.
			for (auto it = ct.analysis_specializations.begin (); it != ct.analysis_specializations.end (); ++it)
			{
				if (it->analysis_index != ad.analysis_index)
					continue;

				// If a FACTORY-PROVIDED cell type already has a specialization for a FACTORY-PROVIDED cell type
				// then do NOT modify it.
				if (!CellTypeDLL::isUserDefined (ct.celltype_index) && !AnalysisDefinitionDLL::isUserDefined (it->analysis_index))
				{
					Logger::L ().Log (MODULENAME, severity_level::error, "SpecializeAnalysisForCellType: cannot replace factory specialization");
					return HawkeyeError::eInvalidArgs;
				}

				found_ad = true;

				//If it is an existing specialized analysis definition then update it, do not erase it.
				modified_ad_str = AnalysisDefinitionDLL::UpdateAnalysisDefinition (AnalysisDefinitionDLL (ad), *it);

				//// Delete existing so it's replaced with new.
				//ct.analysis_specializations.erase(it);
				break;
			}

			std::string des = {};
			auto ct_deatils_des = "CellType :" + ct.label + " Index " + std::to_string (ct.celltype_index) + "\n";

			audit_event_type event_type = evt_analysiscreate;
			// No specialization found in the CellType, So add the new one.
			if (!found_ad)
			{
				AnalysisDefinitionDLL new_adDLL{ ad };
				ct.analysis_specializations.push_back (new_adDLL);
				des = "Added the new specialized analysis definition to " + ct_deatils_des + AnalysisDefinitionDLL::ToStr (new_adDLL);
			}
			else
			{
				event_type = evt_analysismodify;
				if (!modified_ad_str.empty ())
					des = "Modified the specialized analysis definition of " +  ct_deatils_des + modified_ad_str;
			}

			// Update audit log and info file only if there is changes else just exit with success.
			if (!des.empty ())
			{
				AuditLogger::L().Log(generateAuditWriteData(
					loggedInUsername,
					event_type,
					des));
			}

			Logger::L ().Log (MODULENAME, severity_level::debug3, "SpecializeAnalysisForCellType: <exit 2>");
			return HawkeyeError::eSuccess;
		}
	}

	Logger::L ().Log (MODULENAME, severity_level::debug1, "SpecializeAnalysisForCellType: <exit, not found>");
	return HawkeyeError::eEntryNotFound;
}

//*****************************************************************************
bool HawkeyeLogicImpl::GetCellTypeByIndex (uint32_t index, CellTypeDLL& cellType)
{
	cellType = {};

	if ((index == TEMP_CELLTYPE_INDEX) && pTempCellType_)
	{
		cellType = *pTempCellType_;
		return true;
	}

	return CellTypesDLL::getCellTypeByIndex (index, cellType);
}

//*****************************************************************************
std::shared_ptr<CellTypeDLL> HawkeyeLogicImpl::GetTemporaryCellType()
{
	return pTempCellType_;
}

//*****************************************************************************
void HawkeyeLogicImpl::SetTemporaryCellType (std::shared_ptr<CellTypeDLL> cellType)
{
	pTempCellType_.reset (new CellTypeDLL{ *cellType });
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetTemporaryCellType (CellType*& temp_cell)
{
	temp_cell = nullptr;

	// This must be done by elevated user or higher.
	const auto status = UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		if (status == HawkeyeError::eNotPermittedAtThisTime) {
			return status;
		}
		AuditLogger::L().Log(generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized,
			"Get Temporary CellType"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (!pTempCellType_.get())
	{
		temp_cell = nullptr;
		return HawkeyeError::eNoneFound;
	}

	DataConversion::convert_dllType_to_cType (*pTempCellType_, temp_cell);

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetTemporaryCellType (CellType* temp_cell)
{
	// This must be done by elevated user or higher.
	auto status =UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		// No logged in user
		if (status == HawkeyeError::eNotPermittedAtThisTime)
			return status;

		AuditLogger::L().Log(generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized,
			"Set Temporary CellType"));
		return HawkeyeError::eNotPermittedByUser;
	}

	// ToDo - validate the incoming "temp_cell"

	if (temp_cell == nullptr)
	{
		Logger::L ().Log (MODULENAME, severity_level::critical, "SetTemporaryCellType: <exit, input *temp_cell* is nullptr>");
		pTempCellType_.reset ();
		return HawkeyeError::eInvalidArgs;
	}

	temp_cell->celltype_index = TEMP_CELLTYPE_INDEX;
	pTempCellType_.reset (new CellTypeDLL{ *temp_cell });

	if (Logger::L ().IsOfInterest (severity_level::debug1))
	{
		Logger::L ().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("SetTemporaryCellType - Definition\n%s")
				% CellTypeDLL::CellTypeAsString (*pTempCellType_)));
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::SetTemporaryCellTypeFromExisting (uint32_t ct_index)
{
	// This must be done by elevated user or higher.
	auto status =UserList::Instance().CheckPermissionAtLeast (UserPermissionLevel::eElevated);
	if (status != HawkeyeError::eSuccess)
	{
		// No logged in user
		if (status == HawkeyeError::eNotPermittedAtThisTime)
			return status;

		AuditLogger::L().Log(generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized,
			"Set Temporary CellType From Existing"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (!CellTypesDLL::isCellTypeIndexValid (ct_index))
	{
		pTempCellType_.reset();
		Logger::L ().Log (MODULENAME, severity_level::critical, "SetTemporaryCellTypeFromExisting: <exit, not found>");
		return HawkeyeError::eInvalidArgs;
	}

	pTempCellType_ = std::make_shared<CellTypeDLL>();
	if (!CellTypesDLL::getCellTypeByIndex (ct_index, *pTempCellType_))
	{
		pTempCellType_.reset();
		return HawkeyeError::eEntryNotFound;
	}

	pTempCellType_->celltype_index = TEMP_CELLTYPE_INDEX;

	if (Logger::L ().IsOfInterest (severity_level::debug1))
	{
		Logger::L ().Log (MODULENAME, severity_level::debug1,
			boost::str (boost::format ("SetTemporaryCellTypeFromExisting:\nTemporary CellType from CellTypeIndex %d\n%s")
				% ct_index
				% CellTypeDLL::CellTypeAsString (*pTempCellType_)));
	}

	return HawkeyeError::eSuccess;
}
