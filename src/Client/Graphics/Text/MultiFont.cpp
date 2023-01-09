#include <Client/Graphics/Text/MultiFont.hpp>

namespace wos
{
namespace text
{
MultiFont::MultiFont(std::vector<std::shared_ptr<AbstractFont>> fonts) :
	fonts(std::move(fonts))
{
	// Determine main font size
	if (!this->fonts.empty())
	{
		size = this->fonts[0]->getSize();
		lineSpacing = this->fonts[0]->getLineSpacing();
		nativeScale = this->fonts[0]->getNativeScale();
	}

	// Scale other fonts relative to main font
	for (auto & font : this->fonts)
	{
		scaleFactors.push_back(size / font->getSize());
	}
}

const std::vector<std::shared_ptr<AbstractFont>> & MultiFont::getFonts() const
{
	return fonts;
}

const Glyph & MultiFont::getGlyph(sf::Uint32 character) const
{
	for (std::size_t i = 0; i < fonts.size(); ++i)
	{
		const Glyph & glyph = fonts[i]->getGlyph(character);
		if (glyph.valid)
		{
			if (i == 0)
			{
				return glyph;
			}

			// For non-default fonts, apply scale factor to resulting bounding boxes
			float scaleFactor = scaleFactors[i];
			scaledGlyph = glyph;
			scaledGlyph.boundingBox.left *= scaleFactor;
			scaledGlyph.boundingBox.top *= scaleFactor;
			scaledGlyph.boundingBox.width *= scaleFactor;
			scaledGlyph.boundingBox.height *= scaleFactor;
			scaledGlyph.offsetX *= scaleFactor;
			return scaledGlyph;
		}
	}

	static const Glyph empty;
	return empty;
}

float MultiFont::getKerning(sf::Uint32 character1, sf::Uint32 character2) const
{
	for (std::size_t i = 0; i < fonts.size(); ++i)
	{
		if (fonts[i]->getGlyph(character1).valid && fonts[i]->getGlyph(character2).valid)
		{
			return fonts[i]->getKerning(character1, character2) * scaleFactors[i];
		}
	}

	return 0;
}

float MultiFont::getLineSpacing() const
{
	return lineSpacing;
}

float MultiFont::getSize() const
{
	return size;
}

sf::Vector2f MultiFont::getNativeScale() const
{
	return nativeScale;
}

}
}
