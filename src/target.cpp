#include "zimm/target.hpp"
#include "zimm/logger.hpp"
#include "zimm/properties.hpp"

#include <string>
#include <type_traits>
#include <utility>

namespace zimm
{

void Target::add_property_impl(std::vector<PolyProperty> &properties, PolyProperty property)
{
    static constexpr auto equals = [](const PolyProperty &a, const PolyProperty &b) -> bool
    {
        if (a.index() != b.index())
            return false;

        PropertyType propType = prop_type(a);
        switch (propType)
        {
        case PropertyType::Include:
            return std::get<IncludeProperty>(a).include_path().path() ==
                   std::get<IncludeProperty>(b).include_path().path();
        case PropertyType::CompileFlag:
            return std::get<CompileFlagProperty>(a).flag() ==
                   std::get<CompileFlagProperty>(b).flag();
        case PropertyType::LinkFlag:
            return std::get<LinkFlagProperty>(a).flag() == std::get<LinkFlagProperty>(b).flag();
        }
        return false;
    };

    for (auto &existing : properties)
        if (equals(existing, property))
            return;

    properties.push_back(std::move(property));
}

std::string to_string(const Target &target)
{
    return std::format("{}:{}", to_string(target.type()), target.name());
}

std::string to_string(const TargetType &type)
{
    switch (type)
    {
    case TargetType::Executable:
        return "Executable";
    case TargetType::StaticLibrary:
        return "StaticLibrary";
    case TargetType::SharedLibrary:
        return "SharedLibrary";
    case TargetType::HeaderOnlyLibrary:
        return "HeaderOnlyLibrary";
    case TargetType::ThirdPartyTarget:
        return "ThirdPartyTarget";
    case TargetType::CustomTarget:
        return "CustomTarget";
    }
    std::unreachable();
}

void detail::LinkTrait::link_impl(Library *linkLib, bool publicNotPrivate)
{
    Target *thisTarget = dynamic_cast<Target *>(this);
    /*
        executable/shared -> static and shared library
        static library -> static library
    */
    TargetType thisType = thisTarget->type();
    TargetType linkLibType = linkLib->type();

    if (thisType == TargetType::Executable || thisType == TargetType::SharedLibrary)
    {
        if (linkLibType != TargetType::StaticLibrary && linkLibType != TargetType::SharedLibrary)
            LOGF(to_string(*thisTarget)
                 << " can only be linked with Static and Shared Library, not "
                 << to_string(linkLibType));
    }
    else if (thisType == TargetType::StaticLibrary)
    {
        if (linkLibType != TargetType::StaticLibrary)
            LOGF("Static library can only be linked with Static Library, not "
                 << to_string(linkLibType));
    }
    else
        LOGF("Can not link anything to " << to_string(thisType));

    if (publicNotPrivate)
        thisTarget->add_public_dependency(linkLib);
    else
        thisTarget->add_private_dependency(linkLib);

    m_linkSources.emplace_back(linkLib, publicNotPrivate);
}

void detail::LinkTrait::link_with_public(Library *linkLib) { link_impl(linkLib, true); }

void detail::LinkTrait::link_with_private(Library *linkLib) { link_impl(linkLib, false); }

void detail::LinkTrait::link_with_public(std::initializer_list<Library *> linkLibs)
{
    for (auto *linkLib : linkLibs)
        link_with_public(linkLib);
}

void detail::LinkTrait::link_with_private(std::initializer_list<Library *> linkLibs)
{
    for (auto *linkLib : linkLibs)
        link_with_private(linkLib);
}

} // namespace zimm
