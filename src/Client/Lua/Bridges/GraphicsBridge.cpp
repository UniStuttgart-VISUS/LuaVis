#include <Client/GUI3/ResourceManager.hpp>
#include <Client/Graphics/UtilitiesSf.hpp>
#include <Client/Lua/Bridges/GraphicsBridge.hpp>
#include <SFML/Config.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <Shared/Lua/LuaUtils.hpp>
#include <Shared/Utils/Utilities.hpp>
#include <Sol2/sol.hpp>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Shared/External/stb/stb_image_write.h>

namespace lua
{

GraphicsBridge::GraphicsBridge(wos::GraphicsManager & manager) :
	manager(manager)
{
}

GraphicsBridge::~GraphicsBridge()
{
}

void GraphicsBridge::onLoad(BridgeLoader & loader)
{
	loader.bind("gfx.getID", std::function<wosC_gfx_t()>([=]() {
		            return manager.getID();
	            }));

	loader.bind("gfx.clear", std::function<void()>([=]() {
		            manager.resetDrawState();
	            }));

	loader.bind("gfx.loadImage", std::function<wosC_gfx_imageID_t(std::string)>([=](std::string imageName) {
		            return manager.loadImage(imageName.c_str());
	            }));

	loader.bind("gfx.preloadImage", //
	    std::function<void(std::string)>(
	        [=](std::string imageName)
	        {
		        return manager.preloadImage(imageName);
	        }));

	loader.bind("gfx.getAvailablePreloadCapacity", //
	    std::function<int()>(
	        [=]()
	        {
		        return manager.getAvailablePreloadCapacity();
	        }));

	loader.bind("gfx.getImagePixels", std::function<std::tuple<wosc::ArrayContext::ArrayID, int, int>(std::string)>(
	                                      [=](std::string imageName) {
		                                      auto result = manager.getImagePixels(imageName);
		                                      return std::make_tuple(result.pixels, result.width, result.height);
	                                      }));

	loader.bind("gfx.createFramebuffer",
	            std::function<wosC_gfx_imageID_t(int, int)>([=](int width, int height) -> wosC_gfx_imageID_t {
		            if (width > 0 && height > 0)
		            {
			            return manager.createFramebuffer(sf::Vector2u(width, height));
		            }
		            else
		            {
			            return wosc::Graphics::INVALID_IMAGE_ID;
		            }
	            }));

	loader.bind("gfx.updateFramebuffer",
	            std::function<bool(wosC_gfx_imageID_t, int, int, int, int, wosc::ArrayContext::ArrayID, int)>(
	                [=](wosC_gfx_imageID_t image, int x, int y, int width, int height,
	                    wosc::ArrayContext::ArrayID pixels, int offset) -> bool {
		                return manager.updateFramebuffer(image, sf::IntRect(x, y, width, height), pixels, offset);
	                }));

	loader.bind("gfx.takeScreenshot", //
	    std::function<void(wosC_gfx_vertexBuffer_t, std::string, float, float, float, float, int, int)>(
	        [=](wosC_gfx_vertexBuffer_t vertexBuffer, std::string targetFile, float x, float y, float width,
	            float height, int outWidth, int outHeight)
	        {
		        manager.takeScreenshot(
		            vertexBuffer, targetFile, sf::FloatRect(x, y, width, height), sf::Vector2i(outWidth, outHeight));
	        }));

	loader.bind("gfx.resizeImageInMemory", //
	    std::function<std::string(std::string, int, int)>(
	        [=](std::string data, int minWidth, int minHeight) -> std::string
	        {
		        sf::Image image;
		        if (image.loadFromMemory(data.data(), data.size()))
		        {
			        int upscale = 1;
			        if (minWidth > 0 && image.getSize().x < minWidth)
			        {
				        upscale = std::max<int>(upscale, 1 + (minWidth - 1) / image.getSize().x);
			        }
			        if (minHeight > 0 && image.getSize().y < minHeight)
			        {
				        upscale = std::max<int>(upscale, 1 + (minHeight - 1) / image.getSize().y);
			        }

			        if (upscale > 1)
			        {
				        sf::Image outImage;
				        outImage.create(image.getSize().x * upscale, image.getSize().y * upscale);
				        for (std::size_t y = 0; y < outImage.getSize().y; ++y)
				        {
					        for (std::size_t x = 0; x < outImage.getSize().x; ++x)
					        {
						        outImage.setPixel(x, y, image.getPixel(x / upscale, y / upscale));
					        }
				        }
				        std::string result;
				        stbi_write_png_to_func(
				            [](void * output, void * data, int size)
				            {
					            if (size > 0)
					            {
						            std::string * outputString = static_cast<std::string *>(output);
						            outputString->resize(size);
						            std::memcpy(&((*outputString)[0]), data, outputString->size());
					            }
				            },
				            &result, outImage.getSize().x, outImage.getSize().y, 4, outImage.getPixelsPtr(), 0);
				        return result;
			        }
		        }
		        return "";
	        }));

	loader.bind("gfx.displayCurrentFrame", std::function<void()>([=]() {
		            manager.displayCurrentFrame();
	            }));

	loader.bind("gfx.getTimeSinceFrameStart", std::function<double()>([=]() -> double {
		            return manager.getTimeSinceFrameStart().asSeconds();
	            }));

	loader.bind("gfx.getFontTexturePage",
	            std::function<wosC_gfx_textureID_t(std::string)>([=](std::string fontName) -> wosC_gfx_textureID_t {
		            // Deprecated. Texture page is set automatically when rendering text.
		            auto font = manager.acquireFont(fontName, 12);
		            return font ? font->getGlyph('A').textureID : -1;
	            }));

	loader.bind(
	    "gfx.drawText",
	    std::function<sol::table(sol::table, sol::this_state)>([=](sol::table table, sol::this_state state) {
		    auto text = table["text"];
		    sf::String string;

		    if (text.get_type() == sol::type::string)
		    {
			    // UTF-8 string
			    std::string textString = lua::getOr<std::string>(text, "");
			    string = sf::String::fromUtf8(textString.begin(), textString.end());
		    }
		    else if (text.get_type() == sol::type::table)
		    {
			    // UTF-32 string
			    std::vector<sf::Uint32> textVec = lua::tableToVector<sf::Uint32>(text);
			    string = sf::String::fromUtf32(textVec.begin(), textVec.end());
		    }

		    wos::GraphicsManager::TextSettings settings;
		    settings.font = lua::getOr<std::string>(table["font"], "");
		    settings.text = string;
		    settings.position = sf::Vector2f(lua::getOr<float>(table["x"], 0), lua::getOr<float>(table["y"], 0));
		    settings.vertexBuffer = lua::getOr<sf::Int32>(table["vertexBuffer"], -1);
		    settings.vertexOffset = lua::getOr<sf::Uint32>(table["vertexOffset"], 0);
		    settings.size = lua::getOr<float>(table["size"], 12.f);
		    settings.sizeCorrection = lua::getOr<float>(table["szc"], 1.f);
		    settings.characterSize = lua::getOr<unsigned int>(table["characterSize"], settings.size * 2);
		    settings.align = sf::Vector2f(lua::getOr<float>(table["alignX"], 0), lua::getOr<float>(table["alignY"], 0));
		    settings.outlineThickness = lua::getOr<float>(table["outlineThickness"], 0);
		    settings.fillColor = sf::Color(lua::getOr<sf::Int32>(table["fillColor"], sf::Color::White.toInteger()));
		    settings.outlineColor =
		        sf::Color(lua::getOr<sf::Int32>(table["outlineColor"], sf::Color::Transparent.toInteger()));
		    settings.shadowColor =
		        sf::Color(lua::getOr<sf::Int32>(table["shadowColor"], sf::Color::Transparent.toInteger()));
		    settings.spacing =
		        sf::Vector2f(lua::getOr<float>(table["spacingX"], 0), lua::getOr<float>(table["spacingY"], 0));
		    settings.noDraw = lua::getOr<bool>(table["noDraw"], false);
		    settings.maxSize =
		        sf::Vector2f(lua::getOr<float>(table["maxWidth"], -1), lua::getOr<float>(table["maxHeight"], -1));
		    settings.clip = lua::getOr<bool>(table["clip"], false);
		    settings.wordWrap = lua::getOr<bool>(table["wordWrap"], false);
		    settings.maxLines = lua::getOr<int>(table["maxLines"], 0);
		    settings.fixedWidth = lua::getOr<bool>(table["fixedWidth"], false);
		    settings.useCache = lua::getOr<bool>(table["useCache"], true);

		    const auto & result = manager.drawText(settings);
		    const auto & bounds = result.getBoundingBox();

		    sol::table boundsTable = sol::state_view(state).create_table();

		    boundsTable["x"] = bounds.left;
		    boundsTable["y"] = bounds.top;
		    boundsTable["width"] = bounds.width;
		    boundsTable["height"] = bounds.height;

		    if (!result.getCursors().empty())
		    {
			    sol::table cursorTable = sol::state_view(state).create_table();
			    for (std::size_t i = 0; i < result.getCursors().size(); ++i)
			    {
				    const auto & cursor = result.getCursors()[i];
				    cursorTable[i * 2 + 1] = cursor.position.x;
				    cursorTable[i * 2 + 2] = cursor.position.y;
			    }
			    boundsTable["cursors"] = cursorTable;
		    }

		    // Pass modified vertex offset back to Lua
		    table["vertexOffset"] = settings.vertexOffset;

		    return boundsTable;
	    }));
}

}
