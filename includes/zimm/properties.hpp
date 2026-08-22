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
    PrecompiledHeader,
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

// Marks a header to be precompiled and used by the owning target.
//
// PCH is modelled as a property so it flows through the same property system as
// includes/flags and stays generator-agnostic: a generator is free to translate
// it however its native build system expresses precompiled headers (ninja: a
// dedicated compile edge + -include; make: .gch rules; Xcode: GCC_PREFIX_HEADER).
class PrecompiledHeaderProperty : public Property
{
    File m_header;

public:
    explicit PrecompiledHeaderProperty(File header);
    const File &header() const noexcept { return m_header; }
    std::unique_ptr<Property> clone() const override
    {
        return std::make_unique<PrecompiledHeaderProperty>(*this);
    }
};

} // namespace zimm
