#ifndef SRC_CLIENT_GUI3_EVENTS_KEYNATIVE_HPP_
#define SRC_CLIENT_GUI3_EVENTS_KEYNATIVE_HPP_

#include <cstdint>
#include <string>

namespace gui3
{
namespace detail
{

std::string getNativeKeyName(std::int32_t code);

}
}

#endif