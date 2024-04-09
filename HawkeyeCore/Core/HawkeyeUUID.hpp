#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

#include "uuid__t.hpp"

#define BOOST_UUID_SIZE 16

class HawkeyeUUID
{
public:
	HawkeyeUUID() : u(boost::uuids::nil_uuid()) {};
	HawkeyeUUID(const char* str) 
	{ 
		from_string(str);
	}
	HawkeyeUUID(const HawkeyeUUID& lhs)
	{
		u = lhs.u;
	}

	HawkeyeUUID (const uuid__t& uuid)
	{
		memcpy (u.data, uuid.u, sizeof(uuid.u));
	}

	void get_uuid__t (uuid__t& uuid) const
	{
		memcpy (uuid.u, u.data, sizeof(u.data));
	}

	std::string to_string() const
	{
		return boost::uuids::to_string(u);
	}
	
	bool from_string(const char* str)
	{
		try
		{
			boost::uuids::string_generator gen;
			u = gen(str);
			return true;
		}
		catch(...)
		{
			u = boost::uuids::nil_uuid();
			return false;
		}
	}

	bool isNIL() const
	{
		return u.is_nil();
	}

	static bool Getuuid__tFromStr(const std::string& uuidstr, uuid__t& uuid)
	{
		if (uuidstr.empty())
		{
			HawkeyeUUID internal;
			internal.get_uuid__t(uuid);
		}
		else
		{
			HawkeyeUUID internal (uuidstr.c_str());
			internal.get_uuid__t(uuid);
		}

		return true;
	}

	static bool GetStrFromuuid__t(const uuid__t& uuid, std::string& uuidstr)
	{
		try
		{
			static_assert(sizeof(uuid.u) == BOOST_UUID_SIZE, "Unexpected UUID size!");

			boost::uuids::uuid uuidInternal = boost::uuids::nil_uuid();
			memcpy(uuidInternal.data, uuid.u, sizeof(uuid.u));

			if (uuidInternal.is_nil())
				uuidstr = "";
			else
				uuidstr = boost::uuids::to_string(uuidInternal);
		}
		catch (...)
		{
			return false;
		}

		return true;
	}

	static std::string GetStrFromuuid__t(const uuid__t& uuid)
	{
		try
		{
			if (sizeof(uuid.u) != BOOST_UUID_SIZE)
				return "";
			
			boost::uuids::uuid uuidInternal = boost::uuids::nil_uuid();
			
			memcpy(uuidInternal.data, uuid.u, sizeof(uuid.u));
			if (uuidInternal.is_nil())
			{
				return "";
			}
			return boost::uuids::to_string(uuidInternal);
		}
		catch (...)
		{
			return "";
		}

		// We should never get here.
		return "";
	}

	bool operator== (const HawkeyeUUID& rhs) const
	{
		return (memcmp (u.data, rhs.u.data, sizeof(u)) == 0) ? true : false;
	}

	static HawkeyeUUID Generate()
	{
		boost::uuids::random_generator gen;
		boost::uuids::uuid u_new = gen();

		return HawkeyeUUID(boost::uuids::to_string(u_new).c_str());
	}


private:
	boost::uuids::uuid u;
};
