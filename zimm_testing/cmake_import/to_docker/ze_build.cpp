#include <zimm/target.hpp>
#include <zimm/third_party_target.hpp>

#include <filesystem>
#include <format>
#include <iostream>
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

struct Package
{
    std::string_view cmakeName;
    std::string_view cmakeArgs;
    std::vector<Expected> expected;
};

using TargetType::SharedLibrary, TargetType::HeaderOnlyLibrary;

// clang-format off
const std::vector<Package> packages = {
    {"fmt",         "",                           {{"fmt", SharedLibrary}}},
    {"spdlog",      "",                           {{"spdlog", SharedLibrary}}},
    {"GTest",       "",                           {{"gtest", SharedLibrary}, {"gtest_main", SharedLibrary}}},
    {"Catch2",      "",                           {{"Catch2", HeaderOnlyLibrary}}},
    {"benchmark",   "",                           {{"benchmark", SharedLibrary}, {"benchmark_main", SharedLibrary}}},
    {"Eigen3",      "",                           {{"Eigen", HeaderOnlyLibrary}}},
    {"glm",         "",                           {{"glm", HeaderOnlyLibrary}}},
    {"Boost",       "COMPONENTS program_options", {{"program_options", SharedLibrary}, {"headers", HeaderOnlyLibrary}}},
    {"FlatBuffers", "",                           {{"flatbuffers_shared", SharedLibrary}}},
    {"absl",        "",                           {{"strings", SharedLibrary}, {"hash", SharedLibrary}}},
    {"gRPC",       "",                           {{"grpc++", SharedLibrary}, {"grpc", SharedLibrary}}},
    {"SFML",       "COMPONENTS window",          {{"sfml-window", SharedLibrary}, {"sfml-system", SharedLibrary}}},
    {"glfw3",      "",                           {{"glfw", SharedLibrary}}},
    {"raylib",     "",                           {{"raylib", SharedLibrary}}},
    {"box2d",      "",                           {{"box2d", SharedLibrary}}},
    {"Qt6",        "COMPONENTS Core",            {{"Core", SharedLibrary}}},
    {"OpenCV",     "",                           {{"opencv_core", SharedLibrary}}},
    {"Poco",       "COMPONENTS Foundation Net",  {{"Foundation", SharedLibrary}, {"Net", SharedLibrary}}},
    {"TBB",        "",                           {{"tbb", SharedLibrary}, {"tbbmalloc", SharedLibrary}}},
    {"LLVM",       "",                           {{"LLVM", SharedLibrary}}},
    {"Clang",      "",                           {{"clang-cpp", SharedLibrary}}},
    {"CGAL",       "",                           {{"CGAL", HeaderOnlyLibrary}}},
    {"HPX",        "",                           {{"hpx", SharedLibrary}}},
    {"OpenCASCADE","",                           {{"TKernel", SharedLibrary}, {"TKMath", SharedLibrary}}},
    {"ITK",        "",                           {{"ITKCommon", SharedLibrary}}},
    {"Ceres",      "",                           {{"ceres", SharedLibrary}}},

    // Variable only configs -> have no targets -> we synthesize the targets
    {"Bullet",     "",                           {{"Bullet_libs", HeaderOnlyLibrary}}},
};
// clang-format on

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
                                          "from_docker/<pkg>/configure.log for the wrapper log of "
                                          "'{}' (zimm copies it out of the container)",
                                          cmakeName)};
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

    for (const Package &pkg : packages)
    {
        std::cout << "== find_package(" << pkg.cmakeName << " CONFIG REQUIRED";
        if (!pkg.cmakeArgs.empty()) std::cout << " " << pkg.cmakeArgs;
        std::cout << ")\n";

        try
        {
            FindCmakePackageTptStrategy strategy{std::vector<Directory>{Directory::make("/usr")},
                                                 std::string{pkg.cmakeArgs}};
            auto manifest = strategy.attempt(pkg.cmakeName);

            check_import(manifest, pkg.cmakeName);
            check_targets(manifest, pkg);
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
