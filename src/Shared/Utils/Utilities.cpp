#include <SFML/Config.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/MiscMath.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <cctype>
#include <cppfs/FileHandle.h>
#include <cppfs/FileIterator.h>
#include <cppfs/fs.h>
#include <cstddef>
#include <ctime>
#include <deque>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

std::string toUppercase(std::string str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); ++it)
		*it = toupper(*it);
	return str;
}
std::string toLowercase(std::string str)
{
	for (std::string::iterator it = str.begin(); it != str.end(); ++it)
		*it = tolower(*it);
	return str;
}

std::string capitalize(std::string str)
{
	if (!str.empty())
		str[0] = toupper(str[0]);
	return str;
}

std::string uncapitalize(std::string str)
{
	if (!str.empty())
		str[0] = tolower(str[0]);
	return str;
}

bool equalsIgnoreCase(const std::string & a, const std::string & b)
{
	std::size_t size = a.size();

	if (size != b.size())
	{
		return false;
	}

	for (std::size_t i = 0; i < size; ++i)
	{
		if (tolower(a[i]) != tolower(b[i]))
		{
			return false;
		}
	}
	return true;
}

bool stringStartsWith(const std::string & string, const std::string & prefix)
{
	if (prefix.empty())
	{
		return true;
	}
	else if (string.length() >= prefix.length())
	{
		return string.compare(0, prefix.length(), prefix) == 0;
	}
	else
	{
		return false;
	}
}

bool stringEndsWith(const std::string & string, const std::string & suffix)
{
	if (suffix.empty())
	{
		return true;
	}
	else if (string.length() >= suffix.length())
	{
		return string.compare(string.length() - suffix.length(), suffix.length(), suffix) == 0;
	}
	else
	{
		return false;
	}
}

std::vector<std::string> splitString(std::string str, const std::string & separator, bool ignoreEmpty)
{
	std::vector<std::string> results;

	std::size_t found = str.find_first_of(separator);

	while (found != std::string::npos)
	{
		if (found > 0)
		{
			results.push_back(str.substr(0, found));
		}
		else if (!ignoreEmpty)
		{
			results.push_back(std::string());
		}
		str = str.substr(found + 1);
		found = str.find_first_of(separator);
	}
	if (str.length() > 0)
	{
		results.push_back(str);
	}

	return results;
}

std::string joinString(const std::vector<std::string> & strings, const std::string & separator)
{
	std::string ret;

	if (strings.empty())
	{
		return ret;
	}

	for (std::size_t i = 0; i < strings.size() - 1; ++i)
	{
		ret += strings[i];
		ret += separator;
	}

	ret += strings.back();

	return ret;
}

std::string getTimeString()
{
	std::time_t rawtime;
	std::tm * timeinfo;
	char buffer[80];
	std::string ret;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	ret.assign(buffer, strftime(buffer, 80, "%X", timeinfo));

	return ret;
}

template <typename F>
void traverseDirectory(const std::string & baseDir, const std::string & subDir, F visitor)
{
	cppfs::FileHandle directory = cppfs::fs::open(baseDir + "/" + subDir);

	for (auto it = directory.begin(); it != directory.end(); ++it)
	{
		std::string filename = subDir.empty() ? *it : subDir + "/" + *it;
		if (visitor(filename))
		{
			traverseDirectory(baseDir, filename, visitor);
		}
	}
}

template <typename F>
bool listFilesImpl(const std::string & dir, std::vector<std::string> & vec, bool recursive, bool sorted, F filter)
{
	if (!cppfs::fs::open(dir).isDirectory())
	{
		return false;
	}

	std::size_t initSize = vec.size();

	traverseDirectory(dir, "", [&](std::string filename) {
		cppfs::FileHandle file = cppfs::fs::open(dir + "/" + filename);
		if (filter(file))
		{
			vec.push_back(std::move(filename));
		}
		return recursive && file.isDirectory() && !file.isSymbolicLink();
	});

	if (sorted)
	{
		std::sort(vec.begin() + initSize, vec.end());
	}

	return true;
}

bool listFiles(const std::string & dir, std::vector<std::string> & vec, bool recursive, bool sorted)
{
	// TODO possibly optimize this at some point to avoid unnecessary string concatenations
	return listFilesImpl(dir, vec, recursive, sorted, [&](cppfs::FileHandle & file) {
		return file.isFile();
	});
}

bool listDirectories(const std::string & dir, std::vector<std::string> & vec, bool recursive, bool sorted)
{
	return listFilesImpl(dir, vec, recursive, sorted, [&](cppfs::FileHandle & file) {
		return file.isDirectory();
	});
}

