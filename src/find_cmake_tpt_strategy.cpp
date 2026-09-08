#include "zimm/logger.hpp"
#include "zimm/third_party_target.hpp"

#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <generator>
#include <map>
#include <meta>
#include <optional>
#include <ranges>
#include <set>
#include <string>
#include <vector>

namespace meta = std::meta;
namespace fs = std::filesystem;
using namespace zimm;

namespace
{

using CmakeDependency = FindCmakePackageTptStrategy::Dependency;

bool contains_genex(std::string_view s) { return s.contains("$<"); }

// $<LINK_ONLY:x> → x; anything else passes through unchanged.
std::string unwrap_link_only(std::string_view entry)
{
    static constexpr std::string_view PREFIX = "$<LINK_ONLY:";
    if (entry.starts_with(PREFIX) && entry.ends_with('>'))
        return std::string{entry.substr(PREFIX.size(), entry.size() - PREFIX.size() - 1)};
    return std::string{entry};
}

// "my_lib::my_utils" → "my_utils". zimm target names must not contain "::".
std::string strip_namespace(std::string_view name)
{
    auto pos = name.rfind("::");
    return pos == std::string_view::npos ? std::string{name} : std::string{name.substr(pos + 2)};
}

std::string sanitize_pkg_name(std::string_view name)
{
    std::string safe;
    for (char c : name)
        safe += (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.')
                    ? c
                    : '_';
    return safe;
}

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

// CMake target TYPE → zimm TargetType; nullopt for types zimm can't represent.
// UNKNOWN_LIBRARY (what find-modules and hand-written configs give UNKNOWN IMPORTED)
// carries no type information — classify by resolved location: .a/.lib → static,
// .so/.dylib/extensionless → shared, no location → header-only. A location alone
// can't tell an executable from a library; every UNKNOWN target Fedora ships is a
// library, so no /usr/bin heuristic until one actually shows up.
std::optional<TargetType> zimm_type_of(std::string_view cmakeType, const std::string &location)
{
    if (cmakeType == "STATIC_LIBRARY") return TargetType::StaticLibrary;
    if (cmakeType == "SHARED_LIBRARY" || cmakeType == "MODULE_LIBRARY")
        return TargetType::SharedLibrary;
    if (cmakeType == "EXECUTABLE") return TargetType::Executable;
    if (cmakeType == "INTERFACE_LIBRARY") return TargetType::HeaderOnlyLibrary;
    if (cmakeType == "UNKNOWN_LIBRARY")
    {
        if (location.empty()) return TargetType::HeaderOnlyLibrary;
        const std::string ext = fs::path{location}.extension().string();
        if (ext == ".a" || ext == ".lib") return TargetType::StaticLibrary;
        return TargetType::SharedLibrary;
    }
    return std::nullopt;
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

// Resolve an include dir that may be relative to the config file's directory.
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

// Normalize a <Name>_LIBRARIES entry: -lX and paths pass through,
// bare identifiers get -l prefixed.
std::string normalize_lib_entry(std::string_view entry)
{
    if (entry.empty()) return {};
    if (entry.starts_with("-l")) return std::string{entry};
    if (entry.find('/') != std::string_view::npos) return std::string{entry}; // path-ish
    LOGW("FindCmakePackageTptStrategy: bare library name '" << entry << "' normalized to '-l"
                                                            << entry << "'");
    return std::format("-l{}", entry);
}

// Convert an absolute path to one relative to `base` (the tpt dir) — manifest
// paths are always relative. Returns empty on failure.
std::string to_relative(const std::string &absolutePath, const Directory &base)
{
    namespace fs = std::filesystem;
    fs::path rel = fs::path{absolutePath}.lexically_relative(base.path());
    if (rel.empty() || !rel.is_relative())
    {
        LOGW("FindCmakePackageTptStrategy: cannot express '" << absolutePath << "' relative to '"
                                                             << base.path().string() << "'");
        return {};
    }
    return rel.string();
}

struct ImportedTarget
{
    std::string name;
    std::string type;
    std::string location;
    std::string location_fallback;
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

std::optional<ParsedCmakeResult> parse_cmake_result(fs::path cmakeResultsFile)
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
            if (!value) return {};

            if constexpr (std::same_as<MemberType, std::string>)
                result.[:member:] = std::move(*value);
            else if constexpr (std::same_as<MemberType, std::vector<std::string>>)
                result.[:member:] = std::move(*value) | std::views::split(';') |
                                    std::ranges::to<std::vector<std::string>>();
            else
                static_assert(false, "only std::string and std::vector<std::string> are supported");
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
                return {};
            }

            using MemberType = typename[:meta::type_of(member):];
            if constexpr (std::same_as<MemberType, std::string>)
                importedTarget.[:member:] = std::move(*value);
            else if constexpr (std::same_as<MemberType, std::vector<std::string>>)
                importedTarget.[:member:] = std::move(*value) | std::views::split(';') |
                                            std::ranges::to<std::vector<std::string>>();
            else
                static_assert(false, "only std::string and std::vector<std::string> are supported");
        }
    }

