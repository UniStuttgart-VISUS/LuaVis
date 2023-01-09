#include <Client/Graphics/Text/Text.hpp>
#include <Client/Graphics/UtilitiesSf.hpp>
#include <Shared/Utils/StrNumCon.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Shared/Utils/VectorMul.hpp>

namespace wos
{
namespace text
{

Text::Text() :
	cache(*this)
{
}


void Text::setString(sf::String string)
{
	if (this->string != string)
	{
		this->string = string;
		cache.hasPendingFullUpdate = true;
	}
}

const sf::String & Text::getString() const
{
	return string;
}

void Text::setColor(sf::Color color)
{
	if (this->color != color)
	{
		this->color = color;
		if (cache.hasColorModifiers)
		{
			cache.hasPendingFullUpdate = true;
		}
		else
		{
			cache.hasPendingUpdate = true;
		}
	}
}

sf::Color Text::getColor() const
{
	return color;
}

void Text::setAlignment(sf::Vector2f alignment)
{
	if (this->alignment != alignment)
	{
		this->alignment = alignment;
		cache.hasPendingUpdate = true;
	}
}

sf::Vector2f Text::getAlignment() const
{
	return alignment;
}

void Text::setMaximumSize(sf::Vector2f maximumSize)
{
	if (this->maximumSize != maximumSize)
	{
		this->maximumSize = maximumSize;
		if (wordWrapEnabled)
		{
			cache.hasPendingFullUpdate = true;
		}
		else
		{
			cache.hasPendingUpdate = true;
		}
	}
}

sf::Vector2f Text::getMaximumSize() const
{
	return maximumSize;
}

sf::Vector2f Text::getMaximumSizeScaleFactor() const
{
	cache.updateIfNecessary();
	return cache.maxSizeScale;
}

void Text::setSpacing(sf::Vector2f spacing)
{
	if (this->spacing != spacing)
	{
		this->spacing = spacing;
		cache.hasPendingFullUpdate = true;
	}
}

sf::Vector2f Text::getSpacing() const
{
	return spacing;
}

void Text::setPadding(sf::FloatRect padding)
{
	if (this->padding != padding)
	{
		this->padding = padding;
		cache.hasPendingFullUpdate = true;
	}
}

sf::FloatRect Text::getPadding() const
{
	return padding;
}

void Text::setSizeCorrection(float sizeCorrection)
{
	if (this->sizeCorrection != sizeCorrection)
	{
		this->sizeCorrection = sizeCorrection;
		cache.hasPendingFullUpdate = true;
	}
}

float Text::getSizeCorrection() const
{
	return sizeCorrection;
}

void Text::setWordWrapEnabled(bool wordWrapEnabled)
{
	if (this->wordWrapEnabled != wordWrapEnabled)
	{
		this->wordWrapEnabled = wordWrapEnabled;
		cache.hasPendingFullUpdate = true;
	}
}

bool Text::isWordWrapEnabled() const
{
	return wordWrapEnabled;
}

void Text::setColorModifierEnabled(bool colorModifierEnabled)
{
	if (this->colorModifierEnabled != colorModifierEnabled)
	{
		this->colorModifierEnabled = colorModifierEnabled;
		if (cache.hasColorModifiers)
		{
			cache.hasPendingFullUpdate = true;
		}
	}
}

bool Text::isColorModifierEnabled() const
{
	return colorModifierEnabled;
}

void Text::setIconDisplayEnabled(bool iconDisplayEnabled)
{
	if (this->iconDisplayEnabled != iconDisplayEnabled)
	{
		this->iconDisplayEnabled = iconDisplayEnabled;
		if (cache.hasIcons)
		{
			cache.hasPendingFullUpdate = true;
		}
	}
}

bool Text::isIconDisplayEnabled() const
{
	return iconDisplayEnabled;
}

void Text::setFont(std::shared_ptr<AbstractFont> font)
{
	if (this->font != font)
	{
		this->font = font;
		cache.hasPendingFullUpdate = true;
	}
}

std::shared_ptr<AbstractFont> Text::getFont() const
{
	return font;
}

const std::vector<sf::Vertex> & Text::getVertices() const
{
	cache.updateIfNecessary();
	return cache.vertices;
}

const std::vector<TextureID> & Text::getTextureIDs() const
{
	cache.updateIfNecessary();
	return cache.textureIDs;
}

const std::vector<Text::Cursor> & Text::getCursors() const
{
	cache.updateIfNecessary();
	return cache.cursors;
}

const sf::FloatRect & Text::getBoundingBox() const
{
	cache.updateIfNecessary();
	return cache.boundingBox;
}

Text::Cache::Cache(const Text & text) :
	text(text)
{
}

void Text::Cache::updateIfNecessary()
{
	if (hasPendingFullUpdate)
	{
		updateRawVertices();
	}
	else if (hasPendingUpdate || !transformEquals(text.getTransform(), transform))
	{
		updateTransformedVertices();
	}
}

void Text::Cache::updateRawVertices()
{
	static const sf::Uint32 formatChar = 3;

	hasPendingFullUpdate = false;
	hasColorModifiers = false;
	hasIcons = false;

	cursors.clear();
	rawVertices.clear();
	textureIDs.clear();
	lines.clear();

	sf::Vector2f maxSize(text.maximumSize.x > 0 ? text.maximumSize.x : std::numeric_limits<float>::infinity(),
	                     text.maximumSize.y > 0 ? text.maximumSize.y : std::numeric_limits<float>::infinity());

	if (text.string.isEmpty() || text.font == nullptr)
	{
		updateTransformedVertices();
		return;
	}

	float x = 0;
	float y = 0;

	bool first = true;
	bool lineBreakNext = false;
	rawBoundingBox = sf::FloatRect();

	Format format;
	format.textColor = text.color;
	std::vector<Format> formatStack;
	bool autoPopFormat = false;

	auto pushColor = [&](sf::Color newColor)
	{
		// Mark text as having color modifiers
		hasColorModifiers = true;

		if (text.colorModifierEnabled)
		{
			// Preserve previous format
			formatStack.push_back(format);
			format.textColor = newColor;
			format.textColor.a = (format.textColor.a * text.color.a) / 255;
		}
	};

	auto pushBackground = [&](sf::Color backgroundColor)
	{
		// Mark text as having color modifiers
		hasColorModifiers = true;

		if (text.colorModifierEnabled)
		{
			// Preserve previous format
			formatStack.push_back(format);
			format.backgroundColor = backgroundColor;
		}
	};

	auto pushUnderline = [&]()
	{
		formatStack.push_back(format);
		format.underline = true;
	};

	auto pushSkip = [&]()
	{
		formatStack.push_back(format);
		format.skip = true;
	};

	auto popFormat = [&]()
	{
		if (!formatStack.empty())
		{
			format = formatStack.back();
			formatStack.pop_back();
		}
	};

	auto renderBox = [&](sf::FloatRect rect, sf::Color color)
	{
		// Get box glyph
		const auto & glyph = text.font->getGlyph(0);

		sf::Vertex tl(rectTopLeft(rect), color, rectTopLeft(glyph.textureRect));
		sf::Vertex tr(rectTopRight(rect), color, rectTopRight(glyph.textureRect));
		sf::Vertex bl(rectBottomLeft(rect), color, rectBottomLeft(glyph.textureRect));
		sf::Vertex br(rectBottomRight(rect), color, rectBottomRight(glyph.textureRect));

		rawVertices.push_back(tl);
		rawVertices.push_back(tr);
		rawVertices.push_back(bl);
		rawVertices.push_back(bl);
		rawVertices.push_back(tr);
		rawVertices.push_back(br);

		textureIDs.push_back(glyph.textureID);
	};

	auto renderGlyph = [&](Glyph glyph) {
		glyph.boundingBox.left += x;
		glyph.boundingBox.top += y;

		float offset = glyph.offsetX + text.spacing.x;

		if (format.backgroundColor.a != 0)
		{
			renderBox(sf::FloatRect(x - 1, y - 1, offset, text.font->getLineSpacing()), format.backgroundColor);
		}

		sf::Vertex tl(rectTopLeft(glyph.boundingBox), format.textColor, rectTopLeft(glyph.textureRect));
		sf::Vertex tr(rectTopRight(glyph.boundingBox), format.textColor, rectTopRight(glyph.textureRect));
		sf::Vertex bl(rectBottomLeft(glyph.boundingBox), format.textColor, rectBottomLeft(glyph.textureRect));
		sf::Vertex br(rectBottomRight(glyph.boundingBox), format.textColor, rectBottomRight(glyph.textureRect));

		if (!format.skip)
		{
			rawVertices.push_back(tl);
			rawVertices.push_back(tr);
			rawVertices.push_back(bl);
			rawVertices.push_back(bl);
			rawVertices.push_back(tr);
			rawVertices.push_back(br);
		}

		textureIDs.push_back(glyph.textureID);

		if (format.underline)
		{
			float dx = format.underlineOffset ? 1 : 0;
			renderBox(sf::FloatRect(x - dx, y + text.font->getLineSpacing(), offset + dx - 1, 1), format.textColor);
			format.underlineOffset = true;
		}

		x += offset;

		if (first)
		{
			first = false;
			rawBoundingBox = glyph.boundingBox;
		}
		else
		{
			rawBoundingBox = rectUnion(rawBoundingBox, glyph.boundingBox);
		}
	};

	for (std::size_t i = 0; i < text.string.getSize(); ++i)
	{
		auto peek = [&](std::size_t offset) {
			return i + offset < text.string.getSize() ? text.string[i + offset] : 0;
		};

		sf::Uint32 character = text.string[i];

		Glyph glyph = text.font->getGlyph(character);

		bool lineBreak = lineBreakNext;
		lineBreakNext = false;

		// Clear temporary formatting applied for previous glyph
		if (autoPopFormat)
		{
			autoPopFormat = false;
			popFormat();
		}

		if (character == sf::Uint32('\n'))
		{
			lineBreak = true;
		}
		else if (character == formatChar)
		{
			auto hexU8 = [](sf::Uint32 c1, sf::Uint32 c2) -> sf::Uint8
			{
				return hex2Num(c1) * 16 + hex2Num(c2);
			};

			auto numericArg = [&](float defaultValue = 0.f) -> float
			{
				std::string value;
				for (std::size_t digitIndex = 1; true; ++i)
				{
					auto digit = peek(digitIndex);
					if ((digit >= '0' && digit <= '9') || digit == '-' || digit == '.')
					{
						value += digit;
					}
					else
					{
						i += digitIndex;
						return value.empty() ? defaultValue : cStoF(value);
					}
				}
			};

			++i;

			// Handle special formatting character
			switch (peek(0))
			{
			case '*':
				// 3-digit HTML color
				pushColor({hexU8(peek(1), peek(1)), hexU8(peek(2), peek(2)), hexU8(peek(3), peek(3))});
				i += 3;
				continue;

			case '#':
				// 6-digit HTML color
				pushColor({hexU8(peek(1), peek(2)), hexU8(peek(3), peek(4)), hexU8(peek(5), peek(6))});
				i += 6;
				continue;

			case 't':
			case 'T':
				// 2-digit alpha modifier
				pushColor({format.textColor.r, format.textColor.g, format.textColor.b, hexU8(peek(1), peek(2))});
				i += 2;
				continue;

			case 'r':
			case 'R':
				// Reset formatting
				popFormat();
				continue;

			case 'i':
			case 'I':
			{
				hasIcons = true;

				// Un-colorize temporarily
				if (text.iconDisplayEnabled)
				{
					pushColor(sf::Color::White);
				}
				else
				{
					pushSkip();
				}
				autoPopFormat = true;

				// Insert image
				glyph = text.font->getGlyph('A');
				glyph.textureID = numericArg();
				glyph.textureRect.left = numericArg();
				glyph.textureRect.top = numericArg();
				glyph.textureRect.width = numericArg();
				glyph.textureRect.height = numericArg();

				float scale = numericArg(-1.f);
				if (scale < 0.f)
				{
					// Negative scale: auto-fit to font size
					scale *= -glyph.boundingBox.height / glyph.textureRect.height;
				}

				glyph.boundingBox =
				    rectSetSizeCentered(glyph.boundingBox, scale * text.sizeCorrection * glyph.textureRect.getSize());
				glyph.boundingBox.left = 0;
				glyph.boundingBox.top -= std::floor(text.font->getNativeScale().y * scale * 0.5f);
				glyph.offsetX = glyph.boundingBox.width;
				break;
			}

			case 'b':
			case 'B':
			{
				// Modify background color
				pushBackground({hexU8(peek(1), peek(2)), hexU8(peek(3), peek(4)), hexU8(peek(5), peek(6)),
				    hexU8(peek(7), peek(8))});
				i += 8;
				continue;
			}

			case 'c':
			case 'C':
			{
				// Save current cursor position
				cursors.emplace_back(rawVertices.size());
				renderBox(sf::FloatRect(x, y, 0, 0), sf::Color::Transparent);
				continue;
			}

			case 'u':
			case 'U':
			{
				// Toggle underline
				pushUnderline();
				continue;
			}

			default:
				continue;
			}
		}
		else if (!lineBreak && text.wordWrapEnabled && text.maximumSize.x > 0)
		{
			// Spaces are reduced to 0-width for line breaks
			float effectiveOffset = (character == sf::Uint32(' ')) ? 0 : glyph.offsetX;

			// If the next word (at a word boundary) or the next character (anywhere) does not fit, add a newline.
			if ((isWordWrappable(character) && x + glyph.offsetX + getWordWidth(i + 1) > maxSize.x)
			    || x + effectiveOffset > maxSize.x)
			{
				lineBreakNext = true;
				glyph.offsetX = effectiveOffset;
			}
		}

		if (lineBreak)
		{
			// Check if next line fits vertical size limit
			float sizeRound = text.sizeCorrection;
			float nextY = y + std::ceil((text.font->getLineSpacing() + text.spacing.y) / sizeRound) * sizeRound;
			if (text.wordWrapEnabled && nextY > maxSize.y)
			{
				// Draw ellipsis (if possible)
				Glyph dot = text.font->getGlyph('.');
				for (int i = 0; i < 3 && x + dot.offsetX < maxSize.x; ++i)
					renderGlyph(dot);
				break;
			}

			// Add line vertex index to allow horizontal multi-line text alignment
			lines.push_back(rawVertices.size());

			// Update position for the next glyph
			x = 0;
			y = nextY;

			// Don't try to draw newlines
			if (character == sf::Uint32('\n'))
			{
				continue;
			}
		}

		// Compute kerning offset between current and next character
		if (auto next = peek(1))
		{
			glyph.offsetX += text.font->getKerning(character, next);
		}

		renderGlyph(std::move(glyph));
	}

	// Align the text's bounding box to the line count, independent of character sizes
	rawBoundingBox.top = 0;
	rawBoundingBox.height = text.font->getSize() + lines.size() * text.font->getLineSpacing();

	// Apply padding to bounding box
	rawBoundingBox.left += text.padding.left;
	rawBoundingBox.top += text.padding.top;
	rawBoundingBox.width += text.padding.width;
	rawBoundingBox.height += text.padding.height;

	updateTransformedVertices();
}

void Text::Cache::updateTransformedVertices()
{
	hasPendingUpdate = false;

	transform = text.getTransform();
	vertices = rawVertices;

	sf::Vector2f alignmentRounding = text.font ? text.font->getNativeScale() : sf::Vector2f(1, 1);

	sf::Vector2f alignmentOffset(
	    std::floor((text.alignment.x * rawBoundingBox.width / alignmentRounding.x + 0.5f)) * alignmentRounding.x,
	    std::floor((text.alignment.y * rawBoundingBox.height / alignmentRounding.y + 0.5f)) * alignmentRounding.y);

	maxSizeScale.x = text.wordWrapEnabled || text.maximumSize.x <= 0
	                     ? 1
	                     : std::min<float>(1, text.maximumSize.x / rawBoundingBox.width);
	maxSizeScale.y = text.wordWrapEnabled || text.maximumSize.y <= 0
	                     ? 1
	                     : std::min<float>(1, text.maximumSize.y / rawBoundingBox.height);

	boundingBox = transform.transformRect(sf::FloatRect((rawBoundingBox.getPosition() - alignmentOffset) * maxSizeScale,
	                                                    maxSizeScale * rawBoundingBox.getSize()));

	for (auto & vertex : vertices)
	{
		vertex.position = transform.transformPoint((vertex.position - alignmentOffset) * maxSizeScale);
		if (!hasColorModifiers)
		{
			vertex.color = text.color;
		}
	}

	// Apply per-line horizontal alignment for multiline texts
	if (!lines.empty() && std::abs(text.alignment.x) > 0.0001f)
	{
		applyAlignment(0, lines[0]);
		for (std::size_t i = 1; i < lines.size(); ++i)
		{
			applyAlignment(lines[i - 1], lines[i]);
		}
		applyAlignment(lines.back(), vertices.size());
	}

	for (auto & cursor : cursors)
	{
		if (cursor.index < vertices.size())
		{
			cursor.position = vertices[cursor.index].position;
		}
	}
}

void Text::Cache::applyAlignment(std::size_t startIndex, std::size_t endIndex)
{
	if (startIndex >= endIndex || endIndex <= 2 || endIndex > vertices.size())
	{
		return;
	}

	// Compute line width via difference in position of leftmost/rightmost text vertices within line
	float width = vertices[endIndex - 2].position.x - vertices[startIndex].position.x;
	float scale = text.getScale().x;
	float offset =
	    std::floor(((boundingBox.width - width) / scale - text.padding.width) * text.alignment.x + 0.5f) * scale;
	for (std::size_t i = startIndex; i < endIndex; ++i)
	{
		vertices[i].position.x += offset;
	}
}

bool Text::Cache::isWordWrappable(sf::Uint32 character) const
{
	switch (character)
	{
	case ' ':
	case '\t':
	case '\n':
	case '-':
	case '+':
	case '/':
		return true;
	default:
		return false;
	}
}

float Text::Cache::getWordWidth(std::size_t index) const
{
	float width = 0;
	for (std::size_t i = index; i < text.string.getSize(); ++i)
	{
		if (isWordWrappable(text.string[i]) || (text.getMaximumSize().x > 0 && width > text.getMaximumSize().x))
		{
			break;
		}
		else
		{
			width += text.font->getGlyph(text.string[i]).offsetX + text.spacing.x;
			if (i + 1 < text.string.getSize())
			{
				width += text.font->getKerning(text.string[i], text.string[i + 1]);
			}
		}
	}
	return width;
}

}
}
