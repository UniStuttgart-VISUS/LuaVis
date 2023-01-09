#ifndef SRC_SHARED_UTILS_HASHTABLE_HPP_
#define SRC_SHARED_UTILS_HASHTABLE_HPP_

#include <Shared/Utils/DataStream.hpp>

#define WOS_ENABLE_SPARSEPP_HASHTABLE

#ifdef WOS_ENABLE_SPARSEPP_HASHTABLE
#	include <sparsepp/spp.h>

namespace priv
{

namespace spp
{

class DataStreamWrapper
{
public:
	inline DataStreamWrapper(DataStream & stream) :
		stream(stream)
	{
	}

	inline std::size_t Read(void * data, std::size_t bytes)
	{
		return stream.extractData(data, bytes) ? bytes : 0;
	}

	inline std::size_t Write(const void * data, std::size_t bytes)
	{
		stream.addData(data, bytes);
		return stream.isValid() ? bytes : 0;
	}

	inline DataStream & getStream() const
	{
		return stream;
	}

private:
	DataStream & stream;
};

class DataStreamSerializer
{
public:
	template <typename T>
	bool operator()(DataStreamWrapper * wrapper, const T & value)
	{
		return wrapper->getStream() << value;
	}

	template <typename T>
	bool operator()(DataStreamWrapper * wrapper, T * value)
	{
		return wrapper->getStream() >> *value;
	}
};

}

}

template <typename K, typename V, typename HashFunc = spp::spp_hash<K>>
using HashMap = spp::sparse_hash_map<K, V, HashFunc>;

template <typename K, typename HashFunc = spp::spp_hash<K>>
using HashSet = spp::sparse_hash_set<K, HashFunc>;

template <typename K, typename V>
DataStream & operator<<(DataStream & stream, const HashMap<K, V> & map)
{
	priv::spp::DataStreamWrapper wrapper(stream);
	map.serialize(priv::spp::DataStreamSerializer(), &wrapper);
	return stream;
}

template <typename K, typename V>
DataStream & operator>>(DataStream & stream, HashMap<K, V> & map)
{
	priv::spp::DataStreamWrapper wrapper(stream);
	map.unserialize(priv::spp::DataStreamSerializer(), &wrapper);
	return stream;
}

template <typename K>
DataStream & operator<<(DataStream & stream, const HashSet<K> & set)
{
	priv::spp::DataStreamWrapper wrapper(stream);
	set.serialize(priv::spp::DataStreamSerializer(), &wrapper);
	return stream;
}

template <typename K>
DataStream & operator>>(DataStream & stream, HashSet<K> & set)
{
	priv::spp::DataStreamWrapper wrapper(stream);
	set.unserialize(priv::spp::DataStreamSerializer(), &wrapper);
	return stream;
}

#else
#	include <unordered_map>
#	include <unordered_set>

template <typename K, typename V>
using HashMap = std::unordered_map<K, V>;

template <typename K>
using HashSet = std::unordered_set<K>;

// TODO write stream i/o operators for hashset and hashmap

#endif // WOS_ENABLE_SPARSEPP_HASHTABLE


#endif
