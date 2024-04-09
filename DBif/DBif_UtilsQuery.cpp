// Database interface : implementation file
//

#pragma once

#include "pch.h"


#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>


#include "DBif_Impl.hpp"
#include "DBif_ObjTags.hpp"
#ifdef USE_LOGGER
#include "Logger.hpp"
#endif



static const std::string MODULENAME = "DBif_UtilsQuery";

static const char ProtectedStr[] = "Protected";
static const char InStr[] = "IN";

static const char InsertIntoStr[] = "INSERT into";
static const char UpdateStr[] = "UPDATE";

static const char CreateStr[] = "create";
static const char DeleteStr[] = "delete";
static const char RemoveStr[] = "remove";

static const char SetStr[] = "SET";
static const char ValuesStr[] = "VALUES";

static const char RowStr[] = "ROW";


////////////////////////////////////////////////////////////////////////////////
// DBifImpl database interface internal query creation helper methods
////////////////////////////////////////////////////////////////////////////////

// to check if the SQL sqlStr is a select statement to determine open parameters
// and the need to use the ExecuteSQL for inserts, updates or deletions
bool DBifImpl::IsSelect( std::string sqlstr )
{
	TrimWhiteSpace( sqlstr );
	StrToLower( sqlstr );

	std::string cmdStr = sqlstr.substr( 0, 6 );
	if ( cmdStr == SelectStr )
	{
		return 1;
	}

	return 0;
}

// to check if the SQL sqlStr is a privileged operation requiring admin privileges
bool DBifImpl::IsAdminQuery( std::string sqlstr )
{
	TrimWhiteSpace( sqlstr );
	StrToLower( sqlstr );

	std::string cmdStr = sqlstr.substr( 0, 6 );

	if ( cmdStr == DeleteStr || cmdStr == RemoveStr || cmdStr == DropStr || cmdStr == CreateStr || cmdStr == AlterStr )
	{
		return 1;
	}

	return 0;
}

// begin a complex transaction containing multiple actions
void DBifImpl::BeginTransaction( DBApi::eLoginType usertype )
{
	std::string queryStr = "START TRANSACTION";		// may also use the shorter, but less explicit "BEGIN" command
	CRecordset resultRecs;		// no default initializer; initialized by the method; not required for this operation, but required by called method

	RunQuery( usertype, queryStr, resultRecs );
}

// begin a complex transaction containing multiple actions
void DBifImpl::EndTransaction( DBApi::eLoginType usertype )
{
	std::string queryStr = "COMMIT";
	CRecordset resultRecs;		// no default initializer; initialized by the method; not required for this operation, but required by called method

	RunQuery( usertype, queryStr, resultRecs );
}

// begin a complex transaction containing multiple actions
void DBifImpl::CancelTransaction( DBApi::eLoginType usertype )
{
	std::string queryStr = "ROLLBACK";
	CRecordset resultRecs;		// no default initializer; initialized by the method; not required for this operation, but required by called method

	RunQuery( usertype, queryStr, resultRecs );
}

// correctly formats value-names and values for addition to UPDATE or INSERT strings
// used by table manipulation functions
void DBifImpl::AddToInsertUpdateString( bool format_for_update, int32_t paramnum,
										std::string& fullnamesstr, std::string& fullvaluesstr,
										std::string tagstr, std::string valueStr )
{
	if ( format_for_update )
	{
		if ( paramnum > 0 )
		{
			fullvaluesstr.append( ", " );
		}

		fullvaluesstr.append( tagstr );
		fullvaluesstr.append( " = " );
	}
	else
	{
		if ( paramnum > 0 )
		{
			// add the item separator and space
			fullnamesstr.append( ", " );
			fullvaluesstr.append( ", " );
		}

		fullnamesstr.append( tagstr );
	}
	fullvaluesstr.append( valueStr );
}

// creates a formatted insert or update query string using the supplied object, target tablename,
// and data values (key-value pair sets)
//
// for updates, guuid, numeric, or string key/match identifier values are handled;
//
// for inserts, takes a string defining the column identifier tags and a second string containing the associated column data values;
//    e.g. tag string:   "tag1, tag2, tag3,..."
//         value string: "value1,m value2, value3,..."
// for updates, takes a single string formatted as "columntag1 = datavalue1, columntag2 = datavalue2, ...", for all columns to be updated 
// return success = 1, error/failure = -1
int32_t DBifImpl::MakeObjectInsertUpdateQuery( bool format_for_update, std::string& querystr,
											   std::string& schemaname, std::string& tablename,
											   uuid__t objuuid, int64_t objidnum, std::string idnamestr,
											   std::string keynamestr, std::string keymatchstr,
											   std::string coltagsstr, std::string colvaluesstr )
{
	if ( ( schemaname.length() == 0 ) || ( tablename.length() == 0 ) )
	{
		return -1;
	}

	std::string selectStr = "";
	std::string whereStr = "";

	querystr.clear();

	// get a record to use as a reference for the current table column tags
	selectStr = boost::str( boost::format( "%s * %s \"%s\".\"%s\"" ) % SelectStr % FromStr % schemaname.c_str() % tablename.c_str() );
	if ( format_for_update )
	{   // try to get the specific record to be updated...
		if ( GuidValid( objuuid ) )
		{
			std::string guidStr = "";

			FormatGuidString( objuuid, guidStr );
			whereStr = boost::str( boost::format( " %s \"%s\" = %s" ) % WhereStr % idnamestr.c_str() % guidStr.c_str() );
		}
		else if ( objidnum > 0 )
		{
			whereStr = boost::str( boost::format( " %s \"%s\" = '%s'" ) % WhereStr % keynamestr.c_str() % keymatchstr.c_str() );
		}
		else if ( ( keynamestr.length() > 0 ) && ( keymatchstr.length() > 0 ) )
		{
			whereStr = boost::str( boost::format( " %s \"%s\" = '%s'" ) % WhereStr % keynamestr.c_str() % keymatchstr.c_str() );
		}

		if ( whereStr.length() == 0 )
		{
			return -1;
		}
		selectStr.append( whereStr );
	}

	CDatabase* pDb = nullptr;

	GetDBLoginConnection( pDb, DBApi::eLoginType::UserLoginType );

	CRecordset recordSet( pDb );

	selectStr.append( boost::str( boost::format( " %s 1" ) % LimitStr ) );    // just get a single record for the tags

	if ( !GetFullSelectQuery( pDb, recordSet, selectStr ) )
	{
		return -1;
	}

	std::vector<std::string> tagList = {};

	uint32_t colCnt = GetRecordColumnTags( recordSet, tagList );     // even with an empty result, the tags are always available...

	long recLen = recordSet.GetRecordCount();

	if ( recLen < 0 )
	{
		return -1;
	}

	// query was successful and the tags can be extracted
	if ( recordSet.GetRecordCount() > 0 && recordSet.CanScroll() )
	{
		recordSet.MoveFirst();
	}

	if ( ( format_for_update ) && ( recLen > 0 ) )
	{
		bool objectProtected = false;
		uint32_t colIdx = 0;
		int32_t fldIdx = -1;
		CString valStr = _T( "" );

		do
		{
			valStr = ( tagList.at( colIdx ) ).c_str();
			if ( valStr.CompareNoCase( CA2T( ProtectedStr ) ) == 0 )
			{
				fldIdx = colIdx;
			}
		} while ( ( ++colIdx < colCnt ) && ( fldIdx == -1 ) );


		if ( fldIdx >= 0 )
		{
			recordSet.GetFieldValue( fldIdx, valStr );
			objectProtected = ( valStr.CompareNoCase( CA2T( TrueStr )) == 0 ) ? true : false;
		}

		if ( objectProtected )
		{
			return -1;		// disallow updating records which have been marked as protected
		}
	}

	std::string opTag = "";

	// get a record to use as a reference for the current table column tags
	if ( format_for_update )
	{
		opTag = UpdateStr;
	}
	else
	{
		opTag = InsertIntoStr;
	}
	querystr = boost::str( boost::format( "%s \"%s\".\"%s\"" ) % opTag % schemaname.c_str() % tablename.c_str() );

	if ( format_for_update )
	{
		querystr.append( boost::str( boost::format( " %s " ) % SetStr ) );
		querystr.append( colvaluesstr );	// add the names and values section
		querystr.append( whereStr );		// add the separator space and 'where' clause
	}
	else
	{
		querystr.append( " ( " );
		querystr.append( coltagsstr );
		querystr.append( boost::str( boost::format( " ) %s ( " ) % ValuesStr ) );
		querystr.append( colvaluesstr );
		querystr.append( " )" );             // add the data values section trailing parentheses
	}

	return 1;
}

// creates a formatted insert query string using the object column tags and data values supplied,
// and the target table name and schema name supplied
// return success = 1, error/failure = -1
int32_t DBifImpl::MakeObjectAddQuery( std::string& insertstr, std::string& schemaname, std::string& tablename,
									  std::string coltagstr, std::string colvaluesstr )
{
	long recLen = -1;
	std::string queryStr = "";
	std::string tmpStr = "";
	std::string keyStr = "";
	uuid__t tmpUuid = {};

	ClearGuid( tmpUuid );
	insertstr.clear();
	queryStr.clear();
	tmpStr.clear();

	recLen = MakeObjectInsertUpdateQuery( FORMAT_FOR_INSERT, queryStr, schemaname, tablename,
										  tmpUuid, NO_ID_NUM, tmpStr, tmpStr, tmpStr, coltagstr, colvaluesstr );

	if ( recLen > 0 )
	{
		insertstr = queryStr;
	}

	return recLen;
}

// creates a formatted update query string using the column tags and data values supplied,
// and the target table name and schema name supplied.
// handles tables with guid identifiers, or numeric IDs as auto-generated by the DB;
// identifier id priority is given to uuids
// return success = 1, error/failure = -1
int32_t DBifImpl::MakeObjectUpdateQuery( std::string& updatestr, std::string& schemaname, std::string& tablename,
										 uuid__t objuuid, int64_t objidnum, std::string idnamestr,
										 std::string keynamestr, std::string keymatchstr, std::string colvaluesstr )
{
	long recLen = -1;
	std::string queryStr = "";
	std::string tagStr = "";

	updatestr.clear();
	tagStr.clear();

	recLen = MakeObjectInsertUpdateQuery( true, queryStr, schemaname, tablename, objuuid, objidnum, idnamestr, keynamestr, keymatchstr, tagStr, colvaluesstr );

	if ( recLen > 0 )
	{
		updatestr = queryStr;
	}

	return recLen;
}

void DBifImpl::MakeColumnValuesInsertUpdateQueryString( int32_t tagcount, std::string& querystr,
														std::string schemaname, std::string tablename,
														std::string namesstr, std::string valuesstr,
														int64_t itemidx, std::string indexnamestr,
														bool format_for_update )
{
	std::string indexStr = boost::str( boost::format( "%lld" ) % itemidx );

	MakeColumnValuesInsertUpdateQueryString( tagcount, querystr, schemaname, tablename,
											 namesstr, valuesstr, indexnamestr, indexStr, format_for_update );
}

void DBifImpl::MakeColumnValuesInsertUpdateQueryString( int32_t tagcount, std::string& querystr,
														std::string schemaname, std::string tablename,
														std::string namesstr, std::string valuesstr,
														uuid__t itemid, std::string idnamestr,
														bool format_for_update )
{
	std::string guidStr;

	guidStr.clear();
	if ( GuidValid( itemid ) )
	{
		FormatGuidString( itemid, guidStr );
	}

	MakeColumnValuesInsertUpdateQueryString( tagcount, querystr, schemaname, tablename,
											 namesstr, valuesstr, idnamestr, guidStr, format_for_update );
}

void DBifImpl::MakeColumnValuesInsertUpdateQueryString( int32_t tagcount, std::string& querystr,
														std::string schemaname, std::string tablename,
														std::string namesstr, std::string valuesstr,
														std::string idnamestr, std::string itemidstr, bool format_for_update )
{
	if ( tagcount > 0 )
	{
		if ( format_for_update )
		{   // try to get the specific record to be updated...
			std::string whereStr = "";

			if ( ( itemidstr.length() > 0 ) && ( idnamestr.length() > 0 ) )
			{
				whereStr = boost::str( boost::format( " %s \"%s\" = '%s'" ) % WhereStr % idnamestr % itemidstr );
			}

			querystr = boost::str( boost::format( "%s \"%s\".\"%s\" %s " ) % UpdateStr % schemaname % tablename % SetStr );
			querystr.append( valuesstr );        // add the names and values section
			querystr.append( whereStr );         // add the separator space and 'where' clause
		}
		else
		{
			querystr = boost::str( boost::format( "%s \"%s\".\"%s\" (" ) % InsertIntoStr % schemaname % tablename );
			querystr.append( namesstr );
			querystr.append( boost::str( boost::format( " ) %s ( " ) % ValuesStr ) );    // add the names section end indicator and the VALUES section lead-in formatting
			querystr.append( valuesstr );
			querystr.append( " )" );             // add the data values section trailing parentheses
		}
	}
}

