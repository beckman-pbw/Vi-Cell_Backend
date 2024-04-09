#include "stdafx.h"

#include <time.h>

#include "AuditLog.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "QualityControlsDLL.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_QualityControl";

//*****************************************************************************
std::vector<QualityControlDLL> getQualityControlsDLL()
{
	return QualityControlsDLL::Get();
}

//*****************************************************************************
bool HawkeyeLogicImpl::initializeQualityControl() const
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeQualityControl: <enter>");

	bool retStatus = QualityControlsDLL::Initialize();

	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeQualityControl: <exit>");

	return retStatus;
}

//*****************************************************************************
static bool CheckForMatchingCellType(uint32_t ci_toCheck, const std::vector<uint32_t>&v_ct)
{
	auto item = std::find_if(v_ct.begin(), v_ct.end(),
		[ci_toCheck](const auto& item) { return item == ci_toCheck; });

	return (item != v_ct.end());
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::GetQualityControlList (const char* username, const char* password, bool allFlag, QualityControl_t*& qualitycontrols, uint32_t& num_qcs)
{
	num_qcs = 0;
	qualitycontrols = nullptr;

	std::vector<QualityControlDLL>& qcsDLL = QualityControlsDLL::Get();

	std::vector<uint32_t> v_celltypes;
	UserList::Instance().GetAllowedCellTypeIndices (username, v_celltypes);

	// Admin or higher should have access to all CellTypes.
	bool isAdminOrHigher = UserList::Instance().IsUserPermissionAtLeast(username, UserPermissionLevel::eAdministrator);

	size_t index = 0;
	std::vector<size_t> v_matchingQualityControl;

	if (allFlag)
	{
		num_qcs = static_cast<uint32_t>(qcsDLL.size());
		qualitycontrols = new QualityControl_t[num_qcs];

		for (auto qc : qcsDLL)
		{
			if (!qc.retired)
			{
				qualitycontrols[index++] = qc.ToCStyle();
			}
		}

		num_qcs = static_cast<uint32_t>(index);
	}
	else
	{
		for (auto qc : qcsDLL)
		{
			if (!qc.retired)
			{
				if (isAdminOrHigher || CheckForMatchingCellType(qc.cell_type_index, v_celltypes))
				{
					v_matchingQualityControl.push_back(index);
				}
			}

			index++;
		}

		num_qcs = (uint32_t)v_matchingQualityControl.size();
		if (0 == num_qcs)
		{
			return HawkeyeError::eNoneFound;
		}

		qualitycontrols = new QualityControl_t[num_qcs];
		for (index = 0; index < num_qcs; index++)
		{
			qualitycontrols[index] = qcsDLL[v_matchingQualityControl[index]].ToCStyle();
		}
	}

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::AddQualityControl (const char* username, const char* password, QualityControl_t qc, const char* retiredQcName)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "AddQualityControl: <enter>");

	// This must be done by an Elevated or higher.	
	if (!UserList::Instance().IsUserPermissionAtLeast(username, UserPermissionLevel::eElevated))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			username,
			audit_event_type::evt_notAuthorized, 
			"Add Quality Control"));
		return HawkeyeError::eNotPermittedByUser;
	}

	std::vector<QualityControlDLL>& qcsDLL = QualityControlsDLL::Get();

	// When the retired QC name is empty the specified the QC is either a new QC <or>
	// a copy of an existing QC.
	// Otherwise, the QC is an edit of an existing QC.
	if (strlen(retiredQcName) == 0)
	{
		// Check for duplicate name.
		for (auto& eqc : qcsDLL)
		{
			if (eqc.qc_name == qc.qc_name) {
				Logger::L ().Log (MODULENAME, severity_level::debug1, "AddQualityControl: <exit, already exists>");
				return HawkeyeError::eAlreadyExists;
			}
		}
	}
	else
	{
		// If the existing CellType (by name) exists, mark it as "retired".
		for (auto& eqc : qcsDLL)
		{
			if (eqc.qc_name == qc.qc_name)
			{
				eqc.retired = true;
				eqc.qc_name = retiredQcName;

				QualityControlDLL updatedQC = {};
				std::string qcinfo_str = QualityControlDLL::UpdateQualityControl (eqc, updatedQC);
				if (!qcinfo_str.empty ())
				{
					AuditLogger::L().Log(generateAuditWriteData(
						username,
						audit_event_type::evt_qcontrolmodify,
						qcinfo_str));
				}

				break;
			}
		}
	}

	
	QualityControlDLL qcDLL{ qc };
	auto status = QualityControlsDLL::Add (username, qcDLL);

	Logger::L().Log (MODULENAME, severity_level::debug1, "AddQualityControl: <exit>");

	return status;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeListOfQualityControl (QualityControl_t* list, uint32_t n_items)
{
	if (list == nullptr)
	{
		return;
	}

	for (uint32_t i = 0; i < n_items; i++)
	{
		delete[] list[i].qc_name;
		delete[] list[i].lot_information;
		delete[] list[i].comment_text;
	}
	delete[] list;
}
