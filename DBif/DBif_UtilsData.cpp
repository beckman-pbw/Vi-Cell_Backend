// Database interface : implementation file
//

#pragma once

#include "pch.h"


#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>


#include "DBif_Impl.hpp"
#ifdef USE_LOGGER
#include "Logger.hpp"
#endif



static const std::string MODULENAME = "DBif_UtilsData";



////////////////////////////////////////////////////////////////////////////////
// DBifImpl database interface internal private utility methods
////////////////////////////////////////////////////////////////////////////////

// reset a GUID value to the 'nil' condition
void DBifImpl::ClearGuid( GUID& gval )
{
	gval.Data1 = 0;
	gval.Data2 = 0;
	gval.Data3 = 0;
	memset( gval.Data4, 0, sizeof( gval.Data4 ) );
}

// reset a uuid__t value to the 'nil' condition
void DBifImpl::ClearGuid( uuid__t& uval )
{
	memset( uval.u, 0, sizeof( uval.u ) );
}

// takes a standard-format string representation provided by a database query field
// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
//  produces a GUID value
void DBifImpl::GUID_from_DB_UUID_Str( std::string& uuid_str, GUID& guidval )
{
	uuid__t u = {};
	GUID gVal = {};

	ClearGuid( gVal );
	if ( HawkeyeUUID::Getuuid__tFromStr( uuid_str, u ) )
	{
		uuid__t_to_GUID( u, gVal );
	}

	guidval = gVal;
}

// takes a GUID value and produces a standard-format string representation
// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
// adds the single quote delimiters required for use in a DB insert or update statement
void DBifImpl::GUID_to_DB_UUID_Str( GUID& gval, std::string& guidstr )
{
	uuid__t tmpUuid = {};
	std::string uuidStr = "";

	GUID_to_uuid__t( gval, tmpUuid );
	FormatGuidString( tmpUuid, uuidStr );

	guidstr = boost::str( boost::format( "'%s'" ) % uuidStr );
}

// takes a standard-format string representation provided by a database query field
// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
//  produces a uuid__t value
void DBifImpl::uuid__t_from_DB_UUID_Str( std::string& uuid_str, uuid__t& g )
{
	uuid__t u = {};

	if ( !HawkeyeUUID::Getuuid__tFromStr( uuid_str, u ) )
	{
		memset( &u.u, 0, sizeof( u.u ) );
	}

	g = u;
}

// takes a uuid__t value and produces a standard-format string representation
// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
// adds the single quote delimiters required for use in a DB insert or update statement
void DBifImpl::uuid__t_to_DB_UUID_Str( uuid__t& g, std::string& guidstr )
{
	std::string uuidStr = "";
	FormatGuidString( g, uuidStr );

	guidstr = boost::str( boost::format( "'%s'" ) % uuidStr );
}

// takes a GUID value and produces a standard-format string representation
// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
void DBifImpl::FormatGuidString( GUID& gval, std::string& gvalstr )
{
	uuid__t tmpUuid__t = {};
	GUID_to_uuid__t( gval, tmpUuid__t );

	FormatGuidString( tmpUuid__t, gvalstr );
}

// takes a uuid__t value and produces a standard-format string representation
// -> "abcdefgh-ijkl-mnop-qrst-uvwxyzabcdef"
void DBifImpl::FormatGuidString( uuid__t& uval, std::string& gvalstr )
{
	HawkeyeUUID tmpUuid = HawkeyeUUID( uval );

	std::string tmpUuidStr = tmpUuid.GetStrFromuuid__t( uval );
	gvalstr = tmpUuidStr;
}

void DBifImpl::uuid__t_to_GUID( uuid__t& g, GUID& guidval )
{
	GUID gVal = {};
	uint8_t tmp_d0[4] = {};
	uint8_t tmp_d1[2] = {};
	uint8_t tmp_d2[2] = {};

	int srcidx = 0;
	int destidx = 3;

	for ( srcidx = 0, destidx = 3; destidx >= 0; srcidx++, destidx-- )
	{
		tmp_d0[destidx] = g.u[srcidx];
	}

	for ( destidx = 1; destidx >= 0; srcidx++, destidx-- )
	{
		tmp_d1[destidx] = g.u[srcidx];
	}

	for ( destidx = 1; destidx >= 0; srcidx++, destidx-- )
	{
		tmp_d2[destidx] = g.u[srcidx];
	}

	uint64_t* p4s = ( uint64_t* ) &g.u[8];
	uint64_t* p4d = ( uint64_t* ) &gVal.Data4;

	ClearGuid( gVal );
	gVal.Data1 = *( (uint32_t*) tmp_d0 );
	gVal.Data2 = *( (uint16_t*) tmp_d1 );
	gVal.Data3 = *( (uint16_t*) tmp_d2 );
	*p4d = *p4s;

	guidval = gVal;
}

void DBifImpl::GUID_to_uuid__t( GUID& guidval, uuid__t& g )
{
	uuid__t uVal = {};
	uint8_t tmp_d0[4] = {};
	uint8_t tmp_d1[2] = {};
	uint8_t tmp_d2[2] = {};

	memset( &uVal, 0, sizeof( uVal ) );

	uint32_t* p1 = (uint32_t*) tmp_d0;
	uint16_t* p2 = (uint16_t*) tmp_d1;
	uint16_t* p3 = (uint16_t*) tmp_d2;
	uint64_t* p4d = (uint64_t*) &uVal.u[8];
	uint64_t* p4s = (uint64_t*) &guidval.Data4;

	*p1 = guidval.Data1;
	*p2 = guidval.Data2;
	*p3 = guidval.Data3;
	*p4d = *p4s;

	int srcidx = 0;
	int destidx = 3;

	for ( srcidx = 3, destidx = 0; srcidx >= 0; srcidx--, destidx++ )
	{
		uVal.u[destidx] = tmp_d0[srcidx];
	}

	for ( srcidx = 1; srcidx >= 0; srcidx--, destidx++ )
	{
		uVal.u[destidx] = tmp_d1[srcidx];
	}

	for ( srcidx = 1; srcidx >= 0; srcidx--, destidx++ )
	{
		uVal.u[destidx] = tmp_d2[srcidx];
	}

	g = uVal;
}

#define MAX_DUPLICATES	16		// this would be the entire record, but still theoretically possible......

bool DBifImpl::IsUuid__tValid( uuid__t& u )
{
	bool uOk = true;

	try
	{
		static_assert( sizeof( u.u ) == BOOST_UUID_SIZE, "Unexpected UUID size!" );

		boost::uuids::uuid uuidInternal = boost::uuids::nil_uuid();
		memcpy( uuidInternal.data, u.u, sizeof( u.u ) );

		if ( uuidInternal.is_nil() )
			uOk = false;

		if ( uOk )
		{
			int duplicateCnt = 0;
			char lastChar = u.u[0];

			// look for ANY pattern of MAX_DUPLICATES repeating identical characters, since this would likely be an invalid uuid
			// using full record size repeats to avoid false positives, since this is likely an object initialization phenomenon...
			// should catch the ALL-CCCC case checked in the GUID checker above although...
			for ( int i = 1; i < BOOST_UUID_SIZE && uOk; i++ )
			{
				if ( u.u[i] == lastChar )
				{
					if ( ++duplicateCnt >= MAX_DUPLICATES )
					{
						uOk = false;
					}
				}
				else
				{
					duplicateCnt = 0;
				}
				lastChar = u.u[i];
			}
		}
	}
	catch ( ... )
	{
		uOk = false;
	}

	return uOk;
}

bool DBifImpl::GuidValid( uuid__t& g )
{
	if ( !IsUuid__tValid( g ) )
	{
		return false;
	}

	return true;
}

bool DBifImpl::GuidValid( GUID& gval )
{
	uuid__t g = {};

	GUID_to_uuid__t( gval, g );

	if ( !IsUuid__tValid( g ) )
	{
		return false;
	}

	return true;
}

const int GuidRetryCnt = 16;

