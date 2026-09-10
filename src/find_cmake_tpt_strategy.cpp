#include "zimm/logger.hpp"
#include "zimm/third_party_target.hpp"

#include <array>
#include <fstream>
#include <generator>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

namespace meta = std::meta;
namespace fs = std::filesystem;
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
        if (k == buildType) return v;

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
        return "INTERFACE";
    case TargetType::ThirdPartyTarget:
        return "INTERFACE";
    case TargetType::CustomTarget:
        LOGW("Can not convert zimm::TargetType=" << std::quoted(to_string(type))
                                                 << " to cmake type");
    }
    return "";
}

// Derive the package prefix from the found <Name>_CONFIG path, e.g.
//   <prefix>/lib/cmake/<name>/<name>Config.cmake  →  <prefix>
Directory prefix_dir_of(const std::string &configPath)
{
    namespace fs = std::filesystem;
    fs::path p = fs::path{configPath}.parent_path().parent_path(); // strip <file> and <name>/

    const auto isKnown = [](const fs::path &q)
    {
        const auto d = q.filename().string();
        return d == "lib" || d == "share" || d == "cmake";
    };
    while (isKnown(p) && p.has_parent_path()) p = p.parent_path();

    if (p.empty())
    {
        LOGW("FindCmakePackageTptStrategy: could not derive a prefix from config path '"
             << configPath << "' — using '.' (cosmetic; the package's cmds are empty anyway)");
        return Directory::make(".");
    }
    return Directory::make(p.string());
}

std::string resolve_against_config(std::string_view dir, std::string_view configPath)
{
    namespace fs = std::filesystem;
    fs::path p{dir};
    if (p.is_relative())
    {
        fs::path base = fs::path{configPath}.parent_path();
        p = base / p;
        LOGW("FindCmakePackageTptStrategy: relative include dir '" << dir << "' resolved against '"
                                                                   << base.string() << "'");
    }
    return p.string();
}

std::string normalize_lib_entry(std::string_view entry)
{
    if (entry.empty()) return {};
    if (entry.starts_with("-l")) return std::string{entry};
    if (entry.find('/') != std::string_view::npos) return std::string{entry}; // path-ish
    LOGI("FindCmakePackageTptStrategy: bare library name '" << entry << "' normalized to '-l"
                                                            << entry << "'");
    return std::format("-l{}", entry);
}

struct ImportedTarget
{
    std::string name;
    std::string type;
    std::string location;
    std::string implib;
    std::string soname;
    std::vector<std::string> include_dirs;
    std::vector<std::string> system_include_dirs;
    std::vector<std::string> defs;
    std::vector<std::string> compile_opts;
    std::vector<std::string> link_dirs;
    std::vector<std::string> link_opts;
    std::vector<std::string> link_libs_direct;
};

struct ParsedCmakeResult
{
    std::string package;
    std::string config;
    std::string dir;
    std::vector<std::string> include_dirs;
    std::vector<std::string> libraries;

    std::vector<ImportedTarget> imported_targets;
};

