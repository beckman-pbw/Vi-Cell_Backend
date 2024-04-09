#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

typedef struct uuid__t
{
	unsigned char u[16];
} uuid__t;

class Uuid
{
public:
	Uuid (uuid__t uuid)
	{
		this->uuid = uuid;
	}
	static bool IsValid (const uuid__t& u);
	static std::string ToStr (const uuid__t& uuid);
	static uuid__t FromStr (const std::string& uuidstr);
	static void Clear (uuid__t& uuid);
	static bool IsClear (const uuid__t& uuid);

	bool operator== (const Uuid& rhs) const
	{
		if (typeid(*this) != typeid(rhs))
			return false;

		return (memcmp (uuid.u, rhs.uuid.u, sizeof(uuid.u)) == 0) ? true : false;
	}

	bool operator!= ( const Uuid& rhs ) const
	{
		if ( typeid( *this ) != typeid( rhs ) )
			return true;

		return ( memcmp( uuid.u, rhs.uuid.u, sizeof( uuid.u ) ) == 0 ) ? false : true;
	}

private:
	uuid__t uuid;
};
