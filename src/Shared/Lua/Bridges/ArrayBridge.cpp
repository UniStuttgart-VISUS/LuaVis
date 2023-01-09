#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Lua/Bridges/ArrayBridge.hpp>
#include <functional>

namespace lua
{

ArrayBridge::ArrayBridge(wosc::ArrayContext & arrayContext) :
	arrayContext(arrayContext)
{
}

ArrayBridge::~ArrayBridge()
{
}

void ArrayBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("array.getContext", std::function<wosc::ArrayContext::ID()>([=]() {
		            return arrayContext.getID();
	            }));
}

}
