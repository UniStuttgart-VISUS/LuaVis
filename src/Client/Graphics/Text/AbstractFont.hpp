#ifndef SRC_CLIENT_GRAPHICS_TEXT_ABSTRACTFONT_HPP_
#define SRC_CLIENT_GRAPHICS_TEXT_ABSTRACTFONT_HPP_

#include <SFML/Config.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace wos
{
namespace text
{

using TextureID = sf::Int32;

struct Glyph
{
	float offsetX = 0.f;
	sf::FloatRect boundingBox;
	sf::FloatRect textureRect;
	TextureID textureID = 0;
	bool valid = false;
};

class AbstractFont
{
public:
	virtual ~AbstractFont();

	virtual const Glyph & getGlyph(sf::Uint32 character) const = 0;
	virtual float getKerning(sf::Uint32 character1, sf::Uint32 character2) const = 0;
	virtual float getLineSpacing() const = 0;
	virtual float getSize() const = 0;
	virtual sf::Vector2f getNativeScale() const;
};

}
}

#endif
