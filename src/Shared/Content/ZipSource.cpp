#include <Shared/Content/Resource.hpp>
#include <Shared/Content/ResourceEvent.hpp>
#include <Shared/Content/ZipSource.hpp>
#include <Shared/Utils/HashTable.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/Utilities.hpp>

#include <blake2/blake2.h>
#include <miniz/miniz.h>

#include <algorithm>
#include <iterator>

#ifdef WOS_WINDOWS
#	include <cppfs/windows/FileNameConversions.h>
#endif

namespace res
{

const std::string ZipSource::PACKAGE_HASH = "$packageHash";

ZipSource::ZipSource() : logger("ZipSource")
{
}

ZipSource::ZipSource(std::string zipFileName) : ZipSource()
{
	setZipFileName(std::move(zipFileName));
}

ZipSource::~ZipSource()
{
	close();
}

void ZipSource::close()
{
	if (archive)
	{
		mz_zip_reader_end(archive.get());
		archive = nullptr;
	}

	if (file)
	{
		fclose(file);
		file = nullptr;
	}

	zipHash.clear();
}

void ZipSource::setZipFileName(std::string zipFileName)
{
	close();
	this->zipFileName = zipFileName;

#ifdef WOS_WINDOWS
	std::wstring wideFilename = cppfs::convert::utf8ToWideString(zipFileName);
	file = _wfopen(wideFilename.c_str(), L"rb");
#else
	file = fopen(zipFileName.c_str(), "rb");
#endif

	if (file == nullptr)
	{
		zipFileName.clear();
	}
	else
	{
		archive = std::make_unique<mz_zip_archive>();
		mz_zip_reader_init_cfile(archive.get(), file, 0, 0);
	}

	fileListCache.clear();
	directoryListCache.clear();

	fireEvent(ResourceEvent::MultipleResourcesChanged);
}

const std::string & ZipSource::getZipFileName() const
{
	return zipFileName;
}

bool ZipSource::loadResource(const std::string & resourceName, std::vector<char> & dataTarget)
{
	if (!archive)
	{
		logger.warn("Attempt to use uninitialized zip source");
		return false;
	}

	if (resourceName == PACKAGE_HASH)
	{
		updateHash();
		dataTarget = zipHash;
		return !dataTarget.empty();
	}

	auto index = mz_zip_reader_locate_file(archive.get(), resourceName.c_str(), nullptr, 0);
	if (index < 0)
	{
		return false;
	}

	mz_zip_archive_file_stat fileInfo = {};
	if (!mz_zip_reader_file_stat(archive.get(), index, &fileInfo))
	{
		return false;
	}

	if (fileInfo.m_is_directory || !fileInfo.m_is_supported)
	{
		return false;
	}

	if (fileInfo.m_uncomp_size > 32 * 1024 * 1024)
	{
		logger.error("Resource '{}' is too large ({} bytes)", resourceName, fileInfo.m_uncomp_size);
		return false;
	}

	dataTarget.resize(fileInfo.m_uncomp_size);
	return mz_zip_reader_extract_to_mem(archive.get(), index, dataTarget.data(), dataTarget.size(), 0);
}

bool ZipSource::loadResourceLimited(const std::string & resourceName, std::vector<char> & dataTarget)
{
	if (!archive || resourceName == PACKAGE_HASH)
	{
		return AbstractSource::loadResourceLimited(resourceName, dataTarget);
	}

	auto index = mz_zip_reader_locate_file(archive.get(), resourceName.c_str(), nullptr, 0);
	if (index < 0)
	{
		return false;
	}

	mz_zip_archive_file_stat fileInfo = {};
	if (!mz_zip_reader_file_stat(archive.get(), index, &fileInfo))
	{
		return false;
	}

	if (fileInfo.m_is_directory || !fileInfo.m_is_supported)
	{
		return false;
	}

	PartialExtraction partial;
	partial.target = &dataTarget;
	partial.limit = dataTarget.size();
	dataTarget.clear();
	bool success =
	    mz_zip_reader_extract_to_callback(archive.get(), index, &ZipSource::zipSourceExtractCallback, &partial, 0);
	return success || dataTarget.size() == partial.limit;
}

bool ZipSource::resourceExists(const std::string & resourceName) const
{
	if (!archive)
	{
		logger.warn("Attempt to use uninitialized zip source");
		return false;
	}

	auto index = mz_zip_reader_locate_file(archive.get(), resourceName.c_str(), nullptr, 0);
	if (index < 0)
	{
		return false;
	}

	mz_zip_archive_file_stat fileInfo = {};
	if (!mz_zip_reader_file_stat(archive.get(), index, &fileInfo))
	{
		return false;
	}

	return !fileInfo.m_is_directory && fileInfo.m_is_supported;
}

std::vector<std::string> ZipSource::getResourceList(std::string prefix, fs::ListFlags flags) const
{
	if (!archive)
	{
		logger.warn("Attempt to use uninitialized zip source");
		return {};
	}

	if (fileListCache.empty())
	{
		std::size_t fileCount = mz_zip_reader_get_num_files(archive.get());
		if (fileCount > 0)
		{
			fileListCache.reserve(fileCount);

			HashSet<std::string> directories;

			for (std::size_t index = 0; index < fileCount; ++index)
			{
				std::array<char, 256> filename;
				std::size_t length = mz_zip_reader_get_filename(archive.get(), index, filename.data(), filename.size());
				if (length > 0 && !mz_zip_reader_is_file_a_directory(archive.get(), index))
				{
					std::string name = normalizeResourceName(std::string(filename.data()));
					fileListCache.push_back(name);

					auto pos = name.find_first_of('/');
					while (pos != std::string::npos)
					{
						directories.insert(name.substr(0, pos));
						pos = name.find_first_of('/', pos + 1);
					}
				}
			}

			directoryListCache.assign(directories.begin(), directories.end());

			std::sort(fileListCache.begin(), fileListCache.end());
			std::sort(directoryListCache.begin(), directoryListCache.end());
		}
	}

	if (!prefix.empty())
	{
		prefix = normalizeResourceName(prefix + "/");
	}

	std::vector<std::string> resourceList;

	using namespace fs;

	bool recursive = flags & ListRecursive;

	auto addEntriesFromCache = [&](const std::vector<std::string> & cache)
	{
		for (const auto & resourceName : cache)
		{
			if (stringStartsWith(resourceName, prefix) && resourceName.size() > prefix.size()
			    && (recursive || resourceName.find_first_of('/', prefix.size()) == std::string::npos))
			{
				resourceList.push_back((flags & ListFullPath) ? resourceName : resourceName.substr(prefix.size()));
			}
		}
	};

	if (flags & ListDirectories)
	{
		addEntriesFromCache(directoryListCache);
	}

	if (flags & ListFiles)
	{
		addEntriesFromCache(fileListCache);
	}

	return resourceList;
}

void ZipSource::updateHash()
{
	if (zipHash.empty() && file)
	{
		auto offset = ftell(file);
		fseek(file, 0, SEEK_SET);

		blake2s_state hashState;
		blake2s_init(&hashState, BLAKE2S_OUTBYTES);

		std::vector<char> buffer(65536);
		std::size_t readBytes = 0;
		do
		{
			readBytes = fread(buffer.data(), 1, buffer.size(), file);
			blake2s_update(&hashState, buffer.data(), std::min<std::size_t>(readBytes, buffer.size()));
		} while (readBytes != 0);

		zipHash.resize(BLAKE2S_OUTBYTES);
		blake2s_final(&hashState, zipHash.data(), zipHash.size());

		fseek(file, offset, SEEK_SET);
	}
}

size_t ZipSource::zipSourceExtractCallback(void * pOpaque, mz_uint64 file_ofs, const void * pBuf, size_t n)
{
	auto * partial = static_cast<PartialExtraction *>(pOpaque);
	if (file_ofs >= partial->limit)
	{
		return 0;
	}

	n = std::min<std::size_t>(n, partial->limit - file_ofs);
	partial->target->resize(partial->target->size() + n);
	std::memcpy(partial->target->data() + file_ofs, pBuf, n);
	return n;
}
}
