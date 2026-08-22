#include "gen_cc.hpp"
#include "zimm/project.hpp"
#include "zimm/properties.hpp"
#include "zimm/target.hpp"
#include "zimm/third_party_target.hpp"
#include <fstream>
#include <queue>

namespace fs = std::filesystem;

namespace zimm
{
__attribute__((weak)) Directory zimm_dir();

namespace
{
// forward decls to allow free ordering of helpers below
std::string ninja_target_name(const File &file);

const File &get_assumed_path(const Target &target)
{
    return static_cast<const detail::AssumedTrait &>(target).assumed_path();
};

std::string ninja_target_name(const Target &target)
{
    std::string assumedPath = get_assumed_path(target).path().string();
    if (!assumedPath.empty()) return assumedPath;

    switch (target.type())
    {
    case TargetType::Executable:
        return std::string{target.name()};
    case TargetType::StaticLibrary:
        return std::format("lib{}.a", target.name());
    case TargetType::SharedLibrary:
        return std::format("lib{}.so", target.name());
    case TargetType::ThirdPartyTarget:
        return std::format("{}_tpt", target.name());
    case TargetType::CustomTarget:
        return std::format("{}_ct", target.name());
    }
    return "Unknown";
}

std::string get_compile_flags(std::span<const PropertyObject> props)
{
    std::ostringstream oss;
    for (const auto &prop : props)
    {
        if (prop->type() == PropertyType::Include)
            oss << "-I"
                << static_cast<const IncludeProperty &>(*prop).include_path().path().string()
                << " ";
        else if (prop->type() == PropertyType::CompileFlag)
            oss << static_cast<const CompileFlagProperty &>(*prop).flag() << " ";
    }
    return std::move(oss).str();
}

std::string get_link_flags(std::span<const PropertyObject> props)
{
    std::ostringstream oss;
    for (const auto &prop : props)
    {
        if (prop->type() == PropertyType::LinkFlag)
            oss << static_cast<const LinkFlagProperty &>(*prop).flag();
    }
    return std::move(oss).str();
}

std::string get_deps_list(const Target &target)
{
    std::ostringstream oss;
    for (const auto *dep : target.dependencies()) oss << ninja_target_name(*dep) << ' ';
    return std::move(oss.str());
}

std::string get_link_sources(std::span<const PropertyObject> props)
{
    std::ostringstream oss;
    for (const auto &prop : props)
    {
        if (prop->type() != PropertyType::LinkTarget) continue;
        oss << ninja_target_name(*static_cast<const LinkTargetProperty &>(*prop).link_lib()) << " ";
    }
    return std::move(oss).str();
}

// Returns the precompiled header file for a target if one is set, else nullptr.
const File *get_precompiled_header(std::span<const PropertyObject> props)
{
    const File *result = nullptr;
    for (const auto &prop : props)
    {
        if (prop->type() == PropertyType::PrecompiledHeader)
            result = &static_cast<const PrecompiledHeaderProperty &>(*prop).header();
    }
    return result;
}

// Ninja output name for a precompiled-header artifact.
//
// GCC resolves a precompiled header for `#include`/`-include <h>` by looking for
// `<h>.gch` *adjacent to* the header file itself (it does not search include
// paths for the .gch, and for an absolute `-include` it looks next to that
// absolute header). So the .gch must be written next to the header. This
// mirrors what CMake's native PCH support and `cotire` do.
std::string pch_ninja_name(const File &header, const Directory & /*buildDir*/)
{
    return header.path().string() + ".gch";
}

// Collected view of a module interface unit for the generator.
struct ModuleInfo
{
    const Target *owner;
    const File *source;
    std::string name;
    std::vector<std::string> imports;
    std::string bmiName;   // build-dir path of the compiled module interface
    std::string objName;   // object file produced alongside the BMI
};

// BMI path for a module: build_dir/gcm.cache/<name>.gcm, matching GCC's default
// gcm.cache layout so auto-discovery also works when no mapper is used.
std::string module_bmi_name(std::string_view moduleName, const Directory &buildDir)
{
    return (buildDir.path() / "gcm.cache" / moduleName).string() + ".gcm";
}

// Object output for a module unit: same mangling as ordinary sources.
//
// The suffix is `.mo` (module object) rather than `.mod` — GCC's driver treats
// `.mod` as a Modula-2 source and tries to invoke the (often absent) `cc1gm2`
// at link time. `.mo` avoids that collision while staying distinct from the
// plain `.o` produced for ordinary translation units.
std::string module_obj_name(const File &source)
{
    return ninja_target_name(source) + ".mo";
}

// Topologically sort module units so that a module is built before any unit that
// imports it. Returns nullptr-safe ordering; cycles are a hard error.
std::vector<const ModuleInfo *>
sort_module_units(const std::vector<ModuleInfo> &units)
{
    std::unordered_map<std::string, const ModuleInfo *> byName;
    for (const auto &u : units) byName[u.name] = &u;

    // inDegree[name] = number of imported modules (that we build) not yet emitted
    std::unordered_map<std::string, size_t> inDegree;
    // reverse adjacency: for each module m, which modules import m
    std::unordered_map<std::string, std::vector<std::string>> importers;

    for (const auto &u : units)
    {
        size_t imp = 0;
        for (const auto &m : u.imports)
        {
            if (!byName.contains(m)) continue; // external module, ignore
            ++imp;
            importers[m].push_back(u.name);
        }
        inDegree[u.name] = imp;
    }

    std::queue<std::string> ready;
    for (const auto &u : units)
        if (inDegree[u.name] == 0) ready.push(u.name);

    std::vector<const ModuleInfo *> order;
    while (!ready.empty())
    {
        std::string cur = ready.front();
        ready.pop();
        order.push_back(byName[cur]);
        auto it = importers.find(cur);
        if (it == importers.end()) continue;
        for (const auto &dependent : it->second)
            if (--inDegree[dependent] == 0) ready.push(dependent);
    }

    if (order.size() != units.size()) LOGF("module dependency cycle detected");
    return order;
}

std::string ninja_target_name(const File &file /* cxx or c file */)
{
    std::string objectFilePath = file.path().string();
    objectFilePath += ".o";
    for (auto &c : objectFilePath)
        if (c == '/') c = '_';
    return objectFilePath;
}

std::string test_name(const detail::Test &test)
{
    std::ostringstream oss;
    oss << test.exec->name();
    oss << test.conflatedArgs;

    std::string testName = std::move(oss).str();
    for (auto &c : testName)
        if (!std::isalpha(static_cast<unsigned char>(c))) c = '_';
    return testName;
}

std::vector<Target *> top_sort_all_targets(std::ranges::range auto &&allTargets)
{
    std::queue<Target *> visitQueue;
    std::unordered_map<Target *, size_t> inDegreeMap;

    for (Target *t : allTargets)
    {
        auto numDeps = t->dependencies().size();
        if (numDeps == 0) visitQueue.push(t);
        inDegreeMap[t] = numDeps;
    }

    std::vector<Target *> topologicalOrder;
    while (!visitQueue.empty())
    {
        auto front = visitQueue.front();
        visitQueue.pop();

        topologicalOrder.push_back(front);
        for (Target *dependent : front->dependents())
            if (--inDegreeMap[dependent] == 0) visitQueue.push(dependent);
    }

    if (topologicalOrder.size() != allTargets.size()) LOGF("fold failed because graph has cycles");
    return topologicalOrder;
}
} // namespace

void generate_build(Project &project)
{
    auto topSortedTargets = top_sort_all_targets(project.seach_all_targets());

    for (auto tRef : topSortedTargets)
    {
        Target &t = *tRef;
        for (auto dep : t.dependencies())
            for (auto &p : dep->public_properties()) t.add_property(public_, p);
    }

    for (auto t : topSortedTargets)
    {
        if (t->type() != TargetType::ThirdPartyTarget) continue;
        auto &tpt = static_cast<const ThirdPartyTarget &>(*t);
        auto metaCmd = tpt.meta_build_cmd();
        if (metaCmd.empty()) continue;

        std::string metaStamp =
            project.build_dir().file(std::format("{}.meta.stamp", t->name())).path().string();
        std::cout << metaStamp << '\n';
        if (fs::exists(metaStamp)) continue;

        fs::create_directories(tpt.dir().path());
        std::string cmd =
            std::format("(cd {} && {}) && touch {}", tpt.dir().path().string(), metaCmd, metaStamp);
        std::cout << cmd << '\n';

        if (std::system(cmd.c_str()) != 0)
            LOGF("Warning: meta-build step for '" << tpt.name() << "' failed");
    }

    File ninjaFile{"build.ninja"};
    std::ofstream out(ninjaFile.path());
    if (!out) LOGF("Error: could not open " << ninjaFile.path().string() << " for writing");

    out << "# Generated by zimm — do not edit by hand\n";
    out << "ninja_required_version = 1.10\n\n";

    const auto &config = project.config();

    std::string gxx = config.toolchain_prefix + "g++";
    std::string gcc = config.toolchain_prefix + "gcc";
    std::string ar = config.toolchain_prefix + "ar";
    std::string ld = config.toolchain_prefix + "g++";

    out << "rule cxx\n";
    out << "  command = " << gxx << " -MD -MT $out -MF $out.d -c $in -o $out $flags\n";
    out << "  depfile = $out.d\n";
    out << "  deps = gcc\n";
    out << "  description = CXX $out\n\n";

    out << "rule cc\n";
    out << "  command = " << gcc << " -MD -MT $out -MF $out.d -c $in -o $out $flags\n";
    out << "  depfile = $out.d\n";
    out << "  deps = gcc\n";
    out << "  description = CC $out\n\n";

    // Precompiled header: compile a header (-x c++-header) to a .gch artifact.
    out << "rule pch\n";
    out << "  command = " << gxx << " -MD -MT $out -MF $out.d -x c++-header -c $in -o $out $flags\n";
    out << "  depfile = $out.d\n";
    out << "  deps = gcc\n";
    out << "  description = PCH $out\n\n";

    // C++20 module interface unit → compiled module interface (BMI) + object.
    // GCC, given -fmodules-ts and no explicit -fmodule-mapper, writes the BMI
    // to gcm.cache/<name>.gcm relative to the working directory. Ninja runs
    // every command in the build dir, so gcm.cache/ lives there uniformly and
    // importers auto-discover their dependencies. The object ($obj) is the -o
    // target.
    //
    // We deliberately do NOT use ninja's deps=gcc/depfile machinery here:
    // GCC's module depfile emits extra edges (e.g. `bmi:| obj`) that make the
    // object both an output and an order-only input of the BMI, which ninja
    // rejects as "inputs may not also have inputs". Module-to-module ordering
    // is handled explicitly via order-only deps on imported modules' objects.
    out << "rule cxx_module\n";
    out << "  command = " << gxx
        << " -fmodules-ts"
        << " -c $in -o $obj $flags\n";
    out << "  description = MOD $obj\n\n";

    out << "rule ar\n";
    out << "  command = " << ar << " rcs $out $in\n";
    out << "  description = AR $out\n\n";

    out << "# `$in` is objects to be linked together\n";
    out << "rule link\n";
    out << "  command = " << ld << " $in $libs -o $out $ldflags\n";
    out << "  description = LINK $out\n\n";

    out << "rule run_cmd\n";
    out << "  command = (cd $dir && $cmd) && touch $out\n";
    out << "  description = $desc\n\n";

    out << "rule assumed_target\n";
    out << "  command = true\n";
    out << "  description = assumed target stub\n\n";

    std::string globalCompileFlags = get_compile_flags(project.global_properties());
    std::string globalLinkFlags = get_link_flags(project.global_properties());

    GenCc genCc;

    const File &zeBuildCpp = project.main_file_path();
    Directory zeBuildCppDir{zeBuildCpp.path().parent_path()};

    std::string zimmIncludePath = zimm_dir().subdir("include").path().string();
    std::string compileCmdGuess = std::format("g++ -I{} ze_build.cpp -std=c++20", zimmIncludePath);

    genCc.add_entry(std::move(zeBuildCppDir), zeBuildCpp, std::move(compileCmdGuess));

    // --- C++20 modules: collect all module interface units across targets,
    //     topologically sort them by imports, and emit one compile edge per unit
    //     producing the object + BMI. GCC, given -fmodules-ts, writes the BMI to
    //     gcm.cache/<name>.gcm relative to the working directory; ninja runs
    //     every command in the build dir, so the cache lives there uniformly and
    //     importers auto-discover their dependencies. Module-to-module ordering
    //     is enforced with explicit order-only deps on imported modules' objects
    //     (the BMI itself is only discovered via the producing edge's depfile, so
    //     it can't be an order-only input on a clean first build — see objFor).
    std::vector<ModuleInfo> modules;
    for (const auto *targetPtr : topSortedTargets)
    {
        auto sourcesPtr = dynamic_cast<const detail::SourcesTrait *>(targetPtr);
        if (!sourcesPtr) continue;
        for (const auto &unit : sourcesPtr->module_units())
        {
            ModuleInfo info;
            info.owner = targetPtr;
            info.source = &unit.source;
            info.name = unit.name;
            info.imports = unit.imports;
            info.bmiName = module_bmi_name(unit.name, project.build_dir());
            info.objName = module_obj_name(unit.source);
            modules.push_back(std::move(info));
        }
    }

    Directory moduleCacheDir = project.build_dir().subdir("gcm.cache");
    fs::create_directories(moduleCacheDir.path());

    auto sortedModules = sort_module_units(modules);

    // Map: module name → object output (for order-only deps between module
    // edges). We depend on the *object* rather than the BMI because the BMI is
    // only discovered via the producing edge's depfile, which ninja loads after
    // that edge runs — so a depfile-discovered output can't be an order-only
    // input on a first (clean) build. The object is a declared output, so it is
    // always available for ordering, and producing it produces the BMI too.
    auto objFor = [&](std::string_view name) -> const std::string *
    {
        for (const auto &m : modules)
            if (m.name == name) return &m.objName;
        return nullptr;
    };

    for (const auto *m : sortedModules)
    {
        const Target &owner = *m->owner;
        std::string flags = globalCompileFlags;
        flags += " " + get_compile_flags(owner.public_properties());
        flags += " " + get_compile_flags(owner.private_properties());
        flags += " " + config.cxx_flags;

        // The object is the edge output. GCC's -MD depfile records the BMI
        // (gcm.cache/<name>.gcm) as a produced output too, and ninja picks that
        // up via deps=gcc — so the BMI becomes a known ninja output *without*
        // us declaring it explicitly (declaring it would conflict with the
        // depfile and trigger "inputs may not also have inputs").
        out << "build " << m->objName << ": cxx_module "
            << m->source->path().string();
        // order-only: any imported modules' objects (so the imported BMI exists
        // before this unit compiles).
        if (!m->imports.empty())
        {
            out << " ||";
            for (const auto &imp : m->imports)
                if (auto *obj = objFor(imp)) out << " " << *obj;
        }
        out << "\n";
        out << "  flags = " << flags << "\n";
        out << "  obj = " << m->objName << "\n\n";

        genCc.add_entry(project.build_dir(), *m->source,
                        std::format("{} -fmodules-ts -c {} -o {} {}", gxx,
                                    m->source->path().string(), m->objName, flags));
    }

    // A flag string every C++ consumer compile needs to participate in modules.
    // (Empty when no modules exist, so module-free builds are unaffected.)
    std::string moduleConsumerFlags;
    std::vector<std::string> moduleObjNames; // for order-only deps
    if (!modules.empty())
    {
        moduleConsumerFlags = "-fmodules-ts";
        for (const auto &m : modules) moduleObjNames.push_back(m.objName);
    }

    for (const auto *targetPtr : topSortedTargets)
    {
        const Target &target = *targetPtr;
        const File &assumedPath = get_assumed_path(target);
        auto depList = get_deps_list(target);

        // is an assumed target
        if (!assumedPath.path().empty())
        {
            out << "build " << assumedPath.path().string() << ": assumed_target";
            if (!depList.empty()) out << " | " << depList;
            out << "\n\n";
            continue;
        }

        auto ninjaName = ninja_target_name(target);
        std::string localCompileFlags = get_compile_flags(target.public_properties()) + " " +
                                        get_compile_flags(target.private_properties());

        std::string localLinkFlags = get_link_flags(target.public_properties()) + " " +
                                     get_link_flags(target.private_properties());

        // Precompiled header: emit one PCH build edge for the target and have
        // every C++ source depend on it (order-only) so it is built first, then
        // pass -include of the header so consumers pick up the .gch.
        const File *pchHeader = get_precompiled_header(target.private_properties());
        std::string pchName;
        std::string pchIncludeFlag;
        if (pchHeader)
        {
            pchName = pch_ninja_name(*pchHeader, project.build_dir());
            fs::create_directories(fs::path{pchName}.parent_path());

            std::string pchFlags = globalCompileFlags;
            pchFlags += " " + localCompileFlags;
            pchFlags += " " + config.cxx_flags;
            // NOTE: the PCH is deliberately built *without* -fmodules-ts even when
            // the target uses modules. GCC's module-aware PCH currently forces the
            // link step to require the (Modula-2) `gm2` helper, which is not
            // shipped with every GCC build. A non-module PCH is consumed fine by
            // module-compiled sources and avoids the gm2 link dependency.

            out << "build " << pchName << ": pch " << pchHeader->path().string();
            if (!depList.empty()) out << " | " << depList;
            out << "\n  flags = " << pchFlags << "\n\n";

            // -include of the header path (without .gch) makes g++ look for the
            // precompiled form automatically alongside it.
            pchIncludeFlag = std::format("-include {}", pchHeader->path().string());

            genCc.add_entry(project.build_dir(), *pchHeader,
                            std::format("{} -x c++-header -c {} -o {} {}", gxx,
                                        pchHeader->path().string(), pchName, pchFlags));
        }

        std::string sourceObjectsNinjaNames;
        bool depsEnsured = false;
        if (auto sourcesPtr = dynamic_cast<const detail::SourcesTrait *>(targetPtr))
        {
            // Module units owned by this target: their objects link in like
            // ordinary sources, and they are already emitted as cxx_module edges.
            for (const auto &unit : sourcesPtr->module_units())
            {
                std::string objName = module_obj_name(unit.source);
                sourceObjectsNinjaNames += " " + objName;
                depsEnsured = true;
            }

            std::vector<std::string> objectNinjaNames;
            for (const File &src : sourcesPtr->sources())
            {
                depsEnsured = true;
                bool isCxx = utils::is_cxx_source(src.path());
                std::string_view rule = isCxx ? "cxx" : "cc";
                std::string objectNinjaName = ninja_target_name(src);
                const auto srcStr = src.path().string();
                out << "build " << objectNinjaName << ": " << rule << " " << srcStr;
                if (!depList.empty()) out << " | " << depList;
                // order-only deps: PCH (so it's built first) + all module BMIs
                // (so an importing source compiles after the modules it may
                // import exist). We don't prescan plain sources for imports, so
                // we conservatively depend on every BMI.
                std::vector<std::string> orderOnly;
                if (pchHeader && isCxx) orderOnly.push_back(pchName);
                if (!moduleConsumerFlags.empty() && isCxx)
                {
                    for (const auto &b : moduleObjNames) orderOnly.push_back(b);
                }
                if (!orderOnly.empty())
                {
                    out << " ||";
                    for (const auto &o : orderOnly) out << " " << o;
                }
                out << '\n';
                sourceObjectsNinjaNames += " " + objectNinjaName;

                std::string flags = globalCompileFlags;
                flags += " " + localCompileFlags;
                flags += " " + (isCxx ? config.cxx_flags : config.c_flags);
                if (pchHeader && isCxx) flags += " " + pchIncludeFlag;
                if (!moduleConsumerFlags.empty() && isCxx) flags += " " + moduleConsumerFlags;
                out << "  flags = " << flags << "\n\n";

                genCc.add_entry(project.build_dir(), src,
                                std::format("{} -c {} -o {} {}", isCxx ? gxx : gcc, srcStr,
                                            objectNinjaName, flags));
            }
        }

        std::string linkSourcesNinjaNames = get_link_sources(target.public_properties()) +
                                            get_link_sources(target.private_properties());

        switch (target.type())
        {
        case TargetType::StaticLibrary:
        {
            auto &lib = static_cast<const StaticLibrary &>(target);
            out << "build " << ninja_target_name(lib) << ": ar " << sourceObjectsNinjaNames << " "
                << linkSourcesNinjaNames;
            if (!depsEnsured) out << " | " << depList;
            out << "\n\n";
            break;
        }
        case TargetType::SharedLibrary:
        {
            auto &lib = static_cast<const SharedLibrary &>(target);
            out << "build " << ninja_target_name(lib) << ": link " << sourceObjectsNinjaNames;
            if (!depsEnsured) out << " | " << depList;
            out << '\n';
            out << "  ldflags = -shared " << globalLinkFlags << " " << localLinkFlags << "\n";
            out << "  libs = -Wl,--whole-archive " << linkSourcesNinjaNames
                << "-Wl,--no-whole-archive\n\n";
            break;
        }
        case TargetType::Executable:
        {
            auto &exec = static_cast<const Executable &>(target);
            out << "build " << ninja_target_name(exec) << ": link " << sourceObjectsNinjaNames
                << " " << linkSourcesNinjaNames;
            if (!depsEnsured) out << " | " << depList;
            out << '\n';
            out << "  ldflags = " << globalLinkFlags << " " << localLinkFlags << "\n\n";
            break;
        }
        case TargetType::ThirdPartyTarget:
        {
            auto &tpt = static_cast<const ThirdPartyTarget &>(target);
            auto build = tpt.build_cmd();
            if (build.empty()) build = "true";

            // Third Party Target is not expected to have sources
            if (depsEnsured) LOGF("Why does Third Party Target have sources?");
            out << "build " << ninja_target_name(tpt) << ": run_cmd | " << depList << "\n";

            // Directory already created before executing meta command
            out << "  dir = " << tpt.dir().path().string() << "\n";
            out << "  cmd = " << build << "\n";
            out << "  desc = BUILD " << tpt.name() << "\n\n";

            break;
        }
        case TargetType::CustomTarget:
        {
            auto &ct = dynamic_cast<const detail::CustomTargetBase &>(target);
            fs::create_directories(ct.dir().path());

            out << "build";
            for (auto &o : ct.outputs()) out << " " << o;
            out << " " << ninja_target_name(target);
            out << ": run_cmd";
            for (auto &i : ct.inputs()) out << " " << i;
            out << " | " << depList << "\n";

            // Third Party Target is not expected to have sources
            if (depsEnsured) LOGF("Why does CustomTarget have sources?");

            auto genCmd = ct.generate_cmd();
            out << "  dir = " << ct.dir().path().string() << "\n";
            out << "  cmd = " << genCmd << "\n";
            out << "  desc = CUSTOM " << target.name() << "\n\n";

            break;
        }
        }
    }

    // Default targets — what `ninja` (without arguments) builds.
    // Tests are intentionally excluded: they only run via `ninja test`.
    out << "default";
    for (auto *t : project.top_level_targets()) out << " " << ninja_target_name(*t);
    out << "\n\n";

    const auto &install_files = project.installer().files();
    const auto &install_dirs = project.installer().dirs();
    const auto &install_targets = project.installer().targets();

    Directory install_root = config.install_dir;

    out << "\n# --- Install rules ---\n\n";
    out << "rule install_file\n";
    out << "  command = install -D $in $out\n";
    out << "  description = INSTALL $out\n\n";

    out << "rule install_tree\n";
    out << "  command = mkdir -p $dest && cp -r $dir/. $dest/\n";
    out << "  description = INSTALL $dir -> $dest\n\n";

    // Emit per-file install edges first, collecting phony targets.
    std::vector<std::string> phony_targets;

    auto emitInstallFile = [&](std::string_view relDest, const fs::path &src)
    {
        std::string dest = (install_root.path() / relDest / src.filename()).string();
        phony_targets.push_back(dest);
        out << "build " << dest << ": install_file " << src.string() << "\n";
    };

    for (auto &[rel_dest, files] : install_files)
        for (const auto &src : files) emitInstallFile(rel_dest, src.path());

    // target installs → their build-dir artifact
    for (const Target *t : install_targets)
    {
        std::string_view subdir = t->type() == TargetType::Executable ? "bin" : "lib";
        emitInstallFile(subdir, ninja_target_name(*t));
    }

    for (auto &[rel_dest, dirs] : install_dirs)
    {
        for (size_t i = 0; i < dirs.size(); ++i)
        {
            // directory source → tree copy
            auto &src = dirs[i];
            std::string target = "install_" + rel_dest + "_" + std::to_string(i);
            phony_targets.push_back(target);
            out << "build " << target << ": install_tree\n";
            out << "  dir = " << src.path().string() << "\n";
            out << "  dest = " << (install_root.path() / rel_dest).string() << "\n";
        }
    }

    // Emit the `install` phony target pointing at everything above.
    out << "build install: phony";
    for (auto &t : phony_targets) out << " " << t;
    out << "\n\n";

    // TODO: ninja test <args> -> calls executable with args
    out << "\n# --- Test rules ---\n\n";
    out << "rule run_test\n";
    out << "  command = $cmd > $name.stdout 2> $name.stderr && ";
    out << "echo \"SUCCESS: $name\" || (echo \"FAILURE: $name\" && exit 1)\n";
    out << "  description = TEST $name\n\n";

    // maps from executable name to tests
    std::unordered_map<std::string, std::vector<const detail::Test *>> testMap;

    for (const auto &test : project.tester().tests())
    {
        auto testName = test_name(test);
        std::string execName{test.exec->name()};
        out << "build " << testName << ".test.stamp: run_test | " << execName << "\n";
        out << "  name = " << testName << "\n";
        out << "  cmd = ./" << ninja_target_name(*test.exec) << ' ' << test.conflatedArgs;
        out << "\n\n";

        testMap[execName].push_back(&test);
    }

    for (const auto &[execName, tests] : testMap)
    {
        out << "build test-" << execName << ": phony";
        for (const auto *test : tests) out << " " << test_name(*test) << ".test.stamp";
        out << "\n\n";
    }

    out << "build test: phony";
    for (const auto &[execName, _] : testMap) out << " test-" << execName;
    out << "\n\n";

    out.close();
    std::ofstream ccFile(config.compile_commands_path.path());
    genCc.write(ccFile);
    std::cout << "Done. Run `ninja` to build.\n";
}
} // namespace zimm
