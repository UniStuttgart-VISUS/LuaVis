#include <Shared/Config/DataTypes.hpp>

namespace cfg
{
namespace detail
{

CacheKey::CacheKey(std::string key) :
	id(addKey(key)),
	key(std::move(key))
{
}

CacheKey::ID CacheKey::getID() const
{
	return id;
}

const std::string & CacheKey::getKey() const
{
	return key;
}

CacheKey::ID CacheKey::addKey(std::string key)
{
	auto it = getKeyMap().find(key);
	if (it != getKeyMap().end())
	{
		return it->second;
	}
	else
	{
		ID id = getKeyList().size();
		getKeyMap().emplace(key, id);
		getKeyList().push_back(std::move(key));
		return id;
	}
}

CacheKey::ID CacheKey::lookUpKey(const std::string & key)
{
	auto it = getKeyMap().find(key);
	return it != getKeyMap().end() ? it->second : InvalidID;
}

const std::string & CacheKey::lookUpID(ID id)
{
	static const std::string invalidKey;
	return id < getKeyList().size() ? getKeyList()[id] : invalidKey;
}

std::unordered_map<std::string, CacheKey::ID> & CacheKey::getKeyMap()
{
	static std::unordered_map<std::string, CacheKey::ID> keyMap;
	return keyMap;
}

std::vector<std::string> & CacheKey::getKeyList()
{
	static std::vector<std::string> keyList;
	return keyList;
}

KeyBase::KeyBase(std::string key) :
	cacheKey(std::move(key))
{
}

CacheKey::ID KeyBase::getID() const
{
	return cacheKey.getID();
}

const std::string & KeyBase::getKey() const
{
	return cacheKey.getKey();
}

}
}
