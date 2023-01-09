#ifndef DATA_STREAM_HPP
#define DATA_STREAM_HPP

#include <SFML/Config.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdio>
#include <cstring>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

class DataStream
{

public:
	typedef std::uint64_t SizeType;
	typedef std::int64_t OffsetType;

	DataStream();
	DataStream(const DataStream & strm);
	DataStream & operator=(const DataStream & strm);
	DataStream(DataStream && strm);
	DataStream & operator=(DataStream && strm);

	~DataStream();

	// thrown if list size exceeds current indexing maximum.
	class ListSizeException : public std::exception
	{
	public:
		ListSizeException(unsigned int size, unsigned int maxSize);
		~ListSizeException() throw();
		const char * what() const throw();

	private:
		std::string myText;
	};

	// thrown if indexing size is not a valid value.
	class SizerException : public std::exception
	{
	public:
		const char * what() const throw();
	};

	// opens a stream from various stream sources.
	bool openMemory(const void * data = 0, SizeType size = 0);
	bool openMemory(std::vector<char> && data);
	bool openOutFile(std::string filename);
	bool openInFile(std::string filename);

	// moves the get/set pointer to a specific position, offset or back position.
	void seek(SizeType pos);
	void seekOff(OffsetType off);
	void seekEnd(SizeType revOff = 0);

	// returns the current position of the get/set pointer.
	SizeType tell() const;

	// various status information functions.
	bool endReached() const;
	bool isValid() const;
	bool hasMoreData() const;
	bool isOpen() const;

	// deletes all stream content.
	void clear();

	// closes the stream.
	void close();

	// returns a pointer to a straight array of stream data. does NOT work with files.
	const void * getData() const;

	// returns the total size of the stream.
	SizeType getDataSize() const;

	// overwrites the entire data stream at once.
	bool setData(const void * data, SizeType size);

	// exports/imports to/from various data types.
	bool exportToString(std::string & out, std::size_t maxBytes = std::numeric_limits<std::size_t>::max()) const;
	bool exportToVector(std::vector<char> & out, std::size_t maxBytes = std::numeric_limits<std::size_t>::max()) const;
	bool importFromString(const std::string & in);

	// sets the default size for an indexing type for this stream. for setting the size of single
	// data units, use the Sizer<T> class and functions below. size has to be one of 1, 2 and 4 (word sizes).
	void setIndexSize(std::uint8_t size);

	// returns the default size for an indexing type for this stream. will return one of 1, 2 or 4.
	std::uint8_t getIndexSize() const;

	// stream inject operators: write data to stream and move on.
	DataStream & operator<<(bool data);
	DataStream & operator<<(std::int8_t data);
	DataStream & operator<<(std::uint8_t data);
	DataStream & operator<<(std::int16_t data);
	DataStream & operator<<(std::uint16_t data);
	DataStream & operator<<(std::int32_t data);
	DataStream & operator<<(std::uint32_t data);
	DataStream & operator<<(std::int64_t data);
	DataStream & operator<<(std::uint64_t data);
	DataStream & operator<<(float data);
	DataStream & operator<<(double data);
	DataStream & operator<<(const std::string & data);
	DataStream & operator<<(const std::vector<char> & data);
	DataStream & operator<<(const std::wstring & data);
	template <typename T>
	DataStream & operator<<(const sf::Vector2<T> & data);
	template <typename T>
	DataStream & operator<<(const std::vector<T> & data);
	template <typename T>
	DataStream & operator<<(const std::list<T> & data);
	template <typename T1, typename T2>
	DataStream & operator<<(const std::pair<T1, T2> & data);
	template <typename TK, typename TD>
	DataStream & operator<<(const std::map<TK, TD> & data);

	// stream extract operators: read data from stream and move on.
	DataStream & operator>>(bool & data);
	DataStream & operator>>(std::int8_t & data);
	DataStream & operator>>(std::uint8_t & data);
	DataStream & operator>>(std::int16_t & data);
	DataStream & operator>>(std::uint16_t & data);
	DataStream & operator>>(std::int32_t & data);
	DataStream & operator>>(std::uint32_t & data);
	DataStream & operator>>(std::int64_t & data);
	DataStream & operator>>(std::uint64_t & data);
	DataStream & operator>>(float & data);
	DataStream & operator>>(double & data);
	DataStream & operator>>(std::string & data);
	DataStream & operator>>(std::vector<char> & data);
	DataStream & operator>>(std::wstring & data);
	template <typename T>
	DataStream & operator>>(sf::Vector2<T> & data);
	template <typename T>
	DataStream & operator>>(std::vector<T> & data);
	template <typename T>
	DataStream & operator>>(std::list<T> & data);
	template <typename T1, typename T2>
	DataStream & operator>>(std::pair<T1, T2> & data);
	template <typename TK, typename TD>
	DataStream & operator>>(std::map<TK, TD> & data);

	// automatically changes the list indexing size to the specified byte value (1, 2 or 4).
	template <std::uint8_t size, typename T>
	class Sizer
	{

	private:
		Sizer(T & data) :
			myData(data)
		{
		}

		T & myData;

		friend class DataStream;
	};

	// const version.
	template <std::uint8_t size, typename T>
	class CSizer
	{

	private:
		CSizer(const T & data) :
			myData(data)
		{
		}

		const T & myData;

		friend class DataStream;
	};

	// automatically returns sizers of a specific size.
	template <typename T>
	static Sizer<1, T> Sizer8(T & data);
	template <typename T>
	static Sizer<2, T> Sizer16(T & data);
	template <typename T>
	static Sizer<4, T> Sizer32(T & data);
	template <typename T>
	static CSizer<1, T> Sizer8(const T & data);
	template <typename T>
	static CSizer<2, T> Sizer16(const T & data);
	template <typename T>
	static CSizer<4, T> Sizer32(const T & data);

	// adds or extracts data with the specified list index size.
	template <std::uint8_t size, typename T>
	DataStream & operator<<(Sizer<size, T> data);

	template <std::uint8_t size, typename T>
	DataStream & operator<<(CSizer<size, T> data);

	template <std::uint8_t size, typename T>
	DataStream & operator>>(Sizer<size, T> data);

	// move on past data without reading it.
	template <typename T>
	void skip();

	// peek data without moving on.
	template <typename T>
	void peek(T & data);
	template <typename T>
	DataStream & operator>>=(T & data);

	// appends/overwrites data to/on the current pointer position and moves on. may increase stream size.
	bool addData(const void * data, SizeType bytes);

	// reads data from the current pointer position and moves on.
	bool extractData(void * target, SizeType bytes);

	// appends/extracts a single integer the size of the current indexing type, as defined by the Sizer<T>() wrapper
	// or the setDefaultIndexSize() function.
	void addIndexType(std::uint32_t data);
	std::uint32_t extractIndexType();

private:
	class IndexSizeChanger
	{
	public:
		IndexSizeChanger(DataStream & stream, std::uint8_t size);
		~IndexSizeChanger();

	private:
		DataStream & stream;
		std::uint8_t prevSize;

		friend class DataStream;
	};

	static const SizeType maxFileBufferSize;

	std::vector<char> myData;
	SizeType myPos = 0;
	SizeType myFileSize = 0;

	FILE * myFile = nullptr;
	mutable std::vector<char> myTempData;
	std::string myFileName;

	SizeType myBufferStart = 0;

	bool myIsFile = false, myIsOpen = false, myIsOutputEnabled = false;

	std::uint8_t myIndexSize = 4;
};

#include "DataStream.inl"

#endif