// check a supplied uuid for validity; if no uuid supplied, generate a uuid and test resulting pattern for validity;
// repeat validation check and uuid generation for up to GuidRetryCnt times to exclude illegal patterns
DBApi::eQueryResult DBifImpl::GenerateValidGuid( uuid__t& recid )
{
	HawkeyeUUID tmpUuid = HawkeyeUUID::Generate();
	int tryCnt = 0;
	bool uuidOk = false;

	do
	{
		HawkeyeUUID tmpUuid = HawkeyeUUID::Generate();

		if ( !GuidValid( recid ) )
		{
			tmpUuid.get_uuid__t( recid );
		}

		if ( GuidValid( recid ) )
		{
			uuidOk = true;
		}
	} while ( !uuidOk && ++tryCnt < GuidRetryCnt );

	if ( !GuidValid( recid ) )
	{
		ClearGuid( recid );
		return DBApi::eQueryResult::InvalidUUIDError;
	}

	return DBApi::eQueryResult::QueryOk;
}

bool DBifImpl::GuidsEqual( uuid__t g1, uuid__t g2 )
{
	if ( memcmp( &g1, &g2, sizeof( uuid__t ) ) == 0 )
	{
		return true;
	}

	return false;
}

bool DBifImpl::GuidsEqual( const GUID lhsgval, GUID rhsgval )
{
	uint64_t* plhsData4 = (uint64_t*) &lhsgval.Data4;
	uint64_t* prhsData4 = (uint64_t*) &rhsgval.Data4;

	if ( lhsgval.Data1 == rhsgval.Data1 &&
		 lhsgval.Data2 == rhsgval.Data3 &&
		 lhsgval.Data3 == rhsgval.Data3 &&
		 *plhsData4 == *prhsData4 )
	{
		return true;
	}

//	if ( memcmp( &lhsgval, &rhsgval, sizeof( GUID ) ) == 0 )
//	{
//		return true;
//	}

	return false;
}

bool DBifImpl::GuidInsertCheck( uuid__t recid, std::string& idstr, int32_t errcode, bool allowempty )
{
	// may be null at the time of creation for records that are created prior to being run
	if ( GuidValid( recid ) )
	{
		uuid__t_to_DB_UUID_Str( recid, idstr );
	}
	else if ( allowempty )
	{
		idstr = boost::str( boost::format( "'%s'" ) % DBEmptyUuidStr );		// if allowing no valid ID, create the empty ID signature
	}
	else if ( !allowempty )
	{
#ifdef ALLOW_ID_FAILS
		idstr = boost::str( boost::format( "'%s'" ) % DBEmptyUuidStr );
#else
		errcode = MissingCriticalObjectID;			// missing critical identifier
		return false;
#endif // ALLOW_ID_FAILS
	}

	return true;
}

bool DBifImpl::GuidUpdateCheck( uuid__t dbid, uuid__t recid, std::string& idstr, int32_t errcode, bool allow_update, bool allow_empty )
{
	idstr.clear();

	// SHOULD NOT be null at any time, but allow an empty db record field to be updated prior to running...
	if ( !GuidValid( recid ) && !GuidValid( dbid ) )		// neither id value is valid
	{
		if ( !allow_empty )
		{
#ifdef ALLOW_ID_FAILS
			idstr = boost::str( boost::format( "'%s'" ) % DBEmptyUuidStr );
#else
			errcode = MissingCriticalObjectID;				// no valid critical identifier
			return false;
#endif // ALLOW_ID_FAILS
		}
	}
	else if ( !GuidValid( recid ) )							// db id is valid, but supplied record id value is not
	{
#ifdef ALLOW_ID_FAILS
		recid = dbid;
#endif // ALLOW_ID_FAILS

		if ( !allow_empty )
		{
#ifdef ALLOW_ID_FAILS
			uuid__t_to_DB_UUID_Str( dbid, idstr );			// use the db record id value, and update the passed record...;
#else
			errcode = MissingCriticalObjectID;				// missing critical identifier
			return false;
#endif // ALLOW_ID_FAILS
		}
	}
	else	//  supplied record id value is valid but DB record id value may not be
	{
		if ( GuidValid( dbid ) )									// both id values are valid
		{
			if ( !GuidsEqual( dbid, recid ) && !allow_update )		// both id values are valid but not equal and updates are not allowed
			{
#ifdef ALLOW_ID_FAILS
				recid = dbid;
#else
				errcode = BadCriticalObjectID;				// bad critical identifier
				return false;
#endif // ALLOW_ID_FAILS
			}
		}

		// supplied id value is valid but db record id value is not OR id values are not equal but allowed to update; use supplied id to update db
		uuid__t_to_DB_UUID_Str( recid, idstr );
	}

	return true;
}

int32_t DBifImpl::TokenizeStr( std::vector<std::string>& tokenlist, std::string& parsestr,
							   std::string sepstr, char* sepchars )
{
	size_t tokenCnt = 0;
	size_t chCnt = 0;
	size_t sepLen = sepstr.length();

	if ( sepchars != nullptr )
	{
		chCnt = std::strlen( sepchars );    // check count of standard characcter array length
	}

	// can't have both as non-zero length, and can't have both as zero length
	if ( ( ( sepLen > 0 ) && ( chCnt == 0 ) ) || ( ( sepLen == 0 ) && ( chCnt > 0 ) ) )
	{
		std::string::size_type tokenLen = 0;
		std::string::size_type startPos = 0;
		std::string::size_type endPos;
		std::string::size_type parseLen = parsestr.length();
		uint32_t chIdx = 0;

		tokenlist.clear();

		do
		{
			if ( chCnt > 0 )
			{
				chIdx = 0;
				endPos = std::string::npos;
				// look for ANY of the supplied token seperator characters...
				while ( ( sepchars[chIdx] != 0 ) && ( chIdx < chCnt ) && ( endPos == std::string::npos ) )
				{
					endPos = parsestr.find( sepchars[chIdx], startPos );
					if ( endPos == startPos )		// may happen if two identical separator characters in a row (typically spaces)
					{
						startPos++;
						endPos = std::string::npos;
					}
					else
					{
						chIdx++;
					}
				}
			}
			else
			{
				if ( sepstr.length() > 1 )
				{
					endPos = parsestr.find( sepstr, startPos );
				}
				else
				{
					endPos = parsestr.find( sepstr.at( 0 ), startPos );
				}
			}

			if ( endPos == std::string::npos && startPos < parseLen )	// handle search on final string segment...
			{
				endPos = parseLen;
			}

			if ( endPos != std::string::npos )
			{
				if ( endPos > startPos )
				{
					tokenLen = endPos - startPos;
					if ( tokenLen == 1 )
					{	// ensure this is not a token containing only the seperator character
						for ( int chkIdx = 0; chkIdx < chCnt && tokenLen > 0; chkIdx++ )
						{
							if ( parsestr.at( startPos ) == sepchars[chkIdx] )
							{
								tokenLen--;
							}
						}
					}

					if ( tokenLen >= 1 )
					{
						tokenlist.push_back( parsestr.substr( startPos, tokenLen ) );
						TrimWhiteSpace( tokenlist.back() );
					}

					if ( ( chCnt > 0 ) || ( sepstr.length() == 1 ) )
					{
						endPos++;   // account for the found token seperator character in the string being parsed
					}
					else
					{
						endPos += sepstr.length();
					}
				}
				else if ( endPos <= startPos || endPos == parseLen || startPos == parseLen )
				{
					endPos = std::string::npos;
				}

				startPos = endPos;
			}
		} while ( endPos != std::string::npos );
	}
	tokenCnt = tokenlist.size();

	return ( int32_t ) tokenCnt;
}

void DBifImpl::RemoveSubStrFromStr( std::string& tgtstr, std::string substr )
{
	std::string::size_type pos = 0;

	do
	{
		pos = tgtstr.find( substr );
		if ( pos != std::string::npos )
		{
			tgtstr.erase( pos, substr.length() );
		}
	} while ( pos != std::string::npos );
}

void DBifImpl::RemoveTgtCharFromStr( std::string& tgtstr, char tgtchar )
{
	std::string::size_type pos = 0;
	do
	{
		pos = tgtstr.find_first_of( tgtchar );
		if ( pos != std::string::npos )
		{
			tgtstr.erase( pos, 1 );
		}
	} while ( pos != std::string::npos );
}

