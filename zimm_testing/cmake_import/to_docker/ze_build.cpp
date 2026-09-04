#include <zimm/target.hpp>
#include <zimm/third_party_target.hpp>

#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace zimm;

namespace fs = std::filesystem;

namespace
{

struct Expected
{
    std::string_view name;
    TargetType type;
};

// A dependency whose manifest an earlier row found and a later row's config references
// (e.g. absl links GTest::gtest): fabricated into the later row's wrapper under `ns`.
struct DepSpec
{
    std::string_view pkg; // key of the already-found manifest
    std::string_view ns;  // cmake namespace its targets are exposed under
};

struct Package
{
    std::string_view cmakeName;
    std::string_view cmakeArgs;
    std::vector<Expected> expected;
    std::vector<DepSpec> deps;
    std::string_view hints;
};

using TargetType::Executable, TargetType::StaticLibrary, TargetType::SharedLibrary,
    TargetType::HeaderOnlyLibrary;

const std::vector<Package> packages = {};

// hwloc ships no CMake config; TBB's and Ceres's configs reference pkg-config's
// PkgConfig::HWLOC imported target. Fabricated from the dnf-installed files
// (lib64/libhwloc.so under /usr, includes in /usr/include — verified in container).
ThirdPartyTargetManifest fabricate_hwloc()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("hwloc", Directory::make("/usr"));
    zimm::SharedLibrary *hwloc =
        tpt->assume_shared_library("HWLOC", detail::RelativePath{"lib64/libhwloc.so"});
    tpt->add_public_property(IncludeProperty{Directory::make("/usr/include")});
    return {tpt, {hwloc}};
}

// CGALConfig imports CGAL::CGAL_Qt6 only when Qt6 is found in the same CMake session
// (Qt6 is not installed); all its CGAL_Qt6 creation sites are guarded, so a placeholder
// with an empty interface satisfies the CGAL::CGAL_BasicViewer_Qt link reference.
ThirdPartyTargetManifest fabricate_cgal_qt6_placeholder()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("cgal_qt6_placeholder", Directory::make("/usr"));
    zimm::HeaderOnlyLibrary *qt6 = make_header_only_library("CGAL_Qt6");
    add_dependency_rel(qt6, tpt);
    return {tpt, {qt6}};
}

// vtk-config's targets reference ~33 third-party targets it never find_package()s
// (consumer-preload convention), so find_package(VTK) cannot configure — and ITK's
// ITKVtkGlue module find_package(VTK)s internally with the same result. Hand-fabricate
// the union of the VTK targets OpenCASCADE's visualization/draw targets (TKIVtk,
// TKIVtkDraw) and ITK's imported ITKVtkGlue target reference.
ThirdPartyTargetManifest fabricate_vtk()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("vtk", Directory::make("/usr"));
    std::vector<Target *> targets;
    for (std::string_view name : {"CommonCore", "FiltersGeneral", "IOImage", "ImagingCore",
                                  "ImagingSources", "InteractionStyle", "RenderingCore",
                                  "RenderingFreeType", "RenderingGL2PSOpenGL2", "RenderingOpenGL2"})
        targets.push_back(tpt->assume_shared_library(
            std::string{name}, detail::RelativePath{std::format("lib64/libvtk{}.so", name)}));
    tpt->add_public_property(IncludeProperty{Directory::make("/usr/include/vtk")});
    return {tpt, std::move(targets)};
}

struct Error
{
    std::string stage;
    std::string reason;
};

const Target *resolve(std::span<const Target *> materialized, const Expected &exp)
{
    for (const Target *t : materialized)
        if (t->name() == exp.name && t->type() == exp.type) return t;
    return nullptr;
}

void check_import(const ThirdPartyTargetManifest &manifest, std::string_view cmakeName)
{
    if (!manifest.tpt())
        throw Error{.stage = "import",
                    .reason = std::format("empty manifest (configure or parse failed) — see "
                                          "from_docker/.zimm_cmake_find/{}/configure.log for "
                                          "the wrapper log of '{}' (zimm copies it out of the "
                                          "container)",
                                          cmakeName, cmakeName)};
}

void check_targets(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto assumedTargets = manifest.targets();
    std::vector<std::string> missing;

    for (const Expected &exp : pkg.expected)
        if (!resolve(assumedTargets, exp))
            missing.push_back(std::format("{} ({})", exp.name, to_string(exp.type)));

    if (!missing.empty())
        throw Error{.stage = "targets",
                    .reason =
                        "missing: " + (missing | std::views::join_with(std::string_view{", "}) |
                                       std::ranges::to<std::string>())};
}

