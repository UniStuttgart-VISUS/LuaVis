#ifndef TEXTURE_PACKER_HPP
#define TEXTURE_PACKER_HPP

#include <SFML/Config.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <Shared/Utils/Debug/Logger.hpp>
#include <memory>
#include <vector>

namespace sf
{
class Image;
class Texture;
}

// packs images densely in a texture.
class TexturePacker
{
public:
	typedef sf::Int32 NodeID;
	static const NodeID packFailure;
	static const NodeID fullImage;

	TexturePacker(unsigned int minSize = 4096);
	TexturePacker(const sf::Image & image);
	~TexturePacker();

	NodeID add(const sf::Image & image, bool allowResize = true);
	bool update(NodeID index, sf::IntRect rect, const sf::Uint8 * pixels);
	void free(NodeID index);
	bool clear();
	bool empty() const;
	bool isValidNode(NodeID index) const;

	sf::IntRect getImageRect(NodeID index) const;

	void setMinimumTextureSize(sf::Vector2u minimumSize);
	sf::Vector2u getMinimumTextureSize() const;

	const sf::Texture * getTexture() const;

	void setSmooth(bool smooth);
	bool isSmooth() const;

	bool isSingleTexture() const;

	class Handle
	{
	public:
		Handle() = default;
		Handle(TexturePacker & packer, NodeID id);
		~Handle();

		sf::IntRect getImageRect() const;
		NodeID getNodeID() const;

	private:
		class UniqueHandle
		{
		public:
			UniqueHandle(std::shared_ptr<TexturePacker *> packer, NodeID id);
			~UniqueHandle();

			std::weak_ptr<TexturePacker *> packer;
			NodeID id;
		};

		std::shared_ptr<UniqueHandle> myUniqueHandle;
	};

private:
	static const NodeID emptyNode, stubNode;

	struct Node : std::enable_shared_from_this<Node>
	{
		Node();

		std::shared_ptr<Node> sub1, sub2;
		sf::Vector2u pos;
		sf::Vector2u size; // 0 = unlimited.
		NodeID id;

		std::shared_ptr<Node> add(const sf::Image & image, sf::Vector2u maxSize);
		sf::Vector2u getSizeBounds(sf::Vector2u currentMax = sf::Vector2u()) const;
	};

	bool createTransparentTexture(sf::Vector2u size);
	bool resizeToFit();

	void addImageToTexture(const sf::Image & image, sf::Vector2u position);

	std::shared_ptr<Node> myTree;
	std::vector<std::weak_ptr<Node>> myLookupTable;
	NodeID myIdCounter;

	std::unique_ptr<sf::Texture> myTexture;
	sf::Vector2u myMinimumSize;
	std::shared_ptr<TexturePacker *> mySelfPointer;
	bool myIsSingleTexture = false;

	Logger logger;
};

#endif
