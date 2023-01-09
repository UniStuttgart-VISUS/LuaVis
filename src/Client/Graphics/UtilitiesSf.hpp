#ifndef UTILITIES_SF_HPP
#define UTILITIES_SF_HPP

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <cmath>
#include <cstddef>
#include <functional>
#include <vector>

namespace sf
{
class VertexArray;
class Image;
}

/// in-place constructable class to pass vertices to functions taking only sf::Drawable instances (such as
/// drawClipped()).
class VertexWrapper : public sf::Drawable
{
public:
	VertexWrapper() = delete;
	VertexWrapper(const sf::Vertex * vertices, std::size_t vertexCount,
	              sf::PrimitiveType primitiveType = sf::Triangles);

private:
	void draw(sf::RenderTarget & target, sf::RenderStates states) const;

	const sf::Vertex * myVertices;
	std::size_t myVertexCount;
	sf::PrimitiveType myPrimitiveType;
};

/// wraps any function in an sf::Drawable
class DrawableWrapper : public sf::Drawable
{
public:
	using RenderFunction = std::function<void(sf::RenderTarget & target, sf::RenderStates states)>;

	DrawableWrapper() = delete;
	DrawableWrapper(RenderFunction renderFunc);

private:
	void draw(sf::RenderTarget & target, sf::RenderStates states) const;

	RenderFunction renderFunc;
};

sf::FloatRect viewToRect(const sf::View & view);
void zoomView(sf::View & view, float x, float y);

sf::Color fadeColor(const sf::Color & color, float alpha_change);
sf::Color operator*(const sf::Color & left, float right);
sf::Color operator/(const sf::Color & left, float right);

inline sf::IntRect roundRect(sf::FloatRect rect)
{
	int left = std::ceil(rect.left - 0.5f);
	int top = std::ceil(rect.top - 0.5f);
	int right = std::floor(rect.left + rect.width + 0.5f);
	int bottom = std::floor(rect.top + rect.height + 0.5f);
	return sf::IntRect(left, top, right - left, bottom - top);
}

void drawClipped(const sf::Drawable & drawable, sf::RenderTarget & target, sf::RenderStates states,
                 sf::IntRect clipRect);

inline void drawClipped(const sf::Drawable & drawable, sf::RenderTarget & target, sf::RenderStates states,
                        sf::FloatRect clipRect)
{
	drawClipped(drawable, target, states, roundRect(clipRect));
}

inline bool transformEquals(const sf::Transform & a, const sf::Transform & b)
{
	for (std::size_t i = 0; i < 16; ++i)
	{
		if (a.getMatrix()[i] != b.getMatrix()[i])
		{
			return false;
		}
	}
	return true;
}

template <typename T>
sf::Vector2<T> rectTopLeft(const sf::Rect<T> & rect)
{
	return sf::Vector2<T>(rect.left, rect.top);
}

template <typename T>
sf::Vector2<T> rectTopRight(const sf::Rect<T> & rect)
{
	return sf::Vector2<T>(rect.left + rect.width, rect.top);
}

template <typename T>
sf::Vector2<T> rectBottomLeft(const sf::Rect<T> & rect)
{
	return sf::Vector2<T>(rect.left, rect.top + rect.height);
}

template <typename T>
sf::Vector2<T> rectBottomRight(const sf::Rect<T> & rect)
{
	return sf::Vector2<T>(rect.left + rect.width, rect.top + rect.height);
}

void clipVertices(std::vector<sf::Vertex> & vertices, const sf::FloatRect & clipRect);

// Returns the new vertex count in the clipped section.
void clipVertices(std::vector<sf::Vertex> & vertices, std::size_t startPos, std::size_t endPos,
                  const sf::FloatRect & clipRect);
void clipVertices(sf::VertexArray & array, const sf::FloatRect & clipRect);

void appendVertexRect(sf::VertexArray & array, sf::FloatRect rect, sf::Color colorTL, sf::Color colorTR,
                      sf::Color colorBR, sf::Color colorBL);
void appendVertexRect(sf::VertexArray & array, sf::FloatRect rect, sf::Color color);

sf::Vector2f transformVector(const sf::Transform & transform, const sf::Vector2f & vector);

bool loadImage(const sf::Uint8 * data, std::size_t size, sf::Image & image);

#endif
