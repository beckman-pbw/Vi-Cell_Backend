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



static const std::string MODULENAME = "DBif_Connect";



////////////////////////////////////////////////////////////////////////////////
// Internal connect methods
////////////////////////////////////////////////////////////////////////////////

// connect to database engine using the information supplied
bool DBifImpl::DoConnect( CDatabase* pDB, std::string username, std::string password )
{
	if ( pDB == nullptr )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::error, "Failed to connect to the database." );
#endif
		return false;
	}

	bool connectOk = false;
	CString strConnection = _T( "" );
	std::string connectionStr = boost::str( boost::format( "Driver=%s;Database=%s;Server=%s;Port=%s;" ) % strDBDriver % strDBName % strDBAddr % strDBPort );
	std::string logStr = "";
#ifdef USE_LOGGER
	severity_level logLevel = severity_level::error;
#endif

	// should not REQUIRE a valid login to connect, but needs to be checked
	if ( username.length() > 0 || password.length() > 0 )
	{
		connectionStr.append( boost::str( boost::format( "Uid=%s;Pwd=%s;" ) % username % password ) );
	}

	strConnection = connectionStr.c_str();

	try
	{
		pDB->SetLoginTimeout( LoginTimeout );

		//	enum DbOpenOptions
		//	{
		//		openExclusive = 0x0001,		// Not implemented
		//		openReadOnly = 0x0002,		// Open database read only
		//		useCursorLib = 0x0004,		// Use ODBC cursor lib
		//		noOdbcDialog = 0x0008,		// Don't display ODBC Connect dialog
		//		forceOdbcDialog = 0x0010,	// Always display ODBC connect dialog
		//	};

		// CDatabase::UseCursorLib = use (PostgreSQL) ODBC Driver
		if ( pDB->OpenEx( strConnection, CDatabase::noOdbcDialog ) )
		{
			logStr = "Successfully connected to the database.";
			connectOk = true;
			activeConnection = connectionStr;
#ifdef USE_LOGGER
			logLevel = severity_level::debug1;
#endif
		}
		else
		{
			// TODO: check for timeout error
			logStr = "Failed to connect to the database.";
		}
	}
	catch ( CDBException * pErr )
	{
		// Handle DB exceptions first...

		logStr = "Failed to connect to the database: ";

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

		std::string logStr = "Failed to connect to the database: Unrecognized exception error.";

		e->Delete();
	}

#ifdef USE_LOGGER
	if ( logStr.length() > 0 )
	{
		Logger::L().Log( MODULENAME, logLevel, logStr ));
	}
#endif

	return connectOk;
}

// disconnect from a database
void DBifImpl::DoDisconnect( CDatabase* pdb )
{
	if ( ( pdb != nullptr ) && ( pdb->IsOpen() ) )
	{
		pdb->Close();
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, "Disconnected from the database successfully." );
#endif
	}
}

// logout all user types and disconnect from a database
void DBifImpl::DoLogoutAll( void )
{
	if ( adminDb.IsOpen() )
	{
		DoDisconnect( &adminDb );
	}

	if ( userDb.IsOpen() )
	{
		DoDisconnect( &userDb );
	}

	if ( instrumentDb.IsOpen() )
	{
		DoDisconnect( &instrumentDb );
	}

	activeConnection.clear();
}

// do the disconnect from database for the login type specified by the usertype;
// only accepts single logout types (no combination types allowed)
// returns 'true' if a disconnect was performed successfully;
// returns false if failure to disconnect requested type
bool DBifImpl::DoLogoutType( DBApi::eLoginType logouttype )
{
	std::string logStr = "";

	switch ( logouttype )
	{
		case DBApi::eLoginType::AdminLoginType:
		case DBApi::eLoginType::UserLoginType:
		case DBApi::eLoginType::InstrumentLoginType:
			break;

		case DBApi::eLoginType::NoLogin:
			logStr = "DoLogoutType: no logout type requested.";
			break;

		default:
			logStr = "DoLogoutType: logout type not allowed.";
			break;
	}

	if ( logStr.length() > 0 )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, logStr );
#endif
		return false;
	}

	// check connection for requested user type
	if ( !IsLoginType( logouttype ) )
	{
		logStr = "DoLogoutType: no database login for specified type.";
	}

	if ( logStr.length() > 0 )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, logStr );
