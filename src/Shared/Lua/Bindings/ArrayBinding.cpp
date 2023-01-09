#include <Shared/Lua/Bindings/ArrayBinding.h>
#include <Shared/Lua/Bindings/ArrayBinding.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Error.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

extern "C"
{

	wosC_array_id_t wosC_array_new(wosC_array_context_t context, wosC_array_size_t size)
	{
		return wosc::ArrayContext::getContextByID(context).newArray(size);
	}

	wosC_array_info_t wosC_array_getArrayInfo(wosC_array_context_t context, wosC_array_id_t array)
	{
		return wosc::ArrayContext::getContextByID(context).getArrayInfo(array);
	}

	void wosC_array_delete(wosC_array_ref_t reference)
	{
		return wosc::ArrayContext::getContextByID(reference.context).deleteArray(reference.array);
	}

	void wosC_array_setOwnership(wosC_array_context_t context, wosC_array_id_t array, bool own)
	{
		wosc::ArrayContext::getContextByID(context).setArrayOwnershipFlag(array, own);
	}

	bool wosC_array_isOwned(wosC_array_context_t context, wosC_array_id_t array)
	{
		return wosc::ArrayContext::getContextByID(context).getArrayOwnershipFlag(array);
	}
}

namespace wosc
{

std::vector<ArrayContext *> ArrayContext::contexts = {};

ArrayContext::ArrayContext() :
	logger("ArrayContext")
{
	bool found = false;
	for (ID i = 0; i < (ID) contexts.size(); ++i)
	{
		if (contexts[i] == nullptr)
		{
			contexts[i] = this;
			contextID = i;
			found = true;
			break;
		}
	}

	if (!found)
	{
		contexts.push_back(this);
		contextID = contexts.size() - 1;
	}

	logger.trace("ArrayContext created: id = {}", contextID);
}

ArrayContext::~ArrayContext()
{
	contexts[contextID] = nullptr;
	while (!contexts.empty() && contexts.back() == nullptr)
	{
		contexts.pop_back();
	}

	logger.trace("ArrayContext deleted: id = {}", contextID);
}

ArrayContext & ArrayContext::getContextByID(ID contextID)
{
	if (contextID < 0 || contextID >= (ID) contexts.size() || contexts[contextID] == nullptr)
	{
		throw Error("Attempt to access invalid array context " + cNtoS(contextID));
	}

	return *contexts[contextID];
}

ArrayContext::ID ArrayContext::getID() const
{
	return contextID;
}

void ArrayContext::clear()
{
	arrays.clear();
	arrayIDWrapped = false;
	currentArrayID = 0;
	memoryUsage = 0;
}

ArrayContext::ArrayID ArrayContext::newArray(ArraySize size)
{
	auto arrayID = getNextArrayID();
	arrays.emplace(arrayID, ArrayStructure(size));
	memoryUsage += size;
	logger.trace("Array created: ctx = {}, id = {}, size = {}, count = {}", contextID, arrayID, size, arrays.size());
	return arrayID;
}

void ArrayContext::deleteArray(ArrayID id)
{
	auto result = arrays.find(id);
	if (result != arrays.end())
	{
		memoryUsage -= result->second.size;
		arrays.erase(id);

		logger.trace("Array deleted: ctx = {}, id = {}, count = {}", contextID, id, arrays.size());
	}
	else
	{
		logger.trace("Tried to delete non-existing array: ctx = {}, id = {}", contextID, id);
	}
}

ArrayContext::ArrayInfo ArrayContext::getArrayInfo(ArrayID id) const
{
	auto result = arrays.find(id);
	if (result != arrays.end())
	{
		return ArrayInfo {result->second.getPointer(), result->second.size};
	}
	else
	{
		return ArrayInfo {nullptr, 0};
	}
}

void ArrayContext::setArrayOwnershipFlag(ArrayID id, bool owned)
{
	auto result = arrays.find(id);
	if (result != arrays.end())
	{
		result->second.owned = owned;
	}
}

bool ArrayContext::getArrayOwnershipFlag(ArrayID id) const
{
	auto result = arrays.find(id);
	if (result != arrays.end())
	{
		return result->second.owned;
	}
	else
	{
		return false;
	}
}

std::size_t ArrayContext::getTotalMemoryUsage() const
{
	return memoryUsage;
}

ArrayContext::ArrayStructure::ArrayStructure(ArraySize size) :
	data(std::make_unique<ArrayItem[]>(size)),
	size(size),
	owned(false)
{
}

ArrayContext::ArrayPointer ArrayContext::ArrayStructure::getPointer() const
{
	return data.get();
}

ArrayContext::ArrayID ArrayContext::getNextArrayID()
{
	if (currentArrayID == std::numeric_limits<ArrayID>::max())
	{
		currentArrayID = 0;
		arrayIDWrapped = true;
	}
	else
	{
		++currentArrayID;
	}

	if (arrayIDWrapped)
	{
		while (arrays.count(currentArrayID))
		{
			++currentArrayID;
		}
	}

	return currentArrayID;
}

}
