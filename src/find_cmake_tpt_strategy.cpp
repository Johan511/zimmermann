#include "zimm/third_party_target.hpp"
#include "zimm/logger.hpp"

#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>

namespace zimm
{
namespace
{
// ---- helpers for FindCmakePackageTptStrategy --------------------------------

std::string replace_all(std::string str, std::string_view from, std::string_view to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
    return str;
}

std::vector<std::string> split(std::string_view str, char sep)
{
    std::vector<std::string> parts;
    size_t start = 0;
    while (true)
    {
        auto pos = str.find(sep, start);
        auto part = str.substr(start, pos == std::string_view::npos ? pos : pos - start);
        if (!part.empty()) parts.emplace_back(part);
        if (pos == std::string_view::npos) break;
        start = pos + 1;
    }
    return parts;
}

bool contains_genex(std::string_view s) { return s.find("$<") != std::string_view::npos; }

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

// zimm build types → canonical CMake config names (case-insensitive).
// Deliberately NOT naive capitalization: "Relwithdebinfo" is rejected by CMake.
std::string cmake_build_type(std::string_view buildType)
{
    std::string lower;
    lower.reserve(buildType.size());
    for (char c : buildType) lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    static const std::vector<std::pair<std::string_view, std::string_view>> mapping = {
        {"debug", "Debug"},
        {"release", "Release"},
        {"relwithdebinfo", "RelWithDebInfo"},
        {"minsizerel", "MinSizeRel"},
    };
    for (const auto &[from, to] : mapping)
        if (lower == from) return std::string{to};

    LOGW("FindCmakePackageTptStrategy: unknown build type '" << buildType
                                                             << "' — passing through to cmake as-is");
    return std::string{buildType};
}

// CMake target TYPE → zimm TargetType; nullopt for types zimm can't represent.
std::optional<TargetType> zimm_type_of(std::string_view cmakeType)
{
    if (cmakeType == "STATIC_LIBRARY") return TargetType::StaticLibrary;
    if (cmakeType == "SHARED_LIBRARY" || cmakeType == "MODULE_LIBRARY")
        return TargetType::SharedLibrary;
    if (cmakeType == "EXECUTABLE") return TargetType::Executable;
    if (cmakeType == "INTERFACE_LIBRARY") return TargetType::HeaderOnlyLibrary;
    return std::nullopt;
}

// line-oriented KEY=value parser for the wrapper's dump files
std::map<std::string, std::string> parse_dump(const std::string &path)
{
    std::map<std::string, std::string> kv;
    std::ifstream in{path};
    std::string line;
    while (std::getline(in, line))
    {
        auto eq = line.find('=');
        if (eq == std::string::npos || eq == 0) continue;
        kv.emplace(line.substr(0, eq), line.substr(eq + 1));
    }
    return kv;
}

// print the last `lines` lines of a log file
void print_tail(const std::string &logFile, size_t lines)
{
    std::ifstream in{logFile};
    if (!in)
    {
        LOGW("  (no log file at " << logFile << ")");
        return;
    }
    std::vector<std::string> buf;
    std::string line;
    while (std::getline(in, line))
    {
        buf.push_back(std::move(line));
        if (buf.size() > lines) buf.erase(buf.begin());
    }
    for (const auto &l : buf) LOGW("    " << l);
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
        LOGW("FindCmakePackageTptStrategy: relative include dir '" << dir
                                                                   << "' resolved against '"
                                                                   << base.string() << "'");
    }
    return p.lexically_normal().string();
}

// Normalize a <Name>_LIBRARIES entry: -lX and paths pass through,
// bare identifiers get -l prefixed.
std::string normalize_lib_entry(std::string_view entry)
{
    if (entry.empty()) return {};
    if (entry.starts_with("-l")) return std::string{entry};
    if (entry.find('/') != std::string_view::npos) return std::string{entry}; // path-ish
    LOGW("FindCmakePackageTptStrategy: bare library name '" << entry
                                                            << "' normalized to '-l" << entry
                                                            << "'");
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
        LOGW("FindCmakePackageTptStrategy: cannot express '" << absolutePath
                                                             << "' relative to '"
                                                             << base.path().string() << "'");
        return {};
    }
    return rel.string();
}

