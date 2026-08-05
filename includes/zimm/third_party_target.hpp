#pragma once

#include "path.hpp"
#include "target.hpp"
#include <format>

namespace zimm
{

struct BuildCmd
{
    std::string buildCmd;
    explicit BuildCmd(std::string cmd) : buildCmd(std::move(cmd)) {};
    BuildCmd() : buildCmd("") {};
};

struct MetaBuildCmd
{
    std::string metaBuildCmd;
    explicit MetaBuildCmd(std::string cmd) : metaBuildCmd(std::move(cmd)) {};
    MetaBuildCmd() : metaBuildCmd("") {};
};

template <typename Strategy>
concept ThirdPartyTargetStrategy = requires(const Strategy &s) {
    { s.attempt(std::string_view{} /* name */) } -> std::same_as<LeakyPtr<class ThirdPartyTarget>>;
};

class ThirdPartyTarget : public Target
{
    Directory m_dir;
    std::string m_metaBuildCmd;
    std::string m_buildCmd;

    ThirdPartyTarget(std::string name, Directory dir, MetaBuildCmd metaBuildCmd = {},
                     BuildCmd buildCmd = {});

public:
    template <ThirdPartyTargetStrategy... Strategies>
    static LeakyPtr<ThirdPartyTarget> make(std::string name, const Strategies &...strategies)
    {
        static_assert(sizeof...(strategies) > 0);
        LeakyPtr<ThirdPartyTarget> result{};
        if (!(... || (result = strategies.attempt(name), result)))
            LOGI("Failed to make third party target");
        return result;
    }

    static LeakyPtr<ThirdPartyTarget> make(std::string name, Directory dir,
                                           MetaBuildCmd metaBuildCmd = {}, BuildCmd buildCmd = {})
    {
        return LeakyPtr<ThirdPartyTarget>(new ThirdPartyTarget{
            std::move(name), std::move(dir), std::move(metaBuildCmd), std::move(buildCmd)});
    }

    LeakyPtr<Executable> assume_executable(std::string name,
                                           std::string pathRelToThirdPartyBuildDir);
    LeakyPtr<StaticLibrary> assume_static_library(std::string name,
                                                  std::string pathRelToThirdPartyBuildDir);
    LeakyPtr<SharedLibrary> assume_shared_library(std::string name,
                                                  std::string pathRelToThirdPartyBuildDir);

    std::string_view meta_build_cmd() const noexcept { return m_metaBuildCmd; }
    std::string_view build_cmd() const noexcept { return m_buildCmd; }
    std::string_view dir() const noexcept { return m_dir.path(); }
};

class FindPackageTptStrategy
{
    static std::vector<std::string> defaultPaths;

    std::vector<std::string> m_searchPaths;

public:
    FindPackageTptStrategy(std::string searchPath);
    FindPackageTptStrategy(std::vector<std::string> searchPaths);
    FindPackageTptStrategy();
    LeakyPtr<ThirdPartyTarget> attempt(std::string_view name) const;
};

class FetchContentTptStrategy
{
    Directory m_dir;
    std::string m_fetchContentCmd;
    MetaBuildCmd m_metaBuildCmd;
    BuildCmd m_buildCmd;

public:
    FetchContentTptStrategy(Directory dir, std::string fetchContentCmd, MetaBuildCmd metaBuildCmd,
                            BuildCmd buildCmd);
    LeakyPtr<ThirdPartyTarget> attempt(std::string_view name) const;
};

static_assert(ThirdPartyTargetStrategy<FindPackageTptStrategy>);
static_assert(ThirdPartyTargetStrategy<FetchContentTptStrategy>);

inline std::string git_fetch(const Directory &dir, std::string_view url, std::string_view id)
{
    return std::format("cd {0} && git init . && "
                       "git remote add origin {1} && "
                       "git fetch origin {2} && "
                       "git checkout FETCH_HEAD",
                       dir.path(), url, id);
}

} // namespace zimm
