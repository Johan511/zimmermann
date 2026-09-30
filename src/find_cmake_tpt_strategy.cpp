#include "zimm/logger.hpp"
#include "zimm/third_party_target.hpp"

#include <array>
#include <fstream>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace meta = std::meta;
namespace fs = std::filesystem;
namespace ranges = std::ranges;
namespace views = std::views;
using namespace zimm;

namespace
{

using CmakeDependency = FindCmakePackageTptStrategy::Dependency;

constexpr std::string_view cmake_build_type(std::string_view buildType)
{
    static constexpr std::array map = {
        std::pair{"debug", "Debug"},
        std::pair{"release", "Release"},
        std::pair{"relwithdebinfo", "RelWithDebInfo"},
        std::pair{"minsizerel", "MinSizeRel"},
    };

    for (const auto &[k, v] : map)
        if (k == buildType)
            return v;

    LOGF("Invalid build type = " << std::quoted(buildType) << " received");
    std::unreachable();
}

std::string_view cmake_type_of(TargetType type)
{
    switch (type)
    {
    case TargetType::Executable:
        return "EXECUTABLE";
    case TargetType::StaticLibrary:
        return "STATIC";
    case TargetType::SharedLibrary:
        return "SHARED";
    case TargetType::HeaderOnlyLibrary:
    case TargetType::ThirdPartyTarget:
        return "INTERFACE";
    case TargetType::CustomTarget:
        LOGW("Can not convert zimm::TargetType=" << std::quoted(to_string(type))
                                                 << " to cmake type");
    }
    return "";
}

static constexpr auto trimWs = [](const auto &s) -> std::string_view
{
    std::string_view sv{s.begin(), s.end()};
    auto start = sv.find_first_not_of(" \t");
    if (start == std::string_view::npos)
        return {};
    auto end = sv.find_last_not_of(" \t");
    return sv.substr(start, end - start + 1);
};

struct ImportedTarget
{
    std::string name;
    std::string location;
    std::string type;
    std::vector<std::string> includeDirs;
    std::vector<std::string> systemIncludeDirs;
    std::vector<std::string> defs;
    std::vector<std::string> compileFlags;
    std::vector<std::string> linkFlags;
    std::vector<std::string> linkDirs;
    std::vector<std::string> linkLibs;
};

struct ParsedCmakeResult
{
    std::string packageName;
    std::string configPath;
    std::string configDir;
    std::vector<std::string> includeDirs;
    std::vector<std::string> libraries;

