#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/Endian.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <Shared/Utils/OSDetect.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <algorithm>
#include <iostream>
#include <limits>

#ifdef WOS_WINDOWS
#	include <cppfs/windows/FileNameConversions.h>
#endif

const DataStream::SizeType DataStream::maxFileBufferSize = 4096;

DataStream::DataStream()
{
	myIsFile = false;
	myIsOutputEnabled = false;

	setIndexSize(4);
	close();
}
DataStream::DataStream(const DataStream & strm)
{
	myIsFile = false;
	myIsOutputEnabled = false;

	*this = strm;
}
DataStream & DataStream::operator=(const DataStream & strm)
{
	close();

	myFileName = strm.myFileName;
	myIsFile = strm.myIsFile;
	myIsOutputEnabled = strm.myIsOutputEnabled;
	myData = strm.myData;
	myBufferStart = strm.myBufferStart;
	myFileSize = strm.myFileSize;

	if (myIsFile)
	{
		if (myIsOutputEnabled)
			openOutFile(myFileName);
		else
			openInFile(myFileName);
	}

	seek(strm.tell());

	return *this;
}
DataStream::DataStream(DataStream && strm)
{
	myIsFile = false;
	myIsOutputEnabled = false;

	*this = std::move(strm);
}
DataStream & DataStream::operator=(DataStream && strm)
{
	myData = std::move(strm.myData);
	myPos = std::move(strm.myPos);
	myFileSize = std::move(strm.myFileSize);

	myFile = std::move(strm.myFile);
	myTempData = std::move(strm.myTempData);
	myFileName = std::move(strm.myFileName);

	myBufferStart = std::move(strm.myBufferStart);

	myIsFile = std::move(strm.myIsFile);
	myIsOpen = std::move(strm.myIsOpen);
	myIsOutputEnabled = std::move(strm.myIsOutputEnabled);

	myIndexSize = std::move(strm.myIndexSize);

	return *this;
}
DataStream::~DataStream()
{
	if (myFile)
	{
		std::fclose(myFile);
	}
}

DataStream::ListSizeException::ListSizeException(unsigned int size, unsigned int maxSize)
{
	myText = "DataStream: Indexing size (" + cNtoS(size) + ") exceeds maximum allowed value (" + cNtoS(maxSize) + ").";
}

DataStream::ListSizeException::~ListSizeException() throw()
{
}

const char * DataStream::ListSizeException::what() const throw()
{
	return myText.c_str();
}

const char * DataStream::SizerException::what() const throw()
{
	return "DataStream: Invalid indexing size: must be one of 1, 2 or 4.";
}

bool DataStream::openMemory(const void * data, SizeType size)
{
	myTempData.clear();

	myIsFile = false;
	myIsOutputEnabled = false;
	myIsOpen = true;
	myBufferStart = 0;

	if (myFile)
	{
		std::fclose(myFile);
		myFile = nullptr;
	}

	if (data && size)
		setData(data, size);
	else
		myData.clear();

	seek(0);

	return true;
}
bool DataStream::openMemory(std::vector<char> && data)
{
	myTempData.clear();

	myIsFile = false;
	myIsOutputEnabled = false;
	myIsOpen = true;
	myBufferStart = 0;

	if (myFile)
	{
		std::fclose(myFile);
		myFile = nullptr;
	}

	myData = std::move(data);

	seek(0);

	return true;
}
bool DataStream::openOutFile(std::string filename)
{
	close();

	myFileName = filename;

#ifdef WOS_WINDOWS
	std::wstring wideFilename = cppfs::convert::utf8ToWideString(filename);
	myFile = _wfopen(wideFilename.c_str(), L"wb");
#else
	myFile = fopen(filename.c_str(), "wb");
#endif

	if (!myFile)
	{
		return false;
	}

	myIsFile = true;
	myIsOutputEnabled = true;
	myIsOpen = true;
	myBufferStart = 0;

	seek(0);

	return true;
}
bool DataStream::openInFile(std::string filename)
{
	close();

	myFileName = filename;

#ifdef WOS_WINDOWS
	std::wstring wideFilename = cppfs::convert::utf8ToWideString(filename);
	myFile = _wfopen(wideFilename.c_str(), L"rb");
#else
	myFile = fopen(filename.c_str(), "rb");
#endif

	if (!myFile)
	{
		return false;
	}

	fseek(myFile, 0, SEEK_END);
	auto fileSize = ftell(myFile);

	// Apparently, ftell on Linux returns 2^63 - 1 when used on a directory
	if (fileSize < 0 || fileSize == std::numeric_limits<decltype(fileSize)>::max())
	{
		return false;
	}

	myIsFile = true;
	myIsOutputEnabled = false;
	myIsOpen = true;
	myBufferStart = 0;
	myFileSize = fileSize;

	seek(0);

	return true;
}

