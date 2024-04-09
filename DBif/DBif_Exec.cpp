// Database interface : implementation file
//

#pragma once

#include "pch.h"
#include "atlstr.h"
#include "atlconv.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>


#include "DBif_Impl.hpp"
#ifdef USE_LOGGER
#include "Logger.hpp"
#endif



static const std::string MODULENAME = "DBif_Exec";




////////////////////////////////////////////////////////////////////////////////
// DBifImpl database interface internal query execution methods
////////////////////////////////////////////////////////////////////////////////

bool DBifImpl::RunColTagQuery( CDatabase* db, std::string schemaname, std::string tablename,
							   int32_t& colcnt, std::vector<std::string>& taglist )
{
	bool queryOk = false;
	std::string logStr = "";
	CString qStr = _TEXT( "" );
	CStringA qStr2 = "";
	std::string queryStr = "";

	colcnt = 0;
	taglist.clear();

	if ( db != nullptr )
	{
		CRecordset recordSet( db );
		BOOL query_Ok = FALSE;
		DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

		loginType = GetDBLoginConnection( db, DBApi::eLoginType::AnyLoginType );
		recordSet.m_pDatabase = db;

		queryStr = boost::str( boost::format( "%s * %s \"%s\".\"%s\" %s 1" ) % SelectStr % FromStr % schemaname % tablename % LimitStr );
		qStr2 = queryStr.c_str();
		qStr = CA2T( qStr2.GetString(), CP_UTF8 );

		db->SetQueryTimeout( TagQueryTimeout );

		try
		{
			query_Ok = recordSet.Open( AFX_DB_USE_DEFAULT_TYPE, qStr, CRecordset::readOnly );
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

		if ( query_Ok == TRUE )
		{
			queryOk = true;

			if ( recordSet.GetRecordCount() > 0 && recordSet.CanScroll() )
			{
				recordSet.MoveFirst();
			}

			colcnt = GetRecordColumnTags( recordSet, taglist );
//			logStr = boost::str( boost::format( "RunColTagQuery: Retrieved Record column tags for %d tags." ) % colcnt );
		}

		if ( recordSet.IsOpen() )
		{
			recordSet.Close();
		}
	}
	else
	{
		logStr = "Failed to run select query: db object not initialized.";
	}

	std::string logQueryStr = "";

	if ( queryStr.length() > 0 )
	{
		logQueryStr = boost::str( boost::format( "\tqueryStr: '%s'" ) % queryStr );
		if ( qStr.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t    qStr: '%s'" ) % CT2A( qStr ) ) );
		}
#ifdef DEBUG
		if ( qStr2.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t   qStr2: '%s'" ) % qStr2.GetString() ) );
		}
		std::string dbgEntryStr = boost::str( boost::format( "RunColTagQuery:\n%s\n" ) % logQueryStr );
		WriteLogEntry( dbgEntryStr, InfoMsgType );
#endif
	}

	if ( logStr.length() > 0 )
	{
		std::string logEntryStr = boost::str( boost::format( "RunColTagQuery:\n\tlogstr: %s" ) % logStr );
		if ( logQueryStr.length() > 0 )
		{
			logEntryStr.append( logQueryStr );
		}
		WriteLogEntry( logEntryStr, QueryErrorMsgType );
	}

	db->SetQueryTimeout( DefaultQueryTimeout );

	return queryOk;
}

bool DBifImpl::GetFullSelectQueryWithColTags( CDatabase* db, CRecordset& resultset,
											  std::string schemaname, std::string tablename,
											  int32_t& colcnt, std::vector<std::string>& taglist )
{
	bool queryOk = false;
	std::string query_str = "";

	query_str = boost::str( boost::format( "%s * %s \"%s\".\"%s\"" ) % SelectStr % FromStr % schemaname.c_str() % tablename.c_str() );

	queryOk = GetFullSelectQuery( db, resultset, query_str );
	if ( queryOk == true )
	{
		if ( resultset.GetRecordCount() > 0 && resultset.CanScroll() )
		{
			resultset.MoveFirst();
		}
		colcnt = GetRecordColumnTags( resultset, taglist );
	}

	return queryOk;
}

bool DBifImpl::GetFullSelectQueryWithColTags( CDatabase* db, CRecordset& resultset,
											  std::string querystr,
											  int32_t& colcnt, std::vector<std::string>& taglist )
{
	bool queryOk = false;

	queryOk = GetFullSelectQuery( db, resultset, querystr );
	if ( queryOk == true )
	{
		if ( resultset.GetRecordCount() > 0 && resultset.CanScroll() )
		{
			resultset.MoveFirst();
		}
		colcnt = GetRecordColumnTags( resultset, taglist );
	}

	return queryOk;
}

// select-type query executions; should NOT be used for insert, update, or remove actions!
bool DBifImpl::GetFullSelectQuery( CDatabase* db, CRecordset& resultset, std::string querystr )
{
	CString qStr = _TEXT( "" );
	CStringA qStr2 = "";
	BOOL query_Ok = FALSE;
	bool querySuccess = false;
	std::string logStr = "";
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

	qStr2 = querystr.c_str();
	qStr = CA2T( qStr2.GetString(), CP_UTF8 );

	if ( db != nullptr )
	{
		loginType = GetDBLoginConnection( db, DBApi::eLoginType::AnyLoginType );
		resultset.m_pDatabase = db;


		// TODO: wrap in try...catch block to prevent throwing if query failure...
		try
		{
			db->SetQueryTimeout( ReadQueryTimeout );

			//
			//	enum OpenType
			//	{
			//		dynaset,        // uses SQLExtendedFetch, keyset driven cursor
			//		snapshot,       // uses SQLExtendedFetch, static cursor
			//		forwardOnly,    // uses SQLFetch
			//		dynamic         // uses SQLExtendedFetch, dynamic cursor
			//	};
			//
			//	#define AFX_DB_USE_DEFAULT_TYPE     (0xFFFFFFFF)
			//
			//	enum OpenOptions
			//	{
			//		none = 0x0,
			//		readOnly = 0x0004,
			//		appendOnly = 0x0008,
			//		skipDeletedRecords = 0x0010,		// turn on skipping of deleted records, Will slow Move(n).
			//		noDirtyFieldCheck = 0x0020,			// disable automatic dirty field checking
			//		useBookmarks = 0x0100,				// turn on bookmark support
			//		useMultiRowFetch = 0x0200,			// turn on multi-row fetch model
			//		userAllocMultiRowBuffers = 0x0400,	// if multi-row fetch on, user will alloc memory for buffers
			//		useExtendedFetch = 0x0800,			// use SQLExtendedFetch with forwardOnly type recordsets
			//		executeDirect = 0x2000,				// Directly execute SQL rather than prepared execute
			//		optimizeBulkAdd = 0x4000,			// Use prepared HSTMT for multiple AddNews, dirty fields must not change.
			//		firstBulkAdd = 0x8000,				// INTERNAL to MFC, don't specify on Open.
			//	};
			//
			//	virtual BOOL Open(UINT nOpenType = AFX_DB_USE_DEFAULT_TYPE,
			//					  LPCTSTR lpszSQL = NULL, DWORD dwOptions = none);
			//

			query_Ok = resultset.Open( AFX_DB_USE_DEFAULT_TYPE, qStr );

			if ( query_Ok == TRUE )
			{
				querySuccess = true;
			}
			// TODO: check for timeout error
		}
		catch ( CDBException * pErr )
		{
			// Handle DB exceptions first...

			logStr = "Failed to execute FullSelect query request: ";

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

			logStr = "Failed to execute FullSelect query request: Unrecognized exception error.";

			e->Delete();
		}
	}
	else
	{
		logStr = "Failed to run select query: db object not initialized.";
	}

	std::string logQueryStr = "";

	if ( querystr.length() > 0 )
	{
		logQueryStr = boost::str( boost::format( "\tqueryStr: '%s'" ) % querystr );
		if ( qStr.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t    qStr: '%s'" ) % CT2A( qStr ) ) );
		}
#ifdef DEBUG
		if ( qStr2.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t   qStr2: '%s'" ) % qStr2.GetString() ) );
		}
		std::string dbgEntryStr = boost::str( boost::format( "GetFullSelectQuery:\n%s\n" ) % logQueryStr );
		WriteLogEntry( dbgEntryStr, InfoMsgType );
#endif
	}

	if ( logStr.length() > 0 )
	{
		std::string logEntryStr = boost::str( boost::format( "GetFullSelectQuery:\n\tlogstr: %s" ) % logStr );
		if ( logQueryStr.length() > 0 )
		{
			logEntryStr.append( logQueryStr );
		}
		WriteLogEntry( logEntryStr, QueryErrorMsgType );
	}

	db->SetQueryTimeout( DefaultQueryTimeout );

	return querySuccess;
}

// execute query for config info...
DBApi::eQueryResult DBifImpl::RunConfigQuery( DBApi::eLoginType usertype,
											  std::string querystr, std::vector<std::string>& infolist )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;
	CRecordset resultRecs;		// no default initializer; initialized by the method

	queryResult = RunQuery( usertype, querystr, resultRecs );

	if ( queryResult == DBApi::eQueryResult::QueryOk )
	{
		std::string temp, recordLine;
		CString valStr = _T( "" );
		int32_t idx = 0;
		int32_t colCnt = resultRecs.GetODBCFieldCount();

		recordLine.clear();

		if ( resultRecs.GetRecordCount() > 0 && resultRecs.CanScroll() )
		{
			resultRecs.MoveFirst();
		}

		while ( !resultRecs.IsEOF() )
		{
			recordLine.clear();
			for ( idx = 0; idx < colCnt; idx++ )
			{
				resultRecs.GetFieldValue( idx, valStr );
				temp = CT2A( valStr );
				if ( idx < ( colCnt - 1 ) )
				{
					temp += "|";
				}
				recordLine += temp;
			}
			infolist.push_back( recordLine );
			if ( resultRecs.CanScroll() )
			{
				resultRecs.MoveNext();
			}
			else
			{
				break;
			}
		}
	}

	if ( resultRecs.IsOpen() )
	{
		resultRecs.Close();
	}

	return queryResult;
}

