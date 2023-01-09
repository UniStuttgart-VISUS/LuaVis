#include <Shared/Utils/LazyFileInputStream.hpp>

LazyFileInputStream::LazyFileInputStream()
{
}

LazyFileInputStream::~LazyFileInputStream()
{
}

void LazyFileInputStream::openLazy(const std::string & filename)
{
	lazyFile = filename;
}

sf::Int64 LazyFileInputStream::read(void * data, sf::Int64 size)
{
	openIfNecessary();
	return sf::FileInputStream::read(data, size);
}

sf::Int64 LazyFileInputStream::seek(sf::Int64 position)
{
	openIfNecessary();
	return sf::FileInputStream::seek(position);
}

sf::Int64 LazyFileInputStream::tell()
{
	openIfNecessary();
	return sf::FileInputStream::tell();
}

sf::Int64 LazyFileInputStream::getSize()
{
	openIfNecessary();
	return sf::FileInputStream::getSize();
}

void LazyFileInputStream::openIfNecessary()
{
	if (!lazyFile.empty())
	{
		open(lazyFile);
		lazyFile.clear();
	}
}