    return result;
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
  string(APPEND _content "location=$<TARGET_PROPERTY:${{_tgt}},IMPORTED_LOCATION>\n")
  string(APPEND _content "location_fallback=$<TARGET_PROPERTY:${{_tgt}},LOCATION>\n")
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

bool check_cmake_version(const Directory &scratch)
{
    // pre-check cmake presence + version so old-distro users get a real message
    const std::string verFile = (scratch.path() / "cmake_version.txt").string();
    std::system(std::format("cmake --version > {} 2>&1", verFile).c_str());
    std::ifstream in{verFile};
    std::string line;
    int major = 0, minor = 0;
    if (std::getline(in, line)) std::sscanf(line.c_str(), "cmake version %d.%d", &major, &minor);
    if (major == 0)
    {
        LOGE("`cmake` not found on PATH — cannot find CMake packages; falling through");
        return false;
    }
    if (major < 3 || (major == 3 && minor < 25))
    {
        LOGE("zimm find-cmake needs cmake >= 3.25, found " << major << "." << minor
                                                           << " — falling through");
        return false;
    }

    return true;
}

void target_to_cmake(std::string_view cmakeNamespace, const Target *t, std::ostringstream &oss)
{
    /*
        add_library({cmakeName} {libType} IMPORTED) # or add_executable
        set_target_properties({cmakeName} PROPERTIES ZIMM_INJECTED TRUE)
        set_target_properties({cmakeName} PROPERTIES IMPORTED_LOCATION {location})
        set_target_properties({cmakeName} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES {includeDirs})...
    */
    const std::string cmakeName = cmakeNamespace.empty()
                                      ? std::string{t->name()}
                                      : std::format("{}::{}", cmakeNamespace, t->name());

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
        for (const Target *t : dep.manifest.targets()) target_to_cmake(dep.cmakeNamespace, t, oss);
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

// ---- population engine -------------------------------------------------------
//
// One pipeline shared by both config styles:
//   select_targets — the ImportedTargets to materialize (old-style configs get ONE
//                    synthesized header-only carrier "<name>_libs")
//   build_entries  — validate each target and compute its manifest entry (pass 1)
//   wire_usage     — attach usage requirements to the materialized targets (pass 2)

Directory prefix_dir(const ParsedCmakeResult &result)
{
    if (result.config.empty())
        LOGW("config did not report its own path — using '.' as the package dir");
    return result.config.empty() ? Directory::make(".") : prefix_dir_of(result.config);
}

// One materializable entry: target kind, zimm name, and a path relative to the TPT
// dir (the artifact for located targets, the include dir for header-only ones — "."
// when a header-only target carries no include dir).
struct Entry
{
    TargetType type;
    std::string name;
    std::string path;
};

// Validate one parsed CMake target and compute its manifest entry (path relative to
// prefixDir). All skip-decisions and their diagnostics live here.
std::optional<Entry> entry_of(const ImportedTarget &tgt, std::set<std::string> &seenNames,
                              const std::string &configPath, const Directory &prefixDir)
{
    const std::string zimmName = strip_namespace(tgt.name);
    if (!seenNames.insert(zimmName).second)
    {
        LOGW("target name '" << zimmName << "' (from '" << tgt.name
                             << "') collides with an already-populated target — skipping");
        return std::nullopt;
    }

    // resolve the location up front: UNKNOWN_LIBRARY classifies by it, and located
    // types need it for their manifest path — one rule (location, else fallback), no drift
    const std::string location = !tgt.location.empty() ? tgt.location : tgt.location_fallback;

    auto zimmType = zimm_type_of(tgt.type, location);
    if (!zimmType)
    {
        LOGW("unsupported imported target type '" << tgt.type << "' for '" << tgt.name
                                                  << "' — skipping");
        return std::nullopt;
    }

    std::string rel;
    if (*zimmType == TargetType::HeaderOnlyLibrary)
    {
        // INTERFACE targets are retained even when they carry no include dirs: their
        // purpose may be flags only (Boost's dynamic_linking, HPX's flag interfaces,
        // Threads::Threads), and CMake semantics keep the target regardless — dropping
        // it would strand every link reference to it. The manifest path is cosmetic
        // for header-only targets: the first include dir relative to the prefix when
        // one exists, "." otherwise.
        if (tgt.include_dirs.empty()) rel = ".";
        else rel = to_relative(resolve_against_config(tgt.include_dirs[0], configPath), prefixDir);
        if (rel.empty()) rel = "."; // include dir outside the prefix — retain regardless
    }
    else
    {
        if (location.empty())
        {
            LOGW("imported target '" << tgt.name
                                     << "' has no location — check the "
                                        "package's IMPORTED_CONFIGURATIONS or pass buildType=");
            return std::nullopt;
        }
        rel = to_relative(location, prefixDir);
    }
    if (rel.empty()) return std::nullopt;

    return Entry{*zimmType, zimmName, std::move(rel)};
}

struct BuildResult
{
    std::vector<Entry> entries;
    std::vector<bool> accepted; // parallel to the input targets
};

BuildResult build_entries(const std::vector<ImportedTarget> &tgts, const std::string &configPath,
                          const Directory &prefixDir)
{
    BuildResult out;
    std::set<std::string> seenNames;
    for (const auto &tgt : tgts)
        if (auto entry = entry_of(tgt, seenNames, configPath, prefixDir))
        {
            out.accepted.push_back(true);
            out.entries.push_back(std::move(*entry));
        }
        else out.accepted.push_back(false);
    return out;
}

// The six usage-requirement loops differ only in field, wrapper prefix, property kind,
// and whether the value resolves against the config file's directory.
struct UsageField
{
    std::vector<std::string> ImportedTarget::*field;
    char kind;               // 'I' → IncludeProperty, 'c' → compile flag, 'l' → link flag
    std::string_view prefix; // prepended to the value
    bool resolveAgainstConfig;
    bool warnOnGenex;
};

constexpr UsageField USAGE_FIELDS[] = {
    {&ImportedTarget::include_dirs, 'I', "", true, true},
    {&ImportedTarget::system_include_dirs, 'c', "-isystem ", true, false},
    {&ImportedTarget::defs, 'c', "-D", false, false},
    {&ImportedTarget::compile_opts, 'c', "", false, false},
    {&ImportedTarget::link_dirs, 'l', "-L", true, false},
    {&ImportedTarget::link_opts, 'l', "", false, false},
};

void apply_usage(Target &target, const ImportedTarget &tgt, const std::string &configPath)
{
    for (const auto &f : USAGE_FIELDS)
        for (const auto &v : tgt.*f.field)
        {
            if (contains_genex(v))
            {
                if (f.warnOnGenex)
                    LOGW("INCLUDE_DIRS of '" << tgt.name
                                             << "' contains an unresolved "
                                                "generator expression — dropped: "
                                             << v);
                continue;
            }
            std::string value = f.resolveAgainstConfig ? resolve_against_config(v, configPath) : v;
            switch (f.kind)
            {
            case 'I':
                target.add_public_property(IncludeProperty{Directory::make(std::move(value))});
                break;
            case 'c':
                target.add_public_property(CompileFlagProperty{std::string{f.prefix} + value});
                break;
            case 'l':
                target.add_public_property(LinkFlagProperty{std::string{f.prefix} + value});
                break;
            }
        }
}

void apply_link_entries(Target &target, const ImportedTarget &tgt,
                        const std::map<std::string, Target *, std::less<>> &byName)
{
    for (const auto &ref : tgt.link_libs_direct)
    {
        std::string entry = unwrap_link_only(ref);
        if (contains_genex(entry))
        {
            LOGW("unresolvable INTERFACE_LINK_LIBRARIES entry '" << ref << "' of '" << tgt.name
                                                                 << "' — dropped");
            continue;
        }
        auto depIt = byName.find(strip_namespace(entry));
        if (depIt == byName.end())
        {
            // a namespaced ref is a CMake target reference; without an injected (or
            // package-owned) target behind it it must not degrade into a raw link flag
            if (entry.contains("::"))
                LOGW("INTERFACE_LINK_LIBRARIES entry '" << ref << "' of '" << tgt.name
                                                        << "' references unknown target '" << entry
                                                        << "' — dropped");
            else target.add_public_property(LinkFlagProperty{entry});
            continue;
        }
        Target *dep = depIt->second;
        if (dep->type() == TargetType::Executable)
        {
            // executables are not Library*; link by path
            target.add_public_property(LinkFlagProperty{dep->assumed_path()->path().string()});
        }
        else if (dep->type() == TargetType::HeaderOnlyLibrary)
        {
            // nothing to link: the header-only dep's usage requirements already arrived
            // through this target's own resolved dump; no edge needed
        }
        else if (target.type() == TargetType::HeaderOnlyLibrary)
        {
            // header-only consumers have no link_with; link by path
            target.add_public_property(LinkFlagProperty{dep->assumed_path()->path().string()});
            LOGW("header-only target '" << tgt.name << "' links '" << entry
                                        << "' by path (header-only targets have no link edges)");
        }
        else
        {
            // cross-cast: LinkTrait is a sibling base of Target under Static/Shared/Executable
            dynamic_cast<detail::LinkTrait *>(&target)->link_with(public_,
                                                                  static_cast<Library *>(dep));
        }
    }
}

// Pass 2: attach usage requirements to the materialized targets. `injected` are the
// dependency targets fabricated into the wrapper — seeded LAST so a package's own
// targets win name collisions (intra-package refs like OCCT::TKernel must hit the
// package's own targets, not an injected one).
void wire_usage(const std::vector<ImportedTarget> &tgts, const BuildResult &built,
                const std::vector<Executable *> &exes, const std::vector<Library *> &libs,
                const std::string &configPath, std::span<Library *const> injected)
{
    std::map<std::string, Target *, std::less<>> byName;
    for (auto *e : exes) byName.emplace(e->name(), e);
    for (auto *l : libs) byName.emplace(l->name(), l);
    for (Library *inj : injected) byName.emplace(inj->name(), inj);

    for (size_t i = 0; i < tgts.size(); ++i)
    {
        if (!built.accepted[i]) continue;
        Target *target = byName.at(strip_namespace(tgts[i].name));

        if (target->type() != TargetType::HeaderOnlyLibrary && tgts[i].include_dirs.empty())
            LOGW("target '" << tgts[i].name
                            << "' has a library location but no "
                               "include directories — the package's config may be hand-written "
                               "with unresolved $<INSTALL_INTERFACE:...> (broken under CMake too)");

        apply_usage(*target, tgts[i], configPath);
        apply_link_entries(*target, tgts[i], byName);
    }
}

} // namespace

namespace zimm
{
FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(Directory searchPath,
                                                         std::string findPackageArgs,
                                                         std::string buildType,
                                                         std::string findPackageHints)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType)),
      m_findPackageHints(std::move(findPackageHints))
{
    m_searchDirs.push_back(std::move(searchPath));
}

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(std::vector<Directory> searchPaths,
                                                         std::string findPackageArgs,
                                                         std::string buildType,
                                                         std::string findPackageHints)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType)),
      m_findPackageHints(std::move(findPackageHints))
{
    for (auto &searchPath : searchPaths) m_searchDirs.push_back(std::move(searchPath));
}

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(std::string findPackageArgs,
                                                         std::string buildType,
                                                         std::string findPackageHints)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType)),
      m_findPackageHints(std::move(findPackageHints))
{
}

