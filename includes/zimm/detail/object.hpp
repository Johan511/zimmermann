#pragma once

#include <memory>
#include <type_traits>

namespace zimm::detail
{

template <typename T>
class Object
{
    std::unique_ptr<T> m_impl;

public:
    Object() = default;

    template <typename U>
    requires std::derived_from<std::remove_cvref_t<U>, T>
    Object(U &&value) : m_impl(std::make_unique<std::remove_cvref_t<U>>(std::forward<U>(value)))
    {
    }

    // Deep copy on clone
    Object(const Object &other) : m_impl(other.m_impl ? other.m_impl->clone() : nullptr) {}
    Object &operator=(const Object &other)
    {
        if (this != &other)
        {
            m_impl = other.m_impl ? other.m_impl->clone() : nullptr;
        }
        return *this;
    }

    Object(Object &&) = default;
    Object &operator=(Object &&) = default;

    T *operator->() { return m_impl.get(); }
    const T *operator->() const { return m_impl.get(); }
    T &operator*() { return *m_impl; }
    const T &operator*() const { return *m_impl; }
    T *get() { return m_impl.get(); }
    const T *get() const { return m_impl.get(); }

    explicit operator bool() const noexcept { return m_impl != nullptr; }
};

} // namespace zimm::detail