    std::vector<ImportedTarget> importedTargets;
};

std::optional<ParsedCmakeResult> parse_cmake_result(const fs::path &cmakeResultsFile)
{
    std::ifstream in{cmakeResultsFile};
    if (!in)
    {
        LOGI("Could not open file=" << cmakeResultsFile);
        return std::nullopt;
    }

    ParsedCmakeResult result;
    std::string line;

    auto parseNext =
        [&, lineNum = 0ull](std::string_view expectedKey) mutable -> std::optional<std::string>
    {
        lineNum++;
        if (!std::getline(in, line))
        {
            LOGE("CmakeResult: ParseError in line=" << lineNum << " failed to getline");
            return std::nullopt;
        }
        auto eqPosn = line.find('=');
        if (eqPosn == std::string::npos)
            return std::nullopt;
        std::string_view key{line.data(), eqPosn};
        if (key != expectedKey)
        {
            LOGE("CmakeResult: KeyError in line=" << lineNum
                                                  << " expected key=" << std::quoted(expectedKey)
                                                  << ", received key=" << std::quoted(key));
            return std::nullopt;
        }
        return line.substr(eqPosn + 1);
    };

    static constexpr auto parseMember = [](auto &member, std::string value)
    {
        using MemberType = std::remove_reference_t<decltype(member)>;
        if constexpr (std::same_as<MemberType, std::string>)
            member = std::move(value);
        else if constexpr (std::same_as<MemberType, std::vector<std::string>>)
            member = std::move(value) | views::split(';') | views::transform(trimWs) |
                     views::cache_latest | views::filter(ranges::size) |
                     ranges::to<std::vector<std::string>>();
        else
            static_assert(false, "only std::string and std::vector<std::string> are supported");
    };

    template for (constexpr auto member : std::define_static_array(meta::nonstatic_data_members_of(
                      ^^ParsedCmakeResult, meta::access_context::current())))
    {
        if constexpr (meta::identifier_of(member) == "importedTargets")
        {
        }
        else
        {
            auto value = parseNext(meta::identifier_of(member));
            if (!value)
                return std::nullopt;

            parseMember(result.[:member:], std::move(*value));
        }
    }

    while (in.peek() != std::char_traits<char>::eof())
    {
        auto &importedTarget = result.importedTargets.emplace_back();
        template for (constexpr auto member :
                      std::define_static_array(meta::nonstatic_data_members_of(
                          ^^ImportedTarget, meta::access_context::current())))
        {
            auto value = parseNext(meta::identifier_of(member));
            if (!value)
            {
                LOGE("CmakeResult: truncated target block");
                return std::nullopt;
            }

            parseMember(importedTarget.[:member:], std::move(*value));
        }
    }

    for (ImportedTarget &iTgt : result.importedTargets)
    {
        if (iTgt.type == "UNKNOWN_LIBRARY")
        {
            const std::string ext = fs::path{iTgt.location}.extension().string();

            if (iTgt.location.empty())
                iTgt.type = "INTERFACE_LIBRARY";
            else if (ext == ".a" || ext == ".lib")
                iTgt.type = "STATIC_LIBRARY";
            else if (ext == ".so" || ext == ".dll")
                iTgt.type = "SHARED_LIBRARY";
        }
    }

    return result;
}

// TODO: get rid of conditionals in the wrapper
constexpr auto WRAPPER_TEMPLATE = R"CMAKELISTS(
cmake_minimum_required(VERSION 3.25)
project(zimm_find NONE)
enable_language(C CXX)
{3}
find_package({0} CONFIG REQUIRED {1})

set(_content "")
string(APPEND _content "packageName={0}\n")
string(APPEND _content "configPath=${{{0}_CONFIG}}\n")
string(APPEND _content "configDir=${{{0}_DIR}}\n")
string(APPEND _content "includeDirs=${{{0}_INCLUDE_DIRS}}\n")
string(APPEND _content "libraries=${{{0}_LIBRARIES}}\n")

get_property(_imported DIRECTORY "${{CMAKE_CURRENT_SOURCE_DIR}}" PROPERTY IMPORTED_TARGETS)

set(_i 0)
foreach(_tgt IN LISTS _imported)
  get_target_property(_zimm_injected "${{_tgt}}" ZIMM_INJECTED)
  if(_zimm_injected)
    continue()
  endif()

  add_library("zimm_wrap_${{_i}}" INTERFACE)
  target_link_libraries("zimm_wrap_${{_i}}" INTERFACE "${{_tgt}}")

  string(APPEND _content "name=${{_tgt}}\n")
  string(APPEND _content "location=$<TARGET_PROPERTY:${{_tgt}},LOCATION>\n")
  string(APPEND _content "type=$<TARGET_PROPERTY:${{_tgt}},TYPE>\n")
  string(APPEND _content "includeDirs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_INCLUDE_DIRECTORIES>,;>\n")
  string(APPEND _content "systemIncludeDirs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>,;>\n")
  string(APPEND _content "defs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_COMPILE_DEFINITIONS>,;>\n")
  string(APPEND _content "compileFlags=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_COMPILE_OPTIONS>,;>\n")
  string(APPEND _content "linkFlags=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_LINK_OPTIONS>,;>\n")
  string(APPEND _content "linkDirs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_LINK_DIRECTORIES>,;>\n")
  string(APPEND _content "linkLibs=$<TARGET_GENEX_EVAL:zimm_wrap_${{_i}},$<JOIN:$<TARGET_PROPERTY:${{_tgt}},INTERFACE_LINK_LIBRARIES>,;>>\n")
  math(EXPR _i "${{_i}} + 1")
endforeach()
# // TODO: we always dont want CXX
file(GENERATE OUTPUT "{2}/vars_$<CONFIG>.txt" CONTENT "${{_content}}" CONDITION "$<COMPILE_LANGUAGE:CXX>")
)CMAKELISTS";
// clang-format on

