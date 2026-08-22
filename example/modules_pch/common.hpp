// A common header precompiled via PCH and used by the importer.
#pragma once
#include <string>
#include <format>

namespace mpch {
inline std::string banner(std::string_view msg)
{
    return std::format("[modules_pch] {}", msg);
}
} // namespace mpch