// The wrapper project. Findings from the genex spike (plan Step 3.4), pinned here:
//  - INCLUDE_DIRS/DEFS/COMPILE_OPTS/LINK_DIRS/LINK_OPTS via zimm_wrap resolve correctly:
//    $<BUILD_INTERFACE:...> expands to content, $<INSTALL_INTERFACE:...> expands empty
//    (installed exports bake their values at install(EXPORT) time, so real packages
//    carry plain paths), $<CXX_COMPILER_ID:...> resolves thanks to enable_language.
//  - The wrap's INTERFACE_LINK_LIBRARIES is USELESS (it holds the wrap's own link item,
//    e.g. "pkgex::core"). LINK_LIBS_DIRECT — the imported target's raw property — is the
//    authoritative channel: target refs by name, bare items wrapped as $<LINK_ONLY:x>
//    literally, which the C++ side unwraps. Anything still containing $<...> afterwards
//    is logged and dropped.
//  - $<TARGET_FILE:...> resolves IMPORTED_LOCATION_<CONFIG> with generic fallback under
//    -DCMAKE_BUILD_TYPE; paths may be un-normalized (contain "..") — C++ normalizes.
std::string wrapper_template()
{
    // clang-format off
    return R"CMAKELISTS(
cmake_minimum_required(VERSION 3.25)
project(zimm_find NONE)
enable_language(C CXX)

find_package(@PKG@ CONFIG REQUIRED @EXTRA@)

set(_vars "@SCRATCH@/vars.txt")
file(APPEND "${_vars}" "PACKAGE=@PKG@\n")
if(DEFINED @PKG@_CONFIG)
  file(APPEND "${_vars}" "CONFIG=${@PKG@_CONFIG}\n")
endif()
if(DEFINED @PKG@_DIR)
  file(APPEND "${_vars}" "DIR=${@PKG@_DIR}\n")
endif()
if(DEFINED @PKG@_INCLUDE_DIRS)
  file(APPEND "${_vars}" "INCLUDE_DIRS=${@PKG@_INCLUDE_DIRS}\n")
endif()
if(DEFINED @PKG@_LIBRARIES)
  file(APPEND "${_vars}" "LIBRARIES=${@PKG@_LIBRARIES}\n")
endif()
get_property(_imported DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
string(JOIN "|" _imported_joined ${_imported})
file(APPEND "${_vars}" "IMPORTED_TARGETS=${_imported_joined}\n")

set(_i 0)
foreach(_tgt IN LISTS _imported)
  get_target_property(_type "${_tgt}" TYPE)
  set(_content "")
  string(APPEND _content "NAME=${_tgt}\n")
  string(APPEND _content "TYPE=${_type}\n")
  if(_type STREQUAL "STATIC_LIBRARY" OR _type STREQUAL "SHARED_LIBRARY" OR
     _type STREQUAL "EXECUTABLE" OR _type STREQUAL "MODULE_LIBRARY")
    string(APPEND _content "LOCATION=$<TARGET_FILE:${_tgt}>\n")
    string(APPEND _content "LOCATION_FALLBACK=$<TARGET_PROPERTY:${_tgt},IMPORTED_LOCATION>\n")
    string(APPEND _content "IMPLIB=$<TARGET_PROPERTY:${_tgt},IMPORTED_IMPLIB>\n")
    string(APPEND _content "SONAME=$<TARGET_PROPERTY:${_tgt},IMPORTED_SONAME>\n")
  endif()
  add_library("zimm_wrap_${_i}" INTERFACE)
  target_link_libraries("zimm_wrap_${_i}" INTERFACE "${_tgt}")
  string(APPEND _content "INCLUDE_DIRS=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${_i},INTERFACE_INCLUDE_DIRECTORIES>,|>\n")
  string(APPEND _content "SYSTEM_INCLUDE_DIRS=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${_i},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>,|>\n")
  string(APPEND _content "DEFS=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${_i},INTERFACE_COMPILE_DEFINITIONS>,|>\n")
  string(APPEND _content "COMPILE_OPTS=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${_i},INTERFACE_COMPILE_OPTIONS>,|>\n")
  string(APPEND _content "LINK_DIRS=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${_i},INTERFACE_LINK_DIRECTORIES>,|>\n")
  string(APPEND _content "LINK_OPTS=$<JOIN:$<TARGET_PROPERTY:zimm_wrap_${_i},INTERFACE_LINK_OPTIONS>,|>\n")
  string(APPEND _content "LINK_LIBS_DIRECT=$<JOIN:$<TARGET_PROPERTY:${_tgt},INTERFACE_LINK_LIBRARIES>,|>\n")
  file(GENERATE OUTPUT "@SCRATCH@/tgt_${_i}.txt" CONTENT "${_content}" TARGET "zimm_wrap_${_i}")
  math(EXPR _i "${_i} + 1")
endforeach()
)CMAKELISTS";
    // clang-format on
}