// execute a full query
DBApi::eQueryResult DBifImpl::DoExecute( DBApi::eLoginType usertype, CDatabase* pDb,
										 std::string querystr, std::vector<std::string>& resultlist, bool gettags )
{
	if ( pDb == nullptr )
	{
		DBApi::eLoginType requestedType = usertype;
		DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

		if ( ( usertype == DBApi::eLoginType::BothUserLoginTypes ) ||
			 ( usertype == DBApi::eLoginType::InstPlusUserLoginTypes ) ||
			 ( usertype == DBApi::eLoginType::InstPlusAdminLoginTypes ) ||
			 ( usertype == DBApi::eLoginType::AllLoginTypes ) )
		{
			requestedType = DBApi::eLoginType::NoLogin;
		}

		if ( ( requestedType != DBApi::eLoginType::NoLogin ) && ( IsLoginType( requestedType ) ) )
		{
			loginType = GetDBLoginConnection( pDb, usertype );
		}

		if ( loginType == DBApi::eLoginType::NoLogin )
		{
			std::string logEntryStr = "DoExecute: Login failure";
			WriteLogEntry( logEntryStr, ErrorMsgType );

			return DBApi::eQueryResult::NoQuery;
		}
	}

	DBApi::eQueryResult result = DBApi::eQueryResult::QueryFailed;

	CRecordset recordset( pDb );

	resultlist.clear();

	result = RunQuery( usertype, querystr, recordset );

	if ( ( result == DBApi::eQueryResult::NoData ) ||
		 ( result == DBApi::eQueryResult::QueryOk ) )
	{
		std::vector<std::string> tagList = {};
		std::string tagStr = "";
		int32_t recCnt = recordset.GetRecordCount();
		int32_t colCnt = GetRecordColumnTags( recordset, tagList );

		if ( colCnt > 0 )
		{
			std::string recordLine, temp;

			if ( gettags )
			{
				std::string titleLine, spacerLine;
				size_t tagLen = 0;
				auto tagIter = tagList.begin();

				titleLine.clear();
				spacerLine.clear();

				for ( auto tagIter = tagList.begin(); tagIter != tagList.end();  )
				{
					tagStr = *tagIter;
					temp = tagStr;
					temp += "   ";          // add end-of-tag spacing... may not be desirable in true queries that use only the tags
					if ( ++tagIter != tagList.end() )
					{
						temp += "|   ";
					}
					titleLine += temp;
					tagLen = temp.length();
					for ( int32_t i = 0; i < tagLen; i++ )
					{
						spacerLine += "=";
					}
				}
				resultlist.push_back( titleLine );
				resultlist.push_back( spacerLine );
			}

			if ( recCnt > 0 )
			{
				int32_t idx = 0;
				CString valStr = _T( "" );

				recordLine.clear();

				if ( recordset.GetRecordCount() > 0 && recordset.CanScroll() )
				{
					recordset.MoveFirst();
				}

				while ( !recordset.IsEOF() )
				{
					recordLine.clear();
					for ( idx = 0; idx < colCnt; idx++ )
					{
						recordset.GetFieldValue( idx, valStr );
						temp = CT2A( valStr );
						if ( idx < ( colCnt - 1 ) )
						{
							temp += "|";
						}
						recordLine += temp;
					}
					resultlist.push_back( recordLine );
					if ( recordset.CanScroll() )
					{
						recordset.MoveNext();
					}
					else
					{
						break;
					}
				}
			}
		}
		else
		{
			if ( recordset.IsOpen() )
			{
				recordset.Close();
			}

			return DBApi::eQueryResult::NoResults;
		}

		if ( recCnt <= 0 )
		{
			result = DBApi::eQueryResult::NoData;
		}
	}

	if ( recordset.IsOpen() )
	{
		recordset.Close();
	}

	return result;
}

