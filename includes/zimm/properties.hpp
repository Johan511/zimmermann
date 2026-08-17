#pragma once

#include "definitions.hpp"
#include "path.hpp"

#include <string>
#include <type_traits>

namespace zimm
{
struct PublicTag;
struct ProtectedTag;
struct PrivateTag;

template <typename T>
concept PPPTag = std::is_same_v<T, PublicTag> || std::is_same_v<T, ProtectedTag> ||
                 std::is_same_v<T, PrivateTag>;

enum class PropertyType
{
    Include,
    CompileFlag,
    LinkFlag,
    LinkTarget,
};

class Property
{
    PropertyType m_type;

protected:
    Property(PropertyType type);

public:
    PropertyType type() const noexcept { return m_type; }
    virtual ~Property() = default;
    virtual std::unique_ptr<Property> clone() const = 0;
};

class IncludeProperty : public Property
{
    Directory m_includePath;

public:
    explicit IncludeProperty(Directory includePath);
    const Directory &include_path() const noexcept { return m_includePath; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<IncludeProperty>(*this);
    }
};

class CompileFlagProperty : public Property
{
    std::string m_flags;

public:
    explicit CompileFlagProperty(std::string_view flag);
    std::string_view flag() const noexcept { return m_flags; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<CompileFlagProperty>(*this);
    }
};

class LinkFlagProperty : public Property
{
    std::string m_flags;

public:
    explicit LinkFlagProperty(std::string_view flag);
    std::string_view flag() const noexcept { return m_flags; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<LinkFlagProperty>(*this);
    }
};

namespace detail
{
class SourcesTrait;
}

class LinkTargetProperty : public Property
{
    friend detail::SourcesTrait;
    const class Library *m_linkLib;
    explicit LinkTargetProperty(const Library *target);

public:
    const Library *link_lib() const noexcept { return m_linkLib; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<LinkTargetProperty>(*this);
    }
};

} // namespace zimm
