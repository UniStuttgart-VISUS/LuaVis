#ifndef SRC_SHARED_LUA_LUAUTILS_HPP_
#define SRC_SHARED_LUA_LUAUTILS_HPP_

#include <Sol2/sol.hpp>

namespace lua
{

template <typename T>
T cast(const sol::object & object)
{
	return object.as<T>();
}

template <typename T, typename Table, typename Key>
T getOr(const sol::proxy<Table, Key> & proxy, T defaultValue)
{
	return proxy.get_or(defaultValue);
}

template <typename List>
sol::table listToTable(const List & list, sol::state_view state)
{
	std::size_t index = 1;
	auto table = state.create_table();
	for (const auto & element : list)
	{
		table[index++] = element;
	}
	return table;
}

template <typename T>
std::vector<T> tableToVector(const sol::table & table, T fallback = {})
{
	std::vector<T> vector;

	auto size = table.size();
	vector.resize(size);

	for (std::size_t i = 1; i <= size; ++i)
	{
		vector[i - 1] = getOr<T>(table[i], fallback);
	}

	return vector;
}

template <typename Map>
sol::table mapToTable(const Map & map, sol::state_view state)
{
	auto table = state.create_table();
	for (const auto & entry : map)
	{
		table[entry.first] = entry.second;
	}
	return table;
}

}

#endif
