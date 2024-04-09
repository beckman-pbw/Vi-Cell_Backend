#include "stdafx.h"

#include "AuditLog.hpp"
#include "FileSystemUtilities.hpp"
#include "HawkeyeDirectory.hpp"
#include "HawkeyeLogicImpl.hpp"
#include "SetIndentation.hpp"
#include "SignaturesDLL.hpp"

static const char MODULENAME[] = "HawkeyeLogicImpl_Signatures";

//*****************************************************************************
bool HawkeyeLogicImpl::initializeSignatures()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeSignatures: <enter>");

	bool retStatus = SignaturesDLL::Initialize();

	Logger::L().Log (MODULENAME, severity_level::debug2, "initializeSignatures: <exit with " + std::string(retStatus ? "success>" : "failure>"));
	return retStatus;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::AddSignatureDefinition (DataSignature_t* signature)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "AddSignatureDefinition: <enter>");

	// This must be done by an Administrator, restricted to Service Engineer
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Add Signature Definition"));
		return HawkeyeError::eNotPermittedByUser;
	}

	if (signature == nullptr)
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "AddSignatureDefinition: <exit, signature is nullptr>");
		return HawkeyeError::eInvalidArgs;
	}

	DataSignatureDLL signatureDLL{ *signature };

	auto he = SignaturesDLL::Add (signatureDLL);

	Logger::L().Log (MODULENAME, severity_level::debug1, "AddSignatureDefinition: <exit>");

	return he;
}

//*****************************************************************************
HawkeyeError HawkeyeLogicImpl::RemoveSignatureDefinition (char* signature_short_text, uint16_t short_text_len)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "RemoveSignatureDefinition: <enter>");

	// This must be done by an Administrator, restricted to Service Engineer
	// limited to local logins only
	if (!UserList::Instance().IsConsoleUserPermissionAtLeastAndNotService(UserPermissionLevel::eAdministrator))
	{
		AuditLogger::L().Log (generateAuditWriteData(
			UserList::Instance().GetLoggedInUsername(),
			audit_event_type::evt_notAuthorized, 
			"Remove Signature Definition"));
		return HawkeyeError::eNotPermittedByUser;
	}

	std::string shortText = std::string(signature_short_text);

	auto he = SignaturesDLL::Remove (shortText);

	Logger::L().Log (MODULENAME, severity_level::debug1, "RemoveSignatureDefinition: <exit>");

	return he;
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeDataSignature (DataSignature_t* signatures, uint16_t num_signatures)
{
	//Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDataSignature: <enter>");

	if (signatures == nullptr)
	{
		//TODO: save for debugging...
		//Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDataSignature: <exit, null ptr>");
		return;
	}

	for (uint16_t index = 0; index < num_signatures; index++)
	{
		delete[] signatures[index].short_text;
		delete[] signatures[index].long_text;
	}

	delete[] signatures;
	signatures = nullptr;

	//Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDataSignature: <exit>");
}

//*****************************************************************************
void HawkeyeLogicImpl::FreeDataSignatureInstance (DataSignatureInstance_t* signatures, uint16_t num_signatures)
{
	//Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDataSignatureInstance: <enter>");

	if (signatures == nullptr)
	{
		//TODO: save for debugging...
		//Logger::L().Log (MODULENAME, severity_level::debug1, "FreeDataSignatureInstance: <exit, null ptr>");
		return;
	}

	for (uint16_t index = 0; index < num_signatures; index++)
	{
		delete[] signatures[index].signature.short_text;
		delete[] signatures[index].signature.long_text;
		delete[] signatures[index].signing_user;
	}

	delete[] signatures;
	signatures = nullptr;

	//Logger::L().Log (MODULENAME, severity_level::debug1, "FrFreeDataSignatureInstance: <enter>eeDataSignatureInstance: <exit>");
}
