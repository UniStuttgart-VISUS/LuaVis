#include <Client/Graphics/TexturePacker.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <Shared/Utils/MakeUnique.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>

const TexturePacker::NodeID TexturePacker::packFailure = -1;
const TexturePacker::NodeID TexturePacker::fullImage = -3;

const TexturePacker::NodeID TexturePacker::emptyNode = -1;
const TexturePacker::NodeID TexturePacker::stubNode = -2;

TexturePacker::TexturePacker(unsigned int minSize) :
    myTexture(makeUnique<sf::Texture>()),
    myMinimumSize(minSize, minSize),
    logger("TexturePacker")
{
	mySelfPointer = std::make_shared<TexturePacker *>(this);

	// Find suitable texture size
	while (!clear() && myMinimumSize.x > 256)
	{
		myMinimumSize.x /= 2;
		myMinimumSize.y /= 2;
	}
}

TexturePacker::TexturePacker(const sf::Image & image) :
	myTexture(makeUnique<sf::Texture>()),
	myMinimumSize(image.getSize()),
	logger("TexturePacker")
{
	mySelfPointer = std::make_shared<TexturePacker *>(this);
	myTree = std::make_shared<Node>();
	myTexture->loadFromImage(image);
	myIdCounter = 0;

	myIsSingleTexture = true;
}

TexturePacker::~TexturePacker()
{
}

TexturePacker::NodeID TexturePacker::add(const sf::Image & image, bool allowResize)
{
	// reject zero-sized images.
	if (image.getSize().x == 0 || image.getSize().y == 0)
		return packFailure;

	std::shared_ptr<Node> node = myTree->add(image, myTexture->getSize());

	if (node)
	{
		// insertion successful, add lookup table entry, add image to texture, and return node ID.
		node->id = myIdCounter++;

		myLookupTable.resize(std::max<unsigned int>(myLookupTable.size(), node->id + 1));
		myLookupTable[node->id] = node;

		addImageToTexture(image, node->pos);
		return node->id;
	}
	else if (allowResize)
	{
		// retry adding with increased texture size, if possible.
		{
			sf::Image curTexture = myTexture->copyToImage();

			if (!createTransparentTexture(myTexture->getSize() * 2u))
			{
				// free any unnecessarily allocated texture space.
				resizeToFit();
				return packFailure;
			}

			myTexture->update(curTexture, 0, 0);
		}

		return add(image);
	}

	return packFailure;
}

bool TexturePacker::update(NodeID index, sf::IntRect rect, const sf::Uint8 * pixels)
{
	if (rect.left >= 0 && rect.top >= 0)
	{
		if (isValidNode(index))
		{
			auto entry = myLookupTable[index].lock();
			if (rect.left + rect.width <= entry->size.x && rect.top + rect.height <= entry->size.y)
			{
				myTexture->update(
				    pixels, rect.width, rect.height, entry->pos.x + rect.left, entry->pos.y + rect.top);
				return true;
			}
		}
		else if (index == fullImage && rect.left + rect.width <= myTexture->getSize().x
		         && rect.top + rect.height <= myTexture->getSize().y)
		{
			myTexture->update(pixels, rect.width, rect.height, rect.left, rect.top);
			return true;
		}
	}
	return false;
}

void TexturePacker::free(NodeID index)
{
	if (isValidNode(index))
	{
		auto entry = myLookupTable[index].lock();
		entry->id = emptyNode;
		myLookupTable[index].reset();
	}
}

bool TexturePacker::clear()
{
	myLookupTable.clear();
	myTree = std::make_shared<Node>();
	myIdCounter = 0;
	return createTransparentTexture(myMinimumSize);
}

bool TexturePacker::empty() const
{
	return myIdCounter == 0;
}

bool TexturePacker::isValidNode(NodeID index) const
{
	return index >= 0 && (std::size_t) index < myLookupTable.size() && !myLookupTable[index].expired();
}

sf::IntRect TexturePacker::getImageRect(NodeID index) const
{
	if (isValidNode(index))
	{
		auto entry = myLookupTable[index].lock();
		sf::Vector2u pos = entry->pos;
		sf::Vector2u size = entry->size;
		return sf::IntRect(pos.x, pos.y, size.x, size.y);
	}
	else if (index == fullImage)
	{
		return sf::IntRect(0, 0, myTexture->getSize().x, myTexture->getSize().y);
	}
	else
	{
		return sf::IntRect();
	}
}

