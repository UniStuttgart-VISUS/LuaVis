#include <Client/Graphics/Text/VectorFont.hpp>

#include <cmath>

namespace wos
{
namespace text
{

VectorFont::VectorFont(std::shared_ptr<sf::Font> font, unsigned int characterSize, bool bold) :
    font(font),
    characterSize(characterSize),
    bold(bold)
{
	if (font->isSmooth())
	{
		// Smooth font (Noto)
		transform.translate(0, std::ceil(characterSize - font->getUnderlinePosition(characterSize) * 1.25f));
		scale = sf::Vector2f(1.f, 1.f);
		nativeScale = sf::Vector2f(1.f / 3.f, 1.f / 3.f);
		lineSpacing = font->getLineSpacing(characterSize) * 0.8;
	}
	else
	{
		// Pixel font (Silver)
		transform.translate(0, std::ceil(characterSize - font->getUnderlinePosition(characterSize) * 2));
		transform.scale(2, 2);
		scale.x = transform.transformRect(sf::FloatRect(0, 0, 1, 0)).width;
		scale.y = transform.transformRect(sf::FloatRect(0, 0, 0, 1)).height;
		nativeScale = scale;
		lineSpacing = font->getLineSpacing(characterSize) + 1;
	}
}

std::shared_ptr<sf::Font> VectorFont::getFont() const
{
	return font;
}

unsigned int VectorFont::getCharacterSize() const
{
	return characterSize;
}

bool VectorFont::isBold() const
{
	return bold;
}

void VectorFont::setTextureID(TextureID textureID)
{
	this->textureID = textureID;
	glyphs.clear();
}

TextureID VectorFont::getTextureID() const
{
	return textureID;
}

const Glyph & VectorFont::getGlyph(sf::Uint32 character) const
{
	auto it = glyphs.find(character);
	if (it != glyphs.end())
	{
		return it->second;
	}
	else
	{
		sf::Glyph sfmlGlyph = font->getGlyph(character, characterSize, bold, 0);
		Glyph glyph;
		glyph.valid = font->hasGlyph(character);
		glyph.textureRect = sf::FloatRect(sfmlGlyph.textureRect);
		glyph.textureID = textureID;
		glyph.boundingBox = transform.transformRect(sfmlGlyph.bounds);
		glyph.offsetX = sfmlGlyph.advance * scale.x;
		return glyphs[character] = glyph;
	}
}

float VectorFont::getKerning(sf::Uint32 character1, sf::Uint32 character2) const
{
	return font->getKerning(character1, character2, characterSize) * scale.x;
}

float VectorFont::getLineSpacing() const
{
	return lineSpacing;
}

float VectorFont::getSize() const
{
	return characterSize;
}

sf::Vector2f VectorFont::getNativeScale() const
{
	return nativeScale;
}

}
}