// execute query with checks for admin-level requests
DBApi::eQueryResult DBifImpl::RunQuery( DBApi::eLoginType usertype, std::string querystr, CRecordset& resultrecs )
{
	std::string logStr = "";
	std::string queryStr = querystr;
	CString qStr = TEXT("");
	CStringA qStr2 = "";
	bool queryOk = true;
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	uint32_t userTypeBits = static_cast< uint32_t >( usertype );
	uint32_t allUserTypeBits = static_cast< uint32_t >( DBApi::eLoginType::AllLoginTypes );

	// must select a login user type
	if ( ( userTypeBits & allUserTypeBits ) == 0 )
	{
		logStr = "Failed to execute RunQuery request: No user type selected.";
		queryResult = DBApi::eQueryResult::NotConnected;
		queryOk = false;
	}

	if ( queryOk && queryStr.empty() )
	{
		logStr = "Failed to execute RunQuery request: No query entered.";
		queryResult = DBApi::eQueryResult::NoQuery;
		queryOk = false;
	}

	std::vector<std::string> tokenList = {};
	int32_t tokenCnt = 0;

	if ( queryOk )
	{
		std::string sepChars = " ";
		char* pSepChars = (char*) sepChars.c_str();
		std::string sepStr = "";
		std::string trimChars = "";
		std::string parseStr = queryStr;

		tokenCnt = ParseStringToTokenList( tokenList, parseStr, sepStr, pSepChars, true, trimChars );

		if ( tokenCnt <= 0 )
		{
			logStr = "Failed to execute RunQuery request: Invalid query request.";
			queryResult = DBApi::eQueryResult::BadQuery;
			queryOk = false;
		}
	}

	if ( !queryOk )
	{
		std::string logEntryStr = boost::str( boost::format( "RunQuery:\n\tlogstr: %s" ) % logStr );
		if ( queryStr.length() > 0 )
		{
			logEntryStr.append( boost::str( boost::format( "\n\tquerystr: '%s'" ) % queryStr ) );
		}
		WriteLogEntry( logEntryStr, ErrorMsgType );
		return queryResult;
	}

	bool adminQuery = false;
	CDatabase* pDb = nullptr;
	DBApi::eLoginType chkType = DBApi::eLoginType::NoLogin;
	DBApi::eLoginType testType = DBApi::eLoginType::NoLogin;
	uint32_t testTypeBits = static_cast<uint32_t>( DBApi::eLoginType::AllLoginTypes );
	std::string cmdToken = "";

	if ( queryOk )
	{
		// get the first token; this will be the command type: retrieval(selection), data insertion/update, removal
		cmdToken = tokenList.at( 0 );

		adminQuery = IsAdminQuery( cmdToken );

		// must select a login user type
		if ( adminQuery )
		{
			testType = DBApi::eLoginType::AdminLoginType;
			chkType = testType;
			testTypeBits = static_cast<uint32_t>( testType );
		}
		else
		{
			testType = DBApi::eLoginType::AllLoginTypes;
			chkType = DBApi::eLoginType::AnyLoginType;
			testTypeBits = static_cast<uint32_t>( testType );
		}

		if ( ( userTypeBits & testTypeBits ) == 0 )
		{
			logStr = "Failed to execute RunQuery request: User type not compatible with required type.";
			queryResult = DBApi::eQueryResult::QueryFailed;
			queryOk = false;

			std::string logEntryStr = boost::str( boost::format( "RunQuery:\n\tlogstr: %s" ) % logStr );
			if ( queryStr.length() > 0 )
			{
				logEntryStr.append( boost::str( boost::format( "\n\tquerystr: '%s'" ) % queryStr ) );
			}
			WriteLogEntry( logEntryStr, ErrorMsgType );
			return queryResult;
		}
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

	if ( queryOk )
	{
		bool connected = IsLoginType( chkType );

		if ( !connected )
		{
			if ( adminQuery )
			{
				loginType = LoginAsAdmin();
			}
			else
			{
				loginType = LoginAsInstrument();
			}
		}
		loginType = GetDBLoginConnection( pDb, chkType );

		if ( adminQuery && loginType != DBApi::eLoginType::AdminLoginType )
		{
			logStr = "Failed to execute RunQuery request: failed to establish required connection.\n";
			queryResult = DBApi::eQueryResult::NotConnected;
			queryOk = false;
		}
		else if ( ( loginType == DBApi::eLoginType::NoLogin ) || ( pDb == nullptr ) )
		{
			logStr = "Failed to execute RunQuery request: No database connection.\n";
			queryResult = DBApi::eQueryResult::NotConnected;
			queryOk = false;
		}
	}

	if ( queryOk )
	{
		queryOk = false;

		if ( IsSelect( cmdToken ) )
		{   // this is a Select retrieval query command
			std::string schemaName = "";
			std::string tableName = "";
			std::string tokenStr = "";
			int32_t tokenIdx = 0;

			while ( tokenIdx < tokenCnt )
			{
				tokenStr = tokenList.at( tokenIdx );
				StrToLower( tokenStr );
				if ( tokenStr == FromStr )
				{
					schemaName = tokenList.at( tokenIdx );
					tableName = tokenList.at( tokenIdx + 1 );
					break;
				}
			}

			if ( ( !schemaName.empty() ) && ( !tableName.empty() ) )
			{
				queryStr.clear();
				queryStr = boost::str( boost::format( "%s * %s \"%s\".\"%s\"" ) % SelectStr % FromStr % schemaName % tableName );

				resultrecs.m_pDatabase = pDb;

				// get all requested records
				queryOk = GetFullSelectQuery( pDb, resultrecs, queryStr );

				if ( queryOk == true )
				{
					int32_t rec_cnt = resultrecs.GetRecordCount();

					if ( rec_cnt == 0 )
					{
						queryResult = DBApi::eQueryResult::NoResults;
					}
				}
				else
				{
					logStr = "Failed to execute RunQuery request: Failure executing GetFullSelecQuery.\n";
					queryResult = DBApi::eQueryResult::QueryFailed;
				}
			}
			else
			{
				logStr = "Failed to execute RunQuery request: No schema or table targets spccified in RunQuery.\n";
				queryResult = DBApi::eQueryResult::NoTargets;
			}
		}
		else
		{
			// the double conversion is required to properly convert Japanese and Chinese character strings to the CStringT (expands to CStringW) format
			qStr2 = queryStr.c_str();
			qStr = CA2T( qStr2.GetString(), CP_UTF8 );

			try
			{
				// insert into the database or update/removal of the data in the database
				pDb->ExecuteSQL( qStr );		// No indication of failure other than exception?
				queryOk = true;
			}
			catch ( CDBException * pErr )
			{
				// Handle DB exceptions first...

				logStr = "Failed to execute RunQuery request: ";

				if ( pErr->m_nRetCode == SQL_ERROR )
				{
					logStr.append( "SQL Error: exception msg: " );
					logStr.append( CT2A( pErr->m_strError ) );		// do not add a newline; the error message does that
				}
				else
				{
					logStr.append( "Unrecognized Error: exception msg: " );
					logStr.append( CT2A( pErr->m_strError ) );		// do not add a newline; the error message does that
				}

				pErr->Delete();
			}
			catch ( CException * e )
			{
				// Handle all other types of exceptions here.
				logStr = "Failed to execute RunQuery request: Unrecognized exception error.\n";
				e->Delete();
			}
		}
	}

	if ( queryOk == true )
	{
		queryResult = DBApi::eQueryResult::QueryOk;
	}

	std::string logQueryStr = "";

	if ( queryStr.length() > 0 )
	{
		logQueryStr = boost::str( boost::format(   "\tqueryStr: '%s'" ) % queryStr );
		if ( qStr.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t    qStr: '%s'" ) % CT2A(qStr) ) );
		}
#ifdef DEBUG
		if ( qStr2.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t   qStr2: '%s'" ) % qStr2.GetString() ) );
		}
		std::string dbgEntryStr = boost::str( boost::format( "RunQuery:\n%s\n" ) % logQueryStr);
		WriteLogEntry( dbgEntryStr, InfoMsgType );
#endif
	}

	if ( logStr.length() > 0 )	// indicates an error has occurred...
	{
		std::string logEntryStr = boost::str( boost::format( "RunQuery:\n\tlogstr: %s" ) % logStr );
		if ( logQueryStr.length() > 0 )
		{
			logEntryStr.append( logQueryStr );
		}
		WriteLogEntry( logEntryStr, QueryErrorMsgType );
	}

	loginType = SetLoginType( DBApi::eLoginType::InstrumentLoginType );		// restore standard connection type

	return queryResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// object and list data retrieval execution methods
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DBApi::eQueryResult DBifImpl::DoListQuery( std::string schemaname, std::string tablename, CRecordset& resultrec,
										   std::vector<std::string>& taglist, DBApi::eListType listtype,
										   DBApi::eListSortCriteria primarysort, DBApi::eListSortCriteria secondarysort,
										   std::string orderstring, std::string wherestr, std::string idnumtag,
										   int32_t sortdir, int32_t limitcnt, int32_t startindex, int64_t startidnum, std::string custom_query )
{
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::NotConnected;
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	if ( !IsLoginType( DBApi::eLoginType::AnyLoginType ) )
	{
		loginType = LoginAsInstrument();
	}

	loginType = GetDBLoginConnection( pDb, DBApi::eLoginType::AnyLoginType );
	if ( ( loginType == DBApi::eLoginType::NoLogin ) || ( pDb == nullptr ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	resultrec.m_pDatabase = pDb;

	bool queryOk = false;
	std::string queryStr = "";

	queryResult = DBApi::eQueryResult::QueryFailed;

	if ( custom_query.length() > 0 )
	{
		queryStr = custom_query;
	}
	else
	{
		queryStr = boost::str( boost::format( "%s * %s \"%s\".\"%s\"" ) % SelectStr % FromStr % schemaname % tablename );

		if ( wherestr.length() > 0 )
		{
			queryStr.append( boost::str( boost::format( " %s " ) % WhereStr ) );
		}

		if ( startidnum > 0 )
		{
			if ( wherestr.length() > 0 )
			{
				wherestr.append( boost::str( boost::format( " %s " ) % AndStr ) );
			}
			else
			{
				wherestr.append( boost::str( boost::format( " %s " ) % WhereStr ) );
			}
			wherestr.append( boost::str( boost::format( "\"%s\" %s %lld" ) % idnumtag % ( ( sortdir < 0 ) ? "<" : ">" ) % startidnum ) );
		}
		queryStr.append( wherestr );
	}

	GetListSortString( listtype, orderstring, primarysort, secondarysort, sortdir );
	if ( orderstring.length() > 0 )
	{
		queryStr.append( orderstring );
	}

	if ( limitcnt > NO_SQL_QUERY_LIMIT )
	{
		queryStr.append( boost::str( boost::format( " %s %d" ) % LimitStr % limitcnt ) );
	}

	queryOk = GetFullSelectQuery( pDb, resultrec, queryStr );
	if ( queryOk == true )
	{
		int32_t rec_cnt = resultrec.GetRecordCount();

		if ( rec_cnt <= 0 )
		{
			queryResult = DBApi::eQueryResult::NoResults;
		}
		else
		{
			queryResult = DBApi::eQueryResult::QueryOk;

			GetRecordColumnTags( resultrec, taglist );

			if ( resultrec.GetRecordCount() > 0 && resultrec.CanScroll() )
			{
				resultrec.MoveFirst();
			}
		}
	}

	return queryResult;
}

DBApi::eQueryResult DBifImpl::DoObjectQuery( std::string schemaname, std::string tablename, CRecordset& resultrec,
											 std::vector<std::string>& taglist, std::string selecttag,
											 uuid__t objuuid, int64_t objidnum, int64_t objindex, std::string objname,
											 std::string wherestr, std::string custom_query )
{
	if ( !GuidValid( objuuid ) && objidnum < 0 && objindex < 0 && objname.length() <= 0 )
	{	// NO identifiers specified...
		std::string logStr = boost::str( boost::format( "Invalid query paremeters: no identifiers supplied: uuid: '%s'  idnum: %lld  index: %lld  name: '%s'" ) % DBEmptyUuidStr % objidnum % objindex % objname );
		WriteLogEntry( logStr, ErrorMsgType );
		return DBApi::eQueryResult::BadQuery;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	if ( !IsLoginType( DBApi::eLoginType::AnyLoginType ) )
	{
		loginType = LoginAsInstrument();
	}

	loginType = GetDBLoginConnection( pDb, DBApi::eLoginType::AnyLoginType );
	if ( ( loginType == DBApi::eLoginType::NoLogin ) || ( pDb == nullptr ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}
	resultrec.m_pDatabase = pDb;

	bool queryOk = false;
	std::string queryStr = "";

	if ( custom_query.length() > 0 )
	{
		queryStr = custom_query;
	}
	else
	{
		queryStr = boost::str( boost::format( "%s * %s \"%s\".\"%s\" %s \"%s\" = " ) % SelectStr % FromStr % schemaname % tablename % WhereStr % selecttag );
	}

	if ( GuidValid( objuuid ) )
	{
		std::string uuidStr = "";
		FormatGuidString( objuuid, uuidStr );
		queryStr.append( boost::str( boost::format( "'%s'" ) % uuidStr ) );
	}
	else if ( objidnum >= 0 )
	{
		queryStr.append( boost::str( boost::format( "%lld" ) % objidnum ) );
	}
	else if ( objindex >= 0 )
	{
		queryStr.append( boost::str( boost::format( "%ld" ) % (int32_t)objindex ) );		// must cast down to signed 32-bit for proper retrieval.
	}
	else if ( objname.length() > 0 )
	{
		queryStr.append( boost::str( boost::format( "'%s'" ) % objname ) );
	}

	if ( wherestr.length() > 0 )
	{
		queryStr.append( " AND " );
		queryStr.append( wherestr );
	}

	queryOk = GetFullSelectQuery( pDb, resultrec, queryStr );

	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	if ( queryOk == true )
	{
		int32_t rec_cnt = resultrec.GetRecordCount();

		if ( rec_cnt == 0 )
		{
			queryResult = DBApi::eQueryResult::NoResults;
		}
		else if ( rec_cnt > 1 && objname.length() == 0 )    // allow multiple records for a search on 'name'; will have to define the record later
		{
			queryResult = DBApi::eQueryResult::BadQuery;
			std::string logStr = boost::str( boost::format( "Multiple matching records found in single record search without object name:  rec cnt: %d" ) % rec_cnt );
			WriteLogEntry( logStr, ErrorMsgType );
		}
		else
		{
			queryResult = DBApi::eQueryResult::QueryOk;

			GetRecordColumnTags( resultrec, taglist );

			if ( resultrec.GetRecordCount() > 0 && resultrec.CanScroll() )
			{
				resultrec.MoveFirst();
			}

			if ( rec_cnt > 1 && objname.length() > 0 )    // allow multiple records for a search on 'name'; will have to define the record later
			{
				queryResult = DBApi::eQueryResult::QueryOk;
			}
		}
	}

	return queryResult;
}

bool DBifImpl::DoDbRecreate( std::string dbname )
{
//	const char		DBNameStr[] = "ViCellDB";			// default database name for the instrument; the base of instrument database names
//	const char		DBDefaultUser[] = "postgres";		// default installation superuser name and maintenance database name
//	const char		DropStr[] = "drop";					// drop command token
//	const char		CreateStr[] = "create";				// create comand token
//	const char		DBtoken[] = "database";				// database token
//	const char		DBTemplateToken[] = "template";		// template token; used for construction of other DB names 

	std::string tgtDbName = strDBName;
	std::string queryStr = "";
	std::string queryFmtStr = DropStr;															// "drop"
	std::string pgPwd = "$3";
	CString qStr = _TEXT( "" );
	CStringA qStr2 = "";

	if ( dbname.length() > 0 )
	{
		tgtDbName = dbname;
	}

	//  logout all user-type connections and close the connections to the DB
	DoLogoutAll();

	queryFmtStr.append( boost::str( boost::format( " %s" ) % DBtoken ) );						// "drop database"
	pgPwd.append( "rgt" );

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = &adminDb;
	std::string tgtDbNameToken = boost::str( boost::format( " \"%s\"" ) % tgtDbName );			// formatted for inclusion in the drop and create statements

	queryFmtStr.append( boost::str( boost::format( " \"%s\"" ) % tgtDbName ) );					// "drop database \"ViCellDB\"" - will be the currently connected DB or the supplied name

	loginType = LoginAsAdmin();

	queryStr = queryFmtStr;			// save for possible logging statement
	qStr2 = queryFmtStr.c_str();
	qStr = CA2T( qStr2.GetString(), CP_UTF8 );

	// start to format the NEXT query
	queryFmtStr = CreateStr;																	// "create"

	DoDisconnect( pDb );

	pgPwd.append( "$0P" );

	bool queryOk = false;
	std::string logStr = "";
	std::string dbNameSave = strDBName;

	strDBName = DBDefaultUser;									// working database name string now "postgres"

	queryFmtStr.append( boost::str( boost::format( " %s" ) % DBtoken ) );						// "create database"

	queryOk = DoConnect( pDb, strDBName, pgPwd );				// connect using username "postgres", and password for the postgres user
	if ( !queryOk )
	{
		queryOk = DoConnect( pDb, strDBName, "postgres" );		// if not using the obfuscated password, try the old standard password...
	}
	pgPwd.clear();

	if ( queryOk )
	{
		// continue to format the NEXT query...
		queryFmtStr.append( boost::str( boost::format( " \"%s\"" ) % tgtDbName ) );				// "create database \"ViCellDB\""

		queryOk = false;
		try
		{
			// insert into the database or update/removal of the data in the database
			pDb->ExecuteSQL( qStr );	// No indication of failure other than exception?
			queryOk = true;
		}
		catch ( CDBException * pErr )
		{
			// Handle DB exceptions first...
			logStr = "Failed to execute RunQuery request: ";

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
			logStr = "Failed to execute RunQuery request: Unrecognized exception error.\n";

			e->Delete();
		}
	}
	else
	{
		logStr = "Failed to login to maintenance DB using expected name and password\n";
		queryStr.clear();		// didn't actually execute the query, so don't include in logging
	}

	if ( queryOk )
	{
		// continue to format the NEXT query...
		queryFmtStr.append( boost::str( boost::format( " with %s =" ) % DBTemplateToken ) );	// "create database \"ViCellDB\" with template ="

		queryOk = false;

		queryFmtStr.append( boost::str( boost::format( " \"%s_%s\"" ) % DBNameStr % DBTemplateToken ) );	// "create database \"ViCellDB\" with template = \"ViCellDB_template\""

		queryStr = queryFmtStr;			// save for possible logging statement
		qStr2 = queryFmtStr.c_str();
		qStr = CA2T( qStr2.GetString(), CP_UTF8 );

		try
		{
			// insert into the database or update/removal of the data in the database
			pDb->ExecuteSQL( qStr );	// No indication of failure other than exception?
			queryOk = true;
		}
		catch ( CDBException * pErr )
		{
			// Handle DB exceptions first...

			logStr = "Failed to execute RunQuery request: ";

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

			logStr = "Failed to execute RunQuery request: Unrecognized exception error.\n";

			e->Delete();
		}
	}

	strDBName = dbNameSave ;

	if ( queryOk == true )
	{
		//  logout all user-type connections and close the connections to the DB
		DoLogoutAll();

		loginType = LoginAsAdmin();

		loginType = GetDBLoginConnection( pDb, DBApi::eLoginType::AdminLoginType );
		if ( ( loginType == DBApi::eLoginType::NoLogin ) || ( pDb == nullptr ) )
		{
			queryOk = false;
		}
	}

	std::string logQueryStr = "";

	if ( queryStr.length() > 0 )
	{
		logQueryStr = boost::str( boost::format( "\tqueryStr: '%s'" ) % queryStr );
		if ( qStr.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t    qStr: '%s'" ) % CT2A( qStr ) ) );
		}
#ifdef DEBUG
		if ( qStr2.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t   qStr2: '%s'" ) % qStr2.GetString() ) );
		}
		std::string dbgEntryStr = boost::str( boost::format( "DoDbRecreate:\n%s\n" ) % logQueryStr );
		WriteLogEntry( dbgEntryStr, InfoMsgType );
#endif
	}

	if ( logStr.length() > 0 )
	{
		std::string logEntryStr = boost::str( boost::format( "DoDbRecreate:\n\tlogstr: %s" ) % logStr );
		if ( logQueryStr.length() > 0 )
		{
			logEntryStr.append( logQueryStr );
		}
		WriteLogEntry( logEntryStr, QueryErrorMsgType );
	}

	return queryOk;
}

DBApi::eQueryResult DBifImpl::SetDbBackupUserPwd( std::string & pwdstr )
{
	std::string queryStr = "";
	std::string queryFmtStr = AlterStr;													// "alter"
	std::string roleStr = "role";
	std::string backupUserName = "Db";

	if ( pwdstr.empty() )
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	queryFmtStr.append( boost::str( boost::format( " %s" ) % roleStr ) );				// "alter role"

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	if ( !IsLoginType( DBApi::eLoginType::AdminLoginType ) )
	{
		loginType = LoginAsAdmin();
	}

	backupUserName.append( "Backup" );

	loginType = GetDBLoginConnection( pDb, DBApi::eLoginType::AdminLoginType );
	if ( ( loginType == DBApi::eLoginType::NoLogin ) || ( pDb == nullptr ) )
	{
		return DBApi::eQueryResult::NotConnected;
	}

	backupUserName.append( "User" );

	queryFmtStr.append( boost::str( boost::format( " \"%s\"" ) % backupUserName ) );		// "alter role DbBackupUser"
	backupUserName.clear();

//	full role definition statement; do not use for dynamic password changes
//		ALTER ROLE "DbBackupUser" WITH NOSUPERUSER INHERIT NOCREATEROLE NOCREATEDB LOGIN REPLICATION NOBYPASSRLS PASSWORD 'xxxxxxxxx';
//	limited role definition statement for pasword change; note password is a quoted string
//		ALTER ROLE "DbBackupUser" WITH ENCRYPTED PASSWORD 'xxxxxxxxx';

	queryFmtStr.append( boost::str( boost::format( " %s" ) % "with" ) );				// "alter role DbBackupUser with"

	queryFmtStr.append( boost::str( boost::format( " %s" ) % "encrypted" ) );			// "alter role DbBackupUser with encrypted"

	CString logQStr = _TEXT( "" );
	CString qStr = _TEXT( "" );
	CStringA qStr2 = "";

	queryFmtStr.append( boost::str( boost::format( " %s" ) % "password" ) );			// "alter role DbBackupUser with encrypted password"
	queryStr = queryFmtStr;																// save for possible logging statement, but don't include password string
	qStr2 = queryStr.c_str();
	logQStr = CA2T( qStr2.GetString(), CP_UTF8 );										// create a log qstr that omits the password...

	queryFmtStr.append( boost::str( boost::format( " '%s';" ) % pwdstr ) );				// "alter role DbBackupUser with encrypted password xxxxxxxx"
	pwdstr.clear();

	std::string logStr = "";
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	qStr2 = queryFmtStr.c_str();
	qStr = CA2T( qStr2.GetString(), CP_UTF8 );
	queryFmtStr.clear();

	try
	{
		// execute role modification query; roles are global to the PostgreSQL instance and not specific to any database or schema
		pDb->ExecuteSQL( qStr );														// No indication of failure other than exception?
		queryResult = DBApi::eQueryResult::QueryOk;
	}
	catch ( CDBException * pErr )
	{
		// Handle DB exceptions first...
		logStr = "Failed to execute Query request: ";

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

		logStr = "Failed to execute Query request: Unrecognized exception error.\n";

		e->Delete();
	}

	if ( pDb != nullptr )		// an admin connection was present; disconnect it
	{
		DoDisconnect( pDb );
	}

	std::string logQueryStr = "";

	if ( queryStr.length() > 0 )
	{
		logQueryStr = boost::str( boost::format( "\tqueryStr: '%s'" ) % queryStr );
		if ( qStr.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t    qStr: '%s'" ) % CT2A( logQStr ) ) );
		}
#ifdef DEBUG
		// NOTE: when debugging, the password will be written to the log in qStr2.  DO NOT include this in release mode logic...
		if ( qStr2.GetLength() > 0 )
		{
			logQueryStr.append( boost::str( boost::format( "\n\t   qStr2: '%s'" ) % qStr2.GetString() ) );
		}
		std::string dbgEntryStr = boost::str( boost::format( "SetDbBackupUserPwd:\n%s\n" ) % logQueryStr );
		WriteLogEntry( dbgEntryStr, InfoMsgType );
#endif
	}

	if ( logStr.length() > 0 )
	{
		std::string logEntryStr = boost::str( boost::format( "SetDbBackupUserPwd:\n\tlogstr: %s" ) % logStr );
		if ( logQueryStr.length() > 0 )
		{
			logEntryStr.append( logQueryStr );
		}
		WriteLogEntry( logEntryStr, QueryErrorMsgType );
	}

	queryStr.clear();
	backupUserName.clear();
	qStr2 = "";
	qStr = _TEXT( "" );
	logQStr = _TEXT( "" );

	return queryResult;
}


DBApi::eQueryResult DBifImpl::DbTruncateTableContents(std::list<std::pair<std::string, std::string>> tableNames_name_schema)
{
	if (tableNames_name_schema.empty())
	{
		return DBApi::eQueryResult::MissingQueryKey;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CDatabase* pDb = nullptr;

	if (!IsLoginType(DBApi::eLoginType::AdminLoginType))
	{
		loginType = LoginAsAdmin();
	}

	loginType = GetDBLoginConnection(pDb, DBApi::eLoginType::AdminLoginType);
	if ((loginType == DBApi::eLoginType::NoLogin) || (pDb == nullptr))
	{
		return DBApi::eQueryResult::NotConnected;
	}

	CString logQStr = _TEXT("");
	CString qStr = _TEXT("");
	CStringA qStr2 = "";
	std::string queryStr;
	std::string queryFmtStr = TruncateTableStr;

	bool first = true;
	for (auto tbl : tableNames_name_schema)
	{
		if (!first)
		{
			queryFmtStr.append(", ");
		}

		queryFmtStr.append(boost::str(boost::format(" \"%s\".\"%s\"") % tbl.second % tbl.first));
		first = false;
	}

	queryStr = queryFmtStr;												// save for possible logging statement.
	qStr2 = queryStr.c_str();
	logQStr = CA2T(qStr2.GetString(), CP_UTF8);

	qStr2 = queryFmtStr.c_str();
	qStr = CA2T(qStr2.GetString(), CP_UTF8);

	std::string logStr;
	DBApi::eQueryResult queryResult = DBApi::eQueryResult::QueryFailed;

	try
	{
		// execute role modification query; roles are global to the PostgreSQL instance and not specific to any database or schema
		pDb->ExecuteSQL(qStr);											// No indication of failure other than exception?
		queryResult = DBApi::eQueryResult::QueryOk;
	}
	catch (CDBException* pErr)
	{
		// Handle DB exceptions first...
		logStr = "Failed to execute Query request: ";

		if (pErr->m_nRetCode == SQL_ERROR)
		{
			logStr.append("SQL Error: exception msg: ");
			logStr.append(CT2A(pErr->m_strError));
		}
		else
		{
			logStr.append("Unrecognized Error: exception msg: ");
			logStr.append(CT2A(pErr->m_strError));
		}

		pErr->Delete();
	}
	catch (CException* e)
	{
		// Handle all other types of exceptions here.

		logStr = "Failed to execute Query request: Unrecognized exception error.\n";

		e->Delete();
	}

	if (pDb != nullptr)		// an admin connection was present; disconnect it
	{
		DoDisconnect(pDb);
	}

	std::string logQueryStr;

	if (queryStr.length() > 0)
	{
		logQueryStr = boost::str(boost::format("\tqueryStr: '%s'") % queryStr);
		if (qStr.GetLength() > 0)
		{
			logQueryStr.append(boost::str(boost::format("\n\t    qStr: '%s'") % CT2A(logQStr)));
		}
#ifdef DEBUG
		if (qStr2.GetLength() > 0)
		{
			logQueryStr.append(boost::str(boost::format("\n\t   qStr2: '%s'") % qStr2.GetString()));
		}
		std::string dbgEntryStr = boost::str(boost::format("DbTruncateTableContents:\n%s\n") % logQueryStr);
		WriteLogEntry(dbgEntryStr, InfoMsgType);
#endif
	}

	if (logStr.length() > 0)
	{
		std::string logEntryStr = boost::str(boost::format("DbTruncateTableContents:\n\tlogstr: %s") % logStr);
		if (logQueryStr.length() > 0)
		{
			logEntryStr.append(logQueryStr);
		}
		WriteLogEntry(logEntryStr, QueryErrorMsgType);
	}

	return queryResult;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Publicly visible methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// execute query
DBApi::eQueryResult DBifImpl::ExecuteQuery( std::string query_str, std::vector<std::string>& resultList )
{
	DBApi::eQueryResult result = DBApi::eQueryResult::QueryFailed;
	CDatabase* pDb = nullptr;

	result = DoExecute( DBApi::eLoginType::AnyLoginType, pDb, query_str, resultList );

	return result;
}

// execute query; determine appropriate user type automatically
DBApi::eQueryResult DBifImpl::ExecuteQueryType( DBApi::eLoginType usertype, std::string query_str, std::vector<std::string>& resultlist )
{
	DBApi::eQueryResult result = DBApi::eQueryResult::NotConnected;

	if ( ( usertype != DBApi::eLoginType::NoLogin ) && ( IsLoginType( usertype ) ) )
	{
		CDatabase* pDb = nullptr;

		result = DoExecute( usertype, pDb, query_str, resultlist );
	}

	return result;
}