void TexturePacker::setMinimumTextureSize(sf::Vector2u minimumSize)
{
	minimumSize.x = std::max<unsigned int>(1, std::min(minimumSize.x, sf::Texture::getMaximumSize()));
	minimumSize.y = std::max<unsigned int>(1, std::min(minimumSize.y, sf::Texture::getMaximumSize()));

	if (myMinimumSize != minimumSize)
	{
		myMinimumSize = minimumSize;

		// update texture size.
		if (empty())
		{
			if (!createTransparentTexture(myMinimumSize))
			{
				return;
			}
		}
		else
		{
			if (!resizeToFit())
			{
				return;
			}
		}
	}
}

sf::Vector2u TexturePacker::getMinimumTextureSize() const
{
	return myMinimumSize;
}

const sf::Texture * TexturePacker::getTexture() const
{
	return myTexture.get();
}

void TexturePacker::setSmooth(bool smooth)
{
	myTexture->setSmooth(smooth);
}

bool TexturePacker::isSmooth() const
{
	return myTexture->isSmooth();
}

bool TexturePacker::isSingleTexture() const
{
	return myIsSingleTexture;
}

TexturePacker::Handle::Handle(TexturePacker & packer, NodeID id)
{
	myUniqueHandle = std::make_shared<UniqueHandle>(packer.mySelfPointer, id);
}

TexturePacker::Handle::~Handle()
{
}

sf::IntRect TexturePacker::Handle::getImageRect() const
{
	if (myUniqueHandle->packer.expired())
	{
		return sf::IntRect();
	}
	return (*myUniqueHandle->packer.lock())->getImageRect(myUniqueHandle->id);
}

TexturePacker::NodeID TexturePacker::Handle::getNodeID() const
{
	return myUniqueHandle->id;
}

TexturePacker::Handle::UniqueHandle::UniqueHandle(std::shared_ptr<TexturePacker *> packer, NodeID id) :
	packer(packer),
	id(id)
{
}

TexturePacker::Handle::UniqueHandle::~UniqueHandle()
{
	if (!packer.expired())
	{
		(*packer.lock())->free(id);
	}
}

bool TexturePacker::createTransparentTexture(sf::Vector2u size)
{
	logger.debug("Changing texture packer size from {}x{} to {}x{}", myTexture->getSize().x, myTexture->getSize().y,
	             size.x, size.y);

	if (!myTexture->create(size.x, size.y))
	{
		logger.warn("Failed to create texture of size {}x{} (maximum texture size exceeded)", size.x, size.y);
		return false;
	}

	std::vector<sf::Uint8> pixels(myTexture->getSize().x * myTexture->getSize().y * 4);
	myTexture->update(pixels.data());
	return true;
}

bool TexturePacker::resizeToFit()
{
	// traverse tree and calculate bounds.
	sf::Vector2u bounds = myTree->getSizeBounds();

	sf::Vector2u size = myMinimumSize;

	// increase size to at least contain the total bounds.
	while (size.x < bounds.x || size.y < bounds.y)
		size *= (unsigned int) 2;

	// crop texture to new bounds if necessary.
	if (size != myTexture->getSize())
	{
		sf::Image curTextureSub;
		{
			sf::Image curTexture = myTexture->copyToImage();

			if (!createTransparentTexture(size))
				return false;

			curTextureSub.create(size.x, size.y);
			curTextureSub.copy(curTexture, 0, 0, sf::IntRect(0, 0, size.x, size.y), false);
		}
		myTexture->update(curTextureSub);
	}

	return true;
}

TexturePacker::Node::Node() :
	sub1(nullptr),
	sub2(nullptr),
	pos(0, 0),
	size(0, 0),
	id(emptyNode)
{
}