// Writes the wrapper into `scratch`, runs cmake on it, returns the parsed
// vars.txt; nullopt on any failure (strategy miss).
std::optional<std::map<std::string, std::string>>
run_cmake_wrapper(const Directory &scratch, std::string_view name, std::string_view findPackageArgs,
                  std::string_view buildType, const std::vector<Directory> &searchDirs)
{
    namespace fs = std::filesystem;
    const std::string prefix = std::format("FindCmakePackageTptStrategy: '{}': ", name);

    fs::create_directories(scratch.path());
    // stale dumps from a previous run with a different target set must not be parsed
    for (const auto &entry : fs::directory_iterator(scratch.path()))
        if (entry.path().extension() == ".txt") fs::remove(entry.path());

    std::string cmakeLists = wrapper_template();
    cmakeLists = replace_all(std::move(cmakeLists), "@PKG@", name);
    cmakeLists = replace_all(std::move(cmakeLists), "@EXTRA@", findPackageArgs);
    cmakeLists = replace_all(std::move(cmakeLists), "@SCRATCH@", scratch.path().string());
    {
        std::ofstream ofs{scratch.file("CMakeLists.txt").path()};
        ofs << cmakeLists;
    }

    // pre-check cmake presence + version so old-distro users get a real message
    {
        const std::string verFile = (scratch.path() / "cmake_version.txt").string();
        std::system(std::format("cmake --version > {} 2>&1", verFile).c_str());
        std::ifstream in{verFile};
        std::string line;
        int major = 0, minor = 0;
        if (std::getline(in, line))
            std::sscanf(line.c_str(), "cmake version %d.%d", &major, &minor);
        if (major == 0)
        {
            LOGW(prefix << "`cmake` not found on PATH — cannot find CMake packages; falling through");
            return std::nullopt;
        }
        if (major < 3 || (major == 3 && minor < 25))
        {
            LOGW(prefix << "zimm find-cmake needs cmake >= 3.25, found " << major << "." << minor
                        << " — falling through");
            return std::nullopt;
        }
    }

    std::string prefixPathFlag;
    if (!searchDirs.empty())
    {
        std::string joined;
        for (const auto &d : searchDirs)
        {
            if (!joined.empty()) joined += ';';
            joined += d.path().string();
        }
        // quoted: ';' is a shell separator
        prefixPathFlag = std::format("-DCMAKE_PREFIX_PATH='{}'", joined);
    }

    const std::string buildDir = (scratch.path() / "build").string();
    const std::string logFile = (scratch.path() / "configure.log").string();
    const std::string cmd =
        std::format("cmake -S {} -B {} -DCMAKE_BUILD_TYPE={} {} > {} 2>&1", scratch.path().string(),
                    buildDir, cmake_build_type(buildType), prefixPathFlag, logFile);

    if (std::system(cmd.c_str()) != 0)
    {
        LOGW(prefix << "find_package(" << name
                    << " CONFIG) failed — see " << logFile << ". Strategy missed: falling through "
                       "to the next strategy (or failing if this was the last). Last lines:");
        print_tail(logFile, 30);
        return std::nullopt;
    }
    return parse_dump((scratch.path() / "vars.txt").string());
}

} // namespace

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(Directory searchPath,
                                                         std::string findPackageArgs,
                                                         std::string buildType)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType))
{
    m_searchDirs.push_back(std::move(searchPath));
}

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(std::vector<Directory> searchPaths,
                                                         std::string findPackageArgs,
                                                         std::string buildType)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType))
{
    for (auto &searchPath : searchPaths) m_searchDirs.push_back(std::move(searchPath));
}

