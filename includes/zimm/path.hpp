#pragma once

#include <filesystem>
#include <source_location>
#include <string>
#include <string_view>

namespace zimm
{

class File
{
    std::filesystem::path m_path;

public:
    explicit File(std::filesystem::path path) : m_path(std::move(path)) {}
    std::filesystem::path path() && { return std::move(m_path); }
    const std::filesystem::path &path() const & { return m_path; }
};

class Directory
{
    std::filesystem::path m_path;

public:
    explicit Directory(std::filesystem::path path) : m_path(std::move(path)) {}
    std::filesystem::path path() && { return std::move(m_path); }
    const std::filesystem::path &path() const & { return m_path; }

    Directory subdir(std::string_view rel) const { return Directory{m_path / rel}; }
    File file(std::string_view rel) const { return File{m_path / rel}; }
};

namespace detail
{
// TODO: unit tests for this
inline std::filesystem::path rel_path(std::string_view path, std::source_location location)
{
    namespace fs = std::filesystem;
    fs::path absFilePath = fs::absolute(fs::path(location.file_name()));
    return std::move(absFilePath).parent_path() / path;
}
} // namespace detail

// Resolve a path relative to the directory of the file that calls this function
// (through std::source_location).
inline File rel_file(std::string relPath,
                     std::source_location loc = std::source_location::current())
{
    return File{detail::rel_path(relPath, loc)};
}

inline Directory rel_dir(std::string relPath,
                         std::source_location loc = std::source_location::current())
{
    return Directory{detail::rel_path(relPath, loc)};
}

} // namespace zimm