void target_to_cmake(const Target *t, std::ostringstream &oss)
{
    /*
        add_library({cmakeName} {libType} IMPORTED) # or add_executable
        set_target_properties({cmakeName} PROPERTIES ZIMM_INJECTED TRUE)
        set_target_properties({cmakeName} PROPERTIES IMPORTED_LOCATION {location})
        set_target_properties({cmakeName} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES {includeDirs})
        set_target_properties({cmakeName} PROPERTIES INTERFACE_COMPILE_OPTIONS {compileFlags})
        set_target_properties({cmakeName} PROPERTIES INTERFACE_LINK_LIBRARIES {linkFlags})
    */
    const std::string_view cmakeName = t->name();

    // TODO: this only works if depedencies of third party target themselves are assumed
    // But that needn't be the case,
    // we can build something with zimm and inject that into the third party target too
    const std::string location = t->assumed_path() ? t->assumed_path()->path().string() : "";
    const std::string cmakeType = std::string{cmake_type_of(t->type())};

    if (dynamic_cast<const Library *>(t) || t->type() == TargetType::ThirdPartyTarget)
        oss << std::format("add_library({} {} IMPORTED)\n", cmakeName, cmakeType);
    else if (t->type() == TargetType::Executable)
        oss << std::format("add_executable({} IMPORTED)\n", cmakeName, cmakeType);
    else
    {
        LOGW(to_string(*t) << " can not be added to cmake, skipping");
        return;
    }

    oss << std::format("set_target_properties({} PROPERTIES ZIMM_INJECTED TRUE)\n", cmakeName);

    if (!location.empty())
        oss << std::format("set_target_properties({} PROPERTIES IMPORTED_LOCATION {})\n", cmakeName,
                           location);

    std::string incDirs, compileFlags, linkFlags;
    for (const auto &prop : t->public_properties())
    {
        if (auto *p = std::get_if<IncludeProperty>(&prop))
        {
            incDirs += p->include_path().path().string();
            incDirs += ';';
        }
        else if (auto *p = std::get_if<CompileFlagProperty>(&prop))
        {
            compileFlags += p->flag();
            compileFlags += ';';
        }
        else if (auto *p = std::get_if<LinkFlagProperty>(&prop))
        {
            linkFlags += p->flag();
            linkFlags += ';';
        }
    }
    if (!incDirs.empty())
        incDirs.pop_back();
    if (!compileFlags.empty())
        compileFlags.pop_back();
    if (!linkFlags.empty())
        linkFlags.pop_back();

    oss << std::format(
        "set_target_properties({} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES \"{}\")\n", cmakeName,
        incDirs);

    oss << std::format("set_target_properties({} PROPERTIES INTERFACE_COMPILE_OPTIONS \"{}\")\n",
                       cmakeName, compileFlags);

    oss << std::format("set_target_properties({} PROPERTIES INTERFACE_LINK_LIBRARIES \"{}\")\n",
                       cmakeName, linkFlags);

    oss << '\n';
}

std::string define_dependencies(std::span<const CmakeDependency> deps)
{
    std::ostringstream oss;
    for (const CmakeDependency &dep : deps)
    {
        for (const Target *t : dep.targets())
            target_to_cmake(t, oss);
        target_to_cmake(dep.tpt(), oss);
    }
    return std::move(oss).str();
}

std::string cmake_cmd(const Directory &scratchDir, std::string_view zimmBuildType,
                      std::span<const Directory> searchDirs)
{
    constexpr auto dir2Str = [](auto &dir) { return dir.path().string(); };
    std::string prefixPathFlag =
        std::format("-DCMAKE_PREFIX_PATH='{:s}'",
                    searchDirs | views::transform(dir2Str) | views::join_with(';'));
    const std::string buildDir = (scratchDir.path() / "build").string();
    const std::string logFile = (scratchDir.path() / "configure.log").string();
    const std::string scratchPath = scratchDir.path().string();
    return std::format("cmake -S {} -B {} -DCMAKE_BUILD_TYPE={} {} > {} 2>&1", scratchPath,
                       buildDir, cmake_build_type(zimmBuildType), prefixPathFlag, logFile);
}

