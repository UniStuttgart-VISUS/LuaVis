#ifndef DISP_UTIL_HPP
#define DISP_UTIL_HPP

#include <SFML/Graphics.hpp>

class DispGrid : public sf::Drawable, public sf::Transformable
{

public:
	DispGrid();

	void setColor(sf::Color color);
	sf::Color getColor() const;
	void setCenterColor(sf::Color color);
	sf::Color getCenterColor() const;

	void setActiveRect(sf::FloatRect rect);
	sf::FloatRect getActiveRect() const;

	void setGridSize(float gridSize);
	float getGridSize() const;

private:
	sf::Color myColor, myCenterColor;
	sf::FloatRect myRect;
	float myGridSize;

	sf::VertexArray vertices;

	void update();
	void draw(sf::RenderTarget & target, sf::RenderStates states) const;
};

class DispRect : public sf::Drawable
{

public:
	void setTarget(const sf::Sprite & target);
	void setTarget(const sf::VertexArray & target);
	void setRect(sf::FloatRect rect);

private:
	sf::Transform transform;
	sf::FloatRect myRect;

	void draw(sf::RenderTarget & target, sf::RenderStates states) const;
};

#endif
