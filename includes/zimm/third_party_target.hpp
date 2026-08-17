#pragma once

#include "path.hpp"
#include "target.hpp"
#include <format>

namespace zimm
{

struct BuildCmd
{
    std::string cmd;
    explicit BuildCmd(std::string cmd) : cmd(std::move(cmd)) {};
    BuildCmd() : cmd("") {};
};

struct MetaBuildCmd
{
    std::string cmd;
    explicit MetaBuildCmd(std::string cmd) : cmd(std::move(cmd)) {};
    MetaBuildCmd() : cmd("") {};
};

template <typename Strategy>
concept ThirdPartyTargetStrategy = requires(const Strategy &s) {
    { s.attempt(std::string_view{} /* name */) } -> std::same_as<class ThirdPartyTarget *>;
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
    static ThirdPartyTarget *make(std::string name, const Strategies &...strategies)
    {
        static_assert(sizeof...(strategies) > 0);
        ThirdPartyTarget *result = nullptr;
        if (!(... || (result = strategies.attempt(name), result)))
            LOGI("Failed to make third party target");
        return result;
    }

    static ThirdPartyTarget *make(std::string name, Directory dir, MetaBuildCmd metaBuildCmd = {},
                                  BuildCmd buildCmd = {})
    {
        return new ThirdPartyTarget{std::move(name), std::move(dir), std::move(metaBuildCmd),
                                    std::move(buildCmd)};
    }

    Executable *assume_executable(std::string name, std::string pathRelToTptDir);
    StaticLibrary *assume_static_library(std::string name, std::string pathRelToTptDir);
    SharedLibrary *assume_shared_library(std::string name, std::string pathRelToTptDir);

    std::string_view meta_build_cmd() const noexcept { return m_metaBuildCmd; }
    std::string_view build_cmd() const noexcept { return m_buildCmd; }
    const Directory &dir() const noexcept { return m_dir; }
};

class FindPackageTptStrategy
{
    static std::vector<Directory> defaultPaths;
    std::vector<Directory> m_searchDirs;

public:
    FindPackageTptStrategy(Directory searchPath);
    FindPackageTptStrategy(std::vector<Directory> searchPaths);
    FindPackageTptStrategy();
    ThirdPartyTarget *attempt(std::string_view name) const;
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
    ThirdPartyTarget *attempt(std::string_view name) const;
};

static_assert(ThirdPartyTargetStrategy<FindPackageTptStrategy>);
static_assert(ThirdPartyTargetStrategy<FetchContentTptStrategy>);

inline std::string git_fetch(const Directory &dir, std::string_view url, std::string_view id)
{
    return std::format("cd {0} && git init . && "
                       "git remote add origin {1} && "
                       "git fetch origin {2} && "
                       "git checkout FETCH_HEAD",
                       dir.path().string(), url, id);
}

} // namespace zimm