void write_cmakeliststxt_file(std::string_view name, std::string_view findPackageArgs,
                              const Directory &scratchDir, std::span<const CmakeDependency> deps)
{
    std::string cmakeFileContent =
        std::format(WRAPPER_TEMPLATE, name, findPackageArgs, scratchDir.path().string(),
                    define_dependencies(deps));
    std::ofstream ofs{scratchDir.file("CMakeLists.txt").path()};
    ofs << cmakeFileContent;
}

std::optional<ParsedCmakeResult> run_cmake_cmd_and_parse_stdout(const std::string &cmakeCmd,
                                                                const File &stdoutFile)
{
    if (std::system(cmakeCmd.data()) != 0)
    {
        LOGE("cmake find_package run failed");
        return {};
    }

    // TODO: do we really need build type suffix?
    const auto &stdoutPath = stdoutFile.path();
    if (!fs::exists(stdoutPath))
    {
        LOGW("configure reported success but no vars_*.txt dump exists");
        return {};
    }

    return parse_cmake_result(stdoutPath);
}

Target *cmake_to_target(const ImportedTarget &iTgt, ThirdPartyTarget &tpt)
{
    Target *target{};

    if (iTgt.type == "STATIC_LIBRARY")
        target = tpt.assume_static_library(iTgt.name, iTgt.location);
    else if (iTgt.type == "SHARED_LIBRARY" || iTgt.type == "MODULE_LIBRARY")
        target = tpt.assume_shared_library(iTgt.name, iTgt.location);
    else if (iTgt.type == "INTERFACE_LIBRARY")
        target = tpt.assume_ho_library(iTgt.name);
    else if (iTgt.type == "EXECUTABLE")
        target = tpt.assume_executable(iTgt.name, iTgt.location);
    else
    {
        LOGE("Target=" << std::quoted(iTgt.name) << " of cmakeType=" << std::quoted(iTgt.type)
                       << ", not supported");
        return nullptr;
    }

    for (const auto &incDir : iTgt.includeDirs)
        target->add_public_property(IncludeProperty{Directory::make(incDir)});

    for (const auto &incDir : iTgt.systemIncludeDirs)
        target->add_public_property(CompileFlagProperty{std::format("-isystem {}", incDir)});

    for (const auto &def : iTgt.defs)
        target->add_public_property(CompileFlagProperty{std::format("-D{}", def)});

    target->add_public_property(
        CompileFlagProperty{iTgt.compileFlags | views::join_with(' ') | ranges::to<std::string>()});

    for (const auto &linkDir : iTgt.linkDirs)
        target->add_public_property(LinkFlagProperty{std::format("-L{}", linkDir)});

    target->add_public_property(
        LinkFlagProperty{iTgt.linkFlags | views::join_with(' ') | ranges::to<std::string>()});

    return target;
}

std::string link_interface(std::string_view unknownLib)
{
    // Example: "-L/usr/lib64 -lraylib", "-lm", "-Wl,-wrap=main"
    // it is a link flag, just pass it on
    if (unknownLib[0] == '-')
        return std::string{unknownLib};

    // Example: "/usr/lib64/libexpat.so"
    fs::path path{unknownLib};
    if (fs::exists(path) && fs::is_regular_file(path) && path.is_absolute())
        return std::format("-L{} -l:{}", path.parent_path().string(), path.filename());

    // Example: dl
    return "-l" + std::string{unknownLib};
}

std::string link_interface(const Target *t)
{
    switch (t->type())
    {
    case TargetType::StaticLibrary:
    case TargetType::SharedLibrary:
    {
        auto &assumedPathOpt = t->assumed_path();
        if (!assumedPathOpt)
        {
            LOGE("Target=" << to_string(t) << " is not assumed path");
            return "";
        }
        const auto &assumedPath = *assumedPathOpt;
        const auto &dir = assumedPath.path().parent_path().string();
        const auto &filename = assumedPath.path().filename().string();
        return std::format("-L{} -l:{}", dir, filename);
    }
    case TargetType::HeaderOnlyLibrary:
        return "";
    case TargetType::Executable:
    case TargetType::ThirdPartyTarget:
    case TargetType::CustomTarget:
        LOGE("Unsupported target type " << to_string(t) << " for link interface, ignoring it");
        return "";
    }
    std::unreachable();
}
} // namespace

