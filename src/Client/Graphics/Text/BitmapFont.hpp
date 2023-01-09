#ifndef SRC_CLIENT_GRAPHICS_TEXT_BITMAPFONT_HPP_
#define SRC_CLIENT_GRAPHICS_TEXT_BITMAPFONT_HPP_

#include <Client/Graphics/Text/AbstractFont.hpp>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace wos
{
namespace text
{

class BitmapFont : public AbstractFont
{
public:
	BitmapFont(sf::Image & image);

	void setTextureID(TextureID textureID);
	TextureID getTextureID() const;

	void setTextureRect(sf::IntRect textureRect);
	sf::IntRect getTextureRect() const;

	const Glyph & getGlyph(sf::Uint32 character) const override;
	float getKerning(sf::Uint32 character1, sf::Uint32 character2) const override;
	float getLineSpacing() const override;
	float getSize() const override;

private:
	std::vector<Glyph> glyphs;
	int height = 0;

	TextureID textureID = 0;
	sf::IntRect textureRect;
};

}
}

#endif
