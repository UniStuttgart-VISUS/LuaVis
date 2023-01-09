#include <Shared/Content/Detail/PackageFormat.hpp>
#include <Shared/Content/Package.hpp>
#include <Shared/Utils/Hash.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Shared/Utils/Zlib.hpp>
#include <algorithm>
#include <blake2/blake2.h>
#include <cstdio>
#include <deque>
#include <iostream>
#include <iterator>

namespace format = res::detail::PackageFormat;

namespace res
{

const std::vector<char> Package::returnEmpty;

Package::Package()
{
	myStream.setIndexSize(2);
	close();
}
Package::~Package()
{
}

bool Package::openFile(const std::string & filename)
{
	close();

	if (!myStream.openInFile(filename))
		return false;

	if (readHeader())
	{
		if (select("package.id"))
		{
			myHash = acquireContent();
			deselect();
		}

		return true;
	}
	else
	{
		close();
		return false;
	}
}

void Package::close()
{
	myContentCount = 0;
	myCurrentPosition = 0;
	myCurrentContentId.clear();
	myCurrentContentData.clear();
	myStream.close();
	myHash.clear();
}

bool Package::nextContent()
{
	sf::Uint32 contentSize;

	myStream.seek(myCurrentPosition);
	myStream.skip<std::string>(); // id.
	myStream.skip<sf::Uint8>();   // type.
	myStream.skip<bool>();        // compression.
	myStream >> contentSize;

	myStream.seekOff(contentSize);

	myCurrentPosition = myStream.tell();

	return select(myCurrentPosition);
}
bool Package::firstContent()
{
	myCurrentPosition = format::dataBegin;

	return select(myCurrentPosition);
}

bool Package::select(const std::string & id)
{
	return select(findContent(id));
}
bool Package::select(sf::Uint32 pos)
{
	myCurrentContentId.clear();
	myCurrentContentData.clear();
	myIsDataRead = false;

	myCurrentPosition = pos;

	if (!pos)
		return false;

	bool contentCompression;
	sf::Uint32 contentSize;

	myStream.seek(pos);
	myStream >> myCurrentContentId;
	myStream >> myCurrentContentType;
	myStream >> contentCompression;
	myStream >> contentSize;

	if (!myStream.isValid())
	{
		myCurrentContentId.clear();
		return false;
	}

	return true;
}
bool Package::readData() const
{
	if (myIsDataRead)
		return true;

	bool contentCompression;
	sf::Uint32 contentSize;

	myStream.seek(myCurrentPosition);
	myStream.skip<std::string>(); // name.
	myStream.skip<sf::Uint8>();   // content type
	myStream >> contentCompression;
	myStream >> contentSize;

	myIsDataRead = true;

	if (!myStream.isValid())
	{
		return false;
	}

	myCurrentContentData.resize(contentSize);
	myStream.extractData(&myCurrentContentData[0], contentSize);

	if (contentCompression && !zu::decompress(myCurrentContentData))
	{
		myCurrentContentData.clear();
		return false;
	}

	return true;
}

void Package::deselect()
{
	myCurrentContentId.clear();
	myCurrentContentData.clear();
}

bool Package::hasContent(const std::string & id) const
{
	return findContent(id) != 0;
}

std::string Package::getContentId() const
{
	if (!myStream.isOpen() || myCurrentContentId.empty())
		return "";

	return myCurrentContentId;
}

const std::vector<char> & Package::getContentData() const
{
	if (!myStream.isOpen() || myCurrentContentId.empty())
		return returnEmpty;

	readData();

	return myCurrentContentData;
}

sf::Uint32 Package::getContentSize() const
{
	if (!myStream.isOpen() || myCurrentContentId.empty())
		return 0;

	readData();

	return myCurrentContentData.size();
}

std::vector<char> && Package::acquireContent()
{
	if (myStream.isOpen() && !myCurrentContentId.empty())
		readData();

	myIsDataRead = false;

	return std::move(myCurrentContentData);
}

std::size_t Package::getContentCount() const
{
	return myContentCount;
}

const std::vector<char> & Package::getHash() const
{
	if (myHash.empty() && myStream.isOpen())
	{
		updateHash();
		myStream.seek(myCurrentPosition);
	}

	return myHash;
}

sf::Uint32 Package::findContent(const std::string & id) const
{
	if (!myStream.isOpen() || id.empty())
		return 0;

	bool hasExtension = !extractFileExtension(id).empty();
	sf::Uint8 hint = format::nameHint(id);
	sf::Uint32 hintPos = myHintTable[hint];

	if (hintPos == 0) // no content with this hash hint exists.
		return 0;

	// jump to hinted position.
	myStream.seek(hintPos);

	sf::Uint32 currentPosition;
	std::string currentId;
	sf::Uint8 currentType;
	bool curCompressed;
	sf::Uint32 currentSize;

	while (true)
	{
		currentPosition = myStream.tell();

		// read id from current position.
		myStream >> currentId;

		// ignore content type...
		myStream >> currentType;

		// ignore content compression...
		myStream >> curCompressed;

		// read content size from current position.
		myStream >> currentSize;

		// if input string is extensionless, compare found entry and without extensions.
		if (hasExtension ? (currentId == id)
		                 : (format::fileNameToHashable(currentId) == format::fileNameToHashable(id)))
		{
			// found you!
			return currentPosition;
		}
		else if (format::nameHint(currentId) != hint)
		{
			// hash no longer matches, stop searching.
			return 0;
		}
		else
		{
			// advance by content size.
			myStream.seekOff(currentSize);
		}

		// end of stream? do not bother wrapping around, hash will not match anyway.
		if (myStream.endReached() || !myStream.isValid())
		{
			return 0;
		}
	};

	return 0;
}

bool Package::readHeader()
{
	std::string header(format::headerMagic.size(), '\0');
	myStream.seek(0);
	myStream.extractData(&header[0], format::headerMagic.size());

	if (header != format::headerMagic)
		return false;

	myStream >> myContentCount;

	if (!myStream.isValid())
		return false;

	myHintTable.resize(format::tableSize);

	for (unsigned int i = 0; i < format::tableSize; ++i)
		myStream >> myHintTable[i];

	return myStream.isValid();
}

bool Package::updateHash() const
{
	myStream.seek(0);

	blake2s_state hashState;
	blake2s_init(&hashState, BLAKE2S_OUTBYTES);

	std::vector<char> buffer(65536);
	for (std::size_t i = 0; i < myStream.getDataSize() / buffer.size(); ++i)
	{
		if (!myStream.extractData(buffer.data(), buffer.size()))
		{
			return false;
		}

		blake2s_update(&hashState, buffer.data(), buffer.size());
	}

	std::size_t remainder = myStream.getDataSize() % buffer.size();
	if (remainder != 0)
	{
		if (!myStream.extractData(buffer.data(), remainder))
		{
			return false;
		}

		blake2s_update(&hashState, buffer.data(), remainder);
	}

	myHash.resize(BLAKE2S_OUTBYTES);
	blake2s_final(&hashState, myHash.data(), myHash.size());

	return true;
}

}
