#ifndef SRC_CLIENT_GRAPHICS_TEXT_MULTIFONT_HPP_
#define SRC_CLIENT_GRAPHICS_TEXT_MULTIFONT_HPP_

#include <Client/Graphics/Text/AbstractFont.hpp>

#include <memory>
#include <vector>

namespace wos
{
namespace text
{

class MultiFont : public AbstractFont
{
public:
	MultiFont(std::vector<std::shared_ptr<AbstractFont>> fonts);

	const std::vector<std::shared_ptr<AbstractFont>> & getFonts() const;

	const Glyph & getGlyph(sf::Uint32 character) const override;
	float getKerning(sf::Uint32 character1, sf::Uint32 character2) const override;
	float getLineSpacing() const override;
	float getSize() const override;
	sf::Vector2f getNativeScale() const override;

private:
	std::vector<std::shared_ptr<AbstractFont>> fonts;
	float size = 0;
	float lineSpacing = 0;
	std::vector<float> scaleFactors;
	sf::Vector2f nativeScale = sf::Vector2f(1, 1);
	mutable Glyph scaledGlyph;
};

}
}

#endif
