#include "../includes/zimm/target.hpp"
#include "../includes/zimm/logger.hpp"

#include <string>
#include <utility>

namespace zimm
{

void add_dependency_rel(Target *target, Target *dependency)
{
    target->m_dependsOn.push_back(dependency);
    dependency->m_isDependencyOf.push_back(target);
}

void Target::add_property_impl(std::vector<PropertyObject> &properties, PropertyObject property)
{
    static constexpr auto equals = [](const Property &a, const Property &b) -> bool
    {
        if (a.type() != b.type()) return false;
        switch (a.type())
        {
        case PropertyType::Include:
            return static_cast<const IncludeProperty &>(a).include_path() ==
                   static_cast<const IncludeProperty &>(b).include_path();
        case PropertyType::CompileFlag:
            return static_cast<const CompileFlagProperty &>(a).flag() ==
                   static_cast<const CompileFlagProperty &>(b).flag();
        case PropertyType::LinkFlag:
            return static_cast<const LinkFlagProperty &>(a).flag() ==
                   static_cast<const LinkFlagProperty &>(b).flag();
        case PropertyType::LinkTarget:
            return static_cast<const LinkTargetProperty &>(a).link_lib() ==
                   static_cast<const LinkTargetProperty &>(b).link_lib();
        }
        return false;
    };

    for (auto &existing : properties)
        if (equals(*existing, *property)) return;

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
    case TargetType::ThirdPartyTarget:
        return "ThirdPartyTarget";
    case TargetType::CustomTarget:
        return "CustomTarget";
    }
    std::unreachable();
}

void detail::SourcesTrait::link_with(const PublicTag *, Library *linkLib)
{
    Target *thisTarget = dynamic_cast<Target *>(this);
    add_dependency_rel(thisTarget, linkLib);
    thisTarget->add_property(public_, LinkTargetProperty{linkLib});
}

void detail::SourcesTrait::link_with(const PrivateTag *, Library *linkLib)
{
    Target *thisTarget = dynamic_cast<Target *>(this);
    add_dependency_rel(thisTarget, linkLib);
    thisTarget->add_property(private_, LinkTargetProperty{linkLib});
}

} // namespace zimm
