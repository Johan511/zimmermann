#include "zimm/third_party_target.hpp"
#include <ranges>

namespace zimm
{
std::vector<Directory> FindPackageTptStrategy::defaultPaths = {Directory::make("/usr")};

FindPackageTptStrategy::FindPackageTptStrategy(Directory searchPath, MatchingDirPred matchingDir)
    : m_matchingDir(std::move(matchingDir))
{
    m_searchDirs.push_back(std::move(searchPath));
}

FindPackageTptStrategy::FindPackageTptStrategy(std::vector<Directory> searchPaths,
                                               MatchingDirPred matchingDir)
    : m_matchingDir(std::move(matchingDir))
{
    for (auto &searchPath : searchPaths) m_searchDirs.push_back(std::move(searchPath));
}

FindPackageTptStrategy::FindPackageTptStrategy(MatchingDirPred matchingDir)
    : m_searchDirs(defaultPaths), m_matchingDir(std::move(matchingDir))
{
}

ThirdPartyTarget *FindPackageTptStrategy::attempt(std::string_view name) const
{
    namespace fs = std::filesystem;

    for (const Directory &searchDir : m_searchDirs)
    {
        const fs::path &path = searchDir.path();
        if (!fs::is_directory(path)) continue;

        if (m_matchingDir(searchDir.dir_name(), name))
            return ThirdPartyTarget::make(std::string{name}, searchDir);

        using fs::directory_options::skip_permission_denied;
        // TODO: do we need follow_directory_symlink?
        // TODO: do we need to try-catch the iteration increment?
        for (const auto &child : fs::directory_iterator(path, skip_permission_denied))
        {
            if (!child.is_directory()) continue;
            const fs::path &childPath = child.path();
            if (m_matchingDir(childPath.filename().c_str(), name))

                return ThirdPartyTarget::make(std::string{name},
                                              Directory::make(childPath.string()));
        }
    }
    return nullptr;
};

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

Executable *ThirdPartyTarget::assume_executable(std::string name, std::string path)
{
    auto target = make_executable(std::move(name));
    add_dependency_rel(target, this);
    target->m_assumedPath = File::make(m_dir.path() / std::move(path));
    return target;
}

StaticLibrary *ThirdPartyTarget::assume_static_library(std::string name, std::string path)
{
    auto target = make_static_library(std::move(name));
    add_dependency_rel(target, this);
    target->m_assumedPath = File::make(m_dir.path() / std::move(path));
    return target;
}

SharedLibrary *ThirdPartyTarget::assume_shared_library(std::string name, std::string path)
{
    auto target = make_shared_library(std::move(name));
    add_dependency_rel(target, this);
    target->m_assumedPath = File::make(m_dir.path() / std::move(path));
    return target;
}

HeaderOnlyLibrary *ThirdPartyTarget::assume_ho_library(std::string name, std::string path)
{
    auto target = make_header_only_library(std::move(name));
    add_dependency_rel(target, this);
    // header-only targets have assumed path; the path names their include dir
    target->add_public_property(IncludeProperty{Directory::make(m_dir.path() / std::move(path))});
    return target;
}

Target *ThirdPartyTarget::assume_target(TargetType type, std::string name, std::string path)
{
    switch (type)
    {
    case TargetType::Executable:
        return assume_executable(std::move(name), std::move(path));
    case TargetType::StaticLibrary:
        return assume_static_library(std::move(name), std::move(path));
    case TargetType::SharedLibrary:
        return assume_shared_library(std::move(name), std::move(path));
    case TargetType::HeaderOnlyLibrary:
        return assume_ho_library(std::move(name), std::move(path));
    default:
        LOGE("ThirdPartyTarget::assume_target: unsupported TargetType " << to_string(type)
                                                                        << " for '" << name << "'");
        return nullptr;
    }
}

template <typename T>
std::vector<T *> filter(auto &targets, std::optional<TargetType> type, std::string name)
{
    return targets |
           std::views::filter(
               [&](const Target *t)
               {
                   if (type && t->type() != *type) return false;
                   return name.empty() || t->name() == name;
               }) |
           std::views::transform([](Target *t) { return static_cast<T *>(t); }) |
           std::ranges::to<std::vector>();
}

std::vector<StaticLibrary *> ThirdPartyTargetManifest::static_libs(std::string name)
{
    return filter<StaticLibrary>(m_assumed, TargetType::StaticLibrary, std::move(name));
}
std::vector<SharedLibrary *> ThirdPartyTargetManifest::shared_libs(std::string name)
{
    return filter<SharedLibrary>(m_assumed, TargetType::SharedLibrary, std::move(name));
}
std::vector<HeaderOnlyLibrary *> ThirdPartyTargetManifest::ho_libs(std::string name)
{
    return filter<HeaderOnlyLibrary>(m_assumed, TargetType::HeaderOnlyLibrary, std::move(name));
}
std::vector<Executable *> ThirdPartyTargetManifest::execs(std::string name)
{
    return filter<Executable>(m_assumed, TargetType::Executable, std::move(name));
}

std::vector<Target *> ThirdPartyTargetManifest::targets(std::string name)
{
    return filter<Target>(m_assumed, std::nullopt, std::move(name));
}

std::vector<const StaticLibrary *> ThirdPartyTargetManifest::static_libs(std::string name) const
{
    return filter<const StaticLibrary>(m_assumed, TargetType::StaticLibrary, std::move(name));
}

std::vector<const SharedLibrary *> ThirdPartyTargetManifest::shared_libs(std::string name) const
{
    return filter<const SharedLibrary>(m_assumed, TargetType::SharedLibrary, std::move(name));
}

std::vector<const HeaderOnlyLibrary *> ThirdPartyTargetManifest::ho_libs(std::string name) const
{
    return filter<const HeaderOnlyLibrary>(m_assumed, TargetType::HeaderOnlyLibrary,
                                           std::move(name));
}

std::vector<const Executable *> ThirdPartyTargetManifest::execs(std::string name) const
{
    return filter<const Executable>(m_assumed, TargetType::Executable, std::move(name));
}

std::vector<const Target *> ThirdPartyTargetManifest::targets(std::string name) const
{
    return filter<const Target>(m_assumed, std::nullopt, std::move(name));
}
} // namespace zimm