void DataStream::seek(SizeType pos)
{
	myPos = pos;
}
void DataStream::seekOff(OffsetType off)
{
	myPos += off;
}
void DataStream::seekEnd(SizeType revOff)
{
	myPos = getDataSize() - revOff;
}
DataStream::SizeType DataStream::tell() const
{
	return myPos;
}

void DataStream::clear()
{
	if (myIsFile)
	{
		if (myIsOutputEnabled)
			openOutFile(myFileName);
	}
	else
	{
		myData.clear();
	}
	seek(0);
}

void DataStream::close()
{
	openMemory();
	myIsOpen = false;
}

bool DataStream::isValid() const
{
	return isOpen() && tell() <= getDataSize();
}

bool DataStream::endReached() const
{
	return !isOpen() || tell() == getDataSize();
}

bool DataStream::hasMoreData() const
{
	return isOpen() && tell() < getDataSize();
}

bool DataStream::isOpen() const
{
	return myIsOpen;
}

const void * DataStream::getData() const
{
	if (myIsFile)
	{
		if (myTempData.empty())
		{
			SizeType size = getDataSize();
			myTempData.resize(size);
			fseek(myFile, 0, SEEK_SET);
			std::size_t result = fread(myTempData.data(), 1, size, myFile);
			(void) result;
		}
		return myTempData.data();
	}
	else
		return myData.data();
}
DataStream::SizeType DataStream::getDataSize() const
{
	if (myIsFile)
		return myFileSize;
	else
		return myData.size();
}
bool DataStream::setData(const void * data, SizeType size)
{
	if (myIsFile)
	{
		if (myIsOutputEnabled)
		{
			clear();
			bool success = (fwrite(data, 1, size, myFile) == size);
			seek(0);
			return success;
		}
		return false;
	}
	else
	{
		myData.assign((const char *) data, (const char *) data + size);
		seek(0);
		return true;
	}
}

bool DataStream::addData(const void * data, SizeType bytes)
{
	// open data stream if it is not opened yet.
	if (!isOpen())
		openMemory();

	if (bytes == 0)
	{
		return true;
	}
	else if (data == nullptr)
	{
		return false;
	}

	if (myIsFile)
	{
		if (myIsOutputEnabled)
		{
			fseek(myFile, tell(), SEEK_SET);
			std::size_t writtenBytes = fwrite(data, 1, bytes, myFile);
			myFileSize += writtenBytes;
			seekOff(writtenBytes);
			return writtenBytes == bytes;
		}
		else
		{
			return false;
		}
	}
	else
	{
		myData.resize(std::max<std::size_t>(myData.size(), tell() + bytes));
		std::memcpy(&myData[tell()], data, bytes);
		seekOff(bytes);
		return true;
	}
}

bool DataStream::extractData(void * target, SizeType bytes)
{
	if (bytes == 0)
	{
		return true;
	}
	else if (target == nullptr)
	{
		return false;
	}

	if (endReached() || !isValid())
	{
		seekOff(bytes);
		return false;
	}

	bool success = false;

	if (myIsFile)
	{
		fseek(myFile, tell(), SEEK_SET);
		std::size_t readBytes = fread(target, 1, bytes, myFile);
		seekOff(readBytes);
		return readBytes == bytes;
	}
	else
	{
		// read (possibly partial) data from buffer.
		if (tell() < myData.size())
		{
			std::memcpy(target, &myData[tell()], std::min(bytes, myData.size() - tell()));
		}
		seekOff(bytes);
		return true;
	}
}

void DataStream::setIndexSize(std::uint8_t size)
{
	if (size != 1 && size != 2 && size != 4)
		throw(SizerException());

	myIndexSize = size;
}

std::uint8_t DataStream::getIndexSize() const
{
	return myIndexSize;
}

void DataStream::addIndexType(std::uint32_t data)
{
	if (getIndexSize() == 1)
	{
		if (data > std::numeric_limits<std::uint8_t>::max())
			throw(ListSizeException(data, std::numeric_limits<std::uint8_t>::max()));
		*this << (std::uint8_t) data;
	}
	else if (getIndexSize() == 2)
	{
		if (data > std::numeric_limits<std::uint16_t>::max())
			throw(ListSizeException(data, std::numeric_limits<std::uint16_t>::max()));
		*this << (std::uint16_t) data;
	}
	else if (getIndexSize() == 4)
	{
		*this << data;
	}
	else
		throw(SizerException());
}

