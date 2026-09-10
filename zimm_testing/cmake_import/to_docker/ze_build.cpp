#include <zimm/zimm.hpp>

#include <filesystem>
#include <format>
#include <generator>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace zimm;
using namespace std::string_view_literals;

namespace fs = std::filesystem;

namespace
{

struct TargetInfo
{
    TargetType type;
    std::string_view name;

    bool operator==(const TargetInfo &) const = default;
};

struct Package
{
    std::string_view cmakeName;
    std::string_view cmakeArgs;
    std::vector<TargetInfo> expectedTargets;
    std::vector<std::string_view> dependencies;
    std::vector<std::string> searchDirs;
};

const std::vector<Package> packages = {};

ThirdPartyTargetManifest make_hwloc_manifest()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("hwloc", Directory::make("/usr"));
    zimm::SharedLibrary *hwloc =
        tpt->assume_shared_library("PkgConfig::HWLOC", "lib64/libhwloc.so");
    tpt->add_public_property(IncludeProperty{Directory::make("/usr/include")});
    return {tpt, {hwloc}};
}

ThirdPartyTargetManifest fabricate_cgal_qt6_placeholder()
{
    ThirdPartyTarget *tpt = ThirdPartyTarget::make("cgal_qt6_placeholder", Directory::make("/usr"));
    zimm::HeaderOnlyLibrary *qt6 = make_header_only_library("CGAL::CGAL_Qt6");
    add_dependency_rel(qt6, tpt);
    return {tpt, {qt6}};
}

TargetInfo convert(const Target *t) { return TargetInfo{t->type(), t->name()}; }
std::string info_to_str(const TargetInfo &info)
{
    return std::format("{}:{}", to_string(info.type), info.name);
}

void check_found_matches_expectations(const ThirdPartyTargetManifest &manifest, const Package &pkg)
{
    auto found =
        manifest.targets() | std::views::transform(&convert) | std::ranges::to<std::vector>();
    const auto &expected = pkg.expectedTargets;

    std::vector<TargetInfo> missing;
    for (const auto &e : expected)
        if (std::ranges::find(found, e) == found.end()) missing.push_back(e);

    std::vector<TargetInfo> extra;
    for (const auto &f : found)
        if (std::ranges::find(expected, f) == expected.end()) extra.push_back(f);

    if (!missing.empty() || !extra.empty())
    {
        throw std::format(
            "Missing Targets: [{:s}]; Extra Targets: [{:s}]",
            missing | std::views::transform(info_to_str) | std::views::join_with(", "sv),
            extra | std::views::transform(info_to_str) | std::views::join_with(", "sv));
    }
}

std::generator<std::string> missing_paths(const Target *t)
{
    if (t->assumed_path())
    {
        auto &assumedPath = t->assumed_path()->path();
        if (!fs::exists(assumedPath)) co_yield assumedPath.string();
    }

    for (const auto &prop : std::views::concat(t->public_properties(), t->private_properties()))
    {
        if (prop->type() == PropertyType::Include)
        {
            auto includeProp = dynamic_cast<const IncludeProperty *>(prop.get());
            auto &includePath = includeProp->include_path().path();
            if (!fs::exists(includePath)) co_yield includePath.string();
        }
        else if (prop->type() == PropertyType::LinkTarget)
        {
            auto linkProp = dynamic_cast<const LinkTargetProperty *>(prop.get());
            auto linkTarget = linkProp->link_lib();
            if (!linkTarget->assumed_path())
                throw std::format("LinkTarget='{}' of AssumedTarget='{}' is not assumed",
                                  to_string(*linkTarget), to_string(*t));
            auto &linkPath = linkTarget->assumed_path()->path();
            if (!fs::exists(linkPath)) co_yield linkPath.string();
        }
    }
}

void check_paths(const ThirdPartyTargetManifest &manifest)
{
    std::vector<std::string> badPaths;

    badPaths.append_range(missing_paths(manifest.tpt()));
    for (const Target *t : manifest.targets()) badPaths.append_range(missing_paths(t));

    if (!badPaths.empty())
    {
        throw std::format("Paths referenced but not found: [{:s}]",
                          badPaths | std::views::join_with(", "sv));
    }
}

int test_packages()
{
    std::size_t passCount = 0, failCount = 0;
    std::vector<std::string> summary;

    // some packages depend on others
    // example: many packages depend on gtest, they can look up dependencies from here
    // hwloc is not a CMake package (pkgconfig only), so it is constructed manually
    std::unordered_map<std::string, ThirdPartyTargetManifest> manifests;
    manifests.emplace("hwloc", make_hwloc_manifest());
    manifests.emplace("cgal_qt6_placeholder", fabricate_cgal_qt6_placeholder());

    for (const Package &pkg : packages)
    {
        std::cout << "== find_package(" << pkg.cmakeName << " CONFIG REQUIRED";
        if (!pkg.cmakeArgs.empty()) std::cout << " " << pkg.cmakeArgs;
        std::cout << ")\n";

        try
        {
            auto cmakeDeps =
                pkg.dependencies |
                std::views::transform(
                    [&](std::string_view cmakeName)
                    {
                        auto iter = manifests.find(std::string{cmakeName});
                        if (iter == manifests.end())
                            throw std::format("cmake pkg='{}' requires missing dependency='{}'",
                                              pkg.cmakeName, cmakeName);
                        return FindCmakePackageTptStrategy::Dependency{iter->second};
                    }) |
                std::ranges::to<std::vector>();
            auto searchPaths = pkg.searchDirs | std::views::transform(&Directory::make) |
                               std::ranges::to<std::vector>();
            FindCmakePackageTptStrategy strategy{searchPaths, std::string{pkg.cmakeArgs},
                                                 "relwithdebinfo"};
            auto manifest = strategy.attempt(pkg.cmakeName, cmakeDeps);
            manifests[std::string{pkg.cmakeName}] = manifest;

            if (!manifest.tpt()) throw std::format("import error: empty manifest");

            check_found_matches_expectations(manifest, pkg);
            check_paths(manifest);

            ++passCount;
            summary.push_back(std::format("PASS  {:<12} -", pkg.cmakeName));
        }
        catch (const std::string &errStr)
        {
            ++failCount;
            summary.push_back(std::format("FAIL  {:<12} {}", pkg.cmakeName, errStr));
        }
    }

    std::cout << "\n==== summary ====" << std::endl;
    for (const std::string &line : summary) std::cout << line << std::endl;
    std::cout << std::format("\n{} pass, {} fail", passCount, failCount) << std::endl;
    return failCount == 0 ? 0 : 1;
}

} // namespace

int main() { return test_packages(); }
