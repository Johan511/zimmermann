#pragma once

#include "path.hpp"
#include "target.hpp"
#include <format>
#include <functional>
#include <span>
#include <string_view>
#include <tuple>

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

class ThirdPartyTargetManifest
{
    ThirdPartyTarget *m_tpt = nullptr;
    std::vector<Target *> m_assumed;

public:
    ThirdPartyTargetManifest() = default;
    ThirdPartyTargetManifest(ThirdPartyTarget *tpt, std::vector<Target *> assumed)
        : m_tpt(tpt), m_assumed(std::move(assumed))
    {
    }

    ThirdPartyTarget *tpt() { return m_tpt; }
    const ThirdPartyTarget *tpt() const { return m_tpt; }

    std::vector<StaticLibrary *> static_libs(std::string name = "");
    std::vector<const StaticLibrary *> static_libs(std::string name = "") const;

    std::vector<SharedLibrary *> shared_libs(std::string name = "");
    std::vector<const SharedLibrary *> shared_libs(std::string name = "") const;

    std::vector<HeaderOnlyLibrary *> ho_libs(std::string name = "");
    std::vector<const HeaderOnlyLibrary *> ho_libs(std::string name = "") const;

    std::vector<Executable *> execs(std::string name = "");
    std::vector<const Executable *> execs(std::string name = "") const;

    std::vector<Target *> targets(std::string name = "");
    std::vector<const Target *> targets(std::string name = "") const;
};

// User-injected dependency for FindCmakePackageTptStrategy::attempt(): every target of
// `manifest` is fabricated into the wrapper's CMake session as an IMPORTED target named
// `<cmakeNamespace>::<name>` (empty namespace → bare name), so that the searched
// package's link references into other packages' targets resolve into real zimm link
// edges. Shallow copy — the caller keeps the manifest and its targets alive for the
// duration of the attempt() call.
struct CmakeDependency
{
    std::string cmakeNamespace;        // targets exposed as <namespace>::<name>; empty = bare name
    ThirdPartyTargetManifest manifest; // shallow-copied; caller keeps the targets alive
};

class ThirdPartyTarget;
template <typename Strategy>
concept ThirdPartyTargetStrategy = requires(const Strategy &s) {
    { s.attempt(std::string_view{} /* name */) } -> std::same_as<ThirdPartyTarget *>;
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
        ThirdPartyTarget *result{};
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

    Executable *assume_executable(std::string name, detail::RelativePath pathRelToTptDir);
    StaticLibrary *assume_static_library(std::string name, detail::RelativePath pathRelToTptDir);
    SharedLibrary *assume_shared_library(std::string name, detail::RelativePath pathRelToTptDir);
    HeaderOnlyLibrary *assumed_ho_library(std::string name, detail::RelativePath pathRelToTptDir);
    Target *assume_target(TargetType, std::string name, detail::RelativePath pathRelToTptDir);

    std::pair<std::vector<Executable *>, std::vector<Library *>>
    assume_manifest(const ThirdPartyTargetManifest &);

    std::string_view meta_build_cmd() const noexcept { return m_metaBuildCmd; }
    std::string_view build_cmd() const noexcept { return m_buildCmd; }
    const Directory &dir() const noexcept { return m_dir; }
};

struct MatchingDirPredicates
{
    using equality = std::equal_to<std::string_view>;

    class atleast_version
    {
        const std::tuple<uint64_t, uint64_t, uint64_t> majorMinorPatch;

    public:
        atleast_version(uint64_t major = -1, uint64_t minor = -1, uint64_t patch = -1)
        {
            (void)major;
            (void)minor;
            (void)patch;
        }
        bool operator()(std::string_view dir, std::string_view target)
        {
            // TODO: parse the version suffix of dir and compare against
            // majorMinorPatch
            (void)dir;
            (void)target;
            return false;
        }
    };
};

class FindPackageTptStrategy
{
    static std::vector<Directory> defaultPaths;
    std::vector<Directory> m_searchDirs;

    using MatchingDirPred = std::function<bool(std::string_view /* the search directory */,
                                               std::string_view /* targetName */)>;

    MatchingDirPred m_matchingDir;

public:
    // clang-format off
    FindPackageTptStrategy(Directory searchPath, MatchingDirPred = MatchingDirPredicates::equality{});
    FindPackageTptStrategy(std::vector<Directory> searchPaths, MatchingDirPred = MatchingDirPredicates::equality{});
    FindPackageTptStrategy(MatchingDirPred = MatchingDirPredicates::equality{});
    // clang-format on
    ThirdPartyTarget *attempt(std::string_view name) const;
};

class FindCmakePackageTptStrategy
{
    std::vector<Directory> m_searchDirs;
    std::string m_findPackageArgs;
    std::string m_buildType;
    std::string m_findPackageHints;

public:
    // clang-format off
    FindCmakePackageTptStrategy(Directory searchPath, std::string findPackageArgs = {}, std::string buildType = "relwithdebinfo", std::string findPackageHints = {});
    FindCmakePackageTptStrategy(std::vector<Directory> searchPaths, std::string findPackageArgs = {}, std::string buildType = "relwithdebinfo", std::string findPackageHints = {});
    FindCmakePackageTptStrategy(std::string findPackageArgs = {}, std::string buildType = "relwithdebinfo", std::string findPackageHints = {});
    // clang-format on
    // `deps` are user-injected already-found dependencies whose targets the wrapper
    // fabricates as IMPORTED targets under the dep's cmakeNamespace, so the searched
    // package's cross-package link references resolve. Their link edges land on this
    // package's materialized targets; they never enter the returned manifest.
    ThirdPartyTargetManifest attempt(std::string_view name, std::span<CmakeDependency> deps = {}) const;
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
                       "git fetch --depth=1 origin {2} && "
                       "git checkout FETCH_HEAD",
                       dir.path().string(), url, id);
}

} // namespace zimm
