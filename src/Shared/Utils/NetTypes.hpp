#ifndef NET_TYPES_HPP
#define NET_TYPES_HPP

#include <cstdint>

class DataStream;

namespace nt
{

template <typename T>
struct VIntWrapper
{
	VIntWrapper(T & value) :
		value(value)
	{
	}

	T & value;
};

template <typename T>
struct CVIntWrapper
{
	CVIntWrapper(const T & value) :
		value(value)
	{
	}

	const T & value;
};

inline VIntWrapper<std::int64_t> VInt(std::int64_t & value)
{
	return VIntWrapper<std::int64_t>(value);
}
inline CVIntWrapper<std::int64_t> VInt(const std::int64_t & value)
{
	return CVIntWrapper<std::int64_t>(value);
}

inline VIntWrapper<std::uint64_t> VInt(std::uint64_t & value)
{
	return VIntWrapper<std::uint64_t>(value);
}
inline CVIntWrapper<std::uint64_t> VInt(const std::uint64_t & value)
{
	return CVIntWrapper<std::uint64_t>(value);
}

inline VIntWrapper<std::int32_t> VInt(std::int32_t & value)
{
	return VIntWrapper<std::int32_t>(value);
}
inline CVIntWrapper<std::int32_t> VInt(const std::int32_t & value)
{
	return CVIntWrapper<std::int32_t>(value);
}

inline VIntWrapper<std::uint32_t> VInt(std::uint32_t & value)
{
	return VIntWrapper<std::uint32_t>(value);
}
inline CVIntWrapper<std::uint32_t> VInt(const std::uint32_t & value)
{
	return CVIntWrapper<std::uint32_t>(value);
}

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::int64_t> & data);
DataStream & operator<<(DataStream & stream, const VIntWrapper<std::int64_t> & data);
DataStream & operator>>(DataStream & stream, const VIntWrapper<std::int64_t> & data);

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::uint64_t> & data);
DataStream & operator<<(DataStream & stream, const VIntWrapper<std::uint64_t> & data);
DataStream & operator>>(DataStream & stream, const VIntWrapper<std::uint64_t> & data);

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::int32_t> & data);
DataStream & operator<<(DataStream & stream, const VIntWrapper<std::int32_t> & data);
DataStream & operator>>(DataStream & stream, const VIntWrapper<std::int32_t> & data);

DataStream & operator<<(DataStream & stream, const CVIntWrapper<std::uint32_t> & data);
DataStream & operator<<(DataStream & stream, const VIntWrapper<std::uint32_t> & data);
DataStream & operator>>(DataStream & stream, const VIntWrapper<std::uint32_t> & data);

}

#endif
