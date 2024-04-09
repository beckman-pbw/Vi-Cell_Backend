#pragma once

#include <map>
#include <string>
#include <type_traits>

template<typename Enum, class = typename std::enable_if<std::is_enum<Enum>::value>::type>
class EnumConversion
{
public:	
	static std::string enumToString(Enum const& e)
	{
		return enumToStringHelper(e).operator()();
	}

	static Enum enumFromString(std::string& s)
	{
		Enum e = {};
		enumFromStringHelper(e).operator()(s);
		return e;
	}

private:
	template<typename T, class = typename std::enable_if<std::is_enum<T>::value>::type>struct enumStrings
	{
		static const std::map<T, std::string> data;
	};

	struct enumFromStringHelper
	{
		Enum& enumVal;
		enumFromStringHelper (Enum& enumVal) : enumVal (enumVal) {}
		void operator()(std::string& value) const
		{
			auto begin = enumStrings<Enum>::data.begin ();
			auto end = enumStrings<Enum>::data.end ();
			for (auto it = begin; it != end; ++it)
			{
				if (it->second == value)
				{
					enumVal = it->first;
				}
			}
		}
	};

	struct enumToStringHelper
	{
		Enum const& enumVal;
		enumToStringHelper (Enum const& enumVal) : enumVal (enumVal) {}
		std::string operator()(void) const
		{
			if (enumStrings<Enum>::data.find (enumVal) != enumStrings<Enum>::data.end ())
			{
				return enumStrings<Enum>::data.find (enumVal)->second;
			}
			return "Not Stringified Enum";
		}
	};
};