#endif
		return true;
	}

	bool disconnected = true;
	CDatabase* pDb = nullptr;

	// if there is a login of this type, setup for the disconnect which only handles one type at a time
	if ( GetDBLoginConnection( pDb, logouttype ) == logouttype )
	{
		disconnected = false;
		if ( pDb != nullptr )		// a connection was present; disconnect it
		{
			DoDisconnect( pDb );
			disconnected = true;
		}
	}

	if ( disconnected )
	{
		logStr = "DoLogoutType: logged out of the database successfully.";
	}

#ifdef USE_LOGGER
	if ( logStr.length() > 0 )
	{
		Logger::L().Log( MODULENAME, severity_level::debug1, logStr ));
	}
#endif

	return disconnected;
}

/// sets the login and connection type to use
/// returns NoLogin if type is not yet connected or the 'Both' or 'All' or either of the 'InstPlus...' types are requested;
/// returns the requested login type on success
DBApi::eLoginType DBifImpl::GetDBLoginConnection( CDatabase*& pdb, DBApi::eLoginType usertype )
{
	pdb = nullptr;

	switch ( usertype )
	{
		case DBApi::eLoginType::BothUserLoginTypes:
		case DBApi::eLoginType::InstPlusUserLoginTypes:
		case DBApi::eLoginType::InstPlusAdminLoginTypes:
		case DBApi::eLoginType::AllLoginTypes:
			// don't allow 'both user', 'instrument + user', 'instrument + admin', or 'all' to be requested; handle same as not connected
			return DBApi::eLoginType::NoLogin;
	}

	if ( !IsLoginType( usertype ) )
	{
		return DBApi::eLoginType::NoLogin;
	}

	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

	// just set the connection and login type to match expected
	switch ( usertype )
	{
		case DBApi::eLoginType::InstrumentLoginType:
			pdb = &instrumentDb;
			loginType = usertype;
			break;

		case DBApi::eLoginType::UserLoginType:
			pdb = &userDb;
			loginType = usertype;
			break;

		case DBApi::eLoginType::AdminLoginType:
			pdb = &adminDb;
			loginType = usertype;
			break;

		case DBApi::eLoginType::EitherUserLoginType:						// default to standard user connection if 'either user' is requested...
			if ( IsLoginType( DBApi::eLoginType::UserLoginType ) )
			{
				pdb = &userDb;
				loginType = DBApi::eLoginType::UserLoginType;
				break;
			}
			else if ( IsLoginType( DBApi::eLoginType::AdminLoginType ) )
			{
				pdb = &adminDb;
				loginType = DBApi::eLoginType::AdminLoginType;
				break;
			}
			break;

		case DBApi::eLoginType::InstOrUserLoginTypes:						// default to standard instrument connection if 'any' is requested...
			if ( IsLoginType( DBApi::eLoginType::InstrumentLoginType ) )
			{
				pdb = &instrumentDb;
				loginType = DBApi::eLoginType::InstrumentLoginType;
				break;
			}
			else if ( IsLoginType( DBApi::eLoginType::UserLoginType ) )
			{
				pdb = &userDb;
				loginType = DBApi::eLoginType::UserLoginType;
				break;
			}
			break;

		case DBApi::eLoginType::InstOrAdminLoginTypes:						// default to standard instrument connection if 'InstOrAdmin' is requested...
			if ( IsLoginType( DBApi::eLoginType::InstrumentLoginType ) )
			{
				pdb = &instrumentDb;
				loginType = DBApi::eLoginType::InstrumentLoginType;
				break;
			}
			else if ( IsLoginType( DBApi::eLoginType::AdminLoginType ) )
			{
				pdb = &adminDb;
				loginType = DBApi::eLoginType::AdminLoginType;
				break;
			}
			break;

		case DBApi::eLoginType::AnyLoginType:								// default to standard instrument connection if 'any' is requested...
			if ( IsLoginType( DBApi::eLoginType::InstrumentLoginType ) )
			{
				pdb = &instrumentDb;
				loginType = DBApi::eLoginType::InstrumentLoginType;
				break;
			}
			else if ( IsLoginType( DBApi::eLoginType::UserLoginType ) )
			{
				pdb = &userDb;
				loginType = DBApi::eLoginType::UserLoginType;
				break;
			}
			else if ( IsLoginType( DBApi::eLoginType::AdminLoginType ) )
			{
				pdb = &adminDb;
				loginType = DBApi::eLoginType::AdminLoginType;
				break;
			}

		default:	// don't know what user type is requested;
			break;
	}

	return loginType;
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Publicly visible methods
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// database connection methods
////////////////////////////////////////////////////////////////////////////////

// check for any db connection
bool DBifImpl::IsDBPresent( void )
{
	DBApi::eLoginType loginType;

	loginType = LoginAsInstrument();

	return ( IsLoginType( DBApi::eLoginType::AnyLoginType ) );
}

void DBifImpl::Disconnect( void )
{
	DoLogoutAll();
}

// check connection for user type
bool DBifImpl::IsLoginType( DBApi::eLoginType usertype )
{
	switch ( usertype )
	{
		// requests for multi-type conditions are not allowed; will return 'not connected' state;
		case DBApi::eLoginType::BothUserLoginTypes:
		case DBApi::eLoginType::InstPlusUserLoginTypes:
		case DBApi::eLoginType::InstPlusAdminLoginTypes:
		case DBApi::eLoginType::AllLoginTypes:
		case DBApi::eLoginType::NoLogin:
			return false;
	}

	bool isConnected = false;

	// GetLoginType will never return 'EitherUserLogin' or 'AnyLogin'
	// requested check type should not be a multiple type
	DBApi::eLoginType loginType = GetLoginType();

	uint32_t loginTypeBits = static_cast<uint32_t>( loginType );
	uint32_t userTypeBits = static_cast<uint32_t>( usertype );

	switch ( usertype )
	{
		case DBApi::eLoginType::InstrumentLoginType:
			isConnected = ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::InstrumentLoginType )) ? true : false;
			break;

		case DBApi::eLoginType::UserLoginType:
			isConnected = ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::UserLoginType )) ? true : false;
			break;

		case DBApi::eLoginType::AdminLoginType:
			isConnected = ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::AdminLoginType )) ? true : false;
			break;

		case DBApi::eLoginType::EitherUserLoginType:
			isConnected = ( ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::BothUserLoginTypes )) != 0 ) ? true : false;
			break;

		case DBApi::eLoginType::InstOrUserLoginTypes:
			isConnected = ( ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::InstPlusUserLoginTypes )) != 0 ) ? true : false;
			break;

		case DBApi::eLoginType::InstOrAdminLoginTypes:
			isConnected = ( ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::InstPlusAdminLoginTypes )) != 0 ) ? true : false;
			break;

		case DBApi::eLoginType::AnyLoginType:
			isConnected = ( ( loginTypeBits & static_cast< uint32_t >( DBApi::eLoginType::AllLoginTypes )) != 0 ) ? true : false;
			break;

		default:
			break;
	}

	return isConnected;
}

