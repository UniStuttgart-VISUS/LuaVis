#ifndef SRC_CLIENT_GUI3_TYPES_HPP_
#define SRC_CLIENT_GUI3_TYPES_HPP_

#include <memory>

namespace gui3
{

template <typename T>
using Ptr = std::shared_ptr<T>;

template <typename T, typename... Args>
Ptr<T> make(Args &&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

}

#endif
