#pragma once

#include <string_view>

namespace zimm::utils
{
static inline bool is_c_source(std::string_view path) { return path.ends_with(".c"); }
static inline bool is_cxx_source(std::string_view path)
{
    return path.ends_with(".cpp") || path.ends_with(".cc") || path.ends_with(".cxx");
}
static inline bool is_valid_source(std::string_view path)
{
    return is_c_source(path) || is_cxx_source(path);
}
} // namespace zimm::utils