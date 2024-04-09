#pragma once
#include <cstdint>

struct DataSignature_t
{
	char* short_text;
	char* long_text;
};

struct DataSignatureInstance_t
{
	DataSignature_t signature;
	
	char* signing_user;  /// user_id copy

	uint64_t timestamp;  /// seconds from epoch UTC
};
