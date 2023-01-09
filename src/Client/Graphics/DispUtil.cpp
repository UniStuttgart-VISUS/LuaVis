#include "Client/Graphics/DispUtil.hpp"
#include <cmath>

DispGrid::DispGrid()
{
	myGridSize = 1.f;
	vertices.setPrimitiveType(sf::Lines);
	myCenterColor = myColor = sf::Color::Green;
}

void DispGrid::setColor(sf::Color color)
{
	if (myCenterColor == myColor)
		myCenterColor = color;
	myColor = color;
	update();
}

sf::Color DispGrid::getColor() const
{
	return myColor;
}

void DispGrid::setCenterColor(sf::Color color)
{
	myCenterColor = color;
	update();
}

sf::Color DispGrid::getCenterColor() const
{
	return myCenterColor;
}

void DispGrid::setActiveRect(sf::FloatRect rect)
{
	myRect = rect;
	update();
}

sf::FloatRect DispGrid::getActiveRect() const
{
	return myRect;
}

void DispGrid::setGridSize(float gridSize)
{
	myGridSize = gridSize;
	update();
}

float DispGrid::getGridSize() const
{
	return myGridSize;
}

void DispGrid::update()
{
	vertices.clear();

	const float left = myRect.left;
	const float right = myRect.left + myRect.width;
	const float top = myRect.top;
	const float bottom = myRect.top + myRect.height;

	const float x_start = ceil(left / myGridSize) * myGridSize;
	const float x_end = ceil(right / myGridSize) * myGridSize;

	const float y_start = ceil(top / myGridSize) * myGridSize;
	const float y_end = ceil(bottom / myGridSize) * myGridSize;

	for (float x = x_start; x < x_end; x += myGridSize)
	{
		sf::Color color = myColor;
		if (x < 1 && x > -1)
			color = myCenterColor;
		vertices.append(sf::Vertex(sf::Vector2f(x, top), color));
		vertices.append(sf::Vertex(sf::Vector2f(x, bottom), color));
	}

	for (float y = y_start; y < y_end; y += myGridSize)
	{
		sf::Color color = myColor;
		if (y < 1 && y > -1)
			color = myCenterColor;
		vertices.append(sf::Vertex(sf::Vector2f(left, y), color));
		vertices.append(sf::Vertex(sf::Vector2f(right, y), color));
	}
}

void DispGrid::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
	target.draw(vertices, states);
}

void DispRect::setTarget(const sf::Sprite & target)
{
	transform = target.getTransform();
	myRect = target.getLocalBounds();
}
void DispRect::setTarget(const sf::VertexArray & target)
{
	transform = sf::Transform::Identity;
	myRect = target.getBounds();
}
void DispRect::setRect(sf::FloatRect rect)
{
	myRect = rect;
}

void DispRect::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
	states.transform *= transform;

	sf::Color myColor = sf::Color::Green;

	sf::VertexArray vertices;
	vertices.setPrimitiveType(sf::LinesStrip);
	vertices.append(sf::Vertex(sf::Vector2f(myRect.left, myRect.top), myColor));
	vertices.append(sf::Vertex(sf::Vector2f(myRect.left + myRect.width, myRect.top), myColor));
	vertices.append(sf::Vertex(sf::Vector2f(myRect.left + myRect.width, myRect.top + myRect.height), myColor));
	vertices.append(sf::Vertex(sf::Vector2f(myRect.left, myRect.top + myRect.height), myColor));
	vertices.append(vertices[0]);

	target.draw(vertices, states);
}