// check connection for user type
DBApi::eLoginType DBifImpl::GetLoginType( void )
{
	uint32_t loginBits = static_cast< uint32_t >( DBApi::eLoginType::NoLogin );
	bool isConnected = false;
	BOOL instConnected = FALSE;
	BOOL userConnected = FALSE;
	BOOL adminConnected = FALSE;

	instConnected = instrumentDb.IsOpen();
	userConnected = userDb.IsOpen();
	adminConnected = adminDb.IsOpen();

	if ( adminConnected == TRUE )
	{
		loginBits |= static_cast< uint32_t >( DBApi::eLoginType::AdminLoginType );
	}

	if ( userConnected == TRUE )
	{
		loginBits |= static_cast< uint32_t >( DBApi::eLoginType::UserLoginType );
	}

	if ( instConnected == TRUE )
	{
		loginBits |= static_cast< uint32_t >( DBApi::eLoginType::InstrumentLoginType );
	}

	return static_cast< DBApi::eLoginType >( loginBits );
}

// sets the connection type to use
// returns NoConnection if type is not yet connected or the 'Both' type is requested;
// returns the requested connection login type on success
DBApi::eLoginType DBifImpl::SetLoginType( DBApi::eLoginType usertype )
{
	return GetDBLoginConnection( pActiveDb, usertype );
}

