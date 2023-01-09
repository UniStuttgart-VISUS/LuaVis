#ifndef SRC_SHARED_LUA_BINDINGS_ARRAYBINDING_HPP_
#define SRC_SHARED_LUA_BINDINGS_ARRAYBINDING_HPP_

#include <Shared/Lua/Bindings/ArrayBinding.h>
#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <memory>
#include <vector>

namespace wosc
{

class ArrayContext
{
public:
	using ID = wosC_array_context_t;
	using ArrayID = wosC_array_id_t;
	using ArrayPointer = wosC_array_t;
	using ArraySize = wosC_array_size_t;
	using ArrayItem = wosC_array_item_t;
	using ArrayInfo = wosC_array_info_t;

	ArrayContext();
	~ArrayContext();

	ArrayContext(ArrayContext && context) = delete;
	ArrayContext(const ArrayContext & context) = delete;

	ArrayContext & operator=(ArrayContext && context) = delete;
	ArrayContext & operator=(const ArrayContext & context) = delete;

	static ArrayContext & getContextByID(ID contextID);

	void clear();

	ID getID() const;

	ArrayID newArray(ArraySize size);
	void deleteArray(ArrayID id);

	ArrayInfo getArrayInfo(ArrayID id) const;

	void setArrayOwnershipFlag(ArrayID id, bool owned);
	bool getArrayOwnershipFlag(ArrayID id) const;

	std::size_t getTotalMemoryUsage() const;

private:
	struct ArrayStructure
	{
		ArrayStructure(ArraySize size);

		ArrayPointer getPointer() const;

		std::unique_ptr<ArrayItem[]> data;
		ArraySize size;
		bool owned;
	};

	ArrayID getNextArrayID();

	ID contextID;
	ArrayID currentArrayID = 0;
	bool arrayIDWrapped = false;
	HashMap<ArrayID, ArrayStructure> arrays;
	std::size_t memoryUsage = 0;

	Logger logger;

	static std::vector<ArrayContext *> contexts;
};

}

#endif
