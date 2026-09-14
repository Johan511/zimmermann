#include "zimm/target.hpp"
#include "zimm/logger.hpp"
#include "zimm/properties.hpp"

#include <string>
#include <type_traits>
#include <utility>

namespace zimm
{

void add_dependency_rel(Target *target, Target *dependency)
{
    target->m_dependsOn.push_back(dependency);
    dependency->m_isDependencyOf.push_back(target);
}

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
        case PropertyType::LinkTarget:
            return std::get<LinkTargetProperty>(a).link_lib() ==
                   std::get<LinkTargetProperty>(b).link_lib();
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

void detail::LinkTrait::link_with(const PublicTag *, Library *linkLib)
{
    Target *thisTarget = dynamic_cast<Target *>(this);
    add_dependency_rel(thisTarget, linkLib);
    thisTarget->add_property(public_, LinkTargetProperty{linkLib});
}

void detail::LinkTrait::link_with(const PrivateTag *, Library *linkLib)
{
    Target *thisTarget = dynamic_cast<Target *>(this);
    add_dependency_rel(thisTarget, linkLib);
    thisTarget->add_property(private_, LinkTargetProperty{linkLib});
}

void detail::LinkTrait::link_with(const PublicTag *, std::initializer_list<Library *> linkLibs)
{
    for (auto *linkLib : linkLibs)
        link_with(public_, linkLib);
}

void detail::LinkTrait::link_with(const PrivateTag *, std::initializer_list<Library *> linkLibs)
{
    for (auto *linkLib : linkLibs)
        link_with(private_, linkLib);
}

} // namespace zimm
