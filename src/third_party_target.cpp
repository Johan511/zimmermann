#include "zimm/third_party_target.hpp"
#include <iomanip>

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

    for (fs::path path : m_searchPaths)
    {
        if (!fs::exists(path) || !fs::is_directory(path)) continue;

        fs::path folderName =
            path.filename().empty() ? path.parent_path().filename() : path.filename();
        if (folderName == name)
            return ThirdPartyTarget::make(std::string{name}, Directory{std::string{path}});

        for (const auto &child : fs::directory_iterator(path))
        {
            if (!child.is_directory()) continue;

            const fs::path &childPath = child.path();
            if (childPath.filename() != name) continue;
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
    std::string fetchCmd = std::format("cd {} && {}", m_dir.path(), m_fetchContentCmd);
    if (std::system(fetchCmd.c_str()))
    {
        LOGD("FetchContent cmd " << std::quoted(fetchCmd) << " failed");
        return nullptr;
    }

    return ThirdPartyTarget::make(std::string{name}, m_dir, m_metaBuildCmd, m_buildCmd);
}

ThirdPartyTarget::ThirdPartyTarget(std::string name, Directory dir, MetaBuildCmd metaBuildCmd,
                                   BuildCmd buildCmd)
    : Target(TargetType::ThirdPartyTarget, std::move(name)), m_dir(std::move(dir)),
      m_metaBuildCmd(std::move(metaBuildCmd.metaBuildCmd)), m_buildCmd(std::move(buildCmd.buildCmd))
{
}

template <ThirdPartyTargetStrategy... Strategies>
LeakyPtr<ThirdPartyTarget> ThirdPartyTarget::make(const Strategies &...strategies, std::string name,
                                                  std::optional<Directory> dir,
                                                  MetaBuildCmd metaBuildCmd, BuildCmd buildCmd)
{
    LeakyPtr<ThirdPartyTarget> result{};
    if (!(... || (result = strategies.attempt(name), result)))
    {
        if (dir)
            return LeakyPtr<ThirdPartyTarget>(
                new ThirdPartyTarget{std::move(name), std::move(dir).value(),
                                     std::move(metaBuildCmd), std::move(buildCmd)});
        LOGE("Failed to make third party target");
    }
    return result;
}

LeakyPtr<Executable> ThirdPartyTarget::assume_executable(std::string name,
                                                         std::string pathRelToThirdPartyBuildDir)
{
    auto target = make_executable(std::move(name));
    add_dependency_rel(target.get(), this);
    target.get()->m_assumedPath = m_dir.make(pathRelToThirdPartyBuildDir);
    return target;
}

LeakyPtr<StaticLibrary>
ThirdPartyTarget::assume_static_library(std::string name, std::string pathRelToThirdPartyBuildDir)
{
    auto target = make_static_library(std::move(name));
    add_dependency_rel(target.get(), this);
    target.get()->m_assumedPath = m_dir.make(pathRelToThirdPartyBuildDir);
    return target;
}

LeakyPtr<SharedLibrary>
ThirdPartyTarget::assume_shared_library(std::string name, std::string pathRelToThirdPartyBuildDir)
{
    auto target = make_shared_library(std::move(name));
    add_dependency_rel(target.get(), this);
    target.get()->m_assumedPath = m_dir.make(pathRelToThirdPartyBuildDir);
    return target;
}

} // namespace zimm