// connect to database as standard user; construct the username and password for the standard user
//
// NOTE: this method is structured to make it difficult to determine the constructed username and password,
// but is not intended to be completely secure!
//
DBApi::eLoginType DBifImpl::LoginAsInstrument( void )
{
	BOOL loginOk = FALSE;
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CString token6r = token6;

	// do not 'stack' user logins...
	if ( instrumentDb.IsOpen() )
	{
		instrumentDb.Close();
	}

	CString token7r = token7;

	token6r.MakeReverse();

	CString dfltuname = token2 + token3 + token6 + token7;
	CString uname = _T( "" );
	CString pwd = _T( "" );
	TCHAR subchar = _T( ' ' );

	CString token3r = token3;

	int32_t ch6Idx = token6r.Find( _T( 's' ) );

	token7r.MakeReverse();

	if ( strUsername.length() == 0 )
	{
		uname = dfltuname;
	}
	else
	{
		uname = strUsername.c_str();
	}

	subchar = _T( '$' );
	token6r.Delete( ch6Idx, 1 );

	std::string unamestr = std::string( CT2A( uname ) );       // write back to std::string to pass to DoConnect

	token3r.MakeReverse();

	if ( uname.CompareNoCase( dfltuname ) == 0 )
	{
		pwd = token7r;
	}
	token7r.Empty();

	int32_t ch3Idx = token3r.Find( _T( 'l' ) );

	CString token2r = token2;

	token6r.Insert( ch6Idx, subchar );

	subchar = _T( '1' );
	token3r.Delete( ch3Idx, 1 );

	if ( pwd.GetLength() > 0 )
	{
		pwd += token6r;
	}
	token6r.Empty();

	token2r.MakeReverse();

	token3r.Insert( ch3Idx, subchar );

	if ( uname.CompareNoCase( dfltuname ) == 0 )
	{
		if ( pwd.GetLength() > 0 )
		{
			pwd += token3r;
		}
	}
	token3r.Empty();

	if ( uname.CompareNoCase( dfltuname ) == 0 )
	{
		if ( pwd.GetLength() > 0 )
		{
			pwd += token2r;
		}
	}
	token2r.Empty();

	std::string pwdstr = std::string( CT2A( pwd ) );

	if ( DoConnect( &instrumentDb, unamestr, pwdstr ) )
	{
		loginOk = true;
	}

	if ( loginOk == TRUE )
	{
		loginType = SetLoginType( DBApi::eLoginType::InstrumentLoginType );
	}

	return loginType;
}

// connect to database as a specified user (may be service or admin or other supplied username / password combination.
// Does not guarantee the supplied username will be available or have DB privileges;
// Allows passing in full usernames
// Allows passing passwords (not desirable...)
//
DBApi::eLoginType DBifImpl::LoginAsUser( std::string unamestr, std::string passwdhash, std::string passwdstr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;

	if ( unamestr.length() == 0 || passwdstr.length() == 0 )
	{
		return loginType;
	}

	// do not 'stack' user logins...
	if ( userDb.IsOpen() )
	{
		userDb.Close();
	}

	std::string pwdstr = passwdstr;
	if ( passwdhash.length() > 0 )
	{
		std::string pwdhash = passwdhash;
		//        DecryptPassword( pwdhash, pwdstr );
	}

	if ( DoConnect( &userDb, unamestr, pwdstr ) )
	{
		loginType = SetLoginType( DBApi::eLoginType::UserLoginType );
	}

	return loginType;
}