// remove leading and trailing target characters from std::strings
void DBifImpl::TrimStr( std::string trim_chars, std::string& tgtstr )
{
	if ( trim_chars.length() == 0 || tgtstr.length() <= 0 )
	{
		return;
	}

	char tgtChar;
	int trimCharIdx = 0;
	size_t trimCharSize = trim_chars.length();
	bool done = false;

	// first trim the trailing characters of the string if necessary;
	size_t tgtIdx = 0;
	do
	{
		tgtIdx = tgtstr.length() - 1;
		tgtChar = tgtstr.at( tgtIdx );

		trimCharIdx = 0;
		do
		{
			if ( tgtChar == trim_chars[trimCharIdx] )
			{
				tgtstr.erase( tgtIdx, 1 );
				break;
			}

			if ( ++trimCharIdx >= trimCharSize )
			{
				done = true;
			}
		} while ( !done );
	} while ( tgtstr.length() > 0 && !done );

	// now remove any target leading characters if any remaining string...
	if ( tgtstr.length() > 0 )
	{
		done = false;
		do
		{
			tgtChar = tgtstr.at( 0 );

			trimCharIdx = 0;
			do
			{
				if ( ( tgtChar == trim_chars[trimCharIdx] ) )
				{
					tgtstr.erase( 0, 1 );
					break;
				}
				if ( ++trimCharIdx >= trimCharSize )
				{
					done = true;
				}
			} while ( !done );
		} while ( tgtstr.length() > 0 && !done );
	}
}

// remove leading and trailing spaces from std::strings
void DBifImpl::TrimWhiteSpace( std::string& tgtstr )
{
	std::string trimChars = " \t";

	TrimStr( trimChars, tgtstr );
}

void DBifImpl::StrToLower( std::string& tgtstr )
{
	size_t strsize = tgtstr.size();

	for ( uint32_t i = 0; i < strsize; i++ )
	{
		tgtstr.at( i ) = std::tolower( tgtstr.at( i ) );
	}
}

void DBifImpl::StrToUpper( std::string& tgtstr )
{
	size_t strsize = tgtstr.size();

	for ( uint32_t i = 0; i < strsize; i++ )
	{
		tgtstr.at( i ) = std::toupper( tgtstr.at( i ) );
	}
}

// find and remove the escaping backslash from special character sequences in strings;
// trimstrings should always be a backslash and the target escaped character;
// for representation in code that translates to an escaped backslash and an escaped target character;
//		e.g. \" must be entered in the code as \\\"
void DBifImpl::TrimSlashFromStr( std::string& valuestr, std::string trimstr )
{
	size_t idx = 0;
	size_t len = valuestr.length();

	if ( len > 1 )
	{
		do
		{
			idx = valuestr.find( trimstr );
			if ( idx < len )
			{
				valuestr.erase( idx, 1 );				// just removing the leading backslash from the in-string escape sequence
				if ( trimstr == "\\\'" )				// looking for embedded single-quote characters; requires special additional handling...
				{
					if ( valuestr[idx + 1] == '\'' )	// embedded single quotes require special handling with another single-quote character...
					{
						valuestr.erase( ++idx, 1 );		// remove the extra single quotes added...
					}
				}
				len = valuestr.length();
			}
		} while ( idx < len );
	}
}

// check strings to be used in SQL data values for embedded characters that need to be 'escaped'
// It appears that only the single-quote character need special handling...
void DBifImpl::DoStringSanitize( std::string& valuestr, std::string chkchars, bool dofilter )
{
	// first look for embedded double-quotes to convert to single quotes
	if ( valuestr.length() > 1 )
	{
		size_t chkCnt = chkchars.length();    // check count of standard characcter array length
		size_t chkIdx = 0;
		std::string::size_type startPos = 0;
		std::string::size_type endPos;
		std::string::size_type strLen = valuestr.length();

		do
		{
			endPos = std::string::npos;

			// first, serch for any previously escaped sequences...
			// look for ANY of the escapable characters...
			endPos = valuestr.find( chkchars[chkIdx], startPos );
			if ( endPos == std::string::npos )									// no characters found; look for the next character if any
			{
				chkIdx++;
				startPos = 0;
				endPos = startPos;
			}
			else
			{
				if ( ( chkchars[chkIdx] == '\'' || chkchars[chkIdx] == '\"' ) &&	// ignoring first and last character for the single-quote or double-quote characters
					 ( endPos == 0 || endPos == ( strLen - 1 ) ) )
				{
					startPos = endPos+1;
				}
				else if ( dofilter && IsFilterTerminatingCharacter( chkchars[chkIdx] ) )
				{
					valuestr.erase( endPos, ( strLen - endPos) );					// this character can't be properly escaped for handling in a filter statement, clear the rest of the filter string to avoid erroneous failures to finds data
					if ( valuestr[0] == '\'' )
					{
						valuestr.append( "'" );
					}
					else if ( valuestr[0] == '\"' )
					{
						valuestr.append( "\"" );
					}
					strLen = valuestr.length();
					WriteLogEntry( boost::str( boost::format( "Filter string truncation due to non-interpretable character: %c\n\tremaining filter string: '%s'") % chkchars[chkIdx] % valuestr ), InfoMsgType );
				}
				else
				{
					valuestr.insert( endPos, 1, '\\' );
					if ( dofilter )
					{
						if ( chkchars[chkIdx] == '\'' )								// embedded single-quote characters require special handling in filter strings
						{
							endPos++;
							valuestr.insert( endPos, 1, '\\' );
							endPos++;
							valuestr.insert( endPos, 1, chkchars[chkIdx] );
						}
						else if ( chkchars[chkIdx] == '\\' )						// check if embedded backslash is need an additional escape... i.e. \\ must become \\\\ for filter strings...
						{
							endPos++;
							valuestr.insert( endPos, 1, '\\' );
							endPos++;
							valuestr.insert( endPos, 1, '\\' );
						}
					}
					startPos = endPos + 2;
					strLen = valuestr.length();
				}
			}
		} while ( chkIdx < chkCnt );
	}
}

bool DBifImpl::IsFilterTerminatingCharacter( char testchar )
{
	std::string chkChars = "/{}[]";		// these are too tied to regex searches to be escaped and handled properly in a filter string...

	std::string::size_type endPos = chkChars.find( testchar, 0 );
	if ( endPos == std::string::npos )										// no characters found
	{
		return false;
	}
	return true;
}

// check strings to be used in SQL filter statements for embedded characters that need to be 'escaped'
// Most are associated with regex-type character substitution, meta-characters, or character-set definition
void DBifImpl::SanitizeFilterString( std::string& valuestr )
{
	std::string chkChars = "\\\"'()$%^*+=/{}[]";		// do the backslash character check first to avoid multiple backslash additions...

	DoStringSanitize( valuestr, chkChars, true );
}

// check strings to be used in paths for embedded characters that need to be 'escaped'
void DBifImpl::SanitizePathString( std::string& valuestr )
{
	std::string chkChars = "\\";

	DoStringSanitize( valuestr, chkChars, false );
}

// check strings to be used in SQL data values for embedded characters that need to be 'escaped'
// It appears that only the single-quote character need special handling...
void DBifImpl::SanitizeDataString( std::string& valuestr )
{
	std::string chkChars = "\\'";

	DoStringSanitize( valuestr, chkChars, true );
}

// check a list of strings for embedded characters that need to be 'escaped'
void DBifImpl::SanitizeDataStringList( std::vector<std::string> srclist, std::vector<std::string>& destlist )
{
	size_t listSize = srclist.size();
	destlist.clear();

	for ( size_t i = 0; i < listSize; i++ )
	{
		std::string tokenStr = srclist.at( i );

		// first look for embedded double-quotes to convert to single quotes
		if ( tokenStr.length() > 1 )
		{
			SanitizeDataString( tokenStr );
		}
		destlist.push_back( tokenStr );
	}
}

// check a path for embedded escaped '\' characters that need to be cleaned of their escaping backslash;
// some of these are not added deliberately but mnay be added by the database on retrieve
void DBifImpl::DeSanitizePathString( std::string& valuestr )
{
	// first look for embedded double-quotes to convert to single quotes
	if ( valuestr.length() > 1 )
	{
		TrimSlashFromStr( valuestr, "\\\\" );	// remove escaping character from escaped backslash \\ sequences
	}
}

