#include "../includes/zimm/properties.hpp"
#include "../includes/zimm/target.hpp"

namespace zimm
{

Property::Property(PropertyType type) : m_type(type) {}

IncludeProperty::IncludeProperty(std::string includePath)
    : Property(PropertyType::Include), m_includePath(std::move(includePath))
{
}

CompileFlagProperty::CompileFlagProperty(std::string flag)
    : Property(PropertyType::CompileFlag), m_flags(std::move(flag))
{
}

LinkFlagProperty::LinkFlagProperty(std::string flag)
    : Property(PropertyType::LinkFlag), m_flags(std::move(flag))
{
}

LinkTargetProperty::LinkTargetProperty(const Library *target)
    : Property(PropertyType::LinkTarget), m_linkLib(target)
{
}
} // namespace zimm