namespace zimm
{
FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(Directory searchPath,
                                                         std::string findPackageArgs,
                                                         std::string buildType)
    : m_searchDirs({std::move(searchPath)}), m_findPackageArgs(std::move(findPackageArgs)),
      m_buildType(std::move(buildType))
{
}

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(std::vector<Directory> searchPaths,
                                                         std::string findPackageArgs,
                                                         std::string buildType)
    : m_searchDirs(std::from_range, std::move(searchPaths)),
      m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType))
{
}

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(std::string findPackageArgs,
                                                         std::string buildType)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType))
{
}

ThirdPartyTargetManifest FindCmakePackageTptStrategy::attempt(std::string_view name,
                                                              std::span<CmakeDependency> deps) const
{
    Directory scratch = Directory::make(std::format(".zimm_cmake_find/{}", name));

    fs::remove_all(scratch.path());
    fs::create_directories(scratch.path());

    write_cmakeliststxt_file(name, m_findPackageArgs, scratch, deps);

    std::optional<ParsedCmakeResult> cmakeResultOpt = run_cmake_cmd_and_parse_stdout(
        cmake_cmd(scratch, m_buildType, m_searchDirs),
        scratch.file(std::format("vars_{}.txt", cmake_build_type(m_buildType))));

    if (!cmakeResultOpt)
        return {};
    ParsedCmakeResult &cmakeResult = *cmakeResultOpt;

    // Directory is just a placeholder for cmake tpt, it has no meaning
    ThirdPartyTarget *tpt =
        ThirdPartyTarget::make(std::string{name}, Directory::make(cmakeResult.configDir));

    // add properties from old style config to tpt
    for (const std::string &incDir : cmakeResult.includeDirs)
        tpt->add_public_property(IncludeProperty{Directory::make(incDir)});

    // TODO: replace this with link targets
    // for (const std::string &linkLib : cmakeResult.libraries)
    //     tpt->add_public_property(LinkFlagProperty{std::format("-l{}", linkLib)});

    // targetName -> <target, span of linkLibs>
    using StringSpan = std::span<const std::string>;
    std::unordered_map<std::string_view, std::pair<Target *, StringSpan>> zimmTargetsMap;

    const auto tryEmplace = [&zimmTargetsMap](Target *t, StringSpan depNames)
    {
        auto [iter, inserted] = zimmTargetsMap.try_emplace(t->name(), t, depNames);
        zimmTargetsMap.try_emplace(t->name(), t, std::span<const std::string>{});
        if (!inserted)
            LOGW("Could not insert target (" << to_string(t) << ") because (" << iter->second.first
                                             << ") already exists in map");
        return inserted;
    };

    for (auto &cmakeDep : deps)
    {
        for (auto t : cmakeDep.targets())
            tryEmplace(t, {});
        tryEmplace(cmakeDep.tpt(), {});
    }

    std::vector<Target *> importedTargets;
    for (const ImportedTarget &iTgt : cmakeResult.importedTargets)
    {
        Target *t = cmake_to_target(iTgt, *tpt);
        if (!t)
            continue;
        if (tryEmplace(t, iTgt.linkLibs))
            importedTargets.push_back(t);
    }

    for (auto [_, v] : zimmTargetsMap)
    {
        auto [target, linkLibs] = v;
        for (std::string_view libName : linkLibs)
        {
            auto libIter = zimmTargetsMap.find(libName);
            if (libIter == zimmTargetsMap.end())
            {
                if (auto linkFlag = link_interface(libName); !linkFlag.empty())
                    target->add_public_property(LinkFlagProperty{std::move(linkFlag)});
            }
            else
            {
                Target *libTarget = libIter->second.first;
                if (auto linkFlag = link_interface(libTarget); !linkFlag.empty())
                    target->add_public_property(LinkFlagProperty{std::move(linkFlag)});
                target->add_public_dependency(libTarget);
            }
        }
    }

    return {tpt, importedTargets};
}
} // namespace zimm
