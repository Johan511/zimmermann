#include "../includes/zimm/properties.hpp"
#include "../includes/zimm/target.hpp"

namespace zimm
{

Property::Property(PropertyType type) : m_type(type) {}

IncludeProperty::IncludeProperty(std::string_view includePath)
    : Property(PropertyType::Include), m_includePath(std::string{includePath})
{
}

CompileFlagProperty::CompileFlagProperty(std::string_view flag)
    : Property(PropertyType::CompileFlag), m_flags(std::string{flag})
{
}

LinkFlagProperty::LinkFlagProperty(std::string_view flag)
    : Property(PropertyType::LinkFlag), m_flags(std::string{flag})
{
}

LinkTargetProperty::LinkTargetProperty(const Library *target)
    : Property(PropertyType::LinkTarget), m_linkLib(target)
{
}
} // namespace zimm
