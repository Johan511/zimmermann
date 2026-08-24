#pragma once

#include "path.hpp"
#include "target.hpp"
#include <format>
#include <functional>
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
public:
    // name: verbatim CMake target name (may contain "::")
    // file / dir: ALWAYS relative to the ThirdPartyTarget's dir — for located targets the
    //             artifact path, for header-only targets their include dir (non-empty).
    using Entry = std::tuple<TargetType, std::string /*name*/, std::string /* file / dir */>;

private:
    std::vector<Entry> m_manifest;

public:
    ThirdPartyTargetManifest() = default;
    explicit ThirdPartyTargetManifest(std::vector<Entry> manifest) : m_manifest(std::move(manifest))
    {
    }

    std::span<const Entry> entries() const noexcept { return m_manifest; }

    std::vector<Entry> type(TargetType type) const noexcept;
    std::optional<Entry> name(std::string_view name) const noexcept;
    std::optional<Entry>
    name(std::function<bool(std::string_view, std::string_view)> /* name matching function */)
        const noexcept;
};

class ThirdPartyTarget;
template <typename Strategy>
concept ThirdPartyTargetStrategy = requires(const Strategy &s) {
    {
        s.attempt(std::string_view{} /* name */)
    } -> std::same_as<std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest>>;
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
    static std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest>
    make(std::string name, const Strategies &...strategies)
    {
        static_assert(sizeof...(strategies) > 0);
        std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest> result{};
        if (!(... || (result = strategies.attempt(name), result.first)))
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
    std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest> attempt(std::string_view name) const;
};

class FindCmakePackageTptStrategy
{
    std::vector<Directory> m_searchDirs; // empty default → omit CMAKE_PREFIX_PATH (CMake's own defaults)
    std::string m_findPackageArgs;      // e.g. "1.2 COMPONENTS foo", appended after "CONFIG REQUIRED"
    std::string m_buildType;            // zimm-style, default "relwithdebinfo" (matches Config::build_type)

public:
    // clang-format off
    FindCmakePackageTptStrategy(Directory searchPath, std::string findPackageArgs = {}, std::string buildType = "relwithdebinfo");
    FindCmakePackageTptStrategy(std::vector<Directory> searchPaths, std::string findPackageArgs = {}, std::string buildType = "relwithdebinfo");
    FindCmakePackageTptStrategy(std::string findPackageArgs = {}, std::string buildType = "relwithdebinfo");
    // clang-format on
    std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest> attempt(std::string_view name) const;
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
    std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest> attempt(std::string_view name) const;
};

static_assert(ThirdPartyTargetStrategy<FindPackageTptStrategy>);
static_assert(ThirdPartyTargetStrategy<FetchContentTptStrategy>);
static_assert(ThirdPartyTargetStrategy<FindCmakePackageTptStrategy>);

inline std::string git_fetch(const Directory &dir, std::string_view url, std::string_view id)
{
    return std::format("cd {0} && git init . && "
                       "git remote add origin {1} && "
                       "git fetch --depth=1 origin {2} && "
                       "git checkout FETCH_HEAD",
                       dir.path().string(), url, id);
}

} // namespace zimm