std::optional<ParsedCmakeResult> normalize(ParsedCmakeResult cmakeResult)
{
    for (ImportedTarget &iTgt : cmakeResult.imported_targets)
    {
        if (iTgt.type == "UNKNOWN_LIBRARY")
        {
            const std::string ext = fs::path{iTgt.location}.extension().string();

            if (iTgt.location.empty()) iTgt.type = "INTERFACE_LIBRARY";
            else if (ext == ".a" || ext == ".lib") iTgt.type = "STATIC_LIBRARY";
            else if (ext == ".so" || ext == ".dll") iTgt.type = "SHARED_LIBRARY";
        }
    }

    return cmakeResult;
}

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

    auto parse_next =
        [&, lineNum = 0ull](std::string_view expectedKey) mutable -> std::optional<std::string>
    {
        lineNum++;
        if (!std::getline(in, line))
        {
            LOGE("CmakeResult: ParseError in line=" << lineNum << " failed to getline");
            return std::nullopt;
        }
        auto eqPosn = line.find('=');
        if (eqPosn == std::string::npos) return std::nullopt;
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

    static constexpr auto parse_member = [](auto &member, std::string value)
    {
        using MemberType = std::remove_reference_t<decltype(member)>;
        if constexpr (std::same_as<MemberType, std::string>) member = std::move(value);
        else if constexpr (std::same_as<MemberType, std::vector<std::string>>)
            member = std::move(value) | std::views::split(';') |
                     std::ranges::to<std::vector<std::string>>();
        else static_assert(false, "only std::string and std::vector<std::string> are supported");
    };

    template for (constexpr auto member : std::define_static_array(meta::nonstatic_data_members_of(
                      ^^ParsedCmakeResult, meta::access_context::current())))
    {
        using MemberType = typename[:meta::type_of(member):];
        if constexpr (meta::identifier_of(member) == "imported_targets")
        {
        }
        else
        {
            auto value = parse_next(meta::identifier_of(member));
            if (!value) return std::nullopt;

            parse_member(result.[:member:], std::move(*value));
        }
    }

    while (in.peek() != std::char_traits<char>::eof())
    {
        auto &importedTarget = result.imported_targets.emplace_back();
        template for (constexpr auto member :
                      std::define_static_array(meta::nonstatic_data_members_of(
                          ^^ImportedTarget, meta::access_context::current())))
        {
            auto value = parse_next(meta::identifier_of(member));
            if (!value)
            {
                LOGE("CmakeResult: truncated target block");
                return std::nullopt;
            }

            parse_member(importedTarget.[:member:], std::move(*value));
        }
    }

    return normalize(std::move(result));
}

// TODO: get rid of conditionals in the wrapper
static constexpr auto WRAPPER_TEMPLATE = R"CMAKELISTS(
cmake_minimum_required(VERSION 3.25)
project(zimm_find NONE)
enable_language(C CXX)
{3}
find_package({0} CONFIG REQUIRED {1})

set(_content "")
string(TOUPPER "{0}" _zimm_upper)
string(APPEND _content "package={0}\n")
string(APPEND _content "config=${{{0}_CONFIG}}\n")
string(APPEND _content "dir=${{{0}_DIR}}\n")
set(_pkg_inc_dirs "${{{0}_INCLUDE_DIRS}}")
if(_pkg_inc_dirs STREQUAL "")
  set(_pkg_inc_dirs "${{${{_zimm_upper}}_INCLUDE_DIRS}}")
endif()
string(APPEND _content "include_dirs=${{_pkg_inc_dirs}}\n")
set(_pkg_libs "${{{0}_LIBRARIES}}")
if(_pkg_libs STREQUAL "")
  set(_pkg_libs "${{${{_zimm_upper}}_LIBRARIES}}")
endif()
string(APPEND _content "libraries=${{_pkg_libs}}\n")

get_property(_imported DIRECTORY "${{CMAKE_CURRENT_SOURCE_DIR}}" PROPERTY IMPORTED_TARGETS)

set(_i 0)
foreach(_tgt IN LISTS _imported)
  get_target_property(_zimm_injected "${{_tgt}}" ZIMM_INJECTED)
  if(_zimm_injected)
    continue()
  endif()
  get_target_property(_type "${{_tgt}}" TYPE)
  add_library("zimm_wrap_${{_i}}" INTERFACE)
  target_link_libraries("zimm_wrap_${{_i}}" INTERFACE "${{_tgt}}")
  string(APPEND _content "name=${{_tgt}}\n")
  string(APPEND _content "type=${{_type}}\n")
  string(APPEND _content "location=$<TARGET_PROPERTY:${{_tgt}},LOCATION>\n")
  # string(APPEND _content "location_fallback=$<TARGET_PROPERTY:${{_tgt}},IMPORTED_LOCATION>\n")
  string(APPEND _content "implib=$<TARGET_PROPERTY:${{_tgt}},IMPORTED_IMPLIB>\n")
  string(APPEND _content "soname=$<TARGET_PROPERTY:${{_tgt}},IMPORTED_SONAME>\n")
  string(APPEND _content "include_dirs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_INCLUDE_DIRECTORIES>,;>\n")
  string(APPEND _content "system_include_dirs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>,;>\n")
  string(APPEND _content "defs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_COMPILE_DEFINITIONS>,;>\n")
  get_target_property(_zimm_copt "${{_tgt}}" INTERFACE_COMPILE_OPTIONS)
  if(_zimm_copt MATCHES "NOTFOUND")
    set(_zimm_copt "")
  endif()
  # genex-bearing values would make the file(GENERATE) content vary per evaluation pass
  if(_zimm_copt MATCHES "\\$<")
    set(_zimm_copt "")
  endif()
  string(APPEND _content "compile_opts=${{_zimm_copt}}\n")
  string(APPEND _content "link_dirs=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_LINK_DIRECTORIES>,;>\n")
  string(APPEND _content "link_opts=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${{_i}},INTERFACE_LINK_OPTIONS>,;>\n")
  string(APPEND _content "link_libs_direct=$<JOIN:$<TARGET_PROPERTY:${{_tgt}},INTERFACE_LINK_LIBRARIES>,;>\n")
  math(EXPR _i "${{_i}} + 1")
