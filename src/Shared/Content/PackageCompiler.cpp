#include <SFML/Config.hpp>
#include <Shared/Content/Detail/PackageFormat.hpp>
#include <Shared/Content/Package.hpp>
#include <Shared/Content/PackageCompiler.hpp>
#include <Shared/External/spdlog/fmt/fmt.h>
#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Shared/Utils/Zlib.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <iterator>

namespace format = res::detail::PackageFormat;

namespace res
{

bool hashCompare(std::string s1, std::string s2)
{
	auto h1 = format::nameHint(s1);
	auto h2 = format::nameHint(s2);
	return h1 != h2 ? h1 < h2 : format::nameHash(s1) < format::nameHash(s2);
}

PackageCompiler::PackageCompiler() :
	logger("PackageCompiler")
{
	progressCallback = [](int, int) {};

	logCallback = [this](Logger::Level level, std::string message) {
		logger.log(level, "{}", message);
	};
}

void PackageCompiler::setPackageFile(std::string packageFile)
{
	this->packageFile = packageFile;
}

std::string PackageCompiler::getPackageFile() const
{
	return packageFile;
}

void PackageCompiler::setSourcePath(std::string sourcePath)
{
	this->sourcePath = sourcePath;
}

std::string PackageCompiler::getSourcePath() const
{
	return sourcePath;
}

void PackageCompiler::setSourceFiles(std::vector<std::string> sourceFiles)
{
	this->sourceFiles = std::move(sourceFiles);
}

const std::vector<std::string> & PackageCompiler::getSourceFiles() const
{
	return sourceFiles;
}

void PackageCompiler::setLogCallback(LogCallback logCallback)
{
	this->logCallback = logCallback;
}

const PackageCompiler::LogCallback & PackageCompiler::getLogCallback() const
{
	return logCallback;
}

void PackageCompiler::setPermissive(bool permissive)
{
	this->permissive = permissive;
}

bool PackageCompiler::isPermissive() const
{
	return permissive;
}

void PackageCompiler::setProgressCallback(ProgressCallback progressCallback)
{
	this->progressCallback = progressCallback;
}

const PackageCompiler::ProgressCallback & PackageCompiler::getProgressCallback() const
{
	return progressCallback;
}

bool PackageCompiler::compile()
{
	auto logLevelError = permissive ? Logger::Level::Warn : Logger::Level::Error;

	log(Logger::Level::Info, fmt::format("Compiling package from source path '{}'", sourcePath));

	std::sort(sourceFiles.begin(), sourceFiles.end(), &hashCompare);

	DataStream strm;
	strm.setIndexSize(2);

	// open target file.
	if (!strm.openOutFile(packageFile))
	{
		log(Logger::Level::Error, fmt::format("Failed to open package file '{}' for writing", packageFile));
		return false;
	}

	// identifier.
	strm.addData(format::headerMagic.data(), format::headerMagic.size());

	// content count; update this later.
	sf::Uint32 contentCount = 0;
	strm << contentCount;

	// allocate space for the table.
	std::vector<sf::Uint32> hintTable;
	hintTable.resize(format::tableSize);
	std::fill(hintTable.begin(), hintTable.end(), 0);
	strm.addData(hintTable.data(), format::tableSize * sizeof(sf::Uint32));

	// begin adding content.
	for (auto it = sourceFiles.cbegin(); it != sourceFiles.cend(); ++it)
	{
		progressCallback(std::distance(sourceFiles.cbegin(), it), sourceFiles.size());

		// legacy field (content type is deduced from filename instead of being saved separately).
		sf::Uint8 contentType = 0;
		bool contentCompressed = false;

		// determine file size and readability.
		DataStream stream;
		if (!stream.openInFile(sourcePath + "/" + *it))
		{
			log(logLevelError, fmt::format("Failed to open file '{}' for reading", *it));

			if (permissive)
			{
				continue;
			}

			strm.close();
			remove(packageFile.c_str());
			return false;
		}

		sf::Uint64 contentSize = stream.getDataSize();
		if (contentSize > format::maxFileSize)
		{
			log(logLevelError, fmt::format("File '{}' ({}) exceeds maximum size ({})", *it,
			                               getByteSizeString(contentSize), getByteSizeString(format::maxFileSize)));

			if (permissive)
			{
				continue;
			}

			strm.close();
			remove(packageFile.c_str());
			return false;
		}

		// compress all files that at least 100 bytes long.
		if (contentSize >= 100)
		{
			contentCompressed = true;
		}

		// add hint table entry.
		sf::Uint8 curNameHint = format::nameHint(*it);
		if (hintTable[curNameHint] == 0)
		{
			hintTable[curNameHint] = strm.tell();
		}

		// read content data.
		std::vector<char> contentData;
		stream.exportToVector(contentData);
		stream.close();

		// compress (strongly) if necessary.
		if (contentCompressed && !zu::compress(contentData, 8))
		{
			// compression fails? store uncompressed.
			contentCompressed = false;
		}

		// write content header (with size after possible compression).
		strm << *it << contentType << contentCompressed << (sf::Uint32) contentData.size();

		// write content data.
		strm.addData(contentData.data(), contentData.size());

		// log file addition.
		log(Logger::Level::Info, fmt::format("Added file '{}' ({} to {})", *it, getByteSizeString(contentSize),
		                                     getByteSizeString(contentData.size())));

		// increment content count.
		contentCount++;
	}

	// update content size.
	strm.seek(format::headerMagic.size());
	strm << contentCount;

	strm.seek(format::tableBegin);
	for (unsigned int i = 0; i < format::tableSize; ++i)
	{
		strm << hintTable[i];
	}

	strm.close();

	log(Logger::Level::Info, fmt::format("Done! Wrote package file to '{}'.", packageFile));

	return true;
}

void PackageCompiler::log(Logger::Level level, std::string text)
{
	logCallback(level, text);
}

}