// check a list of strings for embedded escaped' characters that need to be cleaned of their escaping backslash;
// some of these are not added deliberately but mnay be added by the database on retrieve
void DBifImpl::DeSanitizeDataString( std::string& valuestr )
{
	// first look for embedded double-quotes to convert to single quotes
	if ( valuestr.length() > 1 )
	{
		TrimSlashFromStr( valuestr, "\\\\" );	// remove escaping character from escaped backslash \\ sequences
		TrimSlashFromStr( valuestr, "\\\"" );	// remove escaping character from escaped double-quote \" sequences
		TrimSlashFromStr( valuestr, "\\\'" );	// remove escaping character from escaped single-quote \' sequences
		TrimSlashFromStr( valuestr, "\\%" );	// remove escaping character from escaped percent character \% sequences
	}
}

// check strings to be used in SQL statements for embedded characters that need to be 'escaped'
void DBifImpl::DeSanitizeDataStringList( std::vector<std::string> srclist, std::vector<std::string>& destlist )
{
	size_t listSize = srclist.size();
	destlist.clear();

	for ( size_t i = 0; i < listSize; i++ )
	{
		std::string tokenStr = srclist.at( i );

		// first look for embedded double-quotes to convert to single quotes
		if ( tokenStr.length() > 1 )
		{
			DeSanitizeDataString( tokenStr );
		}
		destlist.push_back( tokenStr );
	}
}

// Parse a string into a list of sub-string tokens using the token separator charactors or seperator string supplied.
//
// Takes a flag parameter to request optional trimming leading and trailing speces from the supplied string to be parsed
// and the extracted token strings.
//
// Takes a list of token seperator characters, or a specific token seperator string to use to find the tokens.
// Uses EITHER the seperator characters OR the seperator string. If both are supplied, returns '0' as an error indicator.
// If neither seperator characters or seperator string are supplied, returns '0' as error indicator.
//
// Takes a list of chanacters to be removed from the supplied parse string, acted on if the list of characters
// to be removed is non-zero length.
//
// returns the number of sub-strings tokens found or '0' on any error
int32_t DBifImpl::ParseStringToTokenList( std::vector<std::string>& tokenlist,
										  std::string& parsestr, std::string sepstr,
										  char* sepchars, bool do_trim, std::string trimchars )
{
	uint32_t tokenCnt = 0;
	size_t chCnt = 0;
	size_t sepLen = sepstr.length();

	tokenlist.clear();

	if ( sepchars != nullptr )
	{
		chCnt = std::strlen( sepchars );    // check count of standard character array length
	}

	// can't have both as non-zero length, and can't have both as zero length
	if ( ( ( sepLen > 0 ) && ( chCnt == 0 ) ) || ( ( sepLen == 0 ) && ( chCnt > 0 ) ) )
	{
		if ( parsestr.length() > 0 )
		{
			std::vector<std::string> parseTokenList;
			uint32_t idx = 0;

			if ( do_trim )
			{
				TrimWhiteSpace( parsestr );										// remove leading and trailing spaces
			}

			if ( trimchars.length() > 0 )
			{
				for ( idx = 0; idx < trimchars.length(); idx++ )
				{
					RemoveTgtCharFromStr( parsestr, trimchars.at( idx ) );		// remove any other unwanted characters to make parsing consistent
				}
			}

			parseTokenList.clear();

			tokenCnt = TokenizeStr( parseTokenList, parsestr, sepstr, sepchars );
			if ( tokenCnt == parseTokenList.size() )
			{
				std::string tokenStr;

				for ( idx = 0; idx < tokenCnt; idx++ )
				{
					tokenStr = parseTokenList.at( idx );
					if ( do_trim )
					{
						TrimWhiteSpace( tokenStr );								// remove leading and trailing spaces
					}
					tokenlist.push_back(tokenStr);
				}
			}
			else
			{
				tokenCnt = 0;
			}
		}
	}
	return tokenCnt;
}

// parse a string into a list of individual sub-strings using the space character as the delimiter
//
// returns the number of sub-strings found or '0' on error
int32_t DBifImpl::ParseStrToStrList( std::vector<std::string>& strlist, std::string& parsestr )
{
	int32_t tokenCnt = 0;

	if ( parsestr.length() > 0 )
	{
		std::string sepStr = "";
		std::string sepChars = " ";
		std::string trimChars = "";     // don't specify a space in the trim character list when it is the desired separator,
										// or when the strings to be tokenized may contain embedded strings...
		char* pSepChars = ( char* ) sepChars.c_str();

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( strlist, parsestr, sepStr, pSepChars, true, trimChars );
	}
	return tokenCnt;
}

// parse a database 2 dimensional array string of string values into a list of individual sub-strings;
// DOES NOT parse the sub-strings! This must be done in a subsequent call
// to the ParseArrayStrToArrayList method.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// takes a string in the format
//
//    [ {x1,y1,z1},{x2,y2,z2},{x3,y3,z3},...,{xn,yn,zn} ]
//         or
//    [ {'stringx1','stringy1','stringz1'},{'stringx2','stringy2','stringz2'},...,{'stringxn','stringyn','stringzn'} ]
//
// and breaks it into a vector osf strings in format
//
//    {x1,y1,z1}    or    {'stringa1','stringb1','stringc1'}
//    {x2,y2,z2}    or    {'stringa2','stringb2','stringc2'}
//    {x3,y3,z3}    or    {'stringa3','stringb3','stringc3'}
//         .
//         .
//         .
//    {?n,?'n,?"n}    or    {'string?1','string?'1','string?"1'}
//
// returns the number of sub-strings found or '0' on error
int32_t DBifImpl::ParseMultiStrArrayStrToStrList( std::vector<std::string>& subarraystrlist, std::string& arraystr )
{
	int32_t tokenCnt = 0;

	if ( arraystr.length() > 0 )
	{
		std::string parseStr = arraystr;
		std::vector<std::string> parseArrayStrList;

		// for database multi-dimensional array strings, the ),( character sequence 
		// designates the separation of sub-array strings
		std::string sepStr = "),(";
		std::string trimChars = "\"";		// remove any embedded quotes, but do not remove the array seperator characters or spaces at this time

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( parseArrayStrList, parseStr, sepStr, nullptr, true, trimChars );

		if ( ( tokenCnt > 0 ) && ( tokenCnt == parseArrayStrList.size() ) )
		{
			long tokenIdx = 0;

			while ( tokenIdx < tokenCnt )
			{
				parseStr = parseArrayStrList.at( tokenIdx );
				if ( parseStr.length() > 0 )
				{
					// remove the leading and trailing array grouping curly braces and composite-type grouping parentheses
					RemoveTgtCharFromStr( parseStr, '{' );      // remove any array grouping curly braces in the array string
					RemoveTgtCharFromStr( parseStr, '}' );      // remove any array grouping curly braces in the array string
					RemoveTgtCharFromStr( parseStr, '(' );      // remove any composite-type grouping parentheses in the array string
					RemoveTgtCharFromStr( parseStr, ')' );      // remove any composite-type grouping parentheses in the array string
					parseArrayStrList[tokenIdx] = parseStr;
				}
				tokenIdx++;
			}

			subarraystrlist = parseArrayStrList;
		}
		else
		{
			tokenCnt = 0;
		}
	}

	return tokenCnt;
}

// parse a database array string of string values into a list of individual string value strings;
//  works with single-dimension database array strings (not multi-dimensional array strings)
//
// takes a string in the format
//
//    [ a,b,c,d,e,f,g,h,...,?} ]
//         or
//    [ 'stringa','stringb','stringc','stringd',...,'string?' ]
//
// and breaks it into a vector osf strings in format
//
//    a    or    'stringa'
//    b    or    'stringb'
//    c    or    'stringc'
//    .
//    .
//    .
//    ?    or    'string?'
//
// NOTE: when parsing arrays of strings, embedded spaces MUST NOT be removed, as it changes the string content!
//
// returns the number of data element sub-strings found or '0' on error
int32_t DBifImpl::ParseStrArrayStrToStrList( std::vector<std::string>& arraystrlist, std::string& arraystr )
{
	long tokenCnt = 0;

	if ( arraystr.length() > 0 )
	{
		std::string parseStr = arraystr;
		// for data array strings, the "," character
		// sequence designates the separation of data value strings
		std::string sepStr = ",";
		std::string trimChars = "(){}[]\"";            // this will get rid of the array boundary indicator, the sub-element group indicators, and quotes,  but leave embedded spaces...
		std::vector<std::string> parseArrayStrList = {};

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( parseArrayStrList, parseStr, sepStr, nullptr, true, trimChars );

		if ( ( tokenCnt > 0 ) && ( tokenCnt == parseArrayStrList.size() ) )
		{
			arraystrlist = parseArrayStrList;
		}
		else
		{
			tokenCnt = 0;
		}
	}
	return tokenCnt;
}