endforeach()
file(GENERATE OUTPUT "{2}/vars_$<CONFIG>.txt" CONTENT "${{_content}}")
)CMAKELISTS";
// clang-format on

void target_to_cmake(const Target *t, std::ostringstream &oss)
{
    /*
        add_library({cmakeName} {libType} IMPORTED) # or add_executable
        set_target_properties({cmakeName} PROPERTIES ZIMM_INJECTED TRUE)
        set_target_properties({cmakeName} PROPERTIES IMPORTED_LOCATION {location})
        set_target_properties({cmakeName} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES {includeDirs})...
    */
    const std::string_view cmakeName = t->name();

    // TODO: this only works if depedencies of third party target themselves are assumed
    // But that needn't be the case,
    // we can build something with zimm and inject that into the third party target too
    const std::string location = t->assumed_path() ? t->assumed_path()->path().string() : "";
    const std::string cmakeType = std::string{cmake_type_of(t->type())};
    const auto include_dirs_gen = [](const Target *t) -> std::generator<std::string>
    {
        for (const auto &prop : t->public_properties())
        {
            if (prop->type() != PropertyType::Include) continue;
            auto incProp = dynamic_cast<const IncludeProperty *>(prop.get());
            co_yield incProp->include_path().path().string();
        }
    };

    if (dynamic_cast<const Library *>(t))
        oss << std::format("add_library({} {} IMPORTED)\n", cmakeName, cmakeType);
    else if (dynamic_cast<const Executable *>(t))
        oss << std::format("add_executable({} IMPORTED)\n", cmakeName, cmakeType);
    else
    {
        LOGW("Why are we trying to add" << to_string(t->type()) << " to cmake?");
        return;
    }

    oss << std::format("set_target_properties({} PROPERTIES ZIMM_INJECTED TRUE)\n", cmakeName);

    if (!location.empty())
        oss << std::format("set_target_properties({} PROPERTIES IMPORTED_LOCATION {})\n", cmakeName,
                           location);

    for (const auto &incDir : include_dirs_gen(t))
        oss << std::format(
            "set_target_properties({} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES {})\n", cmakeName,
            incDir);

    oss << '\n';
}

std::string define_dependencies(std::span<const CmakeDependency> deps)
{
    std::ostringstream oss;
    for (const CmakeDependency &dep : deps)
        for (const Target *t : dep.targets()) target_to_cmake(t, oss);
    return std::move(oss).str();
}

