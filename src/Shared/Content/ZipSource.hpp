#ifndef SRC_SHARED_CONTENT_ZIPSOURCE_HPP_
#define SRC_SHARED_CONTENT_ZIPSOURCE_HPP_

#include <Shared/Content/AbstractSource.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <memory>
#include <string>
#include <vector>

#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>

namespace res
{

class ZipSource : public AbstractSource
{
public:
	static const std::string PACKAGE_HASH;

	ZipSource();
	ZipSource(std::string zipFileName);
	virtual ~ZipSource();

	void close();

	void setZipFileName(std::string zipFileName);
	const std::string & getZipFileName() const;

	virtual bool loadResource(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget) override;
	virtual bool resourceExists(const std::string & resourceName) const override;
	virtual std::vector<std::string> getResourceList(std::string prefix, fs::ListFlags flags) const override;

private:
	void updateHash();

	struct PartialExtraction
	{
		std::vector<char> * target = nullptr;
		std::size_t limit = 0;
	};

	static size_t zipSourceExtractCallback(void * pOpaque, mz_uint64 file_ofs, const void * pBuf, size_t n);

	Logger logger;

	std::string zipFileName;
	std::vector<char> zipHash;
	std::unique_ptr<mz_zip_archive> archive;
	FILE * file = nullptr;

	mutable std::vector<std::string> fileListCache;
	mutable std::vector<std::string> directoryListCache;
};

}

#endif
