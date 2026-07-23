#pragma once

#include "logger.hpp"
#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>

namespace zimm
{

namespace detail
{

static inline bool is_rel_path(std::string_view path) { return !path.empty() && path[0] != '/'; }
inline std::string get_absolute_path(std::string_view path, std::source_location location)
{
    std::filesystem::path caller_file = location.file_name();
    std::filesystem::path caller_dir = caller_file.parent_path();
    return (caller_dir / path).lexically_normal().string();
}

} // namespace detail

static inline std::string rel_path(std::string relPath,
                                   std::source_location loc = std::source_location::current())
{
    return detail::get_absolute_path(relPath, loc);
}

class Directory
{
    std::string m_dirPath;

public:
    Directory(std::string dirPath) : m_dirPath(std::move(dirPath))
    {
        if (m_dirPath.empty()) { LOGF("dirPath provided is empty"); }
        if (m_dirPath.back() != '/') m_dirPath.push_back('/');
    }
    std::string_view path() const noexcept { return m_dirPath; }

    std::string make(std::string suffix) const { return m_dirPath + std::move(suffix); }
};

static inline std::string_view build_dir() { return "./"; }

} // namespace zimm
