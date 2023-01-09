#include <Client/Graphics/Text/BitmapFont.hpp>

namespace wos
{
namespace text
{

BitmapFont::BitmapFont(sf::Image & image)
{
	static const sf::Color gridColor(128, 128, 128);

	std::size_t width = 0;
	std::size_t fullHeight = 0;
	std::size_t spacing = 1;

	// Find white pixel coords for underlines
	{
		auto getWhitePixel = [&]() -> sf::Vector2f
		{
			for (std::size_t x = 0; x < image.getSize().x; ++x)
			{
				for (std::size_t y = 0; y < image.getSize().y; ++y)
				{
					if (image.getPixel(x, y) == sf::Color::White)
					{
						return sf::Vector2f(x + 0.5f, y + 0.5f);
					}
				}
			}
			return sf::Vector2f(0, 0);
		};

		Glyph glyph;
		glyph.offsetX = 0;
		glyph.boundingBox = sf::FloatRect(0, 0, 1, 1);
		glyph.textureRect = sf::FloatRect(getWhitePixel(), sf::Vector2f(0, 0));
		glyph.valid = true;

		glyphs.push_back(glyph);
	}


	// Detect maximum character width.
	for (std::size_t x = 0; x < image.getSize().x; ++x)
	{
		if (image.getPixel(x, 1) == gridColor)
		{
			width = x;
			break;
		}
	}

	// Detect character height.
	for (std::size_t y = 0; y < image.getSize().y; ++y)
	{
		if (image.getPixel(0, y) == gridColor)
		{
			fullHeight = y;
			break;
		}
	}

	// Detect character effective height.
	for (std::size_t y = 0; y < image.getSize().y; ++y)
	{
		if (image.getPixel(1, y) == gridColor)
		{
			height = y;
			break;
		}
	}

	// Detect grid thickness.
	for (std::size_t y = fullHeight; y < image.getSize().y; ++y)
	{
		if (image.getPixel(0, y) != gridColor)
		{
			spacing = y - fullHeight;
			break;
		}
	}

	std::size_t charsPerLine = std::max<std::size_t>(1, (image.getSize().x + 1) / (width + 1));

	// true as long as character width scanner is within image bounds.
	bool withinBounds = true;

	// determine character widths.
	for (std::size_t i = 1; withinBounds; i++)
	{
		std::size_t fx = (width + spacing) * (i % charsPerLine), fy = (fullHeight + spacing) * (i / charsPerLine);
		std::size_t off = 0;

		for (off = 0; off < width; off++)
		{
			if (fx + off >= image.getSize().x)
				break;

			if (fy >= image.getSize().y)
			{
				withinBounds = false;
				break;
			}

			if (image.getPixel(fx + off, fy) == gridColor)
				break;
		}

		if (withinBounds)
		{
			Glyph glyph;
			glyph.offsetX = off + 1;
			glyph.boundingBox = sf::FloatRect(0, int(height) - int(fullHeight), off, fullHeight);
			glyph.textureRect = sf::FloatRect(fx, fy, off, fullHeight);
			glyph.valid = true;

			glyphs.push_back(glyph);
		}
	}

	// make all background (black) and grid pixels transparent.
	image.createMaskFromColor(sf::Color(0, 0, 0));
	image.createMaskFromColor(gridColor);
}

void BitmapFont::setTextureID(TextureID textureID)
{
	if (this->textureID != textureID)
	{
		this->textureID = textureID;
		for (auto & glyph : glyphs)
		{
			glyph.textureID = textureID;
		}
	}
}

TextureID BitmapFont::getTextureID() const
{
	return textureID;
}

void BitmapFont::setTextureRect(sf::IntRect textureRect)
{
	if (this->textureRect != textureRect)
	{
		for (auto & glyph : glyphs)
		{
			glyph.textureRect.left += textureRect.left - this->textureRect.left;
			glyph.textureRect.top += textureRect.top - this->textureRect.top;
		}
		this->textureRect = textureRect;
	}
}

sf::IntRect BitmapFont::getTextureRect() const
{
	return textureRect;
}

const Glyph & BitmapFont::getGlyph(sf::Uint32 character) const
{
	static Glyph empty;
	return character < glyphs.size() ? glyphs[character] : empty;
}

float BitmapFont::getKerning(sf::Uint32 character1, sf::Uint32 character2) const
{
	return 0;
}

float BitmapFont::getLineSpacing() const
{
	return height + 1;
}

float BitmapFont::getSize() const
{
	return height;
}

}
}
