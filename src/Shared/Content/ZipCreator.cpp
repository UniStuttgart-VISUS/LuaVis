#include <Shared/Content/ZipCreator.hpp>

#include <Shared/Utils/DataStream.hpp>

namespace
{

extern "C" size_t wosMinizWriteCallback(void * pOpaque, mz_uint64 file_ofs, const void * pBuf, size_t n)
{
	// According to miniz docs, file_ofs always increases by n, so we do not need it here
	(void) file_ofs;

	DataStream * stream = static_cast<DataStream *>(pOpaque);
	stream->addData(pBuf, n);

	return n;
}

}

namespace res
{

ZipCreator::ZipCreator(std::shared_ptr<DataStream> stream) : stream(stream), archive(std::make_unique<mz_zip_archive>())
{
	archive->m_pWrite = wosMinizWriteCallback;
	archive->m_pIO_opaque = stream.get();

	mz_zip_writer_init(archive.get(), 0);
}

ZipCreator::~ZipCreator()
{
	finalize();
}

void ZipCreator::addDirectory(std::string zipPath)
{
	if (!archive)
	{
		return;
	}

	if (zipPath.empty() || zipPath.back() != '/')
	{
		zipPath.push_back('/');
	}
	mz_zip_writer_add_mem(archive.get(), zipPath.c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION);
}

void ZipCreator::addFromMemory(std::string zipPath, const char * data, std::size_t size)
{
	if (!archive)
	{
		return;
	}

	mz_zip_writer_add_mem(archive.get(), zipPath.c_str(), data, size, MZ_DEFAULT_COMPRESSION);
}

bool ZipCreator::addFromFile(std::string zipPath, std::string filename)
{
	DataStream input;
	if (!input.openInFile(filename))
	{
		return false;
	}

	std::vector<char> buffer;
	if (!input.exportToVector(buffer))
	{
		return false;
	}
	addFromMemory(zipPath, buffer.data(), buffer.size());
	return true;
}

void ZipCreator::finalize()
{
	if (archive)
	{
		mz_zip_writer_finalize_archive(archive.get());
		mz_zip_writer_end(archive.get());

		archive = nullptr;
	}
}

}
