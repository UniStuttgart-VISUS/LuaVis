#ifndef SRC_CLIENT_GRAPHICS_TEXT_TEXT_HPP_
#define SRC_CLIENT_GRAPHICS_TEXT_TEXT_HPP_

#include <Client/Graphics/Text/AbstractFont.hpp>

#include <SFML/Config.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/System/String.hpp>

#include <memory>
#include <vector>

namespace wos
{
namespace text
{

class Text : public sf::Transformable
{
public:
	struct Cursor
	{
		Cursor(std::size_t index) : index(index)
		{
		}
		std::size_t index = 0;
		sf::Vector2f position;
	};

	Text();

	void setFont(std::shared_ptr<AbstractFont> font);
	std::shared_ptr<AbstractFont> getFont() const;

	void setString(sf::String string);
	const sf::String & getString() const;

	void setColor(sf::Color color);
	sf::Color getColor() const;

	void setAlignment(sf::Vector2f alignment);
	sf::Vector2f getAlignment() const;

	void setMaximumSize(sf::Vector2f maximumSize);
	sf::Vector2f getMaximumSize() const;

	sf::Vector2f getMaximumSizeScaleFactor() const;

	void setSpacing(sf::Vector2f spacing);
	sf::Vector2f getSpacing() const;

	void setPadding(sf::FloatRect padding);
	sf::FloatRect getPadding() const;

	void setSizeCorrection(float sizeCorrection);
	float getSizeCorrection() const;

	void setWordWrapEnabled(bool wordWrapEnabled);
	bool isWordWrapEnabled() const;

	void setColorModifierEnabled(bool colorModifierEnabled);
	bool isColorModifierEnabled() const;

	void setIconDisplayEnabled(bool iconDisplayEnabled);
	bool isIconDisplayEnabled() const;

	const std::vector<sf::Vertex> & getVertices() const;
	const std::vector<TextureID> & getTextureIDs() const;
	const std::vector<Cursor> & getCursors() const;

	const sf::FloatRect & getBoundingBox() const;

private:
	std::shared_ptr<AbstractFont> font;
	sf::String string;
	sf::Color color = sf::Color::White;
	sf::Vector2f alignment;
	sf::Vector2f maximumSize;
	sf::Vector2f spacing;
	sf::FloatRect padding;
	bool wordWrapEnabled = false;
	bool colorModifierEnabled = true;
	bool iconDisplayEnabled = true;
	float sizeCorrection = 1.f;

	struct Format
	{
		sf::Color textColor = sf::Color::White;
		sf::Color backgroundColor = sf::Color::Transparent;
		bool underline = false;
		bool underlineOffset = false;
		bool skip = false;
	};

	struct Cache
	{
		Cache(const Text & text);

		void updateIfNecessary();
		void updateRawVertices();
		void updateTransformedVertices();

		void applyAlignment(std::size_t startIndex, std::size_t endIndex);

		bool isWordWrappable(sf::Uint32 character) const;
		float getWordWidth(std::size_t index) const;

		bool hasPendingFullUpdate = false;
		bool hasPendingUpdate = false;

		bool hasColorModifiers = false;
		bool hasIcons = false;

		std::vector<sf::Vertex> rawVertices;
		std::vector<sf::Vertex> vertices;
		std::vector<Cursor> cursors;
		std::vector<TextureID> textureIDs;
		std::vector<std::size_t> lines;
		sf::Vector2f maxSizeScale = sf::Vector2f(1, 1);
		sf::FloatRect boundingBox;
		sf::FloatRect rawBoundingBox;
		sf::Transform transform;

		const Text & text;
	};

	mutable Cache cache;
};

}
}

#endif
