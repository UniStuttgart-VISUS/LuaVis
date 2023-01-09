#ifndef SRC_SHARED_LUA_LUASTRINGBUFFER_HPP_
#define SRC_SHARED_LUA_LUASTRINGBUFFER_HPP_

#include <Shared/Utils/Debug/Logger.hpp>
#include <Shared/Utils/Endian.hpp>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace lua
{

class StringBufferParser
{
public:
	enum class DeterminizationResult
	{
		Identical,
		Success,
		Error,
	};

	StringBufferParser(char * start, char * end) :
		start(start),
		end(end),
		cursor(start)
	{
	}

	DeterminizationResult determinize()
	{
		try
		{
			if (needsDeterminization(peekObjectType()))
			{
				determinizeImpl();
				return DeterminizationResult::Success;
			}
			else
			{
				return DeterminizationResult::Identical;
			}
		}
		catch (std::exception & e)
		{
			static Logger logger("LuaStringUtils");
			logger.error("Failed to determinize string buffer: {}", e.what());
			return DeterminizationResult::Error;
		}
	}

	static DeterminizationResult determinizeString(std::string & buffer)
	{
		if (!buffer.empty())
		{
			char * data = &buffer[0];
			StringBufferParser parser(data, data + buffer.length());
			return parser.determinize();
		}
		else
		{
			return DeterminizationResult::Identical;
		}
	}


private:
	enum ObjectType : uint8_t
	{
		Nil = 0x00,
		False = 0x01,
		True = 0x02,
		LightUserdataNull = 0x03,
		LightUserdata32 = 0x04,
		LightUserdata64 = 0x05,
		Int32 = 0x06,
		Double = 0x07,
		TableEmpty = 0x08,
		TableKeyValue = 0x09,
		TableArray0 = 0x0a,
		TableMixed0 = 0x0b,
		TableArray1 = 0x0c,
		TableMixed1 = 0x0d,
		Int64 = 0x10,
		Uint64 = 0x11,
		Complex = 0x12,
		String = 0x20,

		Table = TableKeyValue, // Alias for all tables
	};

	struct ObjectHeader
	{
		ObjectType type = Nil;
		std::uint32_t length = 0;
		std::uint32_t entryCount = 0;
	};

	struct Object
	{
		std::vector<char> data;

		bool operator<(const Object & object) const
		{
			return data < object.data;
		}
	};

	struct KeyValuePair
	{
		Object key;
		Object value;

		bool operator<(const KeyValuePair & object) const
		{
			return key < object.key;
		}
	};

	static bool needsDeterminization(ObjectType type)
	{
		return type >= TableKeyValue && type <= TableMixed1;
	}

	void checkBounds(char * start2, char * end2)
	{
		if (start2 < start || end2 > end)
		{
			throw std::runtime_error("Unexpected end of data in Lua string buffer");
		}
	}

	void checkBounds(std::size_t size)
	{
		checkBounds(cursor, cursor + size);
	}

	template <typename T>
	T read()
	{
		checkBounds(sizeof(T));
		T value;
		std::memcpy(&value, cursor, sizeof(T));
		cursor += sizeof(T);
		return value;
	}

	template <typename T>
	T peek()
	{
		checkBounds(sizeof(T));
		T value;
		std::memcpy(&value, cursor, sizeof(T));
		return value;
	}

	template <typename T>
	void skip()
	{
		cursor += sizeof(T);
	}

	ObjectHeader readObjectHeader()
	{
		ObjectHeader header;
		std::uint32_t type = readVar32();

		if (type >= static_cast<std::uint32_t>(String))
		{
			header.type = String;
			header.length = type - static_cast<std::uint32_t>(String);
		}
		else if (type >= static_cast<std::uint32_t>(TableKeyValue) && type <= static_cast<std::uint32_t>(TableMixed1))
		{
			header.type = Table;
			header.length = (type != TableKeyValue) ? readVar32() : 0;
			header.entryCount = ((type & 0x01) != 0) ? readVar32() : 0;

			if (type >= static_cast<std::uint32_t>(TableArray1))
			{
				header.length--;
			}
		}
		else
		{
			header.type = static_cast<ObjectType>(type);
		}

		return header;
	}

	ObjectType peekObjectType()
	{
		return static_cast<ObjectType>(peek<std::uint8_t>());
	}

	std::uint32_t readVar32()
	{
		// See https://github.com/LuaJIT/LuaJIT/blob/aa7ac6606872e4e21f92400d8491564ace10f259/src/lj_serialize.c#L93
		std::uint8_t byte1 = read<std::uint8_t>();
		if (byte1 < 0xe0)
		{
			// Small value
			return byte1;
		}
		else if (byte1 != 0xff)
		{
			// Medium-sized value
			return ((byte1 & 0x1f) << 8) + read<std::uint8_t>() + 0xe0;
		}
		else
		{
			// Large value
			return n2hl(read<std::uint32_t>());
		}
	}

	Object readDeterminizedObject()
	{
		Object object;
		char * objectStart = cursor;
		determinizeImpl();
		checkBounds(objectStart, cursor);
		object.data.assign(objectStart, cursor);
		return object;
	}

	KeyValuePair readDeterminizedKeyValuePair()
	{
		KeyValuePair pair;
		pair.key = readDeterminizedObject();
		pair.value = readDeterminizedObject();
		return pair;
	}

	void writeObject(const Object & object)
	{
		std::size_t size = object.data.size();
		checkBounds(size);
		std::memcpy(cursor, object.data.data(), size);
		cursor += size;
	}

	void determinizeArray(std::uint32_t entryCount)
	{
		// Perform recursive in-place determinization of keys/values
		for (std::size_t i = 0; i < entryCount; ++i)
		{
			determinizeImpl();
		}
	}

	void determinizeKeyValuePairs(std::uint32_t entryCount)
	{
		// Store initial cursor position
		char * kvStart = cursor;

		// Add determinized key/value pairs to list
		std::vector<KeyValuePair> entries;
		entries.reserve(entryCount);
		for (std::size_t i = 0; i < entryCount; ++i)
		{
			entries.push_back(readDeterminizedKeyValuePair());
		}

		// Sort entries by key
		std::sort(entries.begin(), entries.end());

		// Restore original cursor position
		cursor = kvStart;

		// Write back key/value pairs
		for (const auto & entry : entries)
		{
			writeObject(entry.key);
			writeObject(entry.value);
		}
	}

	void determinizeImpl()
	{
		ObjectHeader header = readObjectHeader();
		switch (header.type)
		{
		case Nil:
		case False:
		case True:
		case LightUserdataNull:
		case TableEmpty:
			// Empty primitives
			break;

		case LightUserdata32:
		case Int32:
		case Int64:
			// 32-bit primitives
			skip<std::int32_t>();
			break;

		case LightUserdata64:
		case Double:
		case Uint64:
			// 64-bit primitives
			skip<std::int64_t>();
			break;

		case Complex:
			// 128-bit primitives
			skip<std::int64_t>();
			skip<std::int64_t>();
			break;

		case Table:
			determinizeArray(header.length);
			determinizeKeyValuePairs(header.entryCount);
			break;

		case String:
			// Skip string contents
			cursor += header.length;
			break;

		default:
			throw std::runtime_error("Unexpected object type in Lua String Buffer");
		}
	}

	char * start;
	char * end;
	char * cursor;
};

}

#endif
