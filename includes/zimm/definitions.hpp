#pragma once

#include "detail/object.hpp"

namespace zimm
{
constexpr class PublicTag *public_ = nullptr;
constexpr class PrivateTag *private_ = nullptr;

class Property;
class Target;

using PropertyObject = detail::Object<Property>;

} // namespace zimm
