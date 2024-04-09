#pragma once

#include <string>
#include <vector>
#include <utility>

#include "SetString.hpp"
#include "SystemErrorFeeder.hpp"
#include "uuid__t.hpp"

class DataConversion
{
public:

	static void convertToCharPointer(char*& to, const std::string& from)
	{
		size_t length = from.size() + 1; // +1 to hold to NULL char as last element
		to = new char[length];
		strncpy_s(to, length, from.c_str(), from.size());
	}

	static void convertToCharArray(char* to, size_t to_length, const std::string& from)
	{	
		strncpy_s(to, to_length, from.c_str(), _TRUNCATE);
	}

	static void convertToStandardString(std::string& to, const char* from)
	{
		to = from ? std::string(from) : std::string();
	}

	static bool AreEqual(const uuid__t& first, const uuid__t& second)
	{
		return memcmp(first.u, second.u, sizeof(first.u)) == 0;
	}

	template <typename E>
	static auto enumToRawVal(E e)
	{
		return static_cast<std::underlying_type_t<E>>(e);
	}

	template <typename E>
	static std::string enumToRawValueString(E e)
	{
		return std::to_string(static_cast<uint32_t>(enumToRawVal(e)));
	}

	template<typename T>
	static std::vector<T> create_vector_from_Clist(T* in_list, size_t num_in_list)
	{
		if (!in_list || !num_in_list)
		{
			return{};
		}
		
		std::vector<T> out_list;
		out_list.reserve(num_in_list);

		for (size_t index = 0; index < num_in_list; index++)
		{
			out_list.emplace_back(in_list[index]);
		}
		return out_list;
	}

	template<typename T, typename SIZE>
	static void create_Clist_from_vector(std::vector<T>& in_list, T*& out_list, SIZE& num_out_list)
	{
		out_list = nullptr;
		num_out_list = static_cast<SIZE>(in_list.size());

		if (in_list.empty())
		{
			return;
		}

		out_list = new T[num_out_list];
//#pragma warning(disable: 4996)
		std::copy(in_list.begin(), in_list.end(), out_list);
//#pragma warning(pop)
	}

	template<typename FROM, typename TO>
	static void convert_listOfCType_to_vecOfDllType(FROM* in_list, size_t num_in_list, std::vector<TO>& out_list, bool appendToList=false)
	{
		if (!in_list || !num_in_list)
		{
			return;
		}

		if (!appendToList)
		{
			out_list.clear();
			out_list.reserve (num_in_list);
		}

		for (size_t index = 0; index < num_in_list; index++)
		{
			out_list.emplace_back (TO{ in_list[index] });
		}
	}

	template<typename FROM, typename TO, typename SIZE>
	static void convert_vecOfDllType_to_listOfCType(std::vector<FROM>& in_list, TO*& out_list, SIZE& num_out_list)
	{
		out_list = nullptr;
		num_out_list = static_cast<SIZE>(in_list.size());

		if (in_list.empty())
		{
			return;
		}
		
		out_list = new TO[num_out_list];
		for (SIZE index = 0; index < num_out_list; index++)
		{
			out_list[index] = in_list[index].ToCStyle();
		}
	}

	template<typename FROM, typename TO>
	static void convert_dllType_to_cType(FROM& in, TO*& out)
	{
		out = new TO[1];
		out[0] = in.ToCStyle();
	}

	static std::string boolToString (bool value)
	{
		return value ? "true" : "false";
	}

};