// parse a database 2 dimensional array string of non-string values into a list of individual sub-strings;
// DOES NOT parse the sub-strings! This must be done in a subsequent call
// to the ParseArrayStrToArrayList method.
//
// takes a string in the format
//
//    [ {x1,y1,z1},{x2,y2,z2},{x3,y3,z3},...,{xn,yn,zn} ]
//         or
//    [ {'stringx1','stringy1','stringz1'},{'stringx2','stringy2','stringz2'},...,{'stringxn','stringyn','stringzn'} ]
//
// and breaks it into a vector osf strings in format
//
//    {x1,y1,z1}    or    {'stringa1','stringb1','stringc1'}
//    {x2,y2,z2}    or    {'stringa2','stringb2','stringc2'}
//    {x3,y3,z3}    or    {'stringa3','stringb3','stringc3'}
//         .
//         .
//         .
//    {?n,?'n,?"n}    or    {'string?1','string?'1','string?"1'}
//
// NOTE: when parsing arrays of strings, embedded spaces MUST NOT be removed, as it changes the string content!
//
// returns the number of sub-strings found or '0' on error
int32_t DBifImpl::ParseMultiArrayStrToStrList( std::vector<std::string>& subarraystrlist, std::string& arraystr )
{
	int32_t tokenCnt = 0;

	if ( arraystr.length() > 0 )
	{
		std::string parseStr = arraystr;
		std::vector<std::string> parseArrayStrList = {};

		// for database multi-dimensional array strings, the ),( character
		// sequence designates the separation of sub-array strings
		std::string sepStr = "),(";
		std::string trimChars = " \"";		// remove any embedded quotes or spaces, but do not remove the array seperator characters at this time

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( parseArrayStrList, parseStr, sepStr, nullptr, true, trimChars );

		if ( ( tokenCnt > 0 ) && ( tokenCnt == parseArrayStrList.size() ) )
		{
			long tokenIdx = 0;
			std::string tokenStr = "";

			while ( tokenIdx < tokenCnt )
			{
				tokenStr = parseArrayStrList.at( tokenIdx );
				if ( tokenStr.length() > 0 )
				{
					// remove the leading and trailing array grouping curly braces and composite-type grouping parentheses
					RemoveTgtCharFromStr( tokenStr, '{' );      // remove any array grouping curly braces in the array string
					RemoveTgtCharFromStr( tokenStr, '}' );      // remove any array grouping curly braces in the array string
					RemoveTgtCharFromStr( tokenStr, '(' );      // remove any composite-type grouping parentheses in the array string
					RemoveTgtCharFromStr( tokenStr, ')' );      // remove any composite-type grouping parentheses in the array string
					parseArrayStrList[tokenIdx] = tokenStr;
				}
				tokenIdx++;
			}

			subarraystrlist = parseArrayStrList;
		}
		else
		{
			tokenCnt = 0;
		}
	}

	return tokenCnt;
}

// parse a database array string of non-string values into a list of individual data value strings;
//  works with single-dimension database array strings (not multi-dimensional array strings)
//
// takes a string in the format
//
//    [ a,b,c,d,e,f,g,h,...,?} ]
//         or
//    [ 'stringa','stringb','stringc','stringd',...,'string?' ]
//
// and breaks it into a vector of strings in format
//
//    a    or    'stringa'
//    b    or    'stringb'
//    c    or    'stringc'
//    .
//    .
//    .
//    ?    or    'string?'
//
// NOTE: when parsing arrays of strings, embedded spaces MUST NOT be removed, as it changes the string content!
//
// returns the number of data element sub-strings found or '0' on error
int32_t DBifImpl::ParseArrayStrToStrList( std::vector<std::string>& arraystrlist, std::string& arraystr )
{
	long tokenCnt = 0;

	if ( arraystr.length() > 0 )
	{
		std::string parseStr = arraystr;
		// for data array strings, the "," character
		// sequence designates the separation of data value strings
		std::string sepStr = ",";
		std::string trimChars = " (){}[]\"";            // this will get rid of the array boundary indicator, the sub-element group indicators, quotes, and embedded spaces...
		std::vector<std::string> parseArrayStrList = {};

		// passing 'true' for the 'dotrim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( parseArrayStrList, parseStr, sepStr, nullptr, true, trimChars );

		if ( ( tokenCnt > 0 ) && ( tokenCnt == parseArrayStrList.size() ) )
		{
			arraystrlist = parseArrayStrList;
		}
		else
		{
			tokenCnt = 0;
		}
	}
	return tokenCnt;
}

// parse a string containing an array of composite data-type structures into a list of individual array element sub-strings
// representing each of the composite type; 
// DOES NOT parse the individual composite-type strings sub-strings! This must be done in a subsequent calls
// to composite-specific parsing methods.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//   [ "(x1,y1,z1)","(a1,b1,c1)","(m1,n1,o1)" ]
//
// where each "(....)" element in a bracket represent a composite=type structure element of the parent data string
// and breaks it into a vector of strings in format
//
//    "(x1,y1,z1)"
//    "(a1,b1,c1)"
//    "(m1,n1,o1)"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseCompositeArrayStrToStrList( std::vector<std::string>& subarraystrlist, std::string& arraystr )
{
	int32_t tokenCnt = 0;

	if ( arraystr.length() > 0 )
	{
		std::string parseStr = arraystr;
		std::vector<std::string> parseArrayStrList = {};

		// for database composite types , the )","( character sequence 
		// designates the separation of composite types in the data string
		std::string sepStr = ")\",\"(";
		std::string trimChars = "";		// DO NOT REMOVE ANY EMBEDDED QUOTES, as these composites may contain other embedded composite elements!

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( parseArrayStrList, parseStr, sepStr, nullptr, true, trimChars );

		if ( ( tokenCnt > 0 ) && ( tokenCnt == parseArrayStrList.size() ) )
		{
			subarraystrlist = parseArrayStrList;
		}
		else
		{
			tokenCnt = 0;
		}
	}

	return tokenCnt;
}

// parse a string containing a composite data-type array structure into a list of individual array sub-element strings
// representing each array element of the composite type array; 
// DOES NOT parse the individual array string into the component composite-type sub-strings! This must be done in a subsequent calls
// to several composite-specific parsing methods.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//    [ "(x1,y1,z1)"",""(x2,y2,z2)"",""(x3,y3,z3)"",...""(xn,yn,zn)"" ]
//
// where each "(....)" element enclosed in parentheses represents a composite-type structure element of the parent array
// and breaks it into a vector of strings in format
//
//    "(x1,y1,z1)"
//    "(x2,y2,z2)"
//    "(x3,y3,z3)"
//                   .
//                   .
//                   .
//    "(xn,yn,zn)"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseCompositeElementArrayStrToStrList( std::vector<std::string>& elementarraystrlist, std::string& elementarraystr )
{
	int32_t tokenCnt = 0;

	if ( elementarraystr.length() > 0 )
	{
		std::string parseStr = elementarraystr;
		std::vector<std::string> parseArrayStrList = {};

		// composite structures are enclosed in quoted parentheses, and the ')"",""(' sequence represents the separation sequence between composite array elements
		std::string sepStr = ")\"\",\"\"(";
		std::string trimChars = "";

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( parseArrayStrList, parseStr, sepStr, nullptr, true, trimChars );

		if ( ( tokenCnt > 0 ) && ( tokenCnt == parseArrayStrList.size() ) )
		{
			elementarraystrlist = parseArrayStrList;
		}
		else
		{
			tokenCnt = 0;
		}
	}

	return tokenCnt;
}

