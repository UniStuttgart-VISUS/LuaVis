#include <Client/GUI3/Pieces/Text.hpp>
#include <Client/GUI3/Types.hpp>
#include <Client/GUI3/Utils/Canvas.hpp>
#include <Client/GUI3/Widget.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <Shared/Config/CompositeTypes.hpp>
#include <Shared/Config/Config.hpp>
#include <Shared/Utils/VectorMul.hpp>
#include <algorithm>
#include <cstddef>
#include <vector>

namespace gui3
{

namespace pieces
{

Text::Text() :
	myAlignment(sf::Vector2f(ALIGN_CENTER, ALIGN_CENTER)),
	myMaxSizeBehavior(Ignore),
	myColor(sf::Color::Transparent),
	myIsPositionRounded(false)
{
	setMaxSizeBehavior(Downscale);
	setColor(sf::Color::White);
	setAutoResizeFunction();
}

Text::~Text()
{
}

void Text::paint(Canvas & canvas)
{
	canvas.draw(myText.getVertices().begin(), myText.getVertices().end());
}

void Text::render(sf::RenderTarget & target, sf::RenderStates states)
{
	// TODO reimplement direct text rendering
	// target.draw(myText, states);
}

void Text::setString(std::string text)
{
	if (myString != text)
	{
		myString = text;
		myText.setString(text);
		updateTextPosition();
	}
}

const std::string & Text::getString() const
{
	return myString;
}

void Text::setAlignmentX(float xAlign)
{
	setAlignment(xAlign, getAlignment().y);
}

void Text::setAlignmentY(float yAlign)
{
	setAlignment(getAlignment().x, yAlign);
}

void Text::setAlignment(float xAlign, float yAlign)
{
	setAlignment(sf::Vector2f(xAlign, yAlign));
}

void Text::setAlignment(sf::Vector2f align)
{
	if (myAlignment != align)
	{
		myAlignment = align;
		updateTextPosition();
	}
}

sf::Vector2f Text::getAlignment() const
{
	return myAlignment;
}

void Text::setColor(sf::Color color)
{
	if (myColor != color)
	{
		myColor = color;
		updateColor();
	}
}

sf::Color Text::getColor() const
{
	return myColor;
}

void Text::setScale(sf::Vector2f scale)
{
	if (myText.getScale() != scale)
	{
		myText.setScale(scale);
		updateTextPosition();
	}
}

sf::Vector2f Text::getScale() const
{
	return myText.getScale();
}

void Text::setMaxSizeBehavior(MaxSizeBehavior behavior)
{
	if (myMaxSizeBehavior != behavior)
	{
		myMaxSizeBehavior = behavior;

		// TODO reimplement max size behavior
		updateTextPosition();
	}
}

Text::MaxSizeBehavior Text::getMaxSizeBehavior() const
{
	return myMaxSizeBehavior;
}

void Text::setClipOffset(sf::Vector2f offset)
{
	// TODO reimplement clip offset
}

sf::Vector2f Text::getClipOffset() const
{
	return sf::Vector2f();
}

void Text::setPositionRounded(bool rounded)
{
	if (myIsPositionRounded != rounded)
	{
		myIsPositionRounded = rounded;
		updateTextPosition();
	}
}

bool Text::isPositionRounded() const
{
	return myIsPositionRounded;
}

void Text::setFont(std::string fontName)
{
	if (myFont == nullptr || myFontName != fontName)
	{
		myFontName = fontName;
	}
}

std::string Text::getFont() const
{
	return myFontName;
}

bool Text::isFontDefault() const
{
	return myFontName.empty();
}

sf::FloatRect Text::getTextRect() const
{
	sf::Vector2f pos = getTextPosition(), size = getTextSize();
	return sf::FloatRect(pos.x, pos.y, size.x, size.y);
}

sf::Vector2f Text::getTextPosition() const
{
	return myText.getPosition();
}

sf::Vector2f Text::getTextSize() const
{
	return myText.getBoundingBox().getSize();
}

sf::FloatRect Text::getSelectionBox(std::size_t start, std::size_t length) const
{
	// TODO reimplement selection box
	return sf::FloatRect();
}

std::size_t Text::getCharacterAtPosition(sf::Vector2f position) const
{
	// TODO reimplement character lookup by position
	return 0;
}

void Text::onInit()
{
	getWidget()->addStateCallback(
	    [this](StateEvent e) {
		    handleStateEvent(e);
	    },
	    StateEvent::Any);

	reloadFont();
	updateColor();
}

void Text::onMove()
{
	myText.setMaximumSize(getSize());
}

void Text::onResize()
{
	myText.setMaximumSize(getSize());
	updateTextPosition();
}

void Text::updateTextPosition()
{
	sf::FloatRect boundingBox = getRect();
	sf::Vector2f pos(boundingBox.left, boundingBox.top);

	sf::Vector2f textSize(getTextSize());

	pos.x += (boundingBox.width - textSize.x) * getAlignment().x;
	pos.y += (boundingBox.height - textSize.y) * getAlignment().y;

	if (isPositionRounded())
	{
		pos = sf::Vector2f(sf::Vector2i(pos + sf::Vector2f(0.5f, 0.5f)));
	}

	if (pos != myText.getPosition())
	{
		myText.setPosition(pos);
	}

	repaint();
}

void Text::handleStateEvent(StateEvent event)
{
	switch (event.type)
	{
	case StateEvent::ResourcesChanged:
		reloadFont();
		break;

	case StateEvent::ParentApplicationChanged:
	case StateEvent::ConfigChanged:
		updateConfig();
		break;

	case StateEvent::UsabilityChanged:
		updateColor();
		break;

	default:
		break;
	}
}

void Text::loadFont(std::string fontName)
{
	if (fontName.empty())
	{
		static cfg::String defaultFont("gui.pieces.text.font");
		fontName = config().get(defaultFont);
	}

	auto fontResource = resources().acquireFont(std::move(fontName));

	if (myFont != fontResource)
	{
		myFont = fontResource;
		myText.setFont(myFont ? myFont->getFont(12) : nullptr);
		updateTextPosition();
	}
}

void Text::reloadFont()
{
	// "Forget" currently loaded font to force reload.
	myFont = nullptr;

	// Load the last loaded font.
	loadFont(myFontName);
}

void Text::updateConfig()
{
	if (isFontDefault())
	{
		loadFont("");
	}
}

void Text::updateColor()
{
	sf::Color color = getColor();
	if (getWidget() && !getWidget()->isEnabled())
	{
		color.a /= 2;
	}
	myText.setColor(color);
	repaint();
}

}

}
