#ifndef SRC_SHARED_CONTENT_LUASOURCE_HPP_
#define SRC_SHARED_CONTENT_LUASOURCE_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Sol2/sol.hpp>
#include <string>
#include <vector>

namespace res
{

class LuaSource : public AbstractSource
{
public:
	LuaSource(wosc::ArrayContext * arrayContext);
	LuaSource(wosc::ArrayContext * arrayContext, sol::function function);
	virtual ~LuaSource();

	void setFunction(sol::function function);
	void unsetFunction();

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual std::vector<std::string> getResourceList(std::string prefix, fs::ListFlags flags) const override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual void pollChanges() override;

private:
	sol::optional<sol::function> function;
	wosc::ArrayContext * arrayContext;

	Logger logger;
};

}

#endif