FindCmakePackageTptStrategy::FindCmakePackageTptStrategy(std::string findPackageArgs,
                                                         std::string buildType)
    : m_findPackageArgs(std::move(findPackageArgs)), m_buildType(std::move(buildType))
{
    // empty search dirs → no CMAKE_PREFIX_PATH → CMake's own default search
}

namespace
{
// Materialize every imported target of the package as an assumed target, wire the
// usage requirements from the dumps onto them, and return {tpt, manifest}.
std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest>
populate_from_targets(std::string_view name, const Directory &scratch,
                      const std::map<std::string, std::string> &vars,
                      const std::vector<std::string> &importedTargets)
{
    const std::string prefix = std::format("FindCmakePackageTptStrategy: '{}': ", name);

    const std::string configPath = vars.contains("CONFIG") ? vars.at("CONFIG") : "";
    if (configPath.empty())
        LOGW(prefix << "config did not report its own path — using '.' as the package dir");
    Directory prefixDir = configPath.empty() ? Directory::make(".") : prefix_dir_of(configPath);

    ThirdPartyTarget *tpt = ThirdPartyTarget::make(std::string{name}, prefixDir);

    // ---- pass 1: parse the dumps, build the manifest (paths relative to prefixDir)
    std::vector<ThirdPartyTargetManifest::Entry> entries;
    std::vector<std::map<std::string, std::string>> dumps; // parallel to entries
    std::set<std::string> seenNames;

    for (size_t i = 0; i < importedTargets.size(); ++i)
    {
        auto dump = parse_dump((scratch.path() / std::format("tgt_{}.txt", i)).string());
        if (!dump.contains("TYPE"))
        {
            LOGW(prefix << "no dump for imported target #" << i << " — skipping");
            continue;
        }

        const std::string cmakeName = dump.contains("NAME") ? dump.at("NAME") : importedTargets[i];
        const std::string zimmName = strip_namespace(cmakeName);
        if (!seenNames.insert(zimmName).second)
        {
            LOGW(prefix << "target name '" << zimmName << "' (from '" << cmakeName
                        << "') collides with an already-populated target — skipping");
            continue;
        }

        auto zimmType = zimm_type_of(dump.at("TYPE"));
        if (!zimmType)
        {
            LOGW(prefix << "unsupported imported target type '" << dump.at("TYPE") << "' for '"
                        << cmakeName << "' — skipping");
            continue;
        }

        if (*zimmType == TargetType::HeaderOnlyLibrary)
        {
            // manifest path = the (first) include dir, relative to the prefix
            auto incDirs = split(dump["INCLUDE_DIRS"], '|');
            if (incDirs.empty())
            {
                LOGW(prefix << "header-only target '" << cmakeName
                            << "' has no include dirs — skipping");
                continue;
            }
            std::string rel =
                to_relative(resolve_against_config(incDirs[0], configPath), prefixDir);
            if (rel.empty()) continue;
            entries.emplace_back(*zimmType, zimmName, std::move(rel));
        }
        else
        {
            std::string location = dump.contains("LOCATION") ? dump.at("LOCATION") : "";
            if (location.empty() && dump.contains("LOCATION_FALLBACK"))
                location = dump.at("LOCATION_FALLBACK");
            if (location.empty())
            {
                LOGW(prefix << "imported target '" << cmakeName << "' has no location — check the "
                               "package's IMPORTED_CONFIGURATIONS or pass buildType=");
                continue;
            }
            std::string rel = to_relative(location, prefixDir);
            if (rel.empty()) continue;
            entries.emplace_back(*zimmType, zimmName, std::move(rel));
        }
        dumps.push_back(std::move(dump));
    }

    ThirdPartyTargetManifest manifest{std::move(entries)};
    auto [exes, libs] = tpt->assume_manifest(manifest);

    std::map<std::string, Target *, std::less<>> byName;
    for (auto *e : exes) byName.emplace(e->name(), e);
    for (auto *l : libs) byName.emplace(l->name(), l);

    // ---- pass 2: usage requirements onto the materialized targets
    for (const auto &dump : dumps)
    {
        const std::string zimmName = strip_namespace(dump.at("NAME"));
        auto it = byName.find(zimmName);
        if (it == byName.end()) continue;
        Target *target = it->second;
        const bool located = target->type() != TargetType::HeaderOnlyLibrary;

        auto incDirs = split(dump.at("INCLUDE_DIRS"), '|');
        if (located && incDirs.empty())
            LOGW(prefix << "target '" << dump.at("NAME") << "' has a library location but no "
                           "include directories — the package's config may be hand-written with "
                           "unresolved $<INSTALL_INTERFACE:...> (broken under CMake too)");
        for (const auto &d : incDirs)
        {
            if (contains_genex(d))
            {
                LOGW(prefix << "INCLUDE_DIRS of '" << dump.at("NAME") << "' contains an unresolved "
                               "generator expression — dropped: " << d);
                continue;
            }
            target->add_public_property(
                IncludeProperty{Directory::make(resolve_against_config(d, configPath))});
        }
        for (const auto &d : split(dump.at("SYSTEM_INCLUDE_DIRS"), '|'))
        {
            if (contains_genex(d)) continue;
            target->add_public_property(CompileFlagProperty{
                std::format("-isystem {}", resolve_against_config(d, configPath))});
        }
        for (const auto &d : split(dump.at("DEFS"), '|'))
        {
            if (contains_genex(d)) continue;
            target->add_public_property(CompileFlagProperty{"-D" + d});
        }
        for (const auto &o : split(dump.at("COMPILE_OPTS"), '|'))
        {
            if (contains_genex(o)) continue;
            target->add_public_property(CompileFlagProperty{o});
        }
        for (const auto &d : split(dump.at("LINK_DIRS"), '|'))
        {
            if (contains_genex(d)) continue;
            target->add_public_property(
                LinkFlagProperty{std::format("-L{}", resolve_against_config(d, configPath))});
        }
        for (const auto &o : split(dump.at("LINK_OPTS"), '|'))
        {
            if (contains_genex(o)) continue;
            target->add_public_property(LinkFlagProperty{o});
        }

        for (const auto &ref : split(dump.at("LINK_LIBS_DIRECT"), '|'))
        {
            std::string entry = unwrap_link_only(ref);
            if (contains_genex(entry))
            {
                LOGW(prefix << "unresolvable INTERFACE_LINK_LIBRARIES entry '" << ref << "' of '"
                            << dump.at("NAME") << "' — dropped");
                continue;
            }
            auto depIt = byName.find(strip_namespace(entry));
            if (depIt == byName.end())
            {
                target->add_public_property(LinkFlagProperty{entry});
                continue;
            }
            Target *dep = depIt->second;
            if (dep->type() == TargetType::Executable)
            {
                // executables are not Library*; link by path
                target->add_public_property(
                    LinkFlagProperty{dep->assumed_path()->path().string()});
            }
            else if (dep->type() == TargetType::HeaderOnlyLibrary)
            {
                // nothing to link: the header-only dep's usage requirements already arrived
                // through this target's own resolved dump; no edge needed
            }
            else if (target->type() == TargetType::HeaderOnlyLibrary)
            {
                // header-only consumers have no link_with; link by path
                target->add_public_property(
                    LinkFlagProperty{dep->assumed_path()->path().string()});
                LOGW(prefix << "header-only target '" << dump.at("NAME") << "' links '" << entry
                            << "' by path (header-only targets have no link edges)");
            }
            else
            {
                // cross-cast: LinkTrait is a sibling base of Target under Static/Shared/Executable
                dynamic_cast<detail::LinkTrait *>(target)->link_with(public_,
                                                                     static_cast<Library *>(dep));
            }
        }
    }

    return {tpt, std::move(manifest)};
}

// Old-style configs: no imported targets, only <Name>_INCLUDE_DIRS / <Name>_LIBRARIES.
// Synthesizes one header-only carrier target "<name>_libs".
std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest>
populate_from_variables(std::string_view name, const Directory &scratch,
                        const std::map<std::string, std::string> &vars)
{
    const std::string prefix = std::format("FindCmakePackageTptStrategy: '{}': ", name);

    const std::string configPath = vars.contains("CONFIG") ? vars.at("CONFIG") : "";
    Directory prefixDir = configPath.empty() ? Directory::make(".") : prefix_dir_of(configPath);

    ThirdPartyTarget *tpt = ThirdPartyTarget::make(std::string{name}, prefixDir);

    std::vector<std::string> incDirs;
    if (vars.contains("INCLUDE_DIRS"))
        for (const auto &d : split(vars.at("INCLUDE_DIRS"), ';'))
            incDirs.push_back(resolve_against_config(d, configPath));

    std::vector<std::string> libs;
    if (vars.contains("LIBRARIES"))
        for (const auto &l : split(vars.at("LIBRARIES"), ';'))
            libs.push_back(normalize_lib_entry(l));

    const std::string targetName = std::format("{}_libs", name);

    if (incDirs.empty() && libs.empty())
    {
        LOGI(prefix << "package found but defines no imported targets and no "
                       "<name>_INCLUDE_DIRS/<name>_LIBRARIES — nothing to link");
        return {tpt, {}};
    }

    if (incDirs.empty())
    {
        // libs only: nothing to put in the manifest's path slot — create the carrier directly
        auto *ho = make_header_only_library(targetName);
        add_dependency_rel(ho, tpt);
        for (const auto &l : libs) ho->add_public_property(LinkFlagProperty{l});
        LOGW(prefix << "package has <name>_LIBRARIES but no <name>_INCLUDE_DIRS — its '"
                    << targetName << "' target is not in the manifest");
        return {tpt, {}};
    }

    std::string rel = to_relative(incDirs[0], prefixDir);
    if (rel.empty()) return {tpt, {}};
    ThirdPartyTargetManifest manifest{std::vector<ThirdPartyTargetManifest::Entry>{
        {TargetType::HeaderOnlyLibrary, targetName, rel}}};
    auto [exes, createdLibs] = tpt->assume_manifest(manifest);
    (void)exes;
    auto *ho = createdLibs.empty() ? nullptr : createdLibs.front();
    if (ho)
    {
        for (const auto &d : incDirs)
            ho->add_public_property(IncludeProperty{Directory::make(d)});
        for (const auto &l : libs) ho->add_public_property(LinkFlagProperty{l});
    }
    return {tpt, std::move(manifest)};
}

} // namespace

std::pair<ThirdPartyTarget *, ThirdPartyTargetManifest>
FindCmakePackageTptStrategy::attempt(std::string_view name) const
{
    Directory scratch = Directory::make(std::format(".zimm_cmake_find/{}", sanitize_pkg_name(name)));

    auto vars = run_cmake_wrapper(scratch, name, m_findPackageArgs, m_buildType, m_searchDirs);
    if (!vars) return {nullptr, {}};

    std::vector<std::string> importedTargets;
    if (vars->contains("IMPORTED_TARGETS"))
        importedTargets = split(vars->at("IMPORTED_TARGETS"), '|');

    if (importedTargets.empty())
        return populate_from_variables(name, scratch, *vars);
    return populate_from_targets(name, scratch, *vars, importedTargets);
}

} // namespace zimm

