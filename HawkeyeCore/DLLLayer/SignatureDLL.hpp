#pragma once

#include <iostream>

#include <DBif_Api.h>

#include "ChronoUtilities.hpp"
#include "DataConversion.hpp"
#include "Signature.hpp"

struct DataSignatureDLL
{
	DataSignatureDLL() 
	{
		dbIdNum = 0;
		uuid = {};
	}
	DataSignatureDLL (DataSignature_t ds)
	{
		dbIdNum = 0;
		uuid = {};
		DataConversion::convertToStandardString(short_text, ds.short_text);
		DataConversion::convertToStandardString(long_text, ds.long_text);
	}

	DataSignature_t ToCStyle()
	{
		DataSignature_t ds = {};
		DataConversion::convertToCharPointer(ds.short_text, short_text);
		DataConversion::convertToCharPointer(ds.long_text, long_text);

		return ds;
	}

	DBApi::DB_SignatureRecord ToDbStyle()
	{
		DBApi::DB_SignatureRecord dbSignatureRecord = {};

		dbSignatureRecord.SignatureDefIdNum  = dbIdNum;
		dbSignatureRecord.SignatureDefId     = uuid;
		dbSignatureRecord.ShortSignatureStr  = short_text;
		dbSignatureRecord.ShortSignatureHash = short_text_signature;
		dbSignatureRecord.LongSignatureStr   = long_text;
		dbSignatureRecord.LongSignatureHash  = long_text_signature;

		return dbSignatureRecord;
	}

	void FromDbStyle (const DBApi::DB_SignatureRecord& dbSignatureRecord)
	{
		dbIdNum              = dbSignatureRecord.SignatureDefIdNum;
		uuid                 = dbSignatureRecord.SignatureDefId;
		short_text           = dbSignatureRecord.ShortSignatureStr;
		short_text_signature = dbSignatureRecord.ShortSignatureHash;
		long_text            = dbSignatureRecord.LongSignatureStr;
		long_text_signature  = dbSignatureRecord.LongSignatureHash;
	}

	int64_t dbIdNum;
	uuid__t uuid;
	std::string short_text;
	std::string short_text_signature;
	std::string long_text;
	std::string long_text_signature;
	std::string sig_hash;
};

struct DataSignatureInstanceDLL
{
	DataSignatureInstanceDLL() {}
	DataSignatureInstanceDLL(DataSignatureInstance_t dsi)
	{
		signature = DataSignature_t { dsi.signature };
		DataConversion::convertToStandardString(signing_user, dsi.signing_user);
		timestamp = ChronoUtilities::ConvertToTimePoint<std::chrono::seconds>(dsi.timestamp);
	}

	DataSignatureInstance_t ToCStyle()
	{
		DataSignatureInstance_t dsi = {};

		dsi.signature =  signature.ToCStyle();
		DataConversion::convertToCharPointer(dsi.signing_user, signing_user);
		dsi.timestamp = ChronoUtilities::ConvertFromTimePoint<std::chrono::seconds>(timestamp);

		return dsi;
	}

	DataSignatureDLL signature;

	std::string signing_user;  /// user_id copy
	system_TP timestamp;  /// seconds from epoch UTC
};