bool fileExists(const std::string & filename)
{
	try
	{
		return cppfs::fs::open(filename).exists();
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

bool isRegularFile(const std::string & filename)
{
	try
	{
		return cppfs::fs::open(filename).isFile();
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

bool isDirectory(const std::string & filename)
{
	try
	{
		return cppfs::fs::open(filename).isDirectory();
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

bool createDirectory(const std::string & path, bool recursive)
{
	try
	{
		if (cppfs::fs::open(path).isDirectory())
		{
			return true;
		}

		if (recursive)
		{
			bool firstSegment = true;
			auto pos = path.find_first_of('/');
			while (pos != std::string::npos)
			{
				if (firstSegment)
				{
					firstSegment = false;
				}
				else
				{
					auto dir = cppfs::fs::open(path.substr(0, pos));
					if (!dir.isDirectory() && !dir.createDirectory())
					{
						return false;
					}
				}
				pos = path.find_first_of('/', pos + 1);
			}
		}

		return cppfs::fs::open(path).createDirectory();
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

bool moveFile(const std::string & sourceFilename, const std::string & targetFilename)
{
	try
	{
		auto targetFile = cppfs::fs::open(targetFilename);
		return cppfs::fs::open(sourceFilename).move(targetFile);
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

bool deleteFile(const std::string & filename)
{
	try
	{
		return cppfs::fs::open(filename).remove();
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

bool deleteDirectory(const std::string & path, bool recursive)
{
	try
	{
		auto file = cppfs::fs::open(path);
		if (recursive)
		{
			file.removeDirectoryRec();
			return !file.exists();
		}
		else
		{
			return file.removeDirectory();
		}
	}
	catch (std::exception & ex)
	{
		return false;
	}
}

std::string readFileToString(const std::string & filename, std::size_t maxBytes)
{
	std::string result;
	DataStream stream;
	stream.openInFile(filename);
	stream.exportToString(result, maxBytes);
	return result;
}

bool writeStringToFile(const std::string & filename, const std::string & data)
{
	DataStream stream;
	if (stream.openOutFile(filename))
	{
		stream.addData(data.data(), data.size());
		return stream.isValid();
	}
	return false;
}

std::string extractFileExtension(const std::string & filename)
{
	// no dot found in file name.
	if (filename.find_last_of(".") == std::string::npos)
		return "";

	std::string extension = filename.substr(filename.find_last_of("."));

	// last dot found within path name, so no dot in file name.
	if (extension.find_first_of("/\\") != std::string::npos)
		return "";

	return extension;
}
std::string removeFileExtension(const std::string & filename)
{
	// no dot found in file name.
	if (filename.find_last_of(".") == std::string::npos)
		return filename;

	std::string filestem = filename.substr(0, filename.find_last_of("."));
	std::string extension = filename.substr(filename.find_last_of("."));

	// last dot found within path name, no dot to be removed in file name.
	if (extension.find_first_of("/\\") != std::string::npos)
		return filename;

	return filestem;
}

std::string extractFileName(const std::string & path)
{
	std::size_t found = path.find_last_of("/\\");

	if (found == std::string::npos)
	{
		return path;
	}
	else
	{
		return path.substr(found + 1);
	}
}

std::string removeFileName(const std::string & path)
{
	std::size_t found = path.find_last_of("/\\");

	if (found == std::string::npos)
	{
		return "";
	}
	else
	{
		return path.substr(0, found);
	}
}

std::string joinPaths(const std::string & path1, const std::string & path2)
{
	if (path1.empty() || path1.back() == '/' || path2.empty() || path2.front() == '/')
	{
		return path1 + path2;
	}
	else
	{
		return path1 + "/" + path2;
	}
}

std::string getByteSizeString(sf::Uint64 bytes)
{
	if (bytes < 1000)
	{
		return cNtoS(bytes) + " B";
	}
	else if (bytes < 1000000)
	{
		return cNtoS(round<double>(bytes / 1000.0, 1)) + " KB";
	}
	else if (bytes < 1000000000)
	{
		return cNtoS(round<double>(bytes / 1000000.0, 1)) + " MB";
	}
	else
	{
		return cNtoS(round<double>(bytes / 1000000000.0, 1)) + " GB";
	}
}

std::string fillStringWithChar(std::string str, char chr, std::size_t targetSize)
{
	if (str.size() < targetSize)
		return std::string(targetSize - str.size(), chr) + str;
	else
		return str;
}

std::string getSfTimeString(sf::Time time)
{
	std::ostringstream str;

	str.precision(2);
	str << std::fixed;

	if (time < sf::milliseconds(1))
	{
		str << time.asMicroseconds() << "us";
	}
	else if (time < sf::seconds(1))
	{
		str << round(time.asSeconds() * 1000.f, 2) << "ms";
	}
	else if (time < sf::seconds(60))
	{
		str << round(time.asSeconds(), 2) << "s";
	}
	else if (time < sf::seconds(60 * 60))
	{
		str << int(time.asSeconds()) / 60 << ":" << fillStringWithChar(cNtoS(int(time.asSeconds()) % 60), '0', 2) << "."
		    << fillStringWithChar(cNtoS((time.asMilliseconds() / 10) % 100), '0', 2);
	}
	else
	{
		return getRoughSfTimeString(time);
	}

	return str.str();
}

std::string getRoughSfTimeString(sf::Time time)
{
	if (time > sf::seconds(60 * 60))
	{
		return cNtoS(int(time.asSeconds() / 60 / 60)) + ":"
		       + fillStringWithChar(cNtoS(int(time.asSeconds() / 60) % 60), '0', 2) + ":"
		       + fillStringWithChar(cNtoS(int(time.asSeconds()) % 60), '0', 2);
	}
	else
	{
		return cNtoS(int(time.asSeconds() / 60) % 60) + ":"
		       + fillStringWithChar(cNtoS(int(time.asSeconds()) % 60), '0', 2);
	}
}

bool pointInPoly(const std::vector<sf::Vector2f> & vertices, const sf::Vector2f & point)
{
	bool ret = false;
	for (std::size_t i = 0, j = vertices.size() - 1; i < vertices.size(); j = i++)
	{
		const sf::Vector2f &v1 = vertices[i], &v2 = vertices[j];
		if ((v1.y > point.y) != (v2.y > point.y) && point.x < (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x)
			ret = !ret;
	}
	return ret;
}