std::uint32_t DataStream::extractIndexType()
{
	if (getIndexSize() == 1)
	{
		std::uint8_t data = 0;
		*this >> data;
		return data;
	}
	else if (getIndexSize() == 2)
	{
		std::uint16_t data = 0;
		*this >> data;
		return data;
	}
	else if (getIndexSize() == 4)
	{
		std::uint32_t data = 0;
		*this >> data;
		return data;
	}
	else
		throw(SizerException());
}

DataStream::IndexSizeChanger::IndexSizeChanger(DataStream & stream, std::uint8_t size) :
	stream(stream),
	prevSize(stream.getIndexSize())
{
	stream.setIndexSize(size);
}

DataStream::IndexSizeChanger::~IndexSizeChanger()
{
	stream.setIndexSize(prevSize);
}

bool DataStream::exportToString(std::string & out, std::size_t maxBytes) const
{
	out.resize(std::min<std::size_t>(getDataSize(), maxBytes));
	if (!out.empty())
	{
		if (myIsFile)
		{
			fseek(myFile, 0, SEEK_SET);
			return fread(&out[0], 1, out.size(), myFile) == out.size();
		}
		else
		{
			std::memcpy(&out[0], myData.data(), out.size());
		}
	}
	return true;
}

bool DataStream::exportToVector(std::vector<char> & out, std::size_t maxBytes) const
{
	out.resize(std::min<std::size_t>(getDataSize(), maxBytes));
	if (!out.empty())
	{
		if (myIsFile)
		{
			fseek(myFile, 0, SEEK_SET);
			return fread(out.data(), 1, out.size(), myFile) == out.size();
		}
		else
		{
			std::memcpy(out.data(), myData.data(), out.size());
		}
	}
	return true;
}

bool DataStream::importFromString(const std::string & in)
{
	setData(&in[0], in.size());
	return true;
}

DataStream & DataStream::operator<<(bool data)
{
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::int8_t data)
{
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::uint8_t data)
{
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::int16_t data)
{
	data = n2hs(data);
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::uint16_t data)
{
	data = n2hs(data);
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::int32_t data)
{
	data = n2hl(data);
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::uint32_t data)
{
	data = n2hl(data);
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::int64_t data)
{
	data = n2hll(data);
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(std::uint64_t data)
{
	data = n2hll(data);
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(float data)
{
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(double data)
{
	addData(&data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator<<(const std::string & data)
{
	addIndexType(data.size());
	addData(data.data(), data.size());

	return *this;
}
DataStream & DataStream::operator<<(const std::vector<char> & data)
{
	addIndexType(data.size());
	addData(data.data(), data.size());

	return *this;
}
DataStream & DataStream::operator<<(const std::wstring & data)
{
	return *this;
}

DataStream & DataStream::operator>>(bool & data)
{
	extractData((void *) &data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator>>(std::int8_t & data)
{
	extractData((void *) &data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator>>(std::uint8_t & data)
{
	extractData((void *) &data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator>>(std::int16_t & data)
{
	extractData((void *) &data, sizeof(data));
	data = n2hs(data);
	return *this;
}
DataStream & DataStream::operator>>(std::uint16_t & data)
{
	extractData((void *) &data, sizeof(data));
	data = n2hs(data);
	return *this;
}
DataStream & DataStream::operator>>(std::int32_t & data)
{
	extractData((void *) &data, sizeof(data));
	data = n2hl(data);
	return *this;
}
DataStream & DataStream::operator>>(std::uint32_t & data)
{
	extractData((void *) &data, sizeof(data));
	data = n2hl(data);
	return *this;
}
DataStream & DataStream::operator>>(std::int64_t & data)
{
	extractData((void *) &data, sizeof(data));
	data = n2hll(data);
	return *this;
}
DataStream & DataStream::operator>>(std::uint64_t & data)
{
	extractData((void *) &data, sizeof(data));
	data = n2hll(data);
	return *this;
}
DataStream & DataStream::operator>>(float & data)
{
	extractData((void *) &data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator>>(double & data)
{
	extractData((void *) &data, sizeof(data));
	return *this;
}
DataStream & DataStream::operator>>(std::string & data)
{
	std::uint32_t size = extractIndexType();
	char * buf = new char[size];
	extractData(buf, size);
	data.assign(buf, size);
	delete[] buf;

	return *this;
}
DataStream & DataStream::operator>>(std::vector<char> & data)
{
	data.resize(extractIndexType());
	extractData(&data[0], data.size());

	return *this;
}
DataStream & DataStream::operator>>(std::wstring & data)
{
	return *this;
}
