#include "zimm/third_party_target.hpp"

namespace zimm
{
std::vector<std::string> FindPackageTptStrategy::defaultPaths = {"/usr/"};

FindPackageTptStrategy::FindPackageTptStrategy(std::string searchPath)
{
    m_searchPaths.push_back(std::move(searchPath));
}

FindPackageTptStrategy::FindPackageTptStrategy(std::vector<std::string> searchPaths)
{
    for (auto &searchPath : searchPaths) m_searchPaths.push_back(std::move(searchPath));
}

FindPackageTptStrategy::FindPackageTptStrategy() : FindPackageTptStrategy(defaultPaths) {}

LeakyPtr<class ThirdPartyTarget> FindPackageTptStrategy::attempt(std::string_view name) const
{
    namespace fs = std::filesystem;

    for (std::string_view pathStr : m_searchPaths)
    {
        if (pathStr.empty()) continue;

        // ensure there is no trailing slash because path{"/foo/bar/"}.filename() == ""
        if (pathStr.back() == '/') pathStr.remove_suffix(1);

        fs::path path{pathStr};
        if (!fs::is_directory(path)) continue;

        if (path.has_filename() /* handling the retarded case where path is "/" */
            && path.filename().c_str() == name)
            return ThirdPartyTarget::make(std::string{name}, Directory{std::string{path}});

        using fs::directory_options::skip_permission_denied;
        // TODO: do we need follow_directory_symlink?
        // TODO: do we need to try-catch the iteration increment?
        for (const auto &child : fs::directory_iterator(path, skip_permission_denied))
        {
            if (!child.is_directory()) continue;
            const fs::path &childPath = child.path();

            /* no need special folder name handling as directory_iterator
             * always returns path without trailing slashes */
            if (childPath.filename().c_str() != name) continue;
            return ThirdPartyTarget::make(std::string{name}, Directory{childPath.string()});
        }
    }
    return nullptr;
}

FetchContentTptStrategy::FetchContentTptStrategy(Directory dir, std::string fetchContentCmd,
                                                 MetaBuildCmd metaBuildCmd, BuildCmd buildCmd)
    : m_dir(std::move(dir)), m_fetchContentCmd(std::move(fetchContentCmd)),
      m_metaBuildCmd(std::move(metaBuildCmd)), m_buildCmd(std::move(buildCmd))
{
}

LeakyPtr<class ThirdPartyTarget> FetchContentTptStrategy::attempt(std::string_view name) const
{
    namespace fs = std::filesystem;

    fs::create_directories(m_dir.path());
    std::string fetchContentCmd = m_fetchContentCmd.empty() ? "true" : m_fetchContentCmd;
    std::string metaBuildCmd = m_metaBuildCmd.cmd.empty() ? "true" : m_metaBuildCmd.cmd;
    MetaBuildCmd fetchAndMetaBuild = MetaBuildCmd{std::format(
        "cd {} && {} && {}", m_dir.path(), std::move(fetchContentCmd), std::move(metaBuildCmd))};
    return ThirdPartyTarget::make(std::string{name}, m_dir, std::move(fetchAndMetaBuild),
                                  m_buildCmd);
}

ThirdPartyTarget::ThirdPartyTarget(std::string name, Directory dir, MetaBuildCmd metaBuildCmd,
                                   BuildCmd buildCmd)
    : Target(TargetType::ThirdPartyTarget, std::move(name)), m_dir(std::move(dir)),
      m_metaBuildCmd(std::move(metaBuildCmd.cmd)), m_buildCmd(std::move(buildCmd.cmd))
{
}

LeakyPtr<Executable> ThirdPartyTarget::assume_executable(std::string name,
                                                         std::string pathRelToTptDir)
{
    auto target = make_executable(std::move(name));
    add_dependency_rel(target.get(), this);
    target.get()->m_assumedPath = m_dir.make(pathRelToTptDir);
    return target;
}

LeakyPtr<StaticLibrary> ThirdPartyTarget::assume_static_library(std::string name,
                                                                std::string pathRelToTptDir)
{
    auto target = make_static_library(std::move(name));
    add_dependency_rel(target.get(), this);
    target.get()->m_assumedPath = m_dir.make(pathRelToTptDir);
    return target;
}

LeakyPtr<SharedLibrary> ThirdPartyTarget::assume_shared_library(std::string name,
                                                                std::string pathRelToTptDir)
{
    auto target = make_shared_library(std::move(name));
    add_dependency_rel(target.get(), this);
    target.get()->m_assumedPath = m_dir.make(pathRelToTptDir);
    return target;
}

} // namespace zimm
