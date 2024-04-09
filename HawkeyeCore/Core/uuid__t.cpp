#include "stdafx.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "HawkeyeUUID.hpp"

#define BOOST_UUID_SIZE 16

//*****************************************************************************
bool Uuid::IsValid (const uuid__t& u)
{
	bool uOk = true;

	try
	{
		static_assert(sizeof(u.u) == BOOST_UUID_SIZE, "Unexpected UUID size!");

		boost::uuids::uuid uuidInternal = boost::uuids::nil_uuid();
		memcpy(uuidInternal.data, u.u, sizeof(u.u));

		if (uuidInternal.is_nil())
			uOk = false;
	}
	catch (...)
	{
		return false;
	}

	return uOk;
}

//*****************************************************************************
std::string Uuid::ToStr (const uuid__t& uuid)
{
	return HawkeyeUUID(uuid).to_string();
}

//*****************************************************************************
uuid__t Uuid::FromStr (const std::string& uuidstr)
{
	uuid__t uuid = {};
	HawkeyeUUID internal (uuidstr.c_str());
	internal.get_uuid__t(uuid);

	return uuid;
}

//*****************************************************************************
void Uuid::Clear (uuid__t& uuid)
{
	memset (uuid.u, 0, BOOST_UUID_SIZE);
}

//*****************************************************************************
bool Uuid::IsClear (const uuid__t& uuid)
{
	boost::uuids::uuid uuidInternal = boost::uuids::nil_uuid();
	memcpy(uuidInternal.data, uuid.u, sizeof(uuid.u));

	if (uuidInternal.is_nil())
	{
		return true;
	}
	else
	{
		return false;
	}
}
