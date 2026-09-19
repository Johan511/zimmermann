#pragma once

#include "logger.hpp"
#include <concepts>
#include <filesystem>
#include <format>
#include <iomanip>
#include <source_location>
#include <string>

namespace zimm
{

namespace detail
{
inline std::string to_string(const std::source_location &loc)
{
    return std::format("{}:{}", loc.file_name(), loc.line());
}

class RelativePath
{
    std::filesystem::path m_path;

public:
    // constrained so it doesn't shadow the copy/move ctors for RelativePath arguments
    template <typename P>
    requires(!std::same_as<std::remove_cvref_t<P>, RelativePath>)
    RelativePath(P &&p, std::source_location loc = std::source_location::current())
        : m_path(std::forward<P>(p))
    {
        if (m_path.empty() || !m_path.is_relative())
            LOGF("path=" << m_path
                         << " is not a non-empty relative path, call_site=" << to_string(loc));
    }
    const std::filesystem::path &path() const & noexcept { return m_path; }
    std::filesystem::path path() && noexcept { return std::move(m_path); }
};

template <typename T>
struct MakeImpl
{
    std::source_location loc;
    constexpr MakeImpl(std::source_location loc = std::source_location::current()) : loc(loc) {}

    T operator()(std::string pathStr) const
    {
        std::filesystem::path path{std::move(pathStr)};

        if (path.empty())
            LOGF("Can not be constructed from empty path=" << path << " call site = "
                                                           << detail::to_string(loc));
        path = std::filesystem::absolute(path);
        return T{std::move(path)};
    }
};

} // namespace detail

class File
{
    std::filesystem::path m_path;
    explicit File(std::filesystem::path path) : m_path(std::move(path)) {}

    friend class Directory;
    friend detail::MakeImpl<File>;
    friend File rel_file(std::string, std::source_location);

public:
    std::filesystem::path path() && { return std::move(m_path); }
    const std::filesystem::path &path() const & { return m_path; }

    using Make = detail::MakeImpl<File>;
    static File make(std::string pathStr,
                     std::source_location loc = std::source_location::current())
    {
        return Make{loc}(std::move(pathStr));
    }
};

class Directory
{
    std::filesystem::path m_path;
    explicit Directory(std::filesystem::path path) : m_path(std::move(path)) {}

    friend detail::MakeImpl<Directory>;
    friend Directory rel_dir(std::string, std::source_location);

public:
    std::filesystem::path path() && { return std::move(m_path); }
    const std::filesystem::path &path() const & { return m_path; }

    Directory subdir(detail::RelativePath rel) const { return Directory{m_path / rel.path()}; }
    File file(detail::RelativePath rel) const { return File{m_path / rel.path()}; }

    std::string dir_name() const
    {
        return m_path.has_filename() ? m_path.filename().string()
                                     : m_path.parent_path().filename().string();
    }

    using Make = detail::MakeImpl<Directory>;
    static Directory make(std::string pathStr,
                          std::source_location loc = std::source_location::current())
    {
        return Make{loc}(std::move(pathStr));
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
    return std::move(locPath).parent_path() / rel.path();
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