std::shared_ptr<TexturePacker::Node> TexturePacker::Node::add(const sf::Image & image, sf::Vector2u maxSize)
{
	// try to insert into sub-nodes, if applicable.
	if (sub1 && sub2)
	{
		// try to insert into first node.
		std::shared_ptr<Node> ret = sub1->add(image, maxSize);

		// then, if first node failed, second node.
		if (ret)
			return ret;
		else
			return sub2->add(image, maxSize);
	}
	else
	{
		// node is already occupied or is a stub node (actual zero width/height).
		if (id != emptyNode)
			return std::shared_ptr<Node>();

		// get node's effective size. can be negative.
		sf::Vector2i effSize(size.x == 0 ? (int) maxSize.x - (int) pos.x : size.x,
		                     size.y == 0 ? (int) maxSize.y - (int) pos.y : size.y);

		// image is too big to fit in node.
		if (!((int) image.getSize().x <= effSize.x && (int) image.getSize().y <= effSize.y))
			return std::shared_ptr<Node>();

		// image fits exactly into node.
		if (image.getSize().x == size.x && image.getSize().y == size.y)
			return shared_from_this();

		// image does fit, but not perfectly: create sub-nodes.
		sub1 = std::make_shared<Node>();
		sub2 = std::make_shared<Node>();

		// determine splitting direction: first infinite compare, then finite.
		bool finiteComp = effSize.x - (int) image.getSize().x > effSize.y - (int) image.getSize().y;

		if (size.x == 0 ? (size.y == 0 ? finiteComp : true) : (size.y == 0 ? false : finiteComp))
		{
			// left-right split.

			// left side.
			sub1->pos = pos;
			sub1->size.x = image.getSize().x;
			sub1->size.y = size.y;

			// right side (1px padding, infinitely wide node stays that way).
			sub2->pos.x = pos.x + image.getSize().x + 1;
			sub2->pos.y = pos.y;
			sub2->size.x = size.x == 0 ? 0 : (size.x - image.getSize().x - 1);
			sub2->size.y = size.y;

			// check for and mark stub node (zero-width non-infinite node).
			if (size.x != 0 && size.x <= image.getSize().x + 1)
			{
				sub2->size.x = 0;
				sub2->id = stubNode;
			}
		}
		else
		{
			// top-bottom split.

			// top side.
			sub1->pos = pos;
			sub1->size.x = size.x;
			sub1->size.y = image.getSize().y;

			// bottom side (1px padding, infinitely tall node stays that way).
			sub2->pos.x = pos.x;
			sub2->pos.y = pos.y + image.getSize().y + 1;
			sub2->size.x = size.x;
			sub2->size.y = size.y == 0 ? 0 : (size.y - image.getSize().y - 1);

			// check for and mark stub node (zero-height non-infinite node).
			if (size.y != 0 && size.y <= image.getSize().y + 1)
			{
				sub2->size.y = 0;
				sub2->id = stubNode;
			}
		}

		// attempt to insert into first sub-node.
		return sub1->add(image, maxSize);
	}
}

sf::Vector2u TexturePacker::Node::getSizeBounds(sf::Vector2u currentMax) const
{
	if (sub1)
		currentMax = sub1->getSizeBounds(currentMax);

	if (sub2)
		currentMax = sub2->getSizeBounds(currentMax);

	if (id != emptyNode && id != stubNode)
	{
		currentMax.x = std::max(currentMax.x, pos.x + size.x);
		currentMax.y = std::max(currentMax.y, pos.y + size.y);
	}

	return currentMax;
}

void TexturePacker::addImageToTexture(const sf::Image & image, sf::Vector2u position)
{
	sf::Vector2u size = image.getSize();

	sf::Vector2u offset;
	sf::Vector2u sizeDiff;

	if (position.x != 0)
	{
		offset.x++;
		sizeDiff.x++;
	}

	if (position.y != 0)
	{
		offset.y++;
		sizeDiff.y++;
	}

	if (position.x + size.x < myTexture->getSize().x)
	{
		sizeDiff.x++;
	}

	if (position.y + size.y < myTexture->getSize().y)
	{
		sizeDiff.y++;
	}

	if (sizeDiff.x != 0 || sizeDiff.y != 0)
	{
		sf::Image padded;
		padded.create(size.x + sizeDiff.x, size.y + sizeDiff.y, sf::Color::Transparent);
		padded.copy(image, offset.x, offset.y, sf::IntRect(0, 0, size.x, size.y));
		myTexture->update(padded.getPixelsPtr(), padded.getSize().x, padded.getSize().y, position.x - offset.x,
		    position.y - offset.y);
	}
	else
	{
		myTexture->update(image.getPixelsPtr(), size.x, size.y, position.x, position.y);
	}
}
