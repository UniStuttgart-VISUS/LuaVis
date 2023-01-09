#ifndef SRC_SHARED_CONFIG_CONFIGCACHE_HPP_
#define SRC_SHARED_CONFIG_CONFIGCACHE_HPP_

#include <Shared/Config/ConfigSource.hpp>
#include <Shared/Config/DataTypes.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cfg
{
class ConfigSource;
class Config;

namespace detail
{

template <typename DataType>
struct ValueEntry
{
	ValueEntry() :
		valid(false),
		value()
	{
	}

	bool valid;
	DataType value;

	bool operator==(const ValueEntry<DataType> & other) const
	{
		return valid ? (other.valid && value == other.value) : !other.valid;
	}

	bool operator!=(const ValueEntry<DataType> & other) const
	{
		return !((*this) == other);
	}
};

template <typename KeyType>
class Cache
{
public:
	using ID = CacheKey::ID;
	using ValueType = ValueEntry<typename KeyType::DataType>;

	Cache() :
		dense(false)
	{
	}

	void clear()
	{
		sparseMap = SparseMap();
		denseMap = DenseMap();
	}

	void remove(ID index)
	{
		if (isDense())
		{
			(*this)[index] = ValueType();
		}
		else
		{
			sparseMap.erase(index);
		}
	}

	void setDense(bool dense)
	{
		if (!isDense() && dense)
		{
			for (const auto & it : sparseMap)
			{
				denseMapRef(it.first) = it.second;
			}
			sparseMap = SparseMap();
			this->dense = true;
		}
		else
		{
			static ValueType empty;
			for (ID i = 0; i < denseMap.size(); ++i)
			{
				const auto & entry = denseMap[i];
				if (entry != empty)
				{
					sparseMap[i] = entry;
				}
			}
			denseMap = DenseMap();
			this->dense = false;
		}
	}

	bool isDense() const
	{
		return dense;
	}

	ValueType & operator[](ID index)
	{
		if (isDense())
		{
			return denseMapRef(index);
		}
		else
		{
			return sparseMap[index];
		}
	}

private:
	ValueType & denseMapRef(ID index)
	{
		if (index >= denseMap.size())
		{
			denseMap.resize(index + 1);
		}
		return denseMap[index];
	}

	using SparseMap = std::unordered_map<ID, ValueType>;
	using DenseMap = std::vector<ValueType>;

	bool dense;
	SparseMap sparseMap;
	DenseMap denseMap;
};

template <typename KeyType>
struct CacheGetter
{
	static Cache<KeyType> & get(const Config & configCache) = delete;
};

template <>
struct CacheGetter<StringKey>
{
	static Cache<StringKey> & get(const Config & configCache);
};

template <>
struct CacheGetter<BoolKey>
{
	static Cache<BoolKey> & get(const Config & configCache);
};

template <>
struct CacheGetter<IntKey>
{
	static Cache<IntKey> & get(const Config & configCache);
};

template <>
struct CacheGetter<FloatKey>
{
	static Cache<FloatKey> & get(const Config & configCache);
};

}

class Config
{
public:
	using MissingKeyFunction = std::function<Value(std::string)>;

	Config();
	virtual ~Config();

	void setConfigSource(std::shared_ptr<ConfigSource> config);
	ConfigSource * getConfigSource() const;

	void setMissingKeyFunction(MissingKeyFunction missingKeyFunction);
	MissingKeyFunction getMissingKeyFunction() const;

	void clearCache()
	{
		myCacheString.clear();
		myCacheBool.clear();
		myCacheInt.clear();
		myCacheFloat.clear();
	}

	void setDense(bool dense)
	{
		myCacheString.setDense(dense);
		myCacheBool.setDense(dense);
		myCacheInt.setDense(dense);
		myCacheFloat.setDense(dense);
	}

	bool isDense() const
	{
		return myCacheString.isDense();
	}

	template <typename KeyType>
	typename KeyType::DataType get(const KeyType & key) const
	{
		return key.onGet(*this);
	}

	template <typename KeyType>
	void set(const KeyType & key, typename KeyType::DataType value)
	{
		key.onSet(*this, std::move(value));
	}

	template <typename KeyType>
	typename KeyType::DataType getCachedValue(const KeyType & key) const
	{
		auto & cache = getCache<KeyType>();
		auto & entry = cache[key.getID()];

		if (!entry.valid)
		{
			auto readValue = myConfigSource->readValue(key.getKey());
			if (readValue.type == NodeType::Missing)
			{
				readValue = myMissingKeyFunction(key.getKey());
			}
			entry.value = KeyType::convertTo(readValue);
			entry.valid = true;
		}

		return entry.value;
	}

	template <typename KeyType>
	void setCachedValue(const KeyType & key, typename KeyType::DataType value)
	{
		clearSingleCacheEntry(key.getID());

		auto & cache = getCache<KeyType>();
		auto & entry = cache[key.getID()];

		if (!entry.valid)
		{
			entry.value = value;
			entry.valid = true;
		}

		myConfigSource->writeValue(key.getKey(), KeyType::convertFrom(std::move(value)));
	}

	template <typename KeyType>
	typename KeyType::DataType getTransientValue(const std::string & key) const
	{
		auto readValue = myConfigSource->readValue(key);
		if (readValue.type == NodeType::Missing)
		{
			readValue = myMissingKeyFunction(key);
		}
		return KeyType::convertTo(readValue);
	}

	template <typename KeyType>
	void setTransientValue(const std::string & key, typename KeyType::DataType value)
	{
		// Expunge cache entries, even when a value is set through a transient accessor
		clearSingleCacheEntry(detail::CacheKey::lookUpKey(key));
		myConfigSource->writeValue(key, KeyType::convertFrom(std::move(value)));
	}

	void unset(const std::string & key);

private:
	void clearSingleCacheEntry(detail::CacheKey::ID index);

	template <typename KeyType>
	void unsetCachedValue(detail::CacheKey::ID index)
	{
		getCache<KeyType>().remove(index);
	}

	template <typename KeyType>
	detail::Cache<KeyType> & getCache() const
	{
		return detail::CacheGetter<KeyType>::get(*this);
	}

	bool myHasConfigSource;
	std::shared_ptr<ConfigSource> myConfigSource;
	MissingKeyFunction myMissingKeyFunction;

	mutable detail::Cache<detail::StringKey> myCacheString;
	mutable detail::Cache<detail::BoolKey> myCacheBool;
	mutable detail::Cache<detail::IntKey> myCacheInt;
	mutable detail::Cache<detail::FloatKey> myCacheFloat;

	template <typename KeyType>
	friend struct detail::CacheGetter;
};

}

#endif
