#ifndef SRC_SHARED_UTILS_PACKAGE_HPP_
#define SRC_SHARED_UTILS_PACKAGE_HPP_

#include <SFML/Config.hpp>
#include <Shared/Utils/DataStream.hpp>
#include <Shared/Utils/StringStream.hpp>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace res
{

class Package
{

public:
	Package();
	~Package();

	bool openFile(const std::string & filename);

	void close();

	// selects the next content in the package. returns false if end was reached.
	bool nextContent();

	// selects the first content in the package. returns false if the package is empty or closed.
	bool firstContent();

	// selects a content by its id. specify a file extension to check for a specific type, or none
	// to check for any type.
	bool select(const std::string & id);

	// clears the content data buffer.
	void deselect();

	// returns true if the package contains a specific content, false otherwise. same extension
	// rules apply as with select().
	bool hasContent(const std::string & id) const;

	// returns information about the currently selected content.
	std::string getContentId() const;
	const std::vector<char> & getContentData() const;
	sf::Uint32 getContentSize() const;

	// returns an rvalue reference to the selected content buffer, allowing it to be moved.
	std::vector<char> && acquireContent();

	std::size_t getContentCount() const;

	// returns the cryptographic hash of the whole package.
	const std::vector<char> & getHash() const;

private:
	static const std::vector<char> returnEmpty;

	sf::Uint32 findContent(const std::string & id) const;
	bool select(sf::Uint32 pos);
	bool readData() const;
	bool readHeader();
	bool updateHash() const;

	mutable DataStream myStream;

	sf::Uint32 myCurrentPosition = 0;
	std::string myCurrentContentId;
	sf::Uint8 myCurrentContentType = 0;
	mutable std::vector<char> myCurrentContentData;
	mutable bool myIsDataRead = false;

	sf::Uint32 myContentCount = 0;
	std::vector<sf::Uint32> myHintTable;

	mutable std::vector<char> myHash;
};

}

#endif