std::string cmake_cmd(const Directory &scratchDir, std::string_view zimmBuildType,
                      std::span<const Directory> searchDirs)
{
    constexpr auto dir_to_str = [](auto &dir) { return dir.path().string(); };
    std::string prefixPathFlag =
        std::format("-DCMAKE_PREFIX_PATH='{:s}'",
                    searchDirs | std::views::transform(dir_to_str) | std::views::join_with(';'));
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

Directory prefix_dir(const ParsedCmakeResult &result)
{
    if (result.config.empty())
        LOGW("config did not report its own path — using '.' as the package dir");
    return result.config.empty() ? Directory::make(".") : prefix_dir_of(result.config);
}

std::optional<ParsedCmakeResult> run_cmake_cmd_and_parse_stdout(std::string_view cmakeCmd,
                                                                const File &stdoutFile)
{
    if (std::system(cmakeCmd.data()) != 0)
    {
        LOGI("cmake find_package run failed");
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

void wire_props(const ImportedTarget &iTgt, Target *t)
{
    // TODO: relativeness of these paths?

    for (const auto &incDir : iTgt.include_dirs)
        t->add_public_property(IncludeProperty{Directory::make(incDir)});

    // TODO: add system include property
    for (const auto &incDir : iTgt.include_dirs)
        t->add_public_property(CompileFlagProperty{std::format("-isystem {}", incDir)});

    for (const auto &def : iTgt.defs)
        t->add_public_property(CompileFlagProperty{std::format("-D{}", def)});

    t->add_public_property(CompileFlagProperty{iTgt.compile_opts | std::views::join_with(' ') |
                                               std::ranges::to<std::string>()});

    for (const auto &linkDir : iTgt.link_dirs)
        t->add_public_property(LinkFlagProperty{std::format("-L{}", linkDir)});

    t->add_public_property(LinkFlagProperty{iTgt.link_opts | std::views::join_with(' ') |
                                            std::ranges::to<std::string>()});

    // TODO: do we need to figure out the full name (libzimmermann.so vs zimmermann)
    for (const auto &linkLib : iTgt.link_libs_direct)
        t->add_public_property(LinkFlagProperty{std::format("-l{}", linkLib)});
}

class Zimmify
{
    ThirdPartyTarget &tpt;

public:
    Zimmify(ThirdPartyTarget &tpt) : tpt(tpt) {}

    Target *operator()(const ImportedTarget &iTgt)
    {
        Target *target{};

        if (iTgt.type == "STATIC_LIBRARY")
            target = tpt.assume_static_library(iTgt.name, iTgt.location);
        else if (iTgt.type == "SHARED_LIBRARY" || iTgt.type == "MODULE_LIBRARY")
            target = tpt.assume_shared_library(iTgt.name, iTgt.location);
        else if (iTgt.type == "INTERFACE_LIBRARY")
            target = tpt.assume_ho_library(iTgt.name, iTgt.location);
        else if (iTgt.type == "EXECUTABLE")
            target = tpt.assume_executable(iTgt.name, iTgt.location);
        else
        {
            LOGE("Target=" << std::quoted(iTgt.name) << " of cmakeType=" << std::quoted(iTgt.type)
                           << ", not supported");
            return nullptr;
        }

        wire_props(iTgt, target);
        return target;
    }
};

} // namespace

namespace zimm
{
FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(Directory searchPath,
                                                         std::string findPackageArgs,
                                                         std::string buildType)
    : m_searchDirs({searchPath}), m_findPackageArgs(std::move(findPackageArgs)),
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

    if (!cmakeResultOpt || cmakeResultOpt->config.empty()) return {};
    ParsedCmakeResult &cmakeResult = *cmakeResultOpt;

    Directory prefixDir = prefix_dir(cmakeResult);

    ThirdPartyTarget *tpt = ThirdPartyTarget::make(std::string{name}, prefixDir);

    // add properties from old style config to tpt
    for (const std::string &incDir : cmakeResult.include_dirs)
        tpt->add_public_property(
            IncludeProperty{Directory::make(resolve_against_config(incDir, cmakeResult.config))});
    // TODO: fix this shit, linking is a pile of shit right now, we link randomly with random shit,
    // unify it. We definitely are causing issues with public link lib properties where there exist
    // multiple instances of link object in the linked object
    for (const std::string &linkLib : cmakeResult.libraries)
        tpt->add_public_property(LinkFlagProperty{normalize_lib_entry(linkLib)});

    auto zimmTargets = cmakeResult.imported_targets | std::views::transform(Zimmify{*tpt}) |
                       std::views::filter(std::identity{}) /* filter out nullptr */ |
                       std::ranges::to<std::vector>();

    for (Target *t : zimmTargets) add_dependency_rel(tpt, t);

    return {tpt, std::move(zimmTargets)};
}
} // namespace zimm
