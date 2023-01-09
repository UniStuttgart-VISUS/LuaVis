#ifndef SRC_SHARED_LUA_BINDINGS_BINDINGAPI_HPP_
#define SRC_SHARED_LUA_BINDINGS_BINDINGAPI_HPP_

#include <Shared/Utils/OSDetect.hpp>

#ifdef WOS_WINDOWS
#	define WOSC_API __declspec(dllexport)
#else
#	define WOSC_API
#endif

#endif