// connect to database as a specified user (may be service or admin or other supplied username / password combination.
// Does not guarantee the supplied username will have admin privileges;
// Allows passing in either full usernames or username tags
// Allows passing passwords (not desirable...)
// If known username tags for the admin or service user are supplied, 
// constructs the valid username and password combination for those known user name tags.
// If the admin connection is currenty open, closes the existing connection and (attempts to) opens the new one
//
// NOTE: this method is structured to make it difficult to determine the constructed passwords,
// but is not intended to be completely secure!
//
DBApi::eLoginType DBifImpl::LoginAsAdmin( std::string unamestr, std::string passwdhash, std::string passwdstr )
{
	DBApi::eLoginType loginType = DBApi::eLoginType::NoLogin;
	CString uname = _T( "" );       // CString copy of std::string parameter

	CString token3r = token3;

	uname = unamestr.c_str();

	// do not 'stack' user logins...
	if ( adminDb.IsOpen() )
	{
		adminDb.Close();
	}

	std::string pwdstr = passwdstr;
	CString token9r = token9;
	CString token1r = token1;
	CString token8r = token8;
	CString token9a = token1 + token2 + token3 + token9;
	CString token9b = token1 + token8 + token9;
	CString token9c = token2 + token3 + token9;
	CString token9d = token2 + token3 + token8 + token9;
	CString token10a = token1 + token10;	// currently no service logins to the DB
	CString token10b = token8 + token10;	// currently no service logins to the DB
	bool adminTag = false;
	bool stdAdmin = false;
	bool bciAdmin = false;
	bool serviceTag = false;
	bool stdService = false;
	bool bciService = false;
	CString tmpuname = _T( "" );    // CString construct user name
	CString tmppwd = _T( "" );      // CString construct password
	TCHAR subchar = _T( ' ' );

	if ( passwdhash.length() > 0 )
	{
		std::string pwdhash = passwdhash;
		// will this be necessary? Password is stored as a has, so likely not required...
//		DecryptPassword( pwdhash, pwdstr );
	}
	token9r.MakeReverse();

	if ( ( uname.CompareNoCase( token9 ) == 0 ) ||
		 ( uname.CompareNoCase( token9a ) == 0 ) ||
		 ( uname.CompareNoCase( token9b ) == 0 ) ||
		 ( uname.CompareNoCase( token9c ) == 0 ) ||
		 ( uname.CompareNoCase( token9d ) == 0 ) )
	{
		tmpuname = uname;
		if ( uname.CompareNoCase( token9a ) == 0 )
		{
			bciAdmin = true;
		}
		else if ( uname.CompareNoCase( token9b ) == 0 )
		{
			bciAdmin = true;
		}
		else if ( uname.CompareNoCase( token9c ) == 0 )
		{
			stdAdmin = true;
		}
		else if ( uname.CompareNoCase( token9d ) == 0 )
		{
			stdAdmin = true;
		}
		else
		{
			adminTag = true;
		}
	}
	else if ( ( uname.CompareNoCase( token10 ) == 0 ) ||
			  ( uname.CompareNoCase( token10a ) == 0 ) ||
			  ( uname.CompareNoCase( token10b ) == 0 ) )
	{
		serviceTag = true;
		tmpuname = uname;
		if ( ( uname.CompareNoCase( token10a ) == 0 ) ||
			 ( uname.CompareNoCase( token10b ) == 0 ) )
		{
			bciService = true;
		}
		else
		{
			stdService = true;
		}
	}

	if ( uname.GetLength() == 0  || adminTag || serviceTag )	// construct the database usernames...
	{
		if ( ( bciAdmin ) || ( bciService ) )
		{
			tmpuname = token1;
		}

		tmpuname += token2 + token3;
	}
	subchar = _T( '@' );

	int32_t chIdx9 = token9r.Find( _T( 'A' ) );

	CString token2r = token2;

	if ( ( uname.GetLength() == 0 ) || ( adminTag ) )
	{
		tmpuname += token9;
	}
	else
	{
		if ( serviceTag )
		{
			token9r = token10;
			token9r.MakeReverse();
			chIdx9 = token9r.Find( _T( 'S' ) );
		}
		else
		{
			tmpuname = uname;
		}
	}

	token9r.Delete( chIdx9, 1 );

	token3r.MakeReverse();

	if ( serviceTag )
	{
		tmpuname += token10;
		subchar = _T( '$' );
	}

	int32_t chIdx3 = token3r.Find( _T( 'l' ) );

	unamestr = CT2A( tmpuname );    // write back to parameter to pass to DoConnect

	token9r.Insert( chIdx9, subchar );

	if ( ( uname.GetLength() == 0 ) || ( adminTag ) || ( serviceTag ) )
	{
		tmppwd = token9r;
	}
	token9r.Empty();

	token3r.Delete( chIdx3, 1 );

	token2r.MakeReverse();

	if ( pwdstr.length() > 0 )
	{
		tmppwd = pwdstr.c_str();
	}
	else
	{
		if ( ( bciAdmin ) || ( bciService ) || ( uname.GetLength() == 0 ) )
		{
			token3r.Insert( chIdx3, _T( '1' ) );

			if ( tmppwd.GetLength() > 0 )
			{
				tmppwd += token3r;
			}
		}
	}
	token3r.Empty();

	int32_t chIdx1 = 0;

	if ( ( uname.GetLength() == 0 ) || ( adminTag ) || ( serviceTag ) )
	{
		if ( pwdstr.length() == 0 )
		{
			if ( tmppwd.GetLength() > 0 )
			{
				token1r.MakeReverse();
				tmppwd += token2r;
				chIdx1 = token1r.Find( _T( 'I' ) );
				token1r.Delete( chIdx1, 1 );
			}
		}
	}

	if ( ( uname.GetLength() == 0 ) || ( adminTag ) || ( serviceTag ) )
	{
		if ( pwdstr.length() == 0 )
		{
			if ( tmppwd.GetLength() > 0 )
			{
				token1r.Insert( chIdx1, _T( '1' ) );
				if ( ( bciAdmin ) || ( bciService ) )
				{
					tmppwd += token1r;
				}
			}
		}
	}
	token1r.Empty();

	pwdstr = CT2A( tmppwd );    // write back to parameter to pass to DoConnect

	if ( DoConnect( &adminDb, unamestr, pwdstr ) )
	{
		loginType = SetLoginType( DBApi::eLoginType::AdminLoginType );
	}

	return loginType;
}

