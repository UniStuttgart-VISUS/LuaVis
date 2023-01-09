#ifndef SRC_SHARED_CONTENT_ZIPCREATOR_HPP_
#define SRC_SHARED_CONTENT_ZIPCREATOR_HPP_

#include <memory>
#include <string>
#include <vector>

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>

class DataStream;

namespace res
{

class ZipCreator
{
public:
	ZipCreator(std::shared_ptr<DataStream> stream);
	~ZipCreator();

	void addDirectory(std::string zipPath);
	void addFromMemory(std::string zipPath, const char * data, std::size_t size);
	bool addFromFile(std::string zipPath, std::string filename);

	void finalize();

private:
	std::shared_ptr<DataStream> stream;
	std::unique_ptr<mz_zip_archive> archive;
};

}

#endif