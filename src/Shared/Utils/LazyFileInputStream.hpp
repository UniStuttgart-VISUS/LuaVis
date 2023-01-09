#ifndef SRC_SHARED_UTILS_LAZYFILEINPUTSTREAM_HPP_
#define SRC_SHARED_UTILS_LAZYFILEINPUTSTREAM_HPP_

#include <SFML/System/FileInputStream.hpp>

class LazyFileInputStream : public sf::FileInputStream
{
public:
	LazyFileInputStream();
	virtual ~LazyFileInputStream();

	void openLazy(const std::string & filename);

	virtual sf::Int64 read(void * data, sf::Int64 size) override;
	virtual sf::Int64 seek(sf::Int64 position) override;
	virtual sf::Int64 tell() override;
	virtual sf::Int64 getSize() override;

private:
	void openIfNecessary();

	std::string lazyFile;
};

#endif
