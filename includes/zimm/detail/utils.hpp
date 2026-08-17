#pragma once

#include <filesystem>

namespace zimm::utils
{
static inline bool is_c_source(const std::filesystem::path &path)
{
    return path.extension() == ".c";
}
static inline bool is_cxx_source(const std::filesystem::path &path)
{
    auto ext = path.extension();
    return ext == ".cpp" || ext == ".cc" || ext == ".cxx";
}
static inline bool is_valid_source(const std::filesystem::path &path)
{
    return is_c_source(path) || is_cxx_source(path);
}
} // namespace zimm::utils