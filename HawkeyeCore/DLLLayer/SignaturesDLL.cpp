#include "stdafx.h"

#include "AuditLog.hpp"
#include "HawkeyeDirectory.hpp"
#include "SecurityHelpers.hpp"
#include "SetIndentation.hpp"
#include "SignaturesDLL.hpp"
#include "UserList.hpp"

static std::vector<DataSignatureDLL> signaturesDLL_;

static const char MODULENAME[] = "Signatures";

static const char SALT_MESSAGE[] = "This Is The Signature Salt";
static const int SIGN_REPETITIONS = 5;
static bool isInitialized = false;

//*****************************************************************************
std::vector<DataSignatureDLL> SignaturesDLL::Get()
{
	return signaturesDLL_;
}

//*****************************************************************************
static std::string stringifySignatureText (DataSignatureDLL& ds, uint8_t indent = 0)
{
	return
		SetIndentation(indent) + "Short text: \"" + ds.short_text + "\"" +
		SetIndentation(indent) + "Long text:  \"" + ds.long_text + "\"";
}

//*****************************************************************************
static bool validateSignatureText (const std::string& inputText, const std::string& inputSignatureText)
{
	std::string signText = SignaturesDLL::SignText (inputText);
	return signText == inputSignatureText;
}

//*****************************************************************************
bool loadSignature (
	boost::property_tree::basic_ptree<std::string, std::string>::assoc_iterator& it,
	DataSignatureDLL& signature)
{
	std::string data = it->second.get<std::string>("short_text");
	std::string data_signature = it->second.get<std::string>("short_text_signature");
	if (validateSignatureText (data, data_signature) == false)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "LoadSignature : signature <short text> validation failed!");
		return false;
	}
	signature.short_text = data;
	signature.short_text_signature = data_signature;

	data = it->second.get<std::string>("long_text");
	data_signature = it->second.get<std::string>("long_text_signature");
	if (validateSignatureText (data, data_signature) == false)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "LoadSignature : signature <long text> validation failed!");
		return false;
	}

	signature.long_text = data;
	signature.long_text_signature = data_signature;

	return true;
}

//*****************************************************************************
static void setSignatureDefinitions (
	const DataSignatureDLL& ds, boost::property_tree::ptree& pt_signatureDefinition)
{
	std::string shortTextSignature = SignaturesDLL::SignText (ds.short_text);
	pt_signatureDefinition.put("short_text", ds.short_text);
	pt_signatureDefinition.put("short_text_signature", shortTextSignature);

	std::string longTextSignature = SignaturesDLL::SignText (ds.long_text);
	pt_signatureDefinition.put("long_text", ds.long_text);
	pt_signatureDefinition.put("long_text_signature", longTextSignature);
}

//*****************************************************************************
std::pair<std::string, boost::property_tree::ptree> SignaturesDLL::Export()
{
	boost::property_tree::ptree pt_signatureDefinitions = {};
	for each (auto item in signaturesDLL_)
	{
		boost::property_tree::ptree pt_sig = {};
		setSignatureDefinitions (item, pt_sig);
		pt_signatureDefinitions.add_child("signature_definition", pt_sig);
	}

	std::string combineShortText = {};
	for (auto& item : signaturesDLL_)
	{
		combineShortText.append(item.short_text);
	}
	std::string combineshortTextSignature = SignaturesDLL::SignText (combineShortText);
	pt_signatureDefinitions.put("definition_set_signature", combineshortTextSignature);

	return std::make_pair ("signature_definitions", pt_signatureDefinitions);
}

//*****************************************************************************
std::vector<DataSignatureDLL>& GetDataSignatureDeinitions()
{
	return signaturesDLL_;
}

//*****************************************************************************
bool SignaturesDLL::Initialize()
{
	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <enter>");

	if (isInitialized)
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Initialize: <exit, already initialized>");
		return true;
	}

	std::vector<DBApi::DB_SignatureRecord> dbDataSignaturesDLL_;
	DBApi::eQueryResult dbStatus = DBApi::DbGetSignatureList(
		dbDataSignaturesDLL_,
		DBApi::eListFilterCriteria::NoFilter,
		"",
		"",
		-1,
		DBApi::eListSortCriteria::SortNotDefined,
		DBApi::eListSortCriteria::SortNotDefined,
		0,
		"",
		-1,
		-1,
		"");
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		if (DBApi::eQueryResult::NoResults != dbStatus)
		{
			Logger::L().Log (MODULENAME, severity_level::error,
				boost::str (boost::format ("Initialize: <exit, DbGetSignatureList failed, status: %ld>") % (int32_t)dbStatus));
			ReportSystemError::Instance().ReportError (BuildErrorInstance(
				instrument_error::instrument_storage_writeerror,
				instrument_error::instrument_storage_instance::signatures,
				instrument_error::severity_level::error));
			return false;
		}

		signaturesDLL_.clear();

		return true;

	} // End "if (DBApi::eQueryResult::QueryOk != dbStatus)"

	// Signture definitions were found in DB, convert to SignatureDLL.
	for (auto& v : dbDataSignaturesDLL_)
	{
		DataSignatureDLL sd;
		sd.FromDbStyle (v);
		signaturesDLL_.push_back (sd);
	}

	isInitialized = true;

	Logger::L().Log (MODULENAME, severity_level::debug2, "Initialize: <exit>");

	return true;
}

//*****************************************************************************
void SignaturesDLL::Import (boost::property_tree::ptree& ptParent)
{
	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <enter>");

	Initialize();

	// If the "signature_definitions" tag is found then this data is coming from 
	// the v1.2 SIgnatures.info file whose parent is the "config" tag.
	auto pSignatures = ptParent.get_child_optional ("signature_definitions");
	if (pSignatures) {
		ptParent = pSignatures.get();
	}

	auto signatureRange = ptParent.equal_range("signature_definition");

	std::vector<DataSignatureDLL> sigsToLoadDLL;
	std::string combineShortText = {};

	for (auto it = signatureRange.first; it != signatureRange.second; ++it)
	{
		DataSignatureDLL sigDLL;
		if (!loadSignature(it, sigDLL))
		{
			Logger::L().Log(MODULENAME, severity_level::error, "Import: <exit, <signature_definitions> failures loading signature>");
			return;
		}

		sigsToLoadDLL.push_back (sigDLL);
		combineShortText.append (sigDLL.short_text);
	}

	std::string setSignature = ptParent.get<std::string>("definition_set_signature");

	if (!validateSignatureText (combineShortText, setSignature))
	{
		Logger::L().Log(MODULENAME, severity_level::error, "Import: <exit, failed to validate definition_set_signature tag>");
		return;
	}

	for (auto& sig : sigsToLoadDLL)
	{
		auto item = std::find_if (signaturesDLL_.begin(), signaturesDLL_.end(),
			[sig](const auto& item) { return item.short_text == sig.short_text; });

		// Only add signature if it doesn't already exist.
		if (item == signaturesDLL_.end())
		{
			Add (sig);
		}
		else
		{
			Logger::L().Log(MODULENAME, severity_level::normal,
				boost::str (boost::format ("Import: duplicate signature index found: %s>") % item->short_text));
		}
	}

	Logger::L().Log(MODULENAME, severity_level::debug1, "Import: <exit>");
}

//*****************************************************************************
HawkeyeError SignaturesDLL::Add (DataSignatureDLL& signatureDLL)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <enter>");

	// Only three signatures are allowed.
	if (signaturesDLL_.size() == 3)
	{
		Logger::L().Log (MODULENAME, severity_level::error, "Add: <exit, only three signatues are allowed>");
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_notallowed,
			instrument_error::instrument_storage_instance::signatures,
			instrument_error::severity_level::warning));
		return HawkeyeError::eNotPermittedAtThisTime;
	}

	if (signatureDLL.long_text.empty() ||
		signatureDLL.short_text.empty())
	{
		Logger::L().Log (MODULENAME, severity_level::warning, "Add: <exit, long or short text is empty>");
		return HawkeyeError::eInvalidArgs;
	}

	auto it = std::find_if (signaturesDLL_.begin(), signaturesDLL_.end(),
		[&signatureDLL](const DataSignatureDLL& element) -> bool
		{
			return element.short_text == signatureDLL.short_text || element.long_text == signatureDLL.long_text;
		});

	if (it != signaturesDLL_.end())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <exit, already exists>");
		return HawkeyeError::eAlreadyExists;
	}

	// Add the signature definition to the DB.
	signatureDLL.long_text_signature  = SignaturesDLL::SignText (signatureDLL.long_text);
	signatureDLL.short_text_signature = SignaturesDLL::SignText (signatureDLL.short_text);

	DBApi::DB_SignatureRecord dbSignatureRecord = signatureDLL.ToDbStyle();

	DBApi::eQueryResult dbStatus = DBApi::DbAddSignature (dbSignatureRecord);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("Add: <exit, DbAddSignature failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_writeerror,
			instrument_error::instrument_storage_instance::signatures,
			instrument_error::severity_level::error));
		return HawkeyeError::eStorageFault;
	}

	signatureDLL.uuid = dbSignatureRecord.SignatureDefId;

	signaturesDLL_.push_back(signatureDLL);

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetLoggedInUsername(),
		audit_event_type::evt_signaturedefinitionadd, 
		stringifySignatureText(signatureDLL)));

	Logger::L().Log (MODULENAME, severity_level::debug1, "Add: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
HawkeyeError SignaturesDLL::Remove (const std::string& shortText)
{
	Logger::L().Log (MODULENAME, severity_level::debug1, "Remove: <enter>");

	auto it = std::find_if (signaturesDLL_.begin(), signaturesDLL_.end(),
		[shortText](const DataSignatureDLL& element)
		{
			return element.short_text == shortText;
		});

	if (it == signaturesDLL_.end())
	{
		Logger::L().Log (MODULENAME, severity_level::debug1, "Remove: <exit, not found>");
		return HawkeyeError::eEntryNotFound;
	}

	AuditLogger::L().Log (generateAuditWriteData(
		UserList::Instance().GetLoggedInUsername(),
		audit_event_type::evt_signaturedefinitionremove,
		stringifySignatureText(*it)));

	DBApi::eQueryResult dbStatus = DBApi::DbRemoveSignatureByUuid (it->uuid);
	if (DBApi::eQueryResult::QueryOk != dbStatus)
	{
		Logger::L().Log (MODULENAME, severity_level::error,
			boost::str (boost::format ("Remove: <exit, DbRemoveSignature failed, status: %ld>") % (int32_t)dbStatus));
		ReportSystemError::Instance().ReportError (BuildErrorInstance(
			instrument_error::instrument_storage_deleteerror,
			instrument_error::instrument_storage_instance::signatures,
			instrument_error::severity_level::error));
		return HawkeyeError::eStorageFault;
	}

	signaturesDLL_.erase (it);

	Logger::L().Log (MODULENAME, severity_level::debug1, "Remove: <exit>");

	return HawkeyeError::eSuccess;
}

//*****************************************************************************
std::string SignaturesDLL::SignText (const std::string& inputText)
{
	std::vector<unsigned char> message(inputText.begin(), inputText.end());

	std::string salt_msg = std::string(SALT_MESSAGE);
	std::vector<unsigned char> salt(salt_msg.begin(), salt_msg.end());

	return SecurityHelpers::CalculateHash(message, salt, SIGN_REPETITIONS);
}