// disconnect from database for the login type(s) specified by the usertype bitfield
// returns 'true' if a disconnect was performed successfully (or if not logged-in for the type);
// returns false if noLogin type requested, or failure to disconnect all requested types
bool DBifImpl::LogoutUserType( DBApi::eLoginType usertype )
{
	// requests for NoLogin type or unspecified user-type combinations are not allowed; will return false for 'not successful'
	if ( ( usertype == DBApi::eLoginType::NoLogin ) ||
		 ( usertype == DBApi::eLoginType::AnyLoginType ) )
	{
		return false;
	}

	// check connection for requested user type
	if ( !IsLoginType( usertype ) )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, "LogoutUserType: no database login for specified type(s)." );
#endif
		SetLoginType( DBApi::eLoginType::NoLogin );
		return true;
	}

	uint32_t userTypeBits = static_cast< uint32_t >( usertype );

	// get rid of the 'OR' conditon designator in the user type
	userTypeBits &= static_cast< uint32_t >( DBApi::eLoginType::UserTypeMask );

	if ( userTypeBits == 0 )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, "LogoutUserType: no logout type requested." );
#endif
		return false;
	}
	else if ( ( userTypeBits & static_cast< uint32_t >( DBApi::eLoginType::AllLoginTypes ) ) == 0 )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, "LogoutUserType: invalid or unrecognized logout type." );
#endif
		return false;
	}

	bool disconnected = false;
	uint32_t bitMask = 0;
	int32_t requestCnt = 0;
	int32_t disconnectCnt = 0;
	DBApi::eLoginType logoutType;

	// requesting admin logout?
	logoutType = DBApi::eLoginType::AdminLoginType;
	bitMask = static_cast< uint32_t >( logoutType );
	if ( ( userTypeBits & bitMask ) != 0 )
	{
		requestCnt++;

		if (DoLogoutType( logoutType ))
		{
			userTypeBits &= ~bitMask;
			disconnectCnt++;
		}
	}

	// requesting user logout?
	logoutType = DBApi::eLoginType::UserLoginType;
	bitMask = static_cast< uint32_t >( logoutType );
	if ( ( userTypeBits & bitMask ) != 0 )
	{
		requestCnt++;

		if ( DoLogoutType( logoutType ) )
		{
			userTypeBits &= ~bitMask;
			disconnectCnt++;
		}
	}

	// requesting instrument logout?
	logoutType = DBApi::eLoginType::InstrumentLoginType;
	bitMask = static_cast< uint32_t >( logoutType );
	if ( ( userTypeBits & bitMask ) != 0 )
	{
		requestCnt++;

		if ( DoLogoutType( logoutType ) )
		{
			userTypeBits &= ~bitMask;
			disconnectCnt++;
		}
	}

	if ( userTypeBits != 0 )			// bits still set; unrecognized request...
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, "LogoutUserType: unknown / unrecognized user type specification." );
#endif
		disconnected = false;
	}
	else
	{
		if ( requestCnt == disconnectCnt )
		{
			disconnected = true;
		}
	}

	SetLoginType( DBApi::eLoginType::NoLogin );

	if ( disconnected )
	{
#ifdef USE_LOGGER
		Logger::L().Log( MODULENAME, severity_level::debug1, "LogoutUserType: logged out of the database successfully." );
#endif
	}

	return disconnected;
}

