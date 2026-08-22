#pragma once

#include "logger.hpp"
#include <filesystem>
#include <source_location>
#include <string>

namespace zimm
{

namespace detail
{
class RelativePath : public std::filesystem::path
{
    using Path = std::filesystem::path;

public:
    RelativePath(auto &&p) : Path(std::forward<decltype(p)>(p))
    {
        if (empty() || !is_relative())
            LOGF("path=" << static_cast<const std::filesystem::path &>(*this)
                         << " is not a non-empty relative path");
    }
};
} // namespace detail

class File
{
    std::filesystem::path m_path;
    explicit File(std::filesystem::path path) : m_path(std::move(path)) {}

    friend class Directory;
    friend File rel_file(std::string, std::source_location);

public:
    std::filesystem::path path() && { return std::move(m_path); }
    const std::filesystem::path &path() const & { return m_path; }

    static File make(std::string pathStr)
    {
        std::filesystem::path path{std::move(pathStr)};

        if (path.empty()) LOGF("File can not be constructed from empty path = " << path);
        path = std::filesystem::absolute(std::move(path));
        return File{std::move(path)};
    }
};

class Directory
{
    std::filesystem::path m_path;
    explicit Directory(std::filesystem::path path) : m_path(std::move(path)) {}

    friend Directory rel_dir(std::string, std::source_location);

public:
    std::filesystem::path path() && { return std::move(m_path); }
    const std::filesystem::path &path() const & { return m_path; }

    Directory subdir(detail::RelativePath rel) const { return Directory{m_path / rel}; }
    File file(detail::RelativePath rel) const { return File{m_path / rel}; }

    std::string dir_name() const
    {
        return m_path.has_filename() ? m_path.filename().string()
                                     : m_path.parent_path().filename().string();
    }

    static Directory make(std::string pathStr)
    {
        if (pathStr.empty())
            LOGF("Directory can not be constructed from empty path = " << std::quoted(pathStr));

        auto path = std::filesystem::absolute(std::move(pathStr));
        return Directory{std::move(path)};
    }
};

namespace detail
{
// TODO: unit tests for this
inline std::filesystem::path rel_path(RelativePath rel, std::source_location location)
{
    namespace fs = std::filesystem;
    fs::path locPath = fs::path(location.file_name());
    if (!locPath.is_absolute())
        LOGF("zimmermann requires source_location::file_name() to be an absolute path, "
             "file_name()="
             << locPath);
    return std::move(locPath).parent_path() / rel;
}
} // namespace detail

// Resolve a path relative to the directory of the file that calls this function
// (through std::source_location).
inline File rel_file(std::string relPath,
                     std::source_location loc = std::source_location::current())
{
    return File{detail::rel_path(std::move(relPath), loc)};
}

inline Directory rel_dir(std::string relPath,
                         std::source_location loc = std::source_location::current())
{
    return Directory{detail::rel_path(std::move(relPath), loc)};
}

} // namespace zimm
