#include <Shared/Utils/OwningInputStream.hpp>

OwningMemoryStream::OwningMemoryStream(std::vector<char> data) :
	data(std::move(data))
{
	open(this->data.data(), this->data.size());
}
