#ifndef SHARED_UTILS_CONTAINERUTILS_HPP_
#define SHARED_UTILS_CONTAINERUTILS_HPP_

#include <algorithm>

namespace ContainerUtils
{

template <typename Container, typename Func>
void removeIf(Container & container, const Func & func)
{
	container.erase(std::remove_if(container.begin(), container.end(), func), container.end());
}

template <typename Container, typename Func>
void removeIfNot(Container & container, const Func & func)
{
	removeIf(container, [func](const auto & value) {
		return !func(value);
	});
}

}

#endif
