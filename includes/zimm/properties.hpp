#pragma once

#include "definitions.hpp"

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
    AssumedProperty
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
    std::string m_includePath;

public:
    explicit IncludeProperty(std::string includePath);
    std::string_view include_path() const noexcept { return m_includePath; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<IncludeProperty>(*this);
    }
};

class CompileFlagProperty : public Property
{
    std::string m_flags;

public:
    explicit CompileFlagProperty(std::string flag);
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
    explicit LinkFlagProperty(std::string flag);
    std::string_view flag() const noexcept { return m_flags; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<LinkFlagProperty>(*this);
    }
};

namespace detail
{
class Sources;
}

class LinkTargetProperty : public Property
{
    friend detail::Sources;
    const class Library *m_linkLib;
    explicit LinkTargetProperty(const Library *target);

public:
    const Library *link_lib() const noexcept { return m_linkLib; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<LinkTargetProperty>(*this);
    }
};

class AssumedProperty : public Property
{
    friend class ThirdPartyTarget;
    std::string m_assumedPath;

    explicit AssumedProperty(std::string assumedPath)
        : Property(PropertyType::AssumedProperty), m_assumedPath(std::move(assumedPath))
    {
    }

public:
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<AssumedProperty>(*this);
    }

    const std::string_view path() const noexcept { return m_assumedPath; }
};

} // namespace zimm
