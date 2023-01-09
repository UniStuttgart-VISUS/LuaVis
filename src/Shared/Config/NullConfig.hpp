#ifndef SRC_SHARED_CONFIG_NULLCONFIGSOURCE_HPP_
#define SRC_SHARED_CONFIG_NULLCONFIGSOURCE_HPP_

#include <Shared/Config/ConfigSource.hpp>
#include <string>
#include <vector>

namespace cfg
{

/**
 * Dummy configuration that does not contain any data.
 */
class NullConfig : public ConfigSource
{
public:
	NullConfig();
	virtual ~NullConfig();

	virtual Value readValue(const std::string & key) const override;
	virtual void writeValue(const std::string & key, Value value) override;
};

}

#endif
