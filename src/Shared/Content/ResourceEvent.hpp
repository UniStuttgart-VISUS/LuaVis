#ifndef SRC_SHARED_CONTENT_RESOURCEEVENT_HPP_
#define SRC_SHARED_CONTENT_RESOURCEEVENT_HPP_

#include <string>

namespace res
{

class ResourceEvent
{
public:
	enum Type
	{
		/**
		 * Empty event mask.
		 */
		None = 0,

		/**
		 * Called whenever an individual resource is changed.
		 */
		ResourceChanged = 1 << 0,

		/**
		 * Called whenever all resources starting with a specific prefix changed.
		 */
		MultipleResourcesChanged = 1 << 1,

		/**
		 * Called whenever a new resource is added.
		 */
		ResourceAdded = 1 << 2,

		/**
		 * Called whenever a resource is removed.
		 */
		ResourceRemoved = 1 << 3,

		/**
		 * Full event mask (for event forwarding).
		 */
		Any = 0x7fffffff
	};

	ResourceEvent(Type type, std::string resourceName = "") :
		type(type),
		resourceName(resourceName)
	{
	}

	const Type type;
	const std::string resourceName;
};

inline ResourceEvent::Type operator|(ResourceEvent::Type a, ResourceEvent::Type b)
{
	return ResourceEvent::Type(int(a) | int(b));
}

inline ResourceEvent::Type & operator|=(ResourceEvent::Type & a, ResourceEvent::Type b)
{
	return a = a | b;
}

}

#endif