// Parse a string containing a database blob_data composite data-type array structure into the list of component sub-element strings.
//
// This parser is specific to the blob_data composite type which contains an array of Characteristic_t storage types,
// the blob center point structure, and the array of points delineating the outline of the blob.
//
// DOES NOT parse the individual sub-element strings into their individualized sub-elements! This must be done in subsequent calls
// to several (potentially composite-specific) parsing methods.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  "{""(x1,y1,z1)"",""(x2,y2,z2)"",""(x3,y3,z3)"",...""(xn,yn,zn)""}","(center-x,center-y)","{""(x1,y1)"",""(x2,y2)"",(x3,y3)"",...""(xn,yn)""}" ]
//
// where each "(....)" element enclosed in parentheses represents a composite-type structure element.  The characteristics and outline structures
// are contained in arrays, while the center point is a standalone value.  The method separates each element (or array) into a vector of strings in the format
//
//    ""(x1,y1,z1)"",""(x2,y2,z2)"",""(x3,y3,z3)"",...""(xn,yn,zn)""
//    "(center-x,center-y)"
//    ""(x1,y1)"",""(x2,y2)"",""(x3,y3)"",...""(xn,yn)""
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseBlobCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t totalTokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		int32_t tokenCnt = 0;
		std::string parseStr = compositestr;
		std::vector<std::string> tokenList = {};

		// database blob-data composite-data-type strings have three elements:
		// -an array of characteristics delineated by curly races and ending with the sequence '"}",' 
		// -a single x-y point location denoting the center of the bloc formatted as '"(123,456)"'
		// -a final array of x-y point location elements delineated by curly braces, and ended by the '"}"' sequence.
		//
		// proper parsing requires handling each element separately...

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// first, strip out the leading characteristic array string component, but don't parse the array elements...
		std::string sepStr = ")},(";			// ")},(" designates the end of the characteristics array and the start of the center-point element...
		
		trimChars = "";							// do not remove any embedded characters at this point

		tokenList.clear();
		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 2...
		if ( tokenCnt != 2 )
		{
			return 0;
		}

		parseStr = tokenList.at( 0 );

		// clean up the characteristics array string...
		trimChars = "(){}\"";					// to remove the leading and trailing curly brace, extraneous quotes, and the leading and trailing element delimiter parantheses do not remove any embedded characters at this point
		TrimStr( trimChars, parseStr );

		elementstrlist.push_back(parseStr);		// now add it to the list of element strings
		totalTokenCnt++;

		// next, strip out the center-point x-y coordinate element
		parseStr = tokenList.at( 1 );			// get the remaining string containing the center-point and outline array

		sepStr = "),{(";						// "),"{(" designates the end of the center-point x-y location element and the start of the blob outline x-y point array...
		trimChars = "";							// do not remove any embedded characters at this point

		tokenList.clear();
		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 2...
		if ( tokenCnt != 2 )
		{
			return 0;
		}

		parseStr = tokenList.at( 0 );			// get the center-point string

		// clean up the center-point x-y coordinate string...
		trimChars = "()\"";						// to remove extraneous quotes and the element delimiter parantheses
		TrimStr( trimChars, parseStr );

		elementstrlist.push_back( parseStr );	// now add it to the list of element strings
		totalTokenCnt++;

		parseStr = tokenList.at( 1 );			// get the blob outline x-y point array string

		// clean up the outline point array string...
		trimChars = "(){}\"";					// to remove extraneous quotes, the leading and trailing element delimiter parantheses, and any leftover array braces
		TrimStr( trimChars, parseStr );

		elementstrlist.push_back( parseStr );	// now add it to the list of element strings
		totalTokenCnt++;
	}

	return totalTokenCnt;
}

// Parse a string containing a database cluster_data composite data-type array structure into the list of component sub-element strings.
//
// This parser is specific to the cluster_data composite type which contains a cluster cell count, a vector of blob_point elements describing
// the polygon outline the cluster, and the individual x and y coordinates denoting the start of the cluster bounding rectangle and the height
// and width of the cluster bounding rectangle.
//
// DOES NOT parse the individual sub-element strings into their individualized sub-elements! This must be done in subsequent calls
// to several (potentially composite-specific) parsing methods.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  "cell_count_n, {"(x1,y1)","(x2,y2)","(x3,y3)",..."(xn,yn)"}, start-x, start-y, width, height"
//
// where each "(xn,yn)" element enclosed in parentheses represents a blob_point composite-type structure element.  The polygon vertices
// are contained in an arrays, while the cell count and rectangle descriptors are standalone values.  The method separates each element
// into a vector of strings in the format
//
//    "cell_count"
//    ""(x1,y1)"",""(x2,y2)"",""(x3,y3)"",...""(xn,yn)""
//    "rect-start-x"
//    "rect-start-y"
//    "rect-width"
//    "rect-height"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseClusterCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t totalTokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		int32_t tokenCnt = 0;
		std::string parseStr = compositestr;
		std::vector<std::string> tokenList = {};

		// database cluster-data composite-data-type strings have 6 elements:
		// -the cell count for the cluster, as a standalone integer value
		// -an array of x-y point elements (blob_point elements) delineated by curly braces, starting with the sequence ",{" and ending with the sequence "}," 
		// -a single standalone integer value representing the starting x location of the cluster bounding rectangle
		// -a single standalone integer value representing the starting y location of the cluster bounding rectangle
		// -a single standalone integer value representing the width of the cluster bounding rectangle
		// -a single standalone integer value representing the height of the cluster bounding rectangle
		//
		// proper parsing requires handling each element separately...

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// first, strip out the cell count component; leave the remaining string intact
		std::string sepStr = ",{";				// ",{" designates the end of the cell count value and start of the polygon vertex array

		trimChars = "";							// do not remove any embedded characters at this point

		tokenList.clear();
		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 2...
		if ( tokenCnt != 2 )
		{
			return 0;
		}

		parseStr = tokenList.at( 0 );

		// clean up the cell count string...
		trimChars = "(){}\"";					// to remove the leading and trailing curly brace, extraneous quotes, and the leading and trailing element delimiter parantheses do not remove any embedded characters at this point
		TrimStr( trimChars, parseStr );

		elementstrlist.push_back( parseStr );	// now add it to the list of element strings
		totalTokenCnt++;

		// next, strip out the polygon vertex x-y coordinate array element
		parseStr = tokenList.at( 1 );			// get the remaining string containing the polygon array and the rectangle descriptor values

		sepStr = "},";							// "}," designates the end of the polygon vertex x-y cordinate array...
		trimChars = "";							// do not remove any embedded characters at this point

		tokenList.clear();
		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 2...
		if ( tokenCnt != 2 )
		{
			return 0;
		}

		parseStr = tokenList.at( 0 );			// get the polygon vertex array string

		// clean up the x-y coordinate array string...
		trimChars = "(){}\"";					// to remove extraneous quotes and the element delimiter parantheses
		TrimStr( trimChars, parseStr );

		elementstrlist.push_back( parseStr );	// now add it to the list of element strings
		totalTokenCnt++;

		parseStr = tokenList.at( 1 );			// get the remaining string containing the rectangle descriptor values

		// clean up the string...
		trimChars = "(){}\"";					// to remove extraneous quotes, leading and trailing element delimiter parantheses, and any leftover array braces
		TrimStr( trimChars, parseStr );

		sepStr = ",";							// "," designates the seperator between the remaining discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		tokenList.clear();
		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 4...
		if ( tokenCnt != 4 )
		{
			return 0;
		}

		for ( int i = 0; i < tokenCnt; i++ )
		{
			parseStr = tokenList.at( i );
			elementstrlist.push_back( parseStr );	// now add it to the list of element strings
			totalTokenCnt++;
		}
	}

	return totalTokenCnt;
}

