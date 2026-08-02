#pragma once

#include "detail/object.hpp"

namespace zimm
{
constexpr class PublicTag *public_ = nullptr;
constexpr class PrivateTag *private_ = nullptr;

class Property;
class Target;

using PropertyObject = detail::Object<Property>;

namespace detail
{
/*
 same as default_delete except we don't delete
 Why do we need it? Makes life easier if we just leak targets

 possible TODOs:
    1. make all targets generated have static lifetime?
        - might be possible because we have factory functions for all targets
*/
template <typename T>
struct noop_delete
{
    constexpr noop_delete() noexcept = default;
    template <typename U>
    constexpr noop_delete(const noop_delete<U> &) noexcept requires(std::is_convertible_v<U *, T *>)
    {
    }

    constexpr void operator()(T * /* ptr */) const
    {
        static_assert(!std::is_void<T>::value, "can't delete pointer to incomplete type");
    }
};
} // namespace detail

template <typename T>
using LeakyPtr = std::unique_ptr<T, detail::noop_delete<T>>;

} // namespace zimm