ThirdPartyTargetManifest FindCmakePackageTptStrategy::attempt(std::string_view name,
                                                              std::span<CmakeDependency> deps) const
{
    Directory scratch =
        Directory::make(std::format(".zimm_cmake_find/{}", sanitize_pkg_name(name)));

    fs::remove_all(scratch.path());
    fs::create_directories(scratch.path());

    write_cmakeliststxt_file(name, m_findPackageArgs, scratch, deps);
    const std::string cmakeCmd = cmake_cmd(scratch, m_buildType, m_searchDirs);

    if (std::system(cmakeCmd.c_str()) != 0)
    {
        LOGI("cmake find_package run failed");
        return {};
    }

    // TODO: do we really need build type suffix?
    const std::string varsFileName = std::format("vars_{}.txt", cmake_build_type(m_buildType));
    fs::path varsFilePath = scratch.path() / varsFileName;
    if (!fs::exists(varsFilePath))
    {
        LOGW("configure reported success but no vars_*.txt dump exists");
        return {};
    }

    auto cmakeResultOpt = parse_cmake_result(std::move(varsFilePath));

    if (!cmakeResultOpt) return {};
    auto cmakeResult = std::move(*cmakeResultOpt);

    if (cmakeResult.config.empty()) return {};

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

    auto cmakeTargets = cmakeResult.imported_targets;
    auto built = build_entries(cmakeTargets, cmakeResult.config, prefixDir);

    // Materialize the accepted entries under the TPT (manifest paths are always
    // relative to the tpt dir), split them for wire_usage, and report them in the
    // returned manifest.
    std::vector<Target *> assumed;
    std::vector<Executable *> exes;
    std::vector<Library *> libs;
    for (const auto &entry : built.entries)
    {
        Target *target =
            tpt->assume_target(entry.type, entry.name, detail::RelativePath{entry.path});
        assumed.push_back(target);
        if (target->type() == TargetType::Executable)
            exes.push_back(static_cast<Executable *>(target));
        else libs.push_back(static_cast<Library *>(target));
    }

    // Injected dependency targets only capture link references the package doesn't
    // resolve itself; they are never dumped, never enter built/exes/libs, and stay out
    // of the returned manifest — the user keeps the dep manifests alive.
    std::vector<Library *> injected;
    for (CmakeDependency &dep : deps)
        for (Target *depTarget : dep.manifest.targets())
            if (depTarget->type() != TargetType::Executable)
                injected.push_back(static_cast<Library *>(depTarget));

    wire_usage(cmakeTargets, built, exes, libs, cmakeResult.config, injected);
    return {tpt, std::move(assumed)};
}
} // namespace zimm
