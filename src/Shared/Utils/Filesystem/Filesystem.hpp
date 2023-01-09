#ifndef SRC_SHARED_UTILS_FILESYSTEM_FILESYSTEM_HPP_
#define SRC_SHARED_UTILS_FILESYSTEM_FILESYSTEM_HPP_

namespace fs
{

enum ListFlags
{
	ListNone = 0,

	ListFiles = 1 << 0,
	ListDirectories = 1 << 1,
	ListRecursive = 1 << 2,
	ListSorted = 1 << 3,
	ListFullPath = 1 << 4,
};

inline ListFlags operator|(ListFlags a, ListFlags b)
{
	return fs::ListFlags(int(a) | int(b));
}

inline ListFlags & operator|=(ListFlags & a, ListFlags b)
{
	return a = a | b;
}

}

#endif
