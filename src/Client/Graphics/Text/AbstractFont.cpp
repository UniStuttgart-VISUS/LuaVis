#include <Client/Graphics/Text/AbstractFont.hpp>

namespace wos
{
namespace text
{

AbstractFont::~AbstractFont()
{
}

sf::Vector2f AbstractFont::getNativeScale() const
{
	return sf::Vector2f(1.f, 1.f);
}

}
}
