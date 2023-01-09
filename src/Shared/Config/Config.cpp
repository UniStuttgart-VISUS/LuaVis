#include <Shared/Config/Config.hpp>
#include <Shared/Config/NullConfig.hpp>
#include <iostream>

namespace cfg
{

namespace detail
{

Cache<StringKey> & CacheGetter<StringKey>::get(const Config & configCache)
{
	return configCache.myCacheString;
}
Cache<BoolKey> & CacheGetter<BoolKey>::get(const Config & configCache)
{
	return configCache.myCacheBool;
}
Cache<IntKey> & CacheGetter<IntKey>::get(const Config & configCache)
{
	return configCache.myCacheInt;
}
Cache<FloatKey> & CacheGetter<FloatKey>::get(const Config & configCache)
{
	return configCache.myCacheFloat;
}

}

Config::Config()
{
	setConfigSource(nullptr);
	setMissingKeyFunction([](std::string key) {
		return Value();
	});
}

Config::~Config()
{
}

void Config::setConfigSource(std::shared_ptr<ConfigSource> config)
{
	myHasConfigSource = (config != nullptr);

	if (myHasConfigSource)
	{
		myConfigSource = config;
	}
	else
	{
		myConfigSource = std::make_shared<NullConfig>();
	}
}

ConfigSource * Config::getConfigSource() const
{
	return myHasConfigSource ? myConfigSource.get() : nullptr;
}

void Config::setMissingKeyFunction(MissingKeyFunction missingKeyFunction)
{
	myMissingKeyFunction = missingKeyFunction;
}

Config::MissingKeyFunction Config::getMissingKeyFunction() const
{
	return myMissingKeyFunction;
}

void Config::unset(const std::string & key)
{
	clearSingleCacheEntry(detail::CacheKey::lookUpKey(key));
	myConfigSource->writeValue(key, Value());
}

void Config::clearSingleCacheEntry(detail::CacheKey::ID index)
{
	if (index != detail::CacheKey::InvalidID)
	{
		unsetCachedValue<detail::StringKey>(index);
		unsetCachedValue<detail::BoolKey>(index);
		unsetCachedValue<detail::IntKey>(index);
		unsetCachedValue<detail::FloatKey>(index);
	}
}

}