// Parse a string containing a database signature_info composite data-type array structure into the list of component sub-element strings.
//
// This parser is specific to the signature_info composite type which contains a UserName, a short signature, a long signature, the signature time,
// and a hash for verifying the integrity of the entire signature object.
//
// Signature does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  "("user_name","short_tag","long_tag","2020-07-08 21:52:00","SIGNATURE-HASH-IN-STRING-FORMAT")"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "user_name"
//    "short_tag"
//    "long_tag"
//    "2020-07-08 21:52:00"
//    "SIGNATURE-HASH-IN-STRING-FORMAT"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseSignatureCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database signature_info composite-data-type strings have 5 elements:
		// -a standalone string value representing the signing username
		// -a standalone string value representing the short descriptor tag string for the signing action
		// -a standalone string value  representing the long descriptor tag string for the signing action
		// -a standalone date-time string value representing the time the signature was applied
		// -a standalone string value representing the HASH value of the entire signature, presented in string format
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line, including addition repetitions for all list elements
		// {\"(\"factory_admin\",\"REV\",\"Reviewed\",\"2020-07-08 21:52:00\",\"852CACA0A6CBFB58160F54FB38E58718EAC628A193C71FC6E6B8F896FF2B179D\")\"}\",\"{\"(...)\"}

		// the 'cleaned' single element string should appear like the following line...
		// (factory_admin,REV,Reviewed,2020-07-08 21:52:00,852CACA0A6CBFB58160F54FB38E58718EAC628A193C71FC6E6B8F896FF2B179D)

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// factory_admin,REV,Reviewed,2020-07-08 21:52:00,852CACA0A6CBFB58160F54FB38E58718EAC628A193C71FC6E6B8F896FF2B179D

		// first, strip out the cell count component; leave the remaining string intact
		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 5...
		if ( tokenCnt != 5 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database ad_settings composite data-type structure into the list of component sub-element strings.
//
// This parser is specific to the ad_settings composite type which contains a ServerName, a server IP address, a server port number,
// and the sserver Base DN string.
//
// ad_settings does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  \"(\"AD server\",\"127.0.0.1\",1245,\"Base DN String\",true)\"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "AD Server"
//    "127.0.0.1"
//    "1245"
//    "Base DN String"
//    "true'
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseADSettingsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database Active Directory ad_settings composite-data-type strings have 5 elements:
		// -a standalone string value representing the Active Directory server name
		// -a standalone string value representing the Active Directory server address
		// -a value representing the AD server port number
		// -a standalone string value representing the base DN string
		// a boolean indicating if AD use is enabled (once disabled cannot be re-enabled...)
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		// \"(\"AD server\",\"127.0.0.1\",1245,\"Base DN String\", true)\"

		std::string trimChars = "{}()";		// to remove the leading and trailing curly braces, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// AD server,127.0.0.1,1245,Base DN String

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 5...
		if ( tokenCnt != 5 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database af_settings composite data-type structure into the list of component sub-element strings.
//
// This parser is specific to the af_setings composite type which contains a boolean, a search starting position, a search stopping position,
// a search stepping increment value, a secondary search range, a secondary search stepping increment value, and a threshold value
//
// af_settings does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  "(true,12345,23456,300,2000,15,0)"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "true"
//    "12345"
//    "23456"
//    "300"
//    "2000"
//    "15"
//    "0"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseAFSettingsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database af_settings composite-data-type strings have 7 elements:
		// -a standalone string value representing the save-image boolean
		// -a standalone string value representing the starting position for the coarse focus stage
		// -a standalone string value representing the stopping position for the coarse focus stage
		// -a standalone string value representing the stepping value to be used during the coarse focus stage
		// -a standalone string value representing the range to be searched during the fine focus stage
		// -a standalone string value representing the stepping value to be used during the fine focus stage
		// -a standalone string value representing the low value for the sharpness threshold
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		// \"(true,12345,23456,300,2000,15,0)\"

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// true,12345,23456,300,2000,15,0

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 7...
		if ( tokenCnt != 7 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database email_settings composite data-type structure into the list of component sub-element strings.
//
// This parser is specific to the email_settings composite type which contains a server address string, a port number, an authenticate boolean,
// a username string, and a password string hash,
//
// email_settings does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  \"(\"server_addr_str\",server_port_number,authenticate_boolean,\"SMTP server username\",\"SMTP server password hash\")\"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "server addr"
//    "port number"
//    "authenticate boolean"
//    "username"
//    "password-hash"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseEmailSettingsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database email_settings composite-data-type strings have 5 elements:
		// -a standalone string value representing the SMTP server address
		// -a value representing the SMTP server port number
		// -a boolean representing the authentication boolean
		// -a standalone string value representing the SMTP server username used
		// -a standalone string value representing the HASH value of the password for the username
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		// \"(\"127.0.0.1\",12345,true,\"Joe Smith\",\"852CACA0A6CBFB58160F54FB38E58718EAC628A193C71FC6E6B8F896FF2B179D\")\"

		std::string trimChars = "{}()";		// to remove the leading and trailing curly braces, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// 127.0.0.1,12345,true,Joe Smith,852CACA0A6CBFB58160F54FB38E58718EAC628A193C71FC6E6B8F896FF2B179D

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 5...
		if ( tokenCnt != 5 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database rfid_sim_info composite data-type structure into the list of component sub-element strings.
//
// This parser is specific to the rfid_sim_info composite type which contains a boolean: set_valid_tag_data_tag, a short integer for the total tags,
// and 3 strings containing the reagent tag info file names for the main bay, left door bay, and right door bay,
//
// rfid_sim_info does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  \"(true,x,\"main bay file name\",\"door left file name\",\"door right file name\")\"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "true"
//    "total_tag_count_value"
//    "main bay file name"
//    "door left bay file name"
//    "door right bay file name"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseRfidSimCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database rfid_sim_info composite-data-type strings have 5 elements:
		// -a boolean value representing the set_valid_tag_data field
		// -a small integer representing the total number of tags
		// -a standalone string value representing the simulation main bay reagent info file name
		// -a standalone string value representing the simulation left door reagent bay reagent info file name
		// -a standalone string value representing the simulation right door reagent bay reagent info file name
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		// \"(true,x,\"main bay file name\",\"door left file name\",\"door right file name\")\"

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// true,x,main bay file name,door left file name,door right file name

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 5...
		if ( tokenCnt != 5 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database language_info composite data-type array structure into the list of individual composite data type strings.
//
// This parser is specific to the language_info composite type which contains a two-byte language ID, a language name string, a language locale tag
// string, and a boolean indicating if the language is active.
//
// the language_info composite does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  \"(1033,\"English\",\"en-US\",true)\"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "1033"
//    "English (United States)"
//    "en-US"
//    "true"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseLanguageInfoCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database rfid_sim_info composite-data-type strings have 5 elements:
		// -a small integer representing the language ID value
		// -a standalone string value representing the full language name (display name)
		// -a standalone string value representing the language locale tag
		// -a boolean value representing the set_valid_tag_data field
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		// \"(1033,\"English\",\"en-US\",true)\"

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// true,x,main bay file name,door left file name,door right file name

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 4...
		if ( tokenCnt != 4 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database run_options_info composite data-type structure into the list of component sub-element strings.
//
// This parser is specific to the run_options_info composite type which contains the following elements:
//
//    a string containing the default sample-set name
//    a string containing the default sample name
//    a short integer containing the default number of images to take for each sample
//    a short integer containing the default decimation number for saved images (save every nth image)
//    a boolean noting if the export is enabled
//    a string containing the export folder path location
//    a boolean noting if appending is active for the export
//    a string containing the appended reports folder path location
//    a string containing the default filename used for results
//    a string containing the default results folder path location
//    a boolean noting if auto-export to pdf is enabled
//    a string containing the folder path used for CSV file output
//    a short integer containing the default wash-type
//    a short integer containing the default dilution
//    an unsigned 32 bit value for the default cell type used for BioProcess or QcProcess
//
// run_options_info does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  "{\"(\"test sample-set\",\"test sample\",100,1,true,\"\\Instrument\\Export\",true,\"\\Instrument\\Export\\Append\",\"sample-result\",\"\\Instrument\\Result Binaries\",true,\"\\Instrument\\Result Binaries\\CSV\",1,1,5)\"}"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    "test sample-set"
//    "test sample"
//    "100"
//    "1"
//    "true"
//    "\\Instrument\\Export\"
//    "true"
//    "\\Instrument\\Export\\Append"
//    "sample-result"
//    "\\Instrument\\Result Binaries"
//    "true"
//    "\\Instrument\\Result Bimnaries\\CSV"
//    "1"
//    "1"
//    "5"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseRunOptionsCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database af_settings composite-data-type strings have 15 elements:
		//    a string containing the default sample-set name
		//    a string containing the default sample name
		//    a short integer containing the default number of images to take for each sample
		//    a short integer containing the default decimation number for saved images (save every nth image)
		//    a boolean noting if the export is enabled
		//    a string containing the export folder path location
		//    a boolean noting if appending is active for the export
		//    a string containing the appended reports folder path location
		//    a string containing the default filename used for results
		//    a string containing the default results folder path location
		//    a boolean noting if auto-export to pdf is enabled
		//    a string containing the folder path used for CSV file output
		//    a short integer containing the default wash-type
		//    a short integer containing the default dilution
		//    an unsigned 32 bit value for the default cell type used for BioProcess or QcProcess
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		// "{\"(\"test sample-set\",\"test sample\",100,1,true,\"\\Instrument\\Export\",true,\"\\Instrument\\Export\\Append\",\"sample-result\",\"\\Instrument\\Result Binaries\",true,\"\\Instrument\\Result Binaries\\CSV\",1,1)\"}"
		// Cleaning to remove escaped-quote sequences will result in a single element string tht appears like the following line...
		// "{(test sample-set,test sample,100,1,true,\\Instrument\\Export,true,\\Instrument\\Export\\Append,sample-result,\\Instrument\\Result Binaries,true,\\Instrument\\Result Binaries\\CSV,1,1)}"

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single element string should appear like the following line...
		// "test sample-set,test sample,100,1,true,\\Instrument\\Export,true,\\Instrument\\Export\\Append,sample-result,\\Instrument\\Result Binaries,true,\\Instrument\\Result Binaries\\CSV,1,1"

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 15...
		if ( tokenCnt != 15 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

// Parse a string containing a database cal_consumable composite data-type array structure into the list of individual composite data type strings.
//
// This parser is specific to the cal_consumable composite type which contains a label string, a lot ID string, a calibrator type, an expiration date,
// and an assay value.
//
// the cal_consumable composite does not contain any sub-elements requiring decomposition.
//
// DOES NOT remove space characters embedded in the sub-string tokens to avoid changing the string token content
//
// Note that composite-type data elements are formatted by the DB on retrieval with LOTS of quotes and escape backslashes.
// Backslashes should be removed prior to any parsing operation!
//
// takes a string in the format
//
//  \"(\"Concentration Calibration beads\",\"Lot number 112345\",0,\"2020-07-21 00:00:00\", 10.0)\"
//
// where everything is deliverd by the database as standalone string values.  The method separates each element
// into a vector of strings in the format
//
//    ""Concentration Calibration beads"
//    "Lot number 112345"
//    "0"
//    "2020-07-21 00:00:00"
//    "10.0"
//
// returns the number of array sub-strings found or '0' on error
int32_t DBifImpl::ParseConsumablesCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database rfid_sim_info composite-data-type strings have 5 elements:
		// -a standalone string representing the calibrator label or name
		// -a standalone string value representing the calibrator lot ID
		// -a short integer representing the calibrator type
		// -a standalone string value representing the calibrator expiration date
		// -a float value representing the calibrator assay value
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string as provided by the DB should appear like the following line
		//  \"(\"Concentration Calibration beads\",\"Lot number 112345\",0,\"2020-07-21 00:00:00\", 10.0)\"

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single array element string should appear like the following line...
		// Concentration Calibration beads,Lot number 112345,0,2020-07-21 00:00:00, 10.0

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 5...
		if ( tokenCnt != 5 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}

int32_t DBifImpl::ParseIlluminatorInfoCompositeElementStrToStrList( std::vector<std::string>& elementstrlist, std::string& compositestr )
{
	int32_t tokenCnt = 0;

	elementstrlist.clear();

	if ( compositestr.length() > 0 )
	{
		std::string parseStr = compositestr;

		// database illuminator_info composite-data-type array strings have 2 elements:
		// -a short integer representing the illuminator type
		// -a short integer representing the index of the illuminator
		//
		// all string values will be surrounded by the escaped-quote sequence when retrieved by the database
		//
		// proper parsing requires handling each element separately...

		// the original composite array element string broken out from the array should appear like the following line
		//  "(t1,i1)"

		std::string trimChars = "{}()\"";		// to remove the leading and trailing curly braces, extraneous quotes, and the element delimiter paranthesis;

		TrimStr( trimChars, parseStr );

		// the 'cleaned' single array element string should appear like the following line...
		//  "t1,i1"

		std::string sepStr = ",";				// "," designates the seperator between discrete value elements
		trimChars = "";							// do not remove any embedded characters at this point

		// passing 'true' for the 'do_trim' parameter removes leading and trailing spaces, but does not remove embedded spaces;
		tokenCnt = ParseStringToTokenList( elementstrlist, parseStr, sepStr, nullptr, true, trimChars );

		// token count should be 2...
		if ( tokenCnt != 2 )
		{
			return 0;
		}

		// DO NO FUFRTHER 'HANDLING' of the tokens!
	}

	return tokenCnt;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Class Publicly visible methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Time string helper methods
////////////////////////////////////////////////////////////////////////////////

// returns the current time as a time string with local time zone
void DBifImpl::GetDbCurrentTimeString( std::string& time_string )
{
	system_TP time_pt = ChronoUtilities::CurrentTime();

	GetDbTimeString( time_pt, time_string );
}

// returns the current time as a time string with local time zone
void DBifImpl::GetDbCurrentTimeString( std::string& time_string, system_TP& new_TP )
{
	system_TP time_pt = ChronoUtilities::CurrentTime();

	GetDbTimeString( time_pt, time_string );
	new_TP = time_pt;
}

void DBifImpl::GetDbTimeString( system_TP time_pt, std::string& time_string, bool isutctp )
{
	std::string timeString = "";

	if ( isutctp )
	{
		timeString = ChronoUtilities::ConvertToString( time_pt, DbUtcTimeFmtStr );
	}
	else
	{
		timeString = ChronoUtilities::ConvertToString( time_pt, DbTimeFmtStr );
	}

	size_t tlen = timeString.length();

	if ( tlen < MinTimeStrLen )
	{
		timeString = "'0000-00-00 00:00:00'";
	}

	time_string = timeString;
}

bool DBifImpl::GetTimePtFromDbTimeString( system_TP& time_pt, std::string time_string )
{
	bool timeOk = true;
	system_TP timePt = {};
	std::string fmtStr = DbTimeFmtStr;
	size_t tlen = 0;

	TrimWhiteSpace( fmtStr );
	RemoveTgtCharFromStr( fmtStr, '\'' );

	TrimWhiteSpace( time_string );
	RemoveTgtCharFromStr( time_string, '\'' );
	tlen = time_string.length();

	if ( ( tlen < MinTimeStrLen ) || ( tlen > MaxTimeStrLen ) )
	{
		timeOk = false;
	}
	else
	{
		system_TP zeroTime = {};

		timePt = ChronoUtilities::ConvertToTimePoint( time_string, fmtStr );
		if ( timePt < zeroTime )
		{
			timePt = zeroTime;
		}
	}

	time_pt = timePt;
	return timeOk;
}

static const char InfoMsgTpeStr[] = "[INFO] ";
static const char WarningMsgTypeStr[] = "[WARNING] ";
static const char InputErrorMsgTypeStr[] = "[INPUT_ERROR] ";
static const char QueryErrorMsgTypeStr[] = "[QUERY_ERROR] ";
static const char ErrorMsgTypeStr[] = "[ERROR] ";

void DBifImpl::WriteLogEntry( std::string entry_str, LogMsgType msg_type )
{
	std::ofstream logfile;

	logfile.open( DBLogFile, std::ios_base::app );

	if ( logfile.is_open() )
	{
		auto stime = ChronoUtilities::ConvertToString( ChronoUtilities::CurrentTime(), LogTimeFmtStr );

		std::string msgTypeStr = "";
		if ( msg_type != NoMsgType )
		{
			switch ( msg_type )
			{
				case InfoMsgType:
					msgTypeStr = InfoMsgTpeStr;
					break;

				case WarningMsgType:
					msgTypeStr = WarningMsgTypeStr;
					break;

				case InputErrorMsgType:
					msgTypeStr = InputErrorMsgTypeStr;
					break;

				case QueryErrorMsgType:
					msgTypeStr = QueryErrorMsgTypeStr;
					break;

				case ErrorMsgType:
					msgTypeStr = ErrorMsgTypeStr;
					break;

				case UndefinedMsgType:
				default:
					msgTypeStr = boost::str( boost::format( "[UNKNOWN: %d] " ) % msg_type );
					break;
			}
		}

		std::string logEntryStr = boost::str( boost::format( "[%s] %s%s\n" ) % stime % msgTypeStr % entry_str );

		logfile << logEntryStr;

		logfile.close();
	}
}

