#pragma once

#include "path.hpp"

#include <string>
#include <variant>

namespace zimm
{
class Library;

enum class PropertyType : std::uint8_t
{
    Include,
    CompileFlag,
    LinkFlag,
    LinkTarget,
};

class IncludeProperty
{
    Directory m_includePath;

public:
    explicit IncludeProperty(Directory includePath);
    const Directory &include_path() const noexcept { return m_includePath; }
    PropertyType type() const noexcept { return PropertyType::Include; }
};

class CompileFlagProperty
{
    std::string m_flags;

public:
    explicit CompileFlagProperty(std::string_view flag);
    std::string_view flag() const noexcept { return m_flags; }
    PropertyType type() const noexcept { return PropertyType::CompileFlag; }
};

class LinkFlagProperty
{
    std::string m_flags;

public:
    explicit LinkFlagProperty(std::string_view flag);
    std::string_view flag() const noexcept { return m_flags; }
    PropertyType type() const noexcept { return PropertyType::LinkFlag; }
};

namespace detail
{
class LinkTrait;
} // namespace detail

class LinkTargetProperty
{
    friend detail::LinkTrait;
    const Library *m_linkLib;
    explicit LinkTargetProperty(const Library *target);

public:
    const Library *link_lib() const noexcept { return m_linkLib; }
    PropertyType type() const noexcept { return PropertyType::LinkTarget; }
};

using PolyProperty =
    std::variant<IncludeProperty, CompileFlagProperty, LinkFlagProperty, LinkTargetProperty>;

inline PropertyType prop_type(const PolyProperty &p)
{
    return std::visit([](const auto &p) { return p.type(); }, p);
}

} // namespace zimm
