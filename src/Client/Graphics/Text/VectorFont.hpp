#ifndef SRC_CLIENT_GRAPHICS_TEXT_VECTORFONT_HPP_
#define SRC_CLIENT_GRAPHICS_TEXT_VECTORFONT_HPP_

#include <Client/Graphics/Text/AbstractFont.hpp>
#include <Shared/Utils/HashTable.hpp>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Transform.hpp>

#include <memory>

namespace wos
{
namespace text
{

class VectorFont : public AbstractFont
{
public:
	VectorFont(std::shared_ptr<sf::Font> font, unsigned int characterSize, bool bold);

	std::shared_ptr<sf::Font> getFont() const;
	unsigned int getCharacterSize() const;
	bool isBold() const;

	void setTextureID(TextureID textureID);
	TextureID getTextureID() const;

	const Glyph & getGlyph(sf::Uint32 character) const override;
	float getKerning(sf::Uint32 character1, sf::Uint32 character2) const override;
	float getLineSpacing() const override;
	float getSize() const override;
	sf::Vector2f getNativeScale() const override;

private:
	std::shared_ptr<sf::Font> font;
	unsigned int characterSize = 0;
	bool bold = false;
	TextureID textureID = 0;
	float lineSpacing = 0;

	sf::Transform transform;
	sf::Vector2f scale;
	sf::Vector2f nativeScale;

	mutable HashMap<sf::Uint32, Glyph> glyphs;
};

}
}

#endif
