#include "../includes/zimm/properties.hpp"
#include "../includes/zimm/target.hpp"

namespace zimm
{

IncludeProperty::IncludeProperty(Directory includePath) : m_includePath(std::move(includePath)) {}

CompileFlagProperty::CompileFlagProperty(std::string_view flag) : m_flags(std::string{flag}) {}

LinkFlagProperty::LinkFlagProperty(std::string_view flag) : m_flags(std::string{flag}) {}

LinkTargetProperty::LinkTargetProperty(const Library *target) : m_linkLib(target) {}
} // namespace zimm