// creates a formatted deletion query string using the object id or match string supplied, and the target tablename supplied
// return success = 1, error/failure = -1
bool DBifImpl::MakeObjectRemoveQuery( std::string& deletestr,
									  std::string& schemaname, std::string& tablename,
									  uuid__t objuuid, int64_t objidnum, int64_t objindex, std::string objname,
									  std::string idnamestr, std::string wherestr, bool deleteprotected )
{
	std::string whereStr;

	deletestr.clear();
	whereStr.clear();

	if ( wherestr.length() == 0 )
	{
		if ( idnamestr.length() > 0 )
		{
			if ( GuidValid( objuuid ) )
			{
				std::string uuidStr;

				FormatGuidString( objuuid, uuidStr );
				whereStr = boost::str( boost::format( " %s \"%s\" = '%s'" ) % WhereStr % idnamestr % uuidStr );
			}
			else if ( objidnum > 0 )
			{
				whereStr = boost::str( boost::format( " %s \"%s\" = '%lld'" ) % WhereStr % idnamestr % objidnum );
			}
			else if ( objindex > 0 )
			{
				whereStr = boost::str( boost::format( " %s \"%s\" = '%lld'" ) % WhereStr % idnamestr % objindex );
			}
			else if ( objidnum > 0 )
			{
				whereStr = boost::str( boost::format( " %s \"%s\" = ''%s''" ) % WhereStr % idnamestr % objname );
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		whereStr = boost::str( boost::format( " %s %s" ) % WhereStr % wherestr );
	}

	if ( !deleteprotected )
	{
		whereStr.append( boost::str( boost::format( " %s \"%s\" = %s" ) % AndStr % ProtectedStr % FalseStr ) );
	}

	deletestr = boost::str( boost::format( "%s %s \"%s\".\"%s\"" ) % DeleteStr % FromStr % schemaname % tablename );
	deletestr.append( whereStr );

	return true;
}

// creates a formatted deletion query string using the object id or match string supplied, and the target tablename supplied
// return success = 1, error/failure = -1
DBApi::eQueryResult  DBifImpl::MakeObjectIdListRemoveQuery( std::string& deletestr,
															std::string& schemaname, std::string& tablename,
															std::string idtagstr, std::vector<uuid__t> id_list,
															std::string wherestr, bool deleteprotected )
{
	deletestr.clear();

	// deleting using a uuid list
	size_t idListSize = id_list.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	if ( idtagstr.length() == 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	std::string guidStr = "";
	std::vector<std::string> uuidStrList = {};
	uuid__t uid;

	ClearGuid( uid );

	// ensure all list entries are valid uuids
	for ( auto listIter = id_list.begin(); listIter != id_list.end(); ++listIter )
	{
		ClearGuid( uid );
		uid = *listIter;

		if ( !GuidValid( uid ) )
		{
			return DBApi::eQueryResult::BadOrMissingListIds;
		}

		uuid__t_to_DB_UUID_Str( uid, guidStr );

		uuidStrList.push_back( guidStr );
	}

	std::string whereStr;
	std::string guidArrayStr = {};
	std::string guidListStr = {};

	whereStr.clear();

	if ( idListSize > 0 )
	{
		size_t listCnt = uuidStrList.size();
		size_t listIndex = 0;

		guidArrayStr = " ( ";
		for ( listIndex = 0; listIndex < listCnt; )
		{
			guidListStr = boost::str( boost::format( "%s" ) % uuidStrList.at( listIndex ) );
			guidArrayStr.append( guidListStr );
			if ( ++listIndex < listCnt )
			{
				guidArrayStr.append( ", " );
			}
		}
		guidArrayStr.append( " )" );

		whereStr = boost::str( boost::format( " %s \"%s\" %s %s" ) % WhereStr % idtagstr % InStr % guidArrayStr );

		if ( !deleteprotected )
		{
			whereStr.append( boost::str( boost::format( " %s \"%s\" = %s" ) % AndStr % ProtectedStr % FalseStr ) );
		}

		deletestr = boost::str( boost::format( "%s %s \"%s\".\"%s\"" ) % DeleteStr % FromStr % schemaname % tablename );
		deletestr.append( whereStr );
	}

	return DBApi::eQueryResult::QueryOk;
}

// creates a formatted deletion query string using the object idnum list or match string supplied, and the target tablename supplied
// return success = 1, error/failure = -1
DBApi::eQueryResult  DBifImpl::MakeObjectIdNumListRemoveQuery( std::string& deletestr,
															   std::string& schemaname, std::string& tablename,
															   std::string idtagstr, std::vector<int64_t> idnum_list,
															   std::string wherestr, bool deleteprotected )
{
	deletestr.clear();

	// deleting using a list of idnums
	size_t idListSize = idnum_list.size();

	if ( idListSize == 0 )
	{
		return DBApi::eQueryResult::NoTargets;
	}

	if ( idtagstr.length() == 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	std::vector<int64_t> idNumList = {};
	int64_t idNum = 0;

	// ensure all list entries are valid uuids
	for ( auto listIter = idnum_list.begin(); listIter != idnum_list.end(); ++listIter )
	{
		idNum = *listIter;

		if ( idNum <= 0 )
		{
			return DBApi::eQueryResult::BadOrMissingListIds;
		}

		idNumList.push_back( idNum );
	}

	std::string whereStr;
	std::string idArrayStr = {};
	std::string idListStr = {};

	whereStr.clear();

	if ( idListSize > 0 )
	{
		size_t listCnt = idNumList.size();
		size_t listIndex = 0;

		idArrayStr = " ( ";
		for ( listIndex = 0; listIndex < listCnt; )
		{
			idListStr = boost::str( boost::format( "%lld" ) % idNumList.at( listIndex ) );
			idArrayStr.append( idListStr );
			if ( ++listIndex < listCnt )
			{
				idArrayStr.append( ", " );
			}
		}
		idArrayStr.append( " )" );

		whereStr = boost::str( boost::format( " %s \"%s\" %s %s" ) % WhereStr % idtagstr % InStr % idArrayStr );

		if ( !deleteprotected )
		{
			whereStr.append( boost::str( boost::format( " %s \"%s\" = %s" ) % AndStr % ProtectedStr % FalseStr ) );
		}

		deletestr = boost::str( boost::format( "%s %s \"%s\".\"%s\"" ) % DeleteStr % FromStr % schemaname % tablename );
		deletestr.append( whereStr );
	}

	return DBApi::eQueryResult::QueryOk;
}

// for future use by clients requiring formatted DB query string for direct DBif access
DBApi::eQueryResult DBifImpl::MakeFilterSortQuery( std::string& query_str,
												  DBApi::eListType listtype,
												  std::vector<DBApi::eListFilterCriteria> filtertypes,
												  std::vector<std::string> compareops, std::vector<std::string> comparevals,
												  DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
												  std::string orderstring, int32_t sortdir, int32_t limitcnt,
												  int32_t startindex, int64_t startidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryOk;
	std::string schemaName = "";
	std::string tableName = "";
	std::string whereStr = "";
	std::string idTagStr = "";
	std::string idValStr = "";

	query_str.clear();

	if ( !GetListQueryTags( listtype, schemaName, tableName, idTagStr, idValStr ) )
	{
		return DBApi::eQueryResult::BadQuery;
	}

	if ( filtertypes.size() > 0 )
	{
		if ( !GetListFilterString( listtype, filtertypes, whereStr, compareops, comparevals ) )
		{
			return DBApi::eQueryResult::BadQuery;
		}
	}

	// check for special limit requests.
	// if limit specified as -2, just pass the negative value to the DoListQuery method...
	if ( limitcnt == DFLT_QUERY_AND_LIST_LIMIT || limitcnt == DFLT_QUERY_NO_LIST_LIMIT )		// look for explicitly specified or default parameter default limit request
	{
		limitcnt = DefaultQueryRecordLimit;
	}
	else if ( limitcnt > MaxQueryRecordLimit )
	{
		limitcnt = MaxQueryRecordLimit;
	}

	std::string queryStr = "";
	queryResult = DBApi::eQueryResult::QueryFailed;

	queryStr = boost::str( boost::format( "%s * %s \"%s\".\"%s\"" ) % SelectStr % FromStr % schemaName % tableName );

	if ( whereStr.length() > 0 )
	{
		queryStr.append( boost::str( boost::format( " %s " ) % WhereStr ) );
	}

	if ( startidnum > 0 )
	{
		if ( whereStr.length() > 0 )
		{
			whereStr.append( boost::str( boost::format( " %s " ) % AndStr ) );
		}
		else
		{
			whereStr.append( boost::str( boost::format( " %s " ) % WhereStr ) );
		}
		whereStr.append( boost::str( boost::format( "\"%s\" %s %lld" ) % idTagStr % ( ( sortdir < 0 ) ? "<" : ">" ) % startidnum ) );
	}
	queryStr.append( whereStr );

	GetListSortString( listtype, orderstring, primarysort, secondarysort, sortdir );
	if ( orderstring.length() > 0 )
	{
		queryStr.append( orderstring );
	}

	if ( limitcnt > 0 )
	{
		queryStr.append( boost::str( boost::format( " %s %d" ) % LimitStr % limitcnt ) );
	}

	query_str = queryStr;

	return DBApi::eQueryResult::QueryOk;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// array string formatting support functions
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// formats an array of string data values for addition to an insertion string
// for a database array field;
//
// Does NOT add array enclosure formatting (e.g. ARRAY[]; ROW() )
//
// each call represents the values associated with the array element of a
// column cell, for cells using an array of data values;
//
// the ValueArrayString will contain the data formatted correctly for addition
// to an insertion statement, but will not contain the surrounding array
// designation brackets
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateStringDataArrayString( std::string& ValueArrayString,
											   const int32_t DataCount,
											   const uint32_t StartIndex,
											   const std::vector<std::string>& StrList )
{
	int32_t x = 0;
	int32_t ArrayIndex = StartIndex;
	std::string WorkingString = {};

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( DataCount > 0 && ( StartIndex + DataCount ) <= StrList.size() )
	{
		// if < 0, the loop will terminate immediately, and return 0 elements added
		for ( x = 0; x < DataCount; x++, ArrayIndex++ )
		{
			// strings must be enclosed in single quotes
			WorkingString = "'";
			WorkingString.append( StrList.at( ArrayIndex ) );
			WorkingString.append( "'" );
			if ( ( x + 1 ) < DataCount )
			{
				WorkingString.append( "," );
			}
			ValueArrayString.append( WorkingString );
		}
	}

	return( x );  // return the number of data elements added to the string
}


////////////////////////////////////////////////////////////////////////////////
// formats an array of data value strings (non-string data) for addition to an
// insertion string for a database array field;
//
// Does NOT add array enclosure formatting (e.g. ARRAY[]; ROW() )
//
// each call represents the values associated with the array element of a
// column cell, for cells using an array of data values;
//
// the ValueArrayString will contain the data formatted correctly for addition
// to an insertion statement, but will not contain the surrounding array
// designation brackets
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateDataStringArrayString( std::string& ValueArrayString,
											   const int32_t DataCount,
											   const uint32_t StartIndex,
											   const std::vector<std::string>& StrList )
{
	int32_t x = 0;
	int32_t ArrayIndex = StartIndex;
	std::string WorkingString;

	WorkingString.clear();

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( DataCount > 0 && ( StartIndex + DataCount ) <= StrList.size() )
	{
		// if < 0, the loop will terminate immediately, and return 0 elements added
		for ( x = 0; x < DataCount; x++, ArrayIndex++ )
		{
			// strings must already be enclosed in single quotes
			WorkingString = StrList.at( ArrayIndex );
			if ( ( x + 1 ) < DataCount )
			{
				WorkingString.append( "," );
			}
			ValueArrayString.append( WorkingString );
		}
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format DB GUID string data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of DB UUID values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateGuidDataValueArrayString( std::string& DataString,
												  const int32_t DataCount,
												  const uint32_t StartIndex,
												  const std::vector<std::string>& StrList )
{
	int32_t x = 0;
	std::string ValuesString;

	ValuesString.clear();

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( DataCount > 0 && ( StartIndex + DataCount ) <= StrList.size() )
	{
		// first create the list of array element strings
		// Creates the Array string without any enclosing formatters
		x = CreateStringDataArrayString( ValuesString, DataCount, StartIndex, StrList );

		if ( x > 0 )
		{
			DataString.append( "ARRAY[" );			// the data must be formatted using the ARRAY designator for PostgreSQL array insertions (may also be SQL compliant)
													// the array data must be contained within the square brackets for SQL array insertions;
			DataString.append( ValuesString );
			DataString.append( "]::uuid[]" );		// UUID arrays must ALSO use a cast to ensure the uuid strings are interpreted correctly as uuids, and not simple text
		}
	}
	else
	{
		DataString.append( "ARRAY[]::uuid[]" );
	}

	return( x );  // return the number of data elements added to the string
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// basic data-type formatting functions
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// format integer data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateInt16ValueArrayString( std::string& DataString,
											   const std::string FmtStr,
											   const int32_t DataCount,
											   const uint32_t StartIndex,
											   const std::vector<int16_t>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = "%d";          // define default parameter type
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::smallint[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format integer data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateUint16ValueArrayString( std::string& DataString,
												const std::string FmtStr,
												const int32_t DataCount,
												const uint32_t StartIndex,
												const std::vector<uint16_t>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = "%u";          // define default parameter type
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::smallint[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format integer data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateInt32ValueArrayString( std::string& DataString,
											   const std::string FmtStr,
											   const int32_t DataCount,
											   const uint32_t StartIndex,
											   const std::vector<int32_t>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = "%ld";         // define default parameter type
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::integer[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format integer data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateUint32ValueArrayString( std::string& DataString,
												const std::string FmtStr,
												const int32_t DataCount,
												const uint32_t StartIndex,
												const std::vector<uint32_t>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = "%lu";         // define default parameter type
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::bigint[]" );		// an unsinged 32-bit value had to be stored in a 64-bit DB field to avoid out-of-range errors...
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format integer data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateInt64ValueArrayString( std::string& DataString,
											   const std::string FmtStr,
											   const int32_t DataCount,
											   const uint32_t StartIndex,
											   const std::vector<int64_t>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = "%lld";        // define default parameter type
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::bigint[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format float data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateFloatValueArrayString( std::string& DataString,
											   const std::string FmtStr,
											   const int32_t DataCount,
											   const uint32_t StartIndex,
											   const std::vector<float>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = DbFloatDataFmtStr;				// "%0.8f" define default precision for storage of floats...
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::real[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format double precision data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateDoubleValueArrayString( std::string& DataString,
												const std::string FmtStr,
												const int32_t DataCount,
												const uint32_t StartIndex,
												const std::vector<double>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = DbDoubleDataFmtStr;			// "%0.15f" define default precision for storage of doubles...
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::double precision[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format double precision data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateDoubleExpValueArrayString( std::string& DataString,
												   const std::string FmtStr,
												   const int32_t DataCount,
												   const uint32_t StartIndex,
												   const std::vector<double>& DataList )
{
	std::string fmtStr = FmtStr;

	if ( fmtStr.length() <= 0 )
	{
		fmtStr = DbDoubleExpDataFmtStr;			// "%0.10E" define default precision for storage of doubles using exponential notation
	}

	int32_t x = CreateDataValueArrayString( DataString, fmtStr, DataCount, StartIndex, DataList );
	if ( x == 0 )
	{
		DataString.append( "::double precision[]" );
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format uuid string values to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateGuidValueArrayString( std::string& DataString,
											  const int32_t DataCount,
											  const uint32_t StartIndex,
											  const std::vector<std::string>& UuidStrList )
{
	int32_t x = CreateGuidDataValueArrayString( DataString, DataCount, StartIndex, UuidStrList );

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format uuid__t values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of DB UUID values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateGuidValueArrayString( std::string& DataString,
											  const int32_t DataCount,
											  const uint32_t StartIndex,
											  const std::vector<uuid__t>& UuidList )
{
	int32_t x, cnt;
	int32_t ArrayIndex = StartIndex;
	uuid__t uuidVal = {};
	std::string uuidStr = "";
	std::vector<std::string> gvalStrList = {};

	x = cnt = 0;

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( ( StartIndex + DataCount ) <= UuidList.size() )
	{
		for ( x = 0; x < DataCount; x++, ArrayIndex++ )
		{
			uuidVal = UuidList.at( ArrayIndex );
			if ( GuidValid( uuidVal ) )
			{
				FormatGuidString( uuidVal, uuidStr );	// just convert the uuid to a string without surrounding single quotes...
				gvalStrList.push_back( uuidStr );
				cnt++;
			}
		}

		if ( cnt == DataCount )       // all passed UUIDs were valid..
		{
			x = CreateGuidDataValueArrayString( DataString, DataCount, StartIndex, gvalStrList );
		}
		else
		{
			x = -1;
		}
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format GUID values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of DB UUID values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateGuidValueArrayString( std::string& DataString,
											  const int32_t DataCount,
											  const uint32_t StartIndex,
											  const std::vector<GUID>& DataList )
{
	int32_t x, cnt;
	int32_t ArrayIndex = StartIndex;
	GUID gval = {};
	std::string gvalStr = "";
	std::vector<std::string> gvalStrList = {};

	x = cnt = 0;

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( ( StartIndex + DataCount ) <= DataList.size() )
	{
		for ( x = 0; x < DataCount; x++, ArrayIndex++ )
		{
			gval = DataList.at( ArrayIndex );
			if ( GuidValid( gval ) )
			{
				FormatGuidString( gval, gvalStr );	// just convert the GUID to a string without surrounding single quotes...
				gvalStrList.push_back( gvalStr );
				cnt++;
			}
		}

		if ( cnt == DataCount )       // all passed UUIDs were valid..
		{
			x = CreateGuidDataValueArrayString( DataString, DataCount, StartIndex, gvalStrList );
		}
		else
		{
			x = -1;
		}
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format time values supplied as system_TP values to add to an insertion
// string for a database array field;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateTimePtValueArrayString( std::string& DataString,
												const int32_t DataCount,
												const uint32_t StartIndex,
												const std::vector<system_TP>& TimePtList )
{
	int32_t x, cnt;
	int32_t ArrayIndex = StartIndex;
	system_TP timePt = {};
	system_TP zeroTimePt = {};
	std::string timeStr = "";
	std::vector<std::string> timeStrList = {};

	x = cnt = 0;

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( ( StartIndex + DataCount ) <= TimePtList.size() )
	{
		for ( x = 0; x < DataCount; x++, ArrayIndex++ )
		{
			timePt = TimePtList.at( ArrayIndex );

			if ( timePt != zeroTimePt )
			{
				GetDbTimeString( timePt, timeStr );
				if ( timeStr.length() > 0 )
				{
					timeStrList.push_back( timeStr );
					cnt++;
				}
			}
		}

		if ( cnt == DataCount )       // all passed UUIDs were valid..
		{
			x = CreateStringDataValueArrayString( DataString, DataCount, StartIndex, timeStrList );
		}
		else
		{
			x = -1;
		}
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format string data values to add to an insertion string for a
// database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of string values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateStringDataValueArrayString( std::string& DataString,
													const int32_t DataCount,
													const uint32_t StartIndex,
													const std::vector<std::string>& StrList, int32_t format )
{
	int32_t x = 0;
	std::string ValuesString;

	ValuesString.clear();

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( DataCount > 0 && ( StartIndex + DataCount ) <= StrList.size() )
	{
		// first create the list of array element strings
		// Creates the Array string without any enclosing formatters, but adds single quotes around each string
		x = CreateStringDataArrayString( ValuesString, DataCount, StartIndex, StrList );

		if ( x > 0 )
		{	// add the array designation brackets for standard data array types
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "ARRAY" );		// the data must be formatted using the ARRAY designator for PostgreSQL array insertions (may also be SQL compliant)
			}
			DataString.append( "[" );				// the data must be formatted using the ARRAY designator for PostgreSQL array insertions (may also be SQL compliant)
			DataString.append( ValuesString );
			DataString.append( "]" );				// the array data must be contained within the square brackets for SQL array insertions
		}
	}
	else
	{
		if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
		{
			DataString.append( "ARRAY[]::character varying[]" );
		}
		else
		{
			DataString.append( "[]" );
		}
	}

	return( x );  // return the number of data elements added to the string
}

//////////////////////////////////////////////////////////////////////////////
// format data values presented as strings but not containing string data to
// add to an insertion string for a database array field;
//
// each call represents the values for a column cell, for fields requiring
// an array of data values;
//
// the DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateDataValueStringArrayString( std::string& DataString,
													const int32_t DataCount,
													const uint32_t StartIndex,
													const std::vector<std::string>& StrList, int32_t format )
{
	int32_t x = 0;
	std::string ValuesString = {};

	// if <= 0, the loop will terminate immediately, and return 0 elements added
	if ( DataCount > 0 && ( StartIndex + DataCount ) <= StrList.size() )
	{
		// first create the list of array element strings
		// Creates the Array string without any enclosing formatters and without adding single quotes for each string
		x = CreateDataStringArrayString( ValuesString, DataCount, StartIndex, StrList );

		if ( x > 0 )
		{	// add the array designation brackets for standard data array types
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "ARRAY" );		// the data must be formatted using the ARRAY designator for PostgreSQL array insertions (may also be SQL compliant)
			}
			DataString.append( "[" );				// the data must be formatted using the ARRAY designator for PostgreSQL array insertions (may also be SQL compliant)
			DataString.append( ValuesString );
			DataString.append( "]" );				// the array data must be contained within the square brackets for SQL array insertions
		}
	}
	else
	{
		if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
		{
			DataString.append( "ARRAY[]" );
		}
		else
		{
			DataString.append( "[]" );
		}
	}

	return( x );  // return the number of data elements added to the string
}


//////////////////////////////////////////////////////////////////////////////
//
// These are specialized composite-data-type array string constructors
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// format map data values to add to an insertion string for a database array
// field of composite data-types, in this representing a pair of values;
//
// the map represents the content of a column cell, for fields requiring
// a map of values.
//
// This particular map string constructor handles config maps containing an
// pair of int16_t values for the key and the value associated with the key.
//
// This constructor handles the map as an array of values, and creates a list
// of separated array elements, where each array element represents a line
// in the map.  Output format will resenble the example below:
//
//   [ROW(mapkey1,mapval1),ROW(mapkey2,mapval2),ROW(mapkey3,mapval3)...,ROW(mapkeyn,mapvaln)]
//
// The structure inserted is a custom composite type in the database, so the
// insertion string formatting uses the "ROW(v1a,v1b,v1c,v1d,v1e)" format to
// designate the content of the composite data structure and avoid excessive
// requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire map, and does not
// allow specification of a starting element (StartIndex in other methods).
// This method also uses a fixed format string for the map key and value fields.
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateInt16MapValueArrayString( std::string& DataString, const int32_t DataCount,
												  const std::map<int16_t, int16_t>& DataMap, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t mapIndex = 0;
	size_t mapSize = 0;
	std::vector<std::string> mapStrList = {};

	mapSize = DataMap.size();

	// first create the list of array element strings
	// if <= 0, this loop will terminate immediately, and return 0 elements added, but will also produce an empty array string
	for ( auto mapLineIter = DataMap.begin(); mapLineIter != DataMap.end() && mapIndex < mapSize; ++mapLineIter, mapIndex++ )
	{
		int32_t mapKey = 0;
		double mapVal = 0.0;
		std::string lineStr = "";
		std::string formatStr = "";

		mapKey = static_cast<int32_t>( ( *mapLineIter ).first );
		mapVal = ( *mapLineIter ).second;

#ifdef USE_BOOST_FORMATTING
		formatStr = boost::str( boost::format( "(%ld,%lf)" ) % mapKey % mapVal );
#else
		char fmtBuf[64] = {};

		sprintf( fmtBuf, "(%ld,%lf)", mapKey, mapVal );
		formatStr = fmtBuf;
#endif // USE_BOOST_FORMATTING

		if ( formatStr.length() > 0 )
		{
			if ( use_rows )
			{
				lineStr = RowStr;
			}
			lineStr.append( formatStr );

			mapStrList.push_back( lineStr );
			cnt++;
		}
	}

	if ( cnt == DataCount )       // all passed UUIDs were valid..
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, mapStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::int16_map_pair[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string

}

//////////////////////////////////////////////////////////////////////////////
// format map data values to add to an insertion string for a database array
// field of composite data-types, in this representing a pair of values;
//
// the map represents the content of a column cell, for fields requiring
// a map of values.
//
// This particular map string constructor handles config maps containing an
// enumeration value (cast to an int32_t value) represneting the key value,
// and a double value representing the value to be associated with the key.
//
// This constructor handles the map as an array of values, and creates a list
// of separated array elements, where each array element represents a line
// in the map.  Output format will resenble the example below:
//
//   [ROW(mapkey1,mapval1),ROW(mapkey2,mapval2),ROW(mapkey3,mapval3)...,ROW(mapkeyn,mapvaln)]
//
// The structure inserted is a custom composite type in the database, so the
// insertion string formatting uses the "ROW(v1a,v1b,v1c,v1d,v1e)" format to
// designate the content of the composite data structure and avoid excessive
// requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire map, and does not
// allow specification of a starting element (StartIndex in other methods).
// This method also uses a fixed format string utilizing a shorter precision
// format for the map double value field.
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateConfigParamMapValueArrayString( std::string& DataString, const int32_t DataCount,
														const ConfigParamList_t& ConfigMap, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t mapIndex = 0;
	size_t mapSize = 0;
	std::vector<std::string> mapStrList = {};

	mapSize = ConfigMap.size();

	// first create the list of array element strings
	// if <= 0, this loop will terminate immediately, and return 0 elements added, but will also produce an empty array string
	for ( auto mapLineIter = ConfigMap.begin(); mapLineIter != ConfigMap.end() && mapIndex < mapSize; ++mapLineIter, mapIndex++ )
	{
		int32_t mapKey = 0;
		double mapVal = 0.0;
		std::string lineStr = "";
		std::string formatStr = "";

		mapKey = static_cast<int32_t>( ( *mapLineIter ).first );
		mapVal = ( *mapLineIter ).second;

#ifdef USE_BOOST_FORMATTING
		formatStr = boost::str( boost::format( "(%ld,%0.8lf)" ) % mapKey % mapVal );		// use fixed format string with shorter precision for config map parameter
#else
		char fmtBuf[64] = {};

		sprintf( fmtBuf, "(%ld,%0.8lf)", mapKey, mapVal );								// use fixed format string with shorter precision for config map parameter
		formatStr = fmtBuf;
#endif // USE_BOOST_FORMATTING

		if ( formatStr.length() > 0 )
		{
			if ( use_rows )
			{
				lineStr = RowStr;
			}
			lineStr.append( formatStr );

			mapStrList.push_back( lineStr );
			cnt++;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, mapStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::input_config_params[]" );
		}
	}
	else
	{
		x = 1;
	}

	return( x );  // return the number of data elements added to the string

}

//////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a
// database array field;
//
// the structure represents the content of a column cell, for columns containing
// an array of structures.
//
// This particular string constructor handles the cell identification parameter
// structure containing the following structural elements:
//
//	struct analysis_input_params_t
//  {
//	  uint16_t key;
//	  uint16_t s_key;
//	  uint16_t s_s_key;
//	  float value;
//	  E_POLARITY polarity;
//  };
//
// This constructor handles the vector of structures, and creates a list
// of separated array elements, where each array element represents an element
// in the vector.  Output format will resenble the example below:
//
//   [ ROW(key1,s_key1,s_s_key1,value1,polarity1),
//     ROW(key2,s_key2,s_s_key2,value2,polarity2),
//     ROW(key3,s_key3,s_s_key3,value3,polarity3).
//           .
//           .
//           .
//     ROW(keyn,s_keyn,s_s_keyn,valuen,polarityn) ]
//
// The structure inserted is a custom composite type in the database, so the
// insertion string formatting uses the "ROW(v1a,v1b,v1c,v1d,v1e)" format to
// designate the content of the composite data structure and avoid excessive
// requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not
// allow specification of a starting element (StartIndex in other methods).
// This method also uses a fixed format string utilizing for the srtucture
// elements field values.
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
//////////////////////////////////////////////////////////////////////////////
int32_t DBifImpl::CreateCellIdentParamArrayString( std::string& DataString, const int32_t DataCount,
												   const v_CellIdentParams_t& IdentParamList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	std::vector<std::string> paramStrList = {};

	listSize = IdentParamList.size();

	// first create the list of array element strings
	// if <= 0, this loop will terminate immediately, and return 0 elements added, but will also produce an empty array string
	for ( auto identParamsIter = IdentParamList.begin(); identParamsIter != IdentParamList.end() && listIndex < listSize; ++identParamsIter, listIndex++ )
	{
		uint16_t key = 0;
		uint16_t skey = 0;
		uint16_t sskey = 0;
		float value = 0.0;
		int16_t polarity = static_cast<int16_t>( E_POLARITY::eInvalidPolarity);
		std::string lineStr = "";
		std::string formatStr = "";
		CellIdentParams_t params = *identParamsIter;

		key = params.key;
		skey = params.s_key;
		sskey = params.s_s_key;
		value = params.value;
		polarity = static_cast<int16_t>( params.polarity );

#ifdef USE_BOOST_FORMATTING
		std::string valueStr = boost::str( boost::format( DbFloatThresholdFmtStr ) % value );							// format float value using standard threshold format string
		formatStr = boost::str( boost::format( "(%d,%d,%d,%s,%d)" ) % key % skey % sskey % valueStr % polarity );		// use fixed format string with shorter precision for config map parameter
//		formatStr = boost::str( boost::format( "(%d,%d,%d,%0.8f,%d)" ) % key % skey % sskey % value % polarity );		// use fixed format string with shorter precision for config map parameter
#else
		char fmtBuf[64] = {};
		char valBuf[16] = {};

		sprintf( valBuf, DbFloatThresholdFmtStr, value );																// format float value using standard threshold format string
		sprintf( fmtBuf, "(%u,%u,%u,%s,%d)", key, skey, sskey, valBuf, polarity );										// use fixed format string with shorter precision for config map parameter
		formatStr = fmtBuf;
#endif // USE_BOOST_FORMATTING

		if ( formatStr.length() > 0 )
		{
			if ( use_rows )
			{
				lineStr = RowStr;
			}
			lineStr.append( formatStr );

			paramStrList.push_back( lineStr );
			cnt++;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, paramStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::analysis_input_params[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string

}

////////////////////////////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a database array field;
//
// the structure represents the content of a column cell, for columns containing an array of structures.
//
// This particular string constructor handles the cell identification cell cluster structure containing
// the following structural elements:
//
//  typedef struct blob_point_struct
//  {
//    int16_t		startx;
//    int16_t		starty;
//  } blob_point;
//
//  typedef struct blob_rect_t_struct
//  {
//	  blob_point	start;
//	  int16_t		width;
//	  int16_t		height;
//  } blob_rect_t;
//
//  typedef struct cluster_data_t_struct
//  {
//	  int16_t					cell_count;
//	  std::vector<blob_point>	cluster_polygon;	// all vertices of a (potentially) irregular outline
//	  blob_rect_t				cluster_box;
//  } cluster_data_t;
//
// This constructor handles the vector of cluster data_t structures, and creates a list of separated
// array elements, where each array element represents a single cluster structure element in the vector.
//
// Output format will resenble the example below:
//
//   [ ROW(cell_count,[(x1,y1), (x2,Y2(,...,(Xn,Yn)],((startx, starty),width, height))
//     ROW(cell_count,[(x1,y1), (x2,Y2(,...,(Xn,Yn)],((startx, starty),width, height))
//     ROW(cell_count,[(x1,y1), (x2,Y2(,...,(Xn,Yn)],((startx, starty),width, height))
//           .
//           .
//           .
//     ROW(cell_count,[(x1,y1), (x2,Y2(,...,(Xn,Yn)],((startx, starty),width, height)) ]
//
// The structure inserted is a custom composite type in the database, so the insertion string formatting
// uses the "ROW(cnt,(v1b,v1c,v1d,v1e),((x,ry),rw,rh))r" format to designate the content of the composite
// data structure and avoid excessive requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not allow specification of a
// starting element (StartIndex in other methods).
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateClusterDataArrayString( std::string& DataString, const int32_t DataCount,
												const std::vector<cluster_data_t>& ClusterDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	size_t listSize = 0;
	std::vector<std::string> clusterStrList = {};
	std::string clusterCntStr = "";
	std::string clusterPolygonStr = "";
	std::string clusterBoxStr = "";

	listSize = ClusterDataList.size();

	for (const auto& clusterData : ClusterDataList)
	{
		//TODO: this causes a crash because it causes -1 to be retruned...
				//if ( cluster.cell_count == 0 )
				//{
				//	continue;
				//}

		clusterPolygonStr.clear();
		if (CreateBlobPointArrayString(clusterPolygonStr, (int32_t)clusterData.cluster_polygon.size(), clusterData.cluster_polygon, use_rows, format) == 0)
		{
			return -1;
		}

		clusterBoxStr.clear();
		if (!MakeBlobRectString(clusterBoxStr, clusterData.cluster_box, use_rows))
		{
			return -1;
		}

		std::string lineStr = "";
#ifdef USE_BOOST_FORMATTING
		lineStr.append(boost::str(boost::format("(%d,%s,%s)") % clusterData.cell_count % clusterPolygonStr % clusterBoxStr));
#else
		char fmtBuf[32] = {};

		sprintf(fmtBuf, "(%d,", clusterData.cell_count);

		lineStr.append( fmtBuf );
		lineStr.append( clusterPolygonStr );
		lineStr.append( "," );
		lineStr.append( clusterBoxStr );
		lineStr.append( ")" );
#endif // USE_BOOST_FORMATTING

		clusterStrList.push_back( lineStr );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, clusterStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT )
			{
				DataString.append( "::cluster_data[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}




// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateClusterCellCountArrayString( std::string& DataString, const int32_t DataCount,
													 const std::vector<cluster_data_t>& ClusterDataList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	size_t listSize = 0;
	std::vector<int16_t> clusterCellCntList = {};

	listSize = ClusterDataList.size();

	for ( const auto& clusterData : ClusterDataList )
	{
		clusterCellCntList.push_back( clusterData.cell_count );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateInt16ValueArrayString( DataString, "%d", DataCount, 0, clusterCellCntList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::smallint[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateClusterPolygonArrayString( std::string& DataString, const int32_t DataCount,
												   const std::vector<cluster_data_t>& ClusterDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listCnt = 0;
	size_t listSize = 0;
	int32_t lineSize = 0;
	std::string clusterPolygonStr = "";
	std::string lineStr = "";
	std::vector<std::string> clusterStrList = {};

	listSize = ClusterDataList.size();

	for ( const auto& clusterData : ClusterDataList )
	{
		clusterPolygonStr.clear();
		lineStr.clear();

		listCnt = CreateBlobPointArrayString( clusterPolygonStr, (int32_t) clusterData.cluster_polygon.size(), clusterData.cluster_polygon, use_rows, format );
		if ( listCnt > lineSize )
		{
			lineSize = listCnt;
		}

		if ( listCnt == 0 )
		{
			return -1;
		}

		if ( format == IR_COMPOSITE_LISTS_FORMAT )
		{
			std::string lineStr = "";

#ifdef USE_BOOST_FORMATTING
			lineStr.append( boost::str( boost::format( "(%d,%s)" ) % cnt % clusterPolygonStr ) );
#else

			char fmtBuf[32] = {};

			sprintf( fmtBuf, "(%d,", cnt );
			lineStr.append( fmtBuf );
			lineStr.append( clusterPolygonStr );
			lineStr.append( ")" );

#endif // USE_BOOST_FORMATTING

			clusterStrList.push_back( lineStr );
		}
		else
		{
			clusterStrList.push_back( clusterPolygonStr );
		}
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, clusterStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_outline_array[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateClusterRectArrayString( std::string& DataString, const int32_t DataCount,
												const std::vector<cluster_data_t>& ClusterDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	size_t listSize = 0;
	std::vector<std::string> clusterStrList = {};
	std::string clusterBoxStr = "";

	listSize = ClusterDataList.size();

	for ( const auto& clusterData : ClusterDataList )
	{
		clusterBoxStr.clear();
		if ( !MakeBlobRectString( clusterBoxStr, clusterData.cluster_box, use_rows ) )
		{
			return -1;
		}

		clusterStrList.push_back( clusterBoxStr );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, clusterStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_rect[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a database array field;
//
// the structure represents the content of a column cell, for columns containing an array of structures.
//
// This particular string constructor handles the blob_data_t structure type, which is comprised of the
// following structural elements:
//
//	struct Characteristic_t
//	{
//		uint16_t key;
//		uint16_t s_key;
//		uint16_t s_s_key;
//	};
//  NOTE: the Characteristic_t sttructure nominally contains an operator method not considered or presented here.
//
//  typedef struct blob_point_struct
//  {
//    int16_t		startx;
//    int16_t		starty;
//  } blob_point;
//
//  typedef std::pair<Characteristic_t, float> blob_info_pair;
//
//  typedef struct blob_data_t_struct
//  {
//  	std::vector<blob_info_pair>	blob_info;
//  	blob_point					blob_center;
//  	std::vector<blob_point>		blob_outline;
//  } blob_data_t;
//
// This constructor handles the vector of cluster data_t structures, and creates a list of separated array
// elements, where each array element represents a single cluster structure element in the vector.
//
// Output format will resenble the example below:
//
//   [ ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...],(x,y),[(x1,y1),(x2,y2),...] ),
//     ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...],(x,y),[(x1,y1),(x2,y2),...] ),
//     ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...],(x,y),[(x1,y1),(x2,y2),...] ),
//           .
//           .
//           .
//     ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...],(x,y),[(x1,y1),(x2,y2),...] ) ]
//
// The structure inserted is a custom composite type in the database, so the insertion string formatting
// uses the "ROW(xxxxxxxx)" format to designate the content of the entire composite data structure
// and avoid excessive requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not allow specification of a
// starting element (StartIndex in other methods).
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateBlobDataArrayString( std::string& DataString, const int32_t DataCount,
											 const std::vector<blob_data_t>& BlobDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	std::string blobInfoArrayStr = "";
	std::string blobPointStr = "";
	std::string blobOutlineArrayStr = "";
	std::string lineStr = "";
	std::vector<std::string> blobStrList = {};

	listSize = BlobDataList.size();

	for ( auto blobDataIter = BlobDataList.begin(); blobDataIter != BlobDataList.end() && listIndex < listSize; ++blobDataIter, listIndex++ )
	{
		blob_data_t blob = *blobDataIter;
		std::vector<blob_info_pair>& blobInfoList = blob.blob_info;
		std::vector<blob_point>& pointList = blob.blob_outline;

		blobInfoArrayStr.clear();
		blobPointStr.clear();
		blobOutlineArrayStr.clear();
		lineStr.clear();

		if ( CreateBlobCharacteristicsArrayString( blobInfoArrayStr, (int32_t)blobInfoList.size(), blobInfoList, use_rows, format ) == 0 )
		{
			return -1;
		}

		if ( !MakeBlobPointString( blobPointStr, blob.blob_center, use_rows ) )
		{
			return -1;
		}

		if ( CreateBlobPointArrayString( blobOutlineArrayStr, (int32_t)pointList.size(), pointList, use_rows, format ) == 0 )
		{
			return -1;
		}

#ifdef USE_BOOST_FORMATTING
		lineStr.append( boost::str( boost::format( "(%s,%s,%s)" ) % blobInfoArrayStr % blobPointStr % blobOutlineArrayStr ) );
#else
		lineStr.append( "(" );
		lineStr.append( blobInfoArrayStr );
		lineStr.append( "," );
		lineStr.append( blobPointStr );
		lineStr.append( "," );
		lineStr.append( blobOutlineArrayStr );
		lineStr.append( ")" );
#endif // USE_BOOST_FORMATTING

		blobStrList.push_back( lineStr );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, blobStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT )
			{
				DataString.append( "::blob_data[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a database array field;
//
// the structure represents the content of a column cell, for columns containing an array of structures
// based on the contents of the blob_data_t super-structure.
//
//  typedef struct blob_data_t_struct
//  {
//  	std::vector<blob_info_pair>	blob_info;
//  	blob_point					blob_center;
//  	std::vector<blob_point>		blob_outline;
//  } blob_data_t;
//
// This particular string constructor handles the blob_info sub-element type, which is the
// following 2-element structural element:
//
//		typedef std::pair<Characteristic_t, float> blob_info_pair;
//
//		struct Characteristic_t
//		{
//			uint16_t key;
//			uint16_t s_key;
//			uint16_t s_s_key;
//		};
//		NOTE: the Characteristic_t sttructure nominally contains an operator method not considered or presented here.
//
// Output format will resenble the example below:
//
//   ARRAY[ ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...] ),
//          ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...] ),
//          ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...] ),
//                .
//                .
//                .
//          ROW( [((key1,s_key1,s_s_key1),float_value1),((key2,s_key2,s_s_key2),float_value2),...] ) ]
//
// The structure inserted is a custom composite type in the database, so the insertion string formatting may
// use the "ROW(xxxxxxxx)" format to designate the content of the entire composite data structure and avoid
// excessive requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not allow specification of a
// starting element (StartIndex in other methods).
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateBlobDataBlobInfoArrayString( std::string& DataString, const int32_t DataCount,
													 const std::vector<blob_data_t>& BlobDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	int32_t listCnt = 0;
	int32_t lineSize = 0;
	std::string blobInfoArrayStr = "";
	std::string lineStr = "";
	std::vector<std::string> blobInfoArrayStrList = {};

	listSize = BlobDataList.size();

	for ( auto blobDataIter = BlobDataList.begin(); blobDataIter != BlobDataList.end() && listIndex < listSize; ++blobDataIter, listIndex++ )
	{
		blob_data_t blob = *blobDataIter;
		std::vector<blob_info_pair>& blobInfoList = blob.blob_info;

		blobInfoArrayStr.clear();
		lineStr.clear();

		listCnt = CreateBlobCharacteristicsArrayString( blobInfoArrayStr, (int32_t) blobInfoList.size(), blobInfoList, use_rows, format );
		if ( listCnt > lineSize )
		{
			lineSize = listCnt;
		}

		if ( listCnt == 0 )
		{
			return -1;
		}

		if ( format == IR_COMPOSITE_LISTS_FORMAT )
		{
			std::string lineStr = "";
#ifdef USE_BOOST_FORMATTING
			lineStr.append( boost::str( boost::format( "(%d,%s)" ) % cnt % blobInfoArrayStr ) );
#else
			char fmtBuf[32] = {};

			sprintf( fmtBuf, "(%d,", cnt );
			lineStr.append( fmtBuf );
			lineStr.append( blobInfoArrayStr );
			lineStr.append( ")" );
#endif // USE_BOOST_FORMATTING

			blobInfoArrayStrList.push_back( lineStr );
		}
		else
		{
			blobInfoArrayStrList.push_back( blobInfoArrayStr );
		}
		cnt++;
	}

	if ( cnt == DataCount )
	{
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, blobInfoArrayStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_info_array[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a database array field;
//
// the structure represents the content of a column cell, for columns containing an array of structures
// based on the contents of the blob_data_t super-structure.
//
//  typedef struct blob_data_t_struct
//  {
//  	std::vector<blob_info_pair>	blob_info;
//  	blob_point					blob_center;
//  	std::vector<blob_point>		blob_outline;
//  } blob_data_t;
//
// This particular string constructor handles the blob_point structure sub-element type, which is comprised of the
// following structural element:
//
//  typedef struct blob_point_struct
//  {
//    int16_t		startx;
//    int16_t		starty;
//  } blob_point;
//
// Output format will resenble the example below:
//
//   [ ARRAY[ (x,y),(x,y),(x,y,...(x,y) ]
//
// The structure inserted is a custom composite type in the database, so the insertion string formatting
// may use the "ROW(xxxxxxxx)" format to designate the content of the entire composite data structure
// and avoid excessive requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not allow specification of a
// starting element (StartIndex in other methods).
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateBlobDataBlobLocationArrayString( std::string& DataString, const int32_t DataCount,
														 const std::vector<blob_data_t>& BlobDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	std::string blobPointStr = "";
	std::string lineStr = "";
	std::vector<std::string> blobStrList = {};

	listSize = BlobDataList.size();

	for ( auto blobDataIter = BlobDataList.begin(); blobDataIter != BlobDataList.end() && listIndex < listSize; ++blobDataIter, listIndex++ )
	{
		blob_data_t blob = *blobDataIter;

		blobPointStr.clear();
		lineStr.clear();

		if ( !MakeBlobPointString( blobPointStr, blob.blob_center, use_rows ) )
		{
			return -1;
		}

		blobStrList.push_back( blobPointStr );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, blobStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_point[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a database array field;
//
// the structure represents the content of a column cell, for columns containing an array of structures
// based on the contents of the blob_data_t super-structure.
//
//  typedef struct blob_data_t_struct
//  {
//  	std::vector<blob_info_pair>	blob_info;
//  	blob_point					blob_center;
//  	std::vector<blob_point>		blob_outline;
//  } blob_data_t;
//
// This particular string constructor handles the blob_outline sub-elementstructure type, which is the
// following structural element:
//
//  typedef struct blob_point_struct
//  {
//    int16_t		startx;
//    int16_t		starty;
//  } blob_point;
//
// This constructor handles the vector of blob outline sub-vectors, and creates a list of separated array
// elements, where each array element represents a single cluster structure element in the vector.
//
// Output format will resenble the example below:
//
//   [ ROW( [(x1,y1),(x2,y2),...] ),
//     ROW( [(x1,y1),(x2,y2),...] ),
//     ROW( [(x1,y1),(x2,y2),...] ),
//           .
//           .
//           .
//     ROW( [(x1,y1),(x2,y2),...] ) ]
//
// The structure inserted is a custom composite type in the database, so the insertion string formatting
// may use the "ROW(xxxxxxxx)" format to designate the content of the entire composite data structure
// and avoid excessive requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not allow specification of a
// starting element (StartIndex in other methods).
//
// The DataString will contain the data formatted correctly for addition to an
// insertion string as an array of values to be inserted into database table
// array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateBlobDataBlobOutlineArrayString( std::string& DataString, const int32_t DataCount,
														const std::vector<blob_data_t>& BlobDataList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listCnt = 0;
	int32_t lineSize = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	std::string blobOutlineArrayStr = "";
	std::string lineStr = "";
	std::vector<std::string> blobStrList = {};

	listSize = BlobDataList.size();

	for ( auto blobDataIter = BlobDataList.begin(); blobDataIter != BlobDataList.end() && listIndex < listSize; ++blobDataIter, listIndex++ )
	{
		blob_data_t blob = *blobDataIter;
		std::vector<blob_point>& pointList = blob.blob_outline;

		blobOutlineArrayStr.clear();
		lineStr.clear();

		listCnt = CreateBlobPointArrayString( blobOutlineArrayStr, (int32_t) pointList.size(), pointList, use_rows, format );
		if ( listCnt > lineSize )
		{
			lineSize = listCnt;
		}

		if ( listCnt == 0 )
		{
			return -1;
		}

		if ( format == IR_COMPOSITE_LISTS_FORMAT )
		{
			std::string lineStr = "";
#ifdef USE_BOOST_FORMATTING
			lineStr.append( boost::str( boost::format( "(%d,%s)" ) % cnt % blobOutlineArrayStr ) );
#else
			char fmtBuf[32] = {};

			sprintf( fmtBuf, "(%d,", cnt );
			lineStr.append( fmtBuf );
			lineStr.append( blobOutlineArrayStr );
			lineStr.append( ")" );
#endif // USE_BOOST_FORMATTING

			blobStrList.push_back( lineStr );
		}
		else
		{
			blobStrList.push_back( blobOutlineArrayStr );
		}
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, blobStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_outline_array[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

// format complex structure data values to an insertion string for a database field;
//
// the structure represents the content of a blob_info_pair structure as the elements in the array.
//
int32_t DBifImpl::CreateBlobCharacteristicsArrayString( std::string& DataString, const int32_t DataCount,
														const std::vector<blob_info_pair>& BlobInfoList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	std::string infoStr = "";
	std::string arrayStr = "";
	std::string lineStr = "";
	std::vector<std::string> infoStrList = {};

	for ( x = 0; x < DataCount; x++ )
	{
		blob_info_pair info = BlobInfoList.at( x );

		lineStr.clear();
		if ( MakeBlobInfoPairString( lineStr, info, use_rows ) )
		{
			if ( lineStr.length() <= 0 )
			{
				continue;
			}

			infoStrList.push_back( lineStr );
			cnt++;
		}
		else
		{
			break;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, infoStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_characteristics[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

// these are never used as independent column structures, but always contained in another element;
// Because this creates an aray of composite-type sub-elements, each array element will be enclosed
// in a ROW() container, but it still must be enclosed in an ARRAY[] container and the outer
// composite-type element must place it into an appropriate ROW() container.
int32_t DBifImpl::CreateBlobPointArrayString( std::string& DataString, const int32_t DataCount,
											  const std::vector<blob_point>& BlobPointList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	std::string ptStr = "";
	std::string lineStr = "";
	std::vector<std::string> pointStrList = {};

	for ( x = 0; x < DataCount; x++ )
	{
		blob_point pt = BlobPointList.at( x );

		lineStr.clear();
		if ( MakeBlobPointString( lineStr, pt, use_rows ) )
		{
			if ( lineStr.length() <= 0 )
			{
				continue;
			}

			pointStrList.push_back( lineStr );
			cnt++;
		}
		else
		{
			return -1;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, pointStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_point[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

int32_t DBifImpl::CreateBlobRectArrayString( std::string& DataString, const int32_t DataCount,
											 const std::vector<blob_rect_t>& BlobRectList, bool use_rows, int32_t format )
{
	int32_t x = 0;
	int32_t cnt = 0;
	std::string rectStr = "";
	std::string arrayStr = "";
	std::vector<std::string> rectStrList = {};

	for ( x = 0; x < DataCount; x++ )
	{
		blob_rect_t rect = BlobRectList.at( x );

		rectStr.clear();
		if ( MakeBlobRectString( rectStr, rect, use_rows ) )
		{
			if ( rectStr.length() <= 0 )
			{
				continue;
			}

			rectStrList.push_back( rectStr );
			cnt++;
		}
		else
		{
			return -1;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, rectStrList, format );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			if ( format == IR_COMPOSITE_FORMAT || format == IR_COMPOSITE_LISTS_FORMAT )
			{
				DataString.append( "::blob_rect[]" );
			}
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

int32_t DBifImpl::CreateColumnDisplayInfoArrayString( std::string& DataString, const int32_t DataCount,
													  const std::vector<display_column_info_t>& ColumnInfoList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	std::vector<std::string> colInfoStrList = {};
	std::string lineStr = "";

	for ( auto columnIter = ColumnInfoList.begin(); columnIter != ColumnInfoList.end(); ++columnIter )
	{
		display_column_info_t colInfo = *columnIter;

		lineStr.clear();

		if ( use_rows )
		{
			lineStr = RowStr;
		}

#ifdef USE_BOOST_FORMATTING
		lineStr = boost::str( boost::format( "(%d,%d,%d,%s)" ) % colInfo.ColumnType % colInfo.OrderIndex % colInfo.Width % ( ( colInfo.Visible == true ) ? TrueStr : FalseStr ) );
#else
		char fmtBuf[64] = {};

		sprintf( fmtBuf, "(%ld,%d,%d,%s)", colInfo.ColumnType, colInfo.OrderIndex, colInfo.Width, ( ( colInfo.Visible == true ) ? TrueStr : FalseStr ) );
		lineStr = fmtBuf;
#endif // USE_BOOST_FORMATTING

		if ( lineStr.length() > 0 )
		{
			colInfoStrList.push_back( lineStr );
			cnt++;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, colInfoStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::column_display_info[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

int32_t DBifImpl::CreateSignatureArrayString( std::string& DataString, const int32_t DataCount,
											  const std::vector<db_signature_t>& SignatureList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	std::vector<std::string> sigStrList = {};
	std::string lineStr = "";
	std::string sigTimeStr = "";
	std::string workingStr = "";

	for ( auto sigIter = SignatureList.begin(); sigIter != SignatureList.end(); ++sigIter )
	{
		db_signature_t sigRec = *sigIter;

		lineStr.clear();
		sigTimeStr.clear();
		workingStr.clear();

		GetDbTimeString( sigRec.signatureTime, sigTimeStr );

		if ( use_rows )
		{
			lineStr = RowStr;
		}

#ifdef USE_BOOST_FORMATTING
		lineStr.append( boost::str( boost::format( "('%s','%s','%s',%s,'%s')" ) % sigRec.userName % sigRec.shortSignature % sigRec.longSignature % sigTimeStr /* already quoted */ % sigRec.signatureHash ) );
#else
		lineStr.append( "('" );
		workingStr = sigRec.userName;
		SanitizeDataString( workingStr );
		lineStr.append( workingStr );
		lineStr.append( "','" );

		workingStr = sigRec.shortSignature;
		SanitizeDataString( workingStr );
		lineStr.append( workingStr );
		lineStr.append( "','" );

		workingStr = sigRec.longSignature;
		SanitizeDataString( workingStr );
		lineStr.append( workingStr );
		lineStr.append( "'," );

		lineStr.append( sigTimeStr );
		lineStr.append(",'");

		workingStr = sigRec.signatureHash;
		SanitizeDataString( workingStr );
		lineStr.append( workingStr );
		lineStr.append("')");
#endif // USE_BOOST_FORMATTING

		if ( lineStr.length() > 0 )
		{
			sigStrList.push_back( lineStr );
			cnt++;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, sigStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::signature_info[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// format complex structure data values to add to an insertion string for a database array field;
//
// the structure represents the content of a column cell, for columns containing an array of structures.
//
// This particular string constructor handles the cal_consumable_t structure type, which is comprised of the
// following structural elements:
//
//  typedef struct cal_consumable_struct
//  {
//  	std::string		label;
//  	std::string		lot_id;
//		int16_t			cal_type;
//  	system_TP		expiration_date;
//  	float			assay_val;
//  } cal_consumable_t;
//
// This constructor handles the vector of cal_consumable_t structures, and creates a list of separated array
// elements, where each array element represents a single consumable structure element in the vector.
//
// Output format will resenble the example below:
//
//   [ ROW( ('label','lot_id',cal_type,'expiration_date_str',assay_val),
//     ROW( ('label','lot_id',cal_type,'expiration_date_str',assay_val),
//     ROW( ('label','lot_id',cal_type,'expiration_date_str',assay_val),
//           .
//           .
//           .
//     ROW( ('label','lot_id',cal_type,'expiration_date_str',assay_val),
//
// The structure inserted is a custom composite type in the database, so the insertion string formatting
// uses the "ROW(xxxxxxxx)" format to designate the content of the entire composite data structure
// and avoid excessive requirements for the use of backslashes to escape special characters.
//
// NOTE: this method constructs the string for the entire vector, and does not allow specification of a
// starting element (StartIndex in other methods).
//
// The DataString will contain the data formatted correctly for addition to an insertion string as an array of
// values to be inserted into database table array fields with the required leading and trailing '[' ']' characters
int32_t DBifImpl::CreateCalConsumablesArrayString( std::string& DataString, const int32_t DataCount,
												   const std::vector<cal_consumable_t>& ConsumablesList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	std::vector<std::string> consumablesStrList = {};
	std::string consumablesStr = "";
	std::string lineStr = "";

	listSize = ConsumablesList.size();

	for ( auto cIter = ConsumablesList.begin(); cIter != ConsumablesList.end() && listIndex < listSize; ++cIter, listIndex++ )
	{
		cal_consumable_t consumable = *cIter;

		consumablesStr.clear();
		lineStr.clear();

		if ( !MakeCalConsumablesString( consumablesStr, consumable, use_rows ) )
		{
			return -1;
		}

		consumablesStrList.push_back( consumablesStr );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, consumablesStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::cal_consumable[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

int32_t DBifImpl::CreateLanguageInfoArrayString( std::string& DataString, const int32_t DataCount,
												 const std::vector<language_info_t>& LanguageList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	int32_t listIndex = 0;
	size_t listSize = 0;
	std::vector<std::string> languageStrList = {};
	std::string languageInfoStr = "";

	listSize = LanguageList.size();

	for ( auto lIter = LanguageList.begin(); lIter != LanguageList.end() && listIndex < listSize; ++lIter, listIndex++ )
	{
		language_info_t langInfo = *lIter;

		languageInfoStr.clear();

		if ( !MakeLanguageInfoString( languageInfoStr, langInfo, use_rows ) )
		{
			return -1;
		}

		languageStrList.push_back( languageInfoStr );
		cnt++;
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, languageStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::language_info[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

// these are never used as independent column structures, but always contained in another element;
// Because this creates an aray of composite-type sub-elements, each array element will be enclosed
// in a ROW() container, but it still must be enclosed in an ARRAY[] container and the outer
// composite-type element must place it into an appropriate ROW() container.
int32_t DBifImpl::CreateIlluminatorInfoArrayString( std::string& DataString, const int32_t DataCount,
													const std::vector<illuminator_info_t>& IlInfoList, bool use_rows )
{
	int32_t x = 0;
	int32_t cnt = 0;
	std::string ptStr = "";
	std::string lineStr = "";
	std::vector<std::string> pointStrList = {};

	for ( x = 0; x < DataCount; x++ )
	{
		illuminator_info_t ilInfo = IlInfoList.at( x );

		lineStr.clear();
		if ( MakeIlluminatorInfoString( lineStr, ilInfo, use_rows ) )
		{
			if ( lineStr.length() <= 0 )
			{
				continue;
			}

			pointStrList.push_back( lineStr );
			cnt++;
		}
		else
		{
			return -1;
		}
	}

	if ( cnt == DataCount )
	{
		// Creates the Array string with ARRAY[] enclosing formatters
		x = CreateDataValueStringArrayString( DataString, DataCount, 0, pointStrList );

		if ( x == 0 )
		{
			DataString.clear();
		}
		else
		{
			DataString.append( "::illuminator_info[]" );
		}
	}
	else
	{
		x = -1;
	}

	return( x );  // return the number of data elements added to the string
}

bool DBifImpl::MakeBlobInfoPairString( std::string& DataString, blob_info_pair pair, bool use_rows )
{
	blob_info_storage bInfo = {};

	bInfo.key = pair.first._Myfirst._Val;
	bInfo.s_key = pair.first._Get_rest()._Myfirst._Val;
	bInfo.s_s_key = pair.first._Get_rest()._Get_rest()._Myfirst._Val;
	bInfo.value = pair.second;

	return MakeBlobInfoString( DataString, bInfo, use_rows );
}

bool DBifImpl::MakeBlobInfoString( std::string& DataString, blob_info_storage info, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	std::string valueStr = boost::str( boost::format( DbFloatThresholdFmtStr ) % info.value );						// format float value using standard threshold format string
	formatStr = boost::str( boost::format( "(%d,%d,%d,%s)" ) % info.key % info.s_key % info.s_s_key % valueStr );
//	formatStr = boost::str( boost::format( "(%d,%d,%d,%0.8f" ) % info.key % info.s_key % info.s_s_key % info.value );
#else
	char lineBuf[64] = {};
	char fmtBuf[32] = {};

	sprintf( fmtBuf, DbFloatThresholdFmtStr, info.value );															// format float value using standard threshold format string
	sprintf( lineBuf, "(%u,%u,%u,%s)", info.key, info.s_key, info.s_s_key, fmtBuf );
	formatStr = lineBuf;
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeBlobPointString( std::string& DataString, blob_point point, bool use_rows )
{
	DataString.clear();
	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "(%d,%d)" ) % point.startx % point.starty );
#else
	char fmtBuf[64] = {};

	sprintf( fmtBuf, "(%d,%d)", point.startx, point.starty );
	formatStr = fmtBuf;
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeBlobRectString( std::string& DataString, blob_rect_t rect, bool use_rows )
{
	DataString.clear();
	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "(%d,%d,%d,%d)" ) % rect.start.startx % rect.start.starty % rect.width % rect.height );
#else
	char fmtBuf[64] = {};

	sprintf(fmtBuf, "(%d,%d,%d,%d)", rect.start.startx, rect.start.starty, rect.width, rect.height);
	formatStr = fmtBuf;
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeAdSettingsString( std::string& DataString, ad_settings_t ad_settings, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "('%s','%s',%d,'%s',%s)" ) % ad_settings.servername % ad_settings.server_addr % ad_settings.port_number % ad_settings.base_dn % ( ( ad_settings.enabled == true ) ? TrueStr : FalseStr ) );
#else
	std::string workingStr = "";
	char fmtBuf[16] = {};

	formatStr.append( "('" );
	workingStr = ad_settings.servername;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "','" );

	workingStr = ad_settings.server_addr;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "'," );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, "%d", ad_settings.port_number );
	formatStr.append( fmtBuf );
	formatStr.append( ",'" );

	workingStr = ad_settings.base_dn;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "'," );

	formatStr.append( ( (ad_settings.enabled == true) ? TrueStr : FalseStr) );
	formatStr.append( ")" );
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeAfSettingsString( std::string& DataString, af_settings_t af_settings, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "(%s,%ld,%ld,%d,%ld,%d,%ld)" ) % (( af_settings.save_image == true ) ? TrueStr : FalseStr) % af_settings.coarse_start % af_settings.coarse_end % af_settings.coarse_step % af_settings.fine_range % af_settings.fine_step % af_settings.sharpness_low_threshold );
#else
	char lineBuf[128] = {};

	sprintf( lineBuf, "(%s,%ld,%ld,%d,%ld,%d,%ld)", ( af_settings.save_image == true ) ? TrueStr : FalseStr, af_settings.coarse_start, af_settings.coarse_end, af_settings.coarse_step, af_settings.fine_range, af_settings.fine_step, af_settings.sharpness_low_threshold );
	formatStr = lineBuf;
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeEmailSettingsString( std::string& DataString, email_settings_t email_settings, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "('%s',%d,'%s','%s','%s')" ) % email_settings.server_addr % email_settings.port_number % ( ( email_settings.authenticate == true ) ? TrueStr : FalseStr ) % email_settings.username % email_settings.pwd_hash );
#else
	std::string workingStr = "";
	char fmtBuf[16] = {};

	formatStr.append( "('" );
	workingStr = email_settings.server_addr;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, "',%d,", email_settings.port_number );
	formatStr.append( fmtBuf );

	formatStr.append( ( ( email_settings.authenticate == true ) ? TrueStr : FalseStr ) );
	formatStr.append( ",'" );

	workingStr = email_settings.username;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "','" );

	workingStr = email_settings.pwd_hash;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "')" );
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeRfidSimInfoString( std::string& DataString, rfid_sim_info_t rfid_settings, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "('%s',%d,'%s','%s','%s')" ) % ( ( rfid_settings.set_valid_tag_data == true ) ? TrueStr : FalseStr ) % rfid_settings.total_tags % rfid_settings.main_bay_file % rfid_settings.door_left_file % rfid_settings.door_right_file );
#else
	std::string workingStr = "";
	char fmtBuf[16] = {};

	formatStr.append( "(" );
	formatStr.append( ( ( rfid_settings.set_valid_tag_data == true ) ? TrueStr : FalseStr ) );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, ",%d,'", rfid_settings.total_tags );
	formatStr.append( fmtBuf );

	workingStr = rfid_settings.main_bay_file;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "','" );

	workingStr = rfid_settings.door_left_file;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "','" );

	workingStr = rfid_settings.door_right_file;
	SanitizeDataString( workingStr );
	formatStr.append( workingStr );
	formatStr.append( "')" );
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeLanguageInfoString( std::string& DataString, language_info_t lang_info, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "(%d,'%s','%s','%s')" ) % lang_info.language_id % lang_info.language_name % lang_info.locale_tag % ( ( lang_info.active == true ) ? TrueStr : FalseStr ) );
#else
	char fmtBuf[16] = {};

	formatStr.append( "(" );
	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, "%d,'", lang_info.language_id );
	formatStr.append( fmtBuf );

	// NOTE: language name and locale strings are not user accessible, so no need to sanitize
	formatStr.append( ( ( lang_info.language_name.length() > 0 ) ? lang_info.language_name : " " ) );
	formatStr.append( "','" );

	formatStr.append( ( ( lang_info.locale_tag.length() > 0 ) ? lang_info.locale_tag : " " ) );
	formatStr.append( "'," );

	formatStr.append( ( ( lang_info.active == true ) ? TrueStr : FalseStr ) );
	formatStr.append( ")" );
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeRunOptionsString( std::string& DataString, run_options_t options, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "('%s','%s',%d,%d,%s,'%s',%s,'%s','%s','%s',%s,'%s',%d,%d,%d)" ) % options.sample_set_name % options.sample_name
																											% options.save_image_count % options.save_nth_image
																											% ( ( options.results_export == true ) ? TrueStr : FalseStr ) % options.results_export_folder
																											% ( ( options.append_results_export == true ) ? TrueStr : FalseStr ) % options.append_results_export_folder
																											% options.result_filename % options.results_folder
																											% ( ( options.auto_export_pdf == true ) ? TrueStr : FalseStr )
																											% options.csv_folder % options.wash_type % options.dilution % options.bpqc_cell_type_index );
#else
	std::string workingStr = "";
	char fmtBuf[32] = {};

	formatStr.append( "('" );
	if ( options.sample_set_name.length() > 0 )
	{
		workingStr = options.sample_set_name;
		SanitizeDataString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );
	formatStr.append( "','" );

	if ( options.sample_name.length() > 0 )
	{
		workingStr = options.sample_name;
		SanitizeDataString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, "',%d,%d,", options.save_image_count, options.save_nth_image );
	formatStr.append( fmtBuf );

	formatStr.append( ( ( options.results_export == true ) ? TrueStr : FalseStr ) );
	formatStr.append( ",'" );

	if ( options.results_export_folder.length() > 0 )
	{
		workingStr = options.results_export_folder;
		SanitizePathString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );
	formatStr.append( "'," );

	formatStr.append( ( ( options.append_results_export == true ) ? TrueStr : FalseStr ) );
	formatStr.append( ",'" );

	if ( options.append_results_export_folder.length() > 0 )
	{
		workingStr = options.append_results_export_folder;
		SanitizePathString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );
	formatStr.append( "','" );

	if ( options.result_filename.length() > 0 )
	{
		workingStr = options.result_filename;
		SanitizeDataString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );
	formatStr.append( "','" );

	if ( options.results_folder.length() > 0 )
	{
		workingStr = options.results_folder;
		SanitizePathString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );
	formatStr.append( "'," );

	formatStr.append( ( ( options.auto_export_pdf == true ) ? TrueStr : FalseStr ) );
	formatStr.append( ",'" );

	if ( options.csv_folder.length() > 0 )
	{
		workingStr = options.csv_folder;
		SanitizePathString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, "',%d,%d,%d)", options.wash_type, options.dilution, options.bpqc_cell_type_index );
	formatStr.append( fmtBuf );
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeCalConsumablesString( std::string& DataString, cal_consumable_t consumables, bool use_rows )
{
	DataString.clear();

	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
	std::string timeStr = {};

	GetDbTimeString( consumables.expiration_date, timeStr );

#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "('%s','%s',%d,'%s',%2.2f)" ) % consumables.label % consumables.lot_id % consumables.cal_type % timestr % consumables.assay_value );
#else
	std::string workingStr = "";
	char fmtBuf[16] = {};

	formatStr.append( "('" );

	if ( consumables.label.length() > 0 )
	{
		workingStr = consumables.label;
		SanitizeDataString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );
	formatStr.append( "','" );

	if ( consumables.lot_id.length() > 0 )
	{
		workingStr = consumables.lot_id;
		SanitizeDataString( workingStr );
	}
	else
	{
		workingStr = " ";
	}
	formatStr.append( workingStr );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, "',%d,", consumables.cal_type );
	formatStr.append( fmtBuf );

	formatStr.append( timeStr );

	memset( fmtBuf, 0, sizeof( fmtBuf ) );
	sprintf( fmtBuf, ",%2.2f", consumables.assay_value );
	formatStr.append( fmtBuf );
	formatStr.append( ")" );
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}

bool DBifImpl::MakeIlluminatorInfoString( std::string& DataString, illuminator_info il_info, bool use_rows )
{
	DataString.clear();
	if ( use_rows )
	{
		DataString = RowStr;
	}

	std::string formatStr = "";
#ifdef USE_BOOST_FORMATTING
	formatStr = boost::str( boost::format( "(%d,%d)" ) % il_info.type % il_info.index );
#else
	char fmtBuf[64] = {};

	sprintf( fmtBuf, "(%d,%d)", il_info.type, il_info.index );
	formatStr = fmtBuf;
#endif // USE_BOOST_FORMATTING
	DataString.append( formatStr );

	return true;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


int32_t DBifImpl::GetRecordColumnTags( CRecordset& rs, std::vector<std::string>& taglist )
{
	int32_t colCnt = 0;
	CODBCFieldInfo fieldInfo = {};
	std::string tagStr = "";
	std::string logStr = "";
	CString temp = _T( "" );

	try
	{
		// get field/column count and header tags...
		if ( rs.GetRecordCount() > 0 && rs.CanScroll() )
		{
			rs.MoveFirst();
		}
		colCnt = rs.GetODBCFieldCount();
	}
	catch ( CDBException * pErr )
	{
		// Handle DB exceptions first...

		logStr = "Failed to execute column tag query request: ";

		if ( pErr->m_nRetCode == SQL_ERROR )
		{
			logStr.append( "SQL Error: exception msg: " );
			logStr.append( CT2A( pErr->m_strError ) );
		}
		else
		{
			logStr.append( "Unrecognized Error: exception msg: " );
			logStr.append( CT2A( pErr->m_strError ) );
		}

		pErr->Delete();
	}
	catch ( CException * e )
	{
		// Handle all other types of exceptions here.

		logStr = "Failed to execute column tag query request: Unrecognized exception error.";

		e->Delete();
	}

	if ( colCnt > 0 )
	{
		for ( int32_t idx = 0; idx < colCnt; idx++ )
		{
			rs.GetODBCFieldInfo( idx, fieldInfo );
			temp = fieldInfo.m_strName;
			tagStr = CT2A( temp );
			taglist.push_back( tagStr );
		}
	}

	return colCnt;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Publicly visible methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Query results string helper methods
////////////////////////////////////////////////////////////////////////////////

void DBifImpl::GetQueryResultString( DBApi::eQueryResult resultCode, std::string& resultString )
{
	bool resultOk = true;
	std::string msgStr = "";

	switch ( resultCode )
	{
		case DBApi::eQueryResult::InvalidUUIDError:
			msgStr = "Query failed: Invalid UUID supplied";
			break;

		case DBApi::eQueryResult::StatusRegressionNotAllowed:
			msgStr = "Query failed: attempted regression of sample record status";
			break;

		case DBApi::eQueryResult::DeleteProtectedFailed:
			msgStr = "Query failed: attempted deletion of protected record";
			break;

		case DBApi::eQueryResult::ParseFailure:
			msgStr = "Query failed: error parsing retrieved object";
			break;

		case DBApi::eQueryResult::MultipleObjectsFound:
			msgStr = "Query failed: multiple records found matching single record query";
			break;

		case DBApi::eQueryResult::InsertObjectExists:
			msgStr = "Query failed: attempted insertion of new record matching existing record identifiers";
			break;

		case DBApi::eQueryResult::BadOrMissingListIds:
			msgStr = "Query failed: missing list ID values";
			break;

		case DBApi::eQueryResult::BadOrMissingArrayVals:
			msgStr = "Query failed: midssing list array record or value";
			break;

		case DBApi::eQueryResult::MissingQueryKey:
			msgStr = "Query failed: missing critical identifier key";
			break;

		case DBApi::eQueryResult::NoQuery:
			msgStr = "Query failed: no valid query supplied";
			break;

		case DBApi::eQueryResult::BadQuery:
			msgStr = "Query failed: unknown / illegal / invalid query format supplied";
			break;

		case DBApi::eQueryResult::NoTargets:
			msgStr = "Query failed: mising table name or schema name or no field tage processed";
			break;

		case DBApi::eQueryResult::InsertFailed:
			msgStr = "Query failed: insertion operation failed";
			break;

		case DBApi::eQueryResult::QueryFailed:
			msgStr = "Query failed: unspecified or compound query execution failure.";
			break;

		case DBApi::eQueryResult::NotConnected:
			msgStr = "Query failed: Not connected under required or specified connection type";
			break;

		case DBApi::eQueryResult::NoData:
			msgStr = "Query failed: no data returned";
			break;

		case DBApi::eQueryResult::InternalNotSupported:
			msgStr = "Query failed: internal API not supported";
			break;

		case DBApi::eQueryResult::ApiNotSupported:
			msgStr = "Query failed: API deprecated or not supported";
			break;

		case DBApi::eQueryResult::NoResults:
			msgStr = "Query returned empty results list";
			break;

		case DBApi::eQueryResult::QueryOk:
			msgStr = "Query execution succeeded";
			break;

		default:
			msgStr = "Unknown query error";
			break;
	}
	resultString = msgStr;
}

bool DBifImpl::GetListQueryTags( DBApi::eListType listtype, std::string& schemaname, std::string& tablename,
								 std::string& selecttag, std::string& idvalstr )
{
	uuid__t tmpId = {};
	int64_t tmpIdNum = 1;	// use idnum 1 to force a select tag to be determined...
	int32_t tmpIndex = 0;	// use index 0 to force a select tag to be determined...
	std::string tmpName = "";
	bool tagsOk = false;

	ClearGuid( tmpId );

	// Get the schema and table names; use idnum 1 to force select a tag to be determined,
	// but select tag is not required for list retrieval
	switch ( listtype )
	{
		case DBApi::eListType::WorklistList:
			tagsOk = GetWorklistQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::SampleSetList:
			tagsOk = GetSampleSetQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::SampleItemList:
			tagsOk = GetSampleItemQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::SampleList:
			tagsOk = GetSampleQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::AnalysisList:
			tagsOk = GetAnalysisQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::SummaryResultList:
			tagsOk = GetSummaryResultQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::DetailedResultList:
			tagsOk = GetDetailedResultQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::ImageResultList:
			tagsOk = GetImageResultQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::SResultList:
			tagsOk = GetSResultQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::ImageSetList:
			tagsOk = GetImageSetQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::ImageSequenceList:
			tagsOk = GetImageSequenceQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::ImageList:
			tagsOk = GetImageQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::CellTypeList:
			// supply '0' for index to force tag selection, don't supply a valid idnum value to allow index to take priority...
			tagsOk = GetCellTypeQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIndex, NO_ID_NUM );
			break;

		case DBApi::eListType::ImageAnalysisParamList:
			tagsOk = GetImageAnalysisParamQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::AnalysisInputSettingsList:
			tagsOk = GetAnalysisInputSettingsQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::ImageAnalysisCellIdentParamList:
			tagsOk = GetImageAnalysisParamQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::AnalysisDefinitionList:
			// supply '0' for index to force tag selection...
			tagsOk = GetAnalysisDefinitionQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIndex, tmpIdNum );
			break;

		case DBApi::eListType::AnalysisParamList:
			tagsOk = GetAnalysisParamQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::IlluminatorList:
			// supply empty name and '0' for index to force tag selection...
			tagsOk = GetIlluminatorQueryTag( schemaname, tablename, selecttag, idvalstr, tmpName, tmpIndex, tmpIdNum );
			break;

		case DBApi::eListType::RolesList:
			tagsOk = GetRoleQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::UserList:
			tagsOk = GetUserQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::UserPropertiesList:
			// supply empty name and '0' for index to force tag selection...
			tagsOk = GetUserPropertyQueryTag( schemaname, tablename, selecttag, idvalstr, tmpIndex, tmpName );
			break;

		case DBApi::eListType::SignatureDefList:
			tagsOk = GetSignatureQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::ReagentInfoList:
// TODO: implement all associated object methods
			tagsOk = GetReagentTypeQueryTag( schemaname, tablename, selecttag, idvalstr, tmpIdNum, tmpName );
			break;

		case DBApi::eListType::WorkflowList:
// TODO: implement all associated object methods
//			tagsOk = GetWorkflowQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::BioProcessList:
			tagsOk = GetBioProcessQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::QcProcessList:
			tagsOk = GetQcProcessQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::InstrumentConfigList:
			tagsOk = GetInstConfigQueryTag( schemaname, tablename, selecttag, idvalstr, tmpName, tmpIdNum );
			break;

		case DBApi::eListType::CalibrationsList:
			tagsOk = GetCalibrationQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::LogEntryList:
			tagsOk = GetLogEntryQueryTag( schemaname, tablename, selecttag, idvalstr, tmpIdNum );
			break;

		case DBApi::eListType::SchedulerConfigList:
			tagsOk = GetSchedulerConfigQueryTag( schemaname, tablename, selecttag, idvalstr, tmpId, tmpIdNum );
			break;

		case DBApi::eListType::CellHealthReagentsList:
			tagsOk = GetCellHealthReagentsQueryTag(schemaname, tablename, selecttag, idvalstr, tmpName, tmpIndex, tmpIdNum);
			break;
		
		// invalid or unrecognized list-type; return empty table identifiers
		case DBApi::eListType::NoListType:
		default:
			tagsOk = false;
			break;
	}

	return tagsOk;
}

void DBifImpl::GetListSrcTableInfo( DBApi::eListType listtype, std::string& schemaname, std::string& tablename )
{
	bool typeOk = true;
	bool isDataTable = false;
	std::string tableStr = "";
	std::string schemaStr = "";

	// NOTE that the case identifiers may not be in proper sequential order between the instrument and data groups
	switch ( listtype )
	{
		// the follwoing tables are contained in the ViCellInstrument schema;
		case DBApi::eListType::WorklistList:
			tableStr = "Worklists";
			break;

		case DBApi::eListType::SampleSetList:
			tableStr = "SampleSets";
			break;

		case DBApi::eListType::SampleItemList:
			tableStr = "SampleItems";
			break;

		case DBApi::eListType::CellTypeList:
			tableStr = "CellTypes";
			break;

		case DBApi::eListType::ImageAnalysisParamList:
			tableStr = "ImageAnalysisParams";
			break;

		case DBApi::eListType::AnalysisInputSettingsList:
			tableStr = "AnalysisInputSettings";
			break;

		case DBApi::eListType::ImageAnalysisCellIdentParamList:
			tableStr = "ImageAnalysisCellIdentParams";
			break;

		case DBApi::eListType::AnalysisDefinitionList:
			tableStr = "AnalysisDefinitions";
			break;

		case DBApi::eListType::AnalysisParamList:
			tableStr = "AnalysisParams";
			break;

		case DBApi::eListType::IlluminatorList:
			tableStr = "IlluminatorTypes";
			break;

		case DBApi::eListType::RolesList:
			tableStr = "Roles";
			break;

		case DBApi::eListType::UserList:
			tableStr = "Users";
			break;

		case DBApi::eListType::UserPropertiesList:
			tableStr = "UserProperties";
			break;

		case DBApi::eListType::SignatureDefList:
			tableStr = "SignatureDefinitions";
			break;

		case DBApi::eListType::ReagentInfoList:
			tableStr = "ReagentInfo";
			break;

		case DBApi::eListType::WorkflowList:
			tableStr = "Workflows";
			break;

		case DBApi::eListType::BioProcessList:
			tableStr = "BioProcesses";
			break;

		case DBApi::eListType::QcProcessList:
			tableStr = "QcProcesses";
			break;

		case DBApi::eListType::InstrumentConfigList:
			tableStr = "InstrumentConfig";
			break;

		case DBApi::eListType::CalibrationsList:
			tableStr = "Calibrations";
			break;

		case DBApi::eListType::LogEntryList:
			tableStr = "SystemLogs";
			break;

		case DBApi::eListType::SchedulerConfigList:
			tableStr = "Scheduler";
			break;

		case DBApi::eListType::CellHealthReagentsList:
			tableStr = "CellHealthReagents";
			break;


		// the follwoing tables are contained in the ViCellData schema;
		case DBApi::eListType::SampleList:
			tableStr = "SampleProperties";
			isDataTable = true;
			break;

		case DBApi::eListType::AnalysisList:
			tableStr = "Analyses";
			isDataTable = true;
			break;

		case DBApi::eListType::SummaryResultList:
			tableStr = "SummaryResults";
			isDataTable = true;
			break;

		case DBApi::eListType::DetailedResultList:
			tableStr = "DetailedResults";
			isDataTable = true;
			break;

		case DBApi::eListType::ImageResultList:
			tableStr = "ImageResults";
			isDataTable = true;
			break;

		case DBApi::eListType::SResultList:
			tableStr = "SResults";
			isDataTable = true;
			break;

		case DBApi::eListType::ImageSetList:
			tableStr = "ImageSets";
			isDataTable = true;
			break;

		case DBApi::eListType::ImageSequenceList:
			tableStr = "ImageSequences";
			isDataTable = true;
			break;

		case DBApi::eListType::ImageList:
			tableStr = "ImageReferences";
			isDataTable = true;
			break;
		
		// invalid or unrecognized list-type; return empty table identifiers
		case DBApi::eListType::NoListType:
		default:
			typeOk = false;
			break;
	}

	if ( typeOk )
	{
		if ( isDataTable )
		{
			schemaStr = "ViCellData";
		}
		else
		{
			schemaStr = "ViCellInstrument";
		}
	}

	tablename = tableStr;
	schemaname = schemaStr;
}

void DBifImpl::GetSrcTableIdTags( DBApi::eListType listtype, std::string& idtag, std::string& idnumtag, std::string& indextag, std::string& nametag )
{
	bool resultOk = true;

	idtag = "ID";
	idnumtag = "IdNum";
	indextag = "";
	nametag = "";

	// NOTE that the case identifiers may not be in proper sequential order between the instrument and data groups
	switch ( listtype )
	{
		// the follwoing tables are contained in the ViCellInstrument schema;
		case DBApi::eListType::WorklistList:
			idtag = WL_IdTag;					// "WorklistID"
			idnumtag = WL_IdNumTag;				// "WorklistIdNum"
			break;

		case DBApi::eListType::SampleSetList:
			idtag = SS_IdTag;					// "SampleSetID"
			idnumtag = SS_IdNumTag;				// "SampleSetIdNum"
			break;

		case DBApi::eListType::SampleItemList:
			idtag = SI_IdTag;					// "SampleItemID"
			idnumtag = SI_IdNumTag;				// "SampleItemIdNum"
			break;

		case DBApi::eListType::CellTypeList:
			idtag = CT_IdTag;					// "CellTypeID"
			idnumtag = CT_IdNumTag;				// "CellTypeIdNum"
			indextag = CT_IdxTag;				// "CellTypeIndex"
			break;

		case DBApi::eListType::ImageAnalysisParamList:
			idtag = IAP_IdTag;					// "ImageAnalysisParamID"
			idnumtag = IAP_IdNumTag;			// "ImageAnalysisParamIdNum"
			break;

		case DBApi::eListType::AnalysisInputSettingsList:
			idtag = AIP_IdTag;					// "SettingsID"
			idnumtag = AIP_IdNumTag;			// "settingsIdNum"
			break;

		case DBApi::eListType::ImageAnalysisCellIdentParamList:
			idtag = "IdentParamID";
			idnumtag = "IdentParamIdNum";
			break;

		case DBApi::eListType::AnalysisDefinitionList:
			idtag = AD_IdTag;					// "AnalysisDefinitionID"
			idnumtag = AD_IdNumTag;				// "AnalysisDefinitionIdNum"
			indextag = AD_IdxTag;				// "AnalysisDefinitionIndex"
			break;

		case DBApi::eListType::AnalysisParamList:
			idtag = AP_IdTag;					// "AnalysisParamID"
			idnumtag = AP_IdNumTag;				// "AnalysisParamIdNum"
			break;

		case DBApi::eListType::IlluminatorList:
			idnumtag = IL_IdNumTag;				// "IlluminatorIdNum"
			indextag = IL_IdxTag;				// "IlluminatorIndex"
			nametag = IL_NameTag;				// "IlluminatorName"
			break;

		case DBApi::eListType::RolesList:
			idtag = RO_IdTag;					// "RoleID"
			idnumtag = RO_IdNumTag;				// "RoleIdNum"
			nametag = RO_NameTag;				// "RoleName"
			break;

		case DBApi::eListType::UserList:
			idtag = UR_IdTag;					// "UserID"
			idnumtag = UR_IdNumTag;				// "UserIdNum"
			nametag = UR_UserNameTag;				// "UserName"
			break;

		case DBApi::eListType::UserPropertiesList:
			idnumtag = UP_IdNumTag;				// "PropertyIdNum"
			indextag = UP_IdxTag;				// "PropertyIndex"
			nametag = UP_NameTag;				// "PropertyName";
			break;

		case DBApi::eListType::SignatureDefList:
			idtag = SG_IdTag;					// "SignatureDefID"
			idnumtag = SG_IdNumTag;				// "SignatureDefIdNum"
			break;

		case DBApi::eListType::ReagentInfoList:
			idtag = "";
			idnumtag = RX_IdNumTag;				// "ReagentIdNum"
			indextag = RX_TypeNumTag;			// "ReagentTypeNum"
			nametag = RX_ContainerRfidSNTag;	// "ContainerTagSN"
			break;

		case DBApi::eListType::WorkflowList:
			idtag = WF_IdTag;					// "WorkflowID"
			idnumtag = WF_IdNumTag;				// "WorkflowIdNum"
			nametag = WF_NameTag;				// "WorkflowName"
			break;

		case DBApi::eListType::BioProcessList:
			idtag = BP_IdTag;					// "BioProcessID"
			idnumtag = BP_IdNumTag;				// "BioProcessIdNum"
			break;

		case DBApi::eListType::QcProcessList:
			idtag = QC_IdTag;					// "QcProcessID"
			idnumtag = QC_IdNumTag;				// "QcProcessIdNum"
			nametag = QC_NameTag;				// "QcName"
			break;

		case DBApi::eListType::InstrumentConfigList:
			idnumtag = CFG_IdNumTag;
			nametag = CFG_InstSNTag;
			break;

		case DBApi::eListType::CalibrationsList:
			idtag = CC_IdTag;
			idnumtag = CC_IdNumTag;
			break;

		case DBApi::eListType::LogEntryList:
			idtag = "";
			idnumtag = LOG_IdNumTag;			// "IdNum"
			break;

		case DBApi::eListType::SchedulerConfigList:
			idtag = SCH_IdTag;					// "SchedulerConfigID"
			idnumtag = SCH_IdNumTag;			// "SchedulerConfigIdNum"
			break;


		// the follwoing tables are contained in the ViCellData schema;
		case DBApi::eListType::SampleList:
			idtag = SM_IdTag;					// "SampleID"
			idnumtag = SM_IdNumTag;				// "SampleIdNum"
			break;

		case DBApi::eListType::AnalysisList:
			idtag = AN_IdTag;					// "AnalysisID"
			idnumtag = AN_IdNumTag;				// "AnalysisIdNum"
			break;

		case DBApi::eListType::SummaryResultList:
			idtag = RS_IdTag;					// "SummaryResultID"
			idnumtag = RS_IdNumTag;				// "SummaryResultIdNum"
			break;

		case DBApi::eListType::DetailedResultList:
			idtag = RD_IdTag;					// "DetailedResultID"
			idnumtag = RD_IdNumTag;				// "DetailedResultIdNum"
			break;

		case DBApi::eListType::ImageResultList:
			idtag = RI_IdTag;					// "ResultsID"
			idnumtag = RI_IdNumTag;				// "ResultsIdNum"
			break;

		case DBApi::eListType::SResultList:
			idtag = SR_IdTag;					// "ResultsID"
			idnumtag = SR_IdNumTag;				// "ResultsIdNum"
			break;

		case DBApi::eListType::ImageSetList:
			idtag = IC_IdTag;					// "ImageSetID"
			idnumtag = IC_IdNumTag;				// "ImageSetIdNum"
			break;

		case DBApi::eListType::ImageSequenceList:
			idtag = IS_IdTag;					// "ImageSequenceID"
			idnumtag = IS_IdNumTag;				// "ImageSequenceIdNum"
			break;

		case DBApi::eListType::ImageList:
			idtag = IM_IdTag;					// "ImageID"
			idnumtag = IM_IdNumTag;				// "ImageIdNum"
			break;

		case DBApi::eListType::CellHealthReagentsList:
			idtag = CH_IdTag;					// "CellHealth ID"
			idnumtag = CH_IdNumTag;				// "CellHealth IdNum"
			nametag = CH_NameTag;				// "CellHealth Name"
			break;

		case DBApi::eListType::NoListType:
		default:
			break;
	}
}

bool DBifImpl::GetSrcTableQueryInfo( DBApi::eListType listtype, std::string& schemaname, std::string& tablename,
									 std::string& selecttag, std::string& idstr,
									 uuid__t id, int64_t idnum, int64_t index, std::string name )
{
	selecttag.clear();
	idstr.clear();

	GetListSrcTableInfo( listtype, schemaname, tablename );

	std::string idTag = "";
	std::string idnumTag = "";
	std::string indexTag = "";
	std::string nameTag = "";

	GetSrcTableIdTags( listtype, idTag, idnumTag, indexTag, nameTag );

	// shouldn't ever have multiple identifiers specified...
	// but process in priority order
	if ( GuidValid( id ) )
	{
		selecttag = idTag;
		FormatGuidString( id, idstr );
	}
	else if ( idnum > 0 )
	{
		selecttag = idnumTag;
		idstr = boost::str( boost::format( "%lld" ) % idnum );
	}
	else if ( index >= 0 )
	{
		selecttag = indexTag;
		idstr = boost::str( boost::format( "%lld" ) % index );
	}
	else if ( name.length() > 0 )
	{
		selecttag = nameTag;
		idstr = name;
	}
	else
	{
		WriteLogEntry( "GetSrcTableQueryInfo: failed: no valid identifier supplied", ErrorMsgType );
		return false;
	}

	return true;
}

DBApi::eLoginType DBifImpl::ConnectionCheck( CDatabase*& pDb, DBApi::eLoginType requestedType )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

	if ( !IsLoginType( requestedType ) )
	{
		if ( requestedType == DBApi::eLoginType::AdminLoginType ||
			 requestedType == DBApi::eLoginType::InstOrAdminLoginTypes )
		{
			loginType = LoginAsAdmin();
		}
		else
		{
			loginType = LoginAsInstrument();
		}
	}

	loginType = GetDBLoginConnection( pDb, requestedType );

	if ( ( loginType != DBApi::eLoginType::InstrumentLoginType && loginType != DBApi::eLoginType::AdminLoginType ) || ( pDb == nullptr ) )
	{
		return DBApi::eLoginType::NoLogin;
	}

	return loginType;
}

DBApi::eQueryResult DBifImpl::FindSampleItemListMatch( DB_SampleSetRecord& ssr, uuid__t ssitemid, int64_t ssitemidnum )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( !GuidValid( ssitemid ) && ssitemidnum <= 0 )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	size_t listCnt = ssr.SSItemsList.size();

	DB_SampleItemRecord tmpItem = {};
	bool idOk = false;

	for ( auto iter = ssr.SSItemsList.begin(); iter != ssr.SSItemsList.end(); ++iter )
	{
		idOk = false;
		if ( GuidValid( iter->SampleItemId ) )
		{
			idOk = GuidsEqual( iter->SampleItemId, ssitemid );
		}
		else if ( iter->SampleItemIdNum > 0 && iter->SampleItemIdNum == ssitemidnum )
		{
			idOk = true;
		}

		if ( idOk )
		{
			ssr.SSItemsList.erase( iter );
			listCnt = ssr.SSItemsList.size();
			break;
		}
	}
	listCnt = ssr.SSItemsList.size();
	if ( ssr.SampleItemCount != listCnt )
	{
		ssr.SampleItemCount = static_cast< int16_t >( listCnt );
		queryResult = UpdateSampleSetRecord( ssr );
	}
	else
	{
		queryResult = DBApi::eQueryResult::QueryOk;
	}

	return queryResult;
}

bool DBifImpl::GetUserNameString( uuid__t userid, std::string & username )
{
	username.clear();

	if ( !GuidValid( userid ) )
	{
#ifdef ALLOW_ID_FAILS
		return true;
#else
		return false;
#endif // ALLOW_ID_FAILS
	}

	DB_UserRecord  ur = {};
	ur.UserNameStr.clear();

	if ( GetUser( ur, userid ) != DBApi::eQueryResult::QueryOk )
	{
#ifdef ALLOW_ID_FAILS
		return true;
#else
		return false;
#endif // ALLOW_ID_FAILS
	}

	username = ur.UserNameStr;

	return true;
}

bool DBifImpl::GetProcessNameString( DBApi::eListType processtype, uuid__t processid, std::string& processname )
{
	processname.clear();

	if ( !GuidValid( processid ) )
	{
		return false;
	}

	bool success = true;

	switch ( processtype )
	{
		case DBApi::eListType::BioProcessList:
		{
			DB_BioProcessRecord  bpr = {};

			if ( GetBioProcess( bpr, processid ) != DBApi::eQueryResult::QueryOk )
			{
				return false;
			}
			processname = bpr.BioProcessName;
			break;
		}

		case DBApi::eListType::QcProcessList:
		{
			DB_QcProcessRecord  qcr = {};

			if ( GetQcProcess( qcr, processid ) != DBApi::eQueryResult::QueryOk )
			{
				return false;
			}
			processname = qcr.QcName;
			break;
		}

		default:
			return false;
	}

	return true;
}
