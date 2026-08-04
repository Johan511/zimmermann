#pragma once

#include "path.hpp"
#include "target.hpp"

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
    static LeakyPtr<ThirdPartyTarget> make(const Strategies &...strategies, std::string name,
                                           std::optional<Directory> dir = {},
                                           MetaBuildCmd metaBuildCmd = {}, BuildCmd buildCmd = {});

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
    LeakyPtr<class ThirdPartyTarget> attempt(std::string_view name) const;
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
    LeakyPtr<class ThirdPartyTarget> attempt(std::string_view name) const;
};

static_assert(ThirdPartyTargetStrategy<FindPackageTptStrategy>);
static_assert(ThirdPartyTargetStrategy<FetchContentTptStrategy>);

inline std::string git_fetch(std::string_view url, std::string_view id)
{
    return std::format("git init . && "
                       "git remote add origin {0} && "
                       "git fetch origin {1} && "
                       "git checkout FETCH_HEAD",
                       url, id);
}

} // namespace zimm