// Inverse of check_targets: every materialized target must be expected. Catches the
// strategy inventing targets the config does not expose (or name/type drift).
void check_no_extras(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto assumedTargets = manifest.targets();
    std::vector<std::string> extras;

    for (const Target *t : assumedTargets)
    {
        bool known = false;
        for (const Expected &exp : pkg.expected)
            if (t->name() == exp.name && t->type() == exp.type)
            {
                known = true;
                break;
            }
        if (!known) extras.push_back(std::format("{} ({})", t->name(), to_string(t->type())));
    }

    if (!extras.empty())
        throw Error{.stage = "extras",
                    .reason = "materialized but not expected: " +
                              (extras | std::views::join_with(std::string_view{", "}) |
                               std::ranges::to<std::string>())};
}

void check_paths(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto assumedTargets = manifest.targets();

    std::vector<std::string> badPaths;
    for (const Expected &exp : pkg.expected)
    {
        if (exp.type == HeaderOnlyLibrary) continue;

        const Target *t = resolve(assumedTargets, exp);
        const auto &assumed = t->assumed_path();

        if (!assumed || !fs::exists(assumed->path()))
            badPaths.push_back(
                std::format("{}: {}", t->name(),
                            assumed ? assumed->path().string() : std::string{"no assumed path"}));
    }

    if (!badPaths.empty())
        throw Error{.stage = "paths",
                    .reason = "missing artifacts: " +
                              (badPaths | std::views::join_with(std::string_view{", "}) |
                               std::ranges::to<std::string>())};
}

int test_packages()
{
    std::size_t passCount = 0, failCount = 0;
    std::vector<std::string> summary;

    // Manifests rows can inject: hwloc (TBB/Ceres rows), the CGAL_Qt6 placeholder (CGAL
    // row), the VTK targets OpenCASCADE's visualization targets reference (OpenCASCADE
    // row), plus every row's own manifest once found (GTest row 3 feeds absl 10/gRPC 11).
    std::map<std::string, ThirdPartyTargetManifest, std::less<>> found;
    found.emplace("hwloc", fabricate_hwloc());
    found.emplace("placeholder", fabricate_cgal_qt6_placeholder());
    found.emplace("VTK", fabricate_vtk());

    for (const Package &pkg : packages)
    {
        std::cout << "== find_package(" << pkg.cmakeName << " CONFIG REQUIRED";
        if (!pkg.cmakeArgs.empty()) std::cout << " " << pkg.cmakeArgs;
        std::cout << ")\n";

        try
        {
            std::vector<CmakeDependency> deps;
            for (const DepSpec &spec : pkg.deps)
            {
                auto it = found.find(spec.pkg);
                if (it == found.end())
                {
                    std::cout << std::format(
                        "WARN  no manifest '{}' to inject — '{}' runs uninjected\n", spec.pkg,
                        pkg.cmakeName);
                    continue;
                }
                deps.push_back({std::string{spec.ns}, it->second});
            }

            FindCmakePackageTptStrategy strategy{std::vector<Directory>{Directory::make("/usr")},
                                                 std::string{pkg.cmakeArgs}, "relwithdebinfo",
                                                 std::string{pkg.hints}};
            auto manifest = strategy.attempt(pkg.cmakeName, deps);
            found[std::string{pkg.cmakeName}] = manifest; // rows feed later rows

            check_import(manifest, pkg.cmakeName);
            check_targets(manifest, pkg);
            check_no_extras(manifest, pkg);
            check_paths(manifest, pkg);

            ++passCount;
            std::string expectedNames = pkg.expected | std::views::transform(&Expected::name) |
                                        std::views::join_with(std::string_view{", "}) |
                                        std::ranges::to<std::string>();
            summary.push_back(
                std::format("PASS  {:<12} {:<8} {}", pkg.cmakeName, "-", expectedNames));
        }
        catch (const Error &e)
        {
            ++failCount;
            summary.push_back(
                std::format("FAIL  {:<12} {:<8} {}", pkg.cmakeName, e.stage, e.reason));
        }
    }

    std::cout << "\n==== summary ====" << std::endl;
    for (const std::string &line : summary) std::cout << line << std::endl;
    std::cout << std::format("\n{} pass, {} fail", passCount, failCount) << std::endl;
    return failCount == 0 ? 0 : 1;
}

} // namespace

int main() { return test_packages(); }
