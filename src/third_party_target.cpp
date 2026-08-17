#include "zimm/third_party_target.hpp"

namespace zimm
{
std::vector<Directory> FindPackageTptStrategy::defaultPaths = {Directory{"/usr"}};

FindPackageTptStrategy::FindPackageTptStrategy(Directory searchPath)
{
    m_searchDirs.push_back(std::move(searchPath));
}

FindPackageTptStrategy::FindPackageTptStrategy(std::vector<Directory> searchPaths)
{
    for (auto &searchPath : searchPaths) m_searchDirs.push_back(std::move(searchPath));
}

FindPackageTptStrategy::FindPackageTptStrategy() : FindPackageTptStrategy(defaultPaths) {}

ThirdPartyTarget *FindPackageTptStrategy::attempt(std::string_view name) const
{
    namespace fs = std::filesystem;

    for (const Directory &searchDir : m_searchDirs)
    {
        const fs::path &path = searchDir.path();
        if (!fs::is_directory(path)) continue;

        if (path.has_filename() && path.filename().c_str() == name)
            return ThirdPartyTarget::make(std::string{name}, searchDir);

        using fs::directory_options::skip_permission_denied;
        // TODO: do we need follow_directory_symlink?
        // TODO: do we need to try-catch the iteration increment?
        for (const auto &child : fs::directory_iterator(path, skip_permission_denied))
        {
            if (!child.is_directory()) continue;
            const fs::path &childPath = child.path();
            if (childPath.filename().c_str() != name) continue;
            return ThirdPartyTarget::make(std::string{name}, Directory{childPath});
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

ThirdPartyTarget *FetchContentTptStrategy::attempt(std::string_view name) const
{
    namespace fs = std::filesystem;

    fs::create_directories(m_dir.path());
    std::string fetchContentCmd = m_fetchContentCmd.empty() ? "true" : m_fetchContentCmd;
    std::string metaBuildCmd = m_metaBuildCmd.cmd.empty() ? "true" : m_metaBuildCmd.cmd;
    MetaBuildCmd fetchAndMetaBuild =
        MetaBuildCmd{std::format("cd {} && {} && {}", m_dir.path().string(),
                                 std::move(fetchContentCmd), std::move(metaBuildCmd))};
    return ThirdPartyTarget::make(std::string{name}, m_dir, std::move(fetchAndMetaBuild),
                                  m_buildCmd);
}

ThirdPartyTarget::ThirdPartyTarget(std::string name, Directory dir, MetaBuildCmd metaBuildCmd,
                                   BuildCmd buildCmd)
    : Target(TargetType::ThirdPartyTarget, std::move(name)), m_dir(std::move(dir)),
      m_metaBuildCmd(std::move(metaBuildCmd.cmd)), m_buildCmd(std::move(buildCmd.cmd))
{
}

Executable *ThirdPartyTarget::assume_executable(std::string name, std::string pathRelToTptDir)
{
    auto target = make_executable(std::move(name));
    add_dependency_rel(target, this);
    target->m_assumedPath = m_dir.file(pathRelToTptDir);
    return target;
}

StaticLibrary *ThirdPartyTarget::assume_static_library(std::string name,
                                                       std::string pathRelToTptDir)
{
    auto target = make_static_library(std::move(name));
    add_dependency_rel(target, this);
    target->m_assumedPath = m_dir.file(pathRelToTptDir);
    return target;
}

SharedLibrary *ThirdPartyTarget::assume_shared_library(std::string name,
                                                       std::string pathRelToTptDir)
{
    auto target = make_shared_library(std::move(name));
    add_dependency_rel(target, this);
    target->m_assumedPath = m_dir.file(pathRelToTptDir);
    return target;
}

} // namespace zimm
