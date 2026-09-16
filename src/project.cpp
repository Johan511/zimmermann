#include "../includes/zimm/project.hpp"
#include "../includes/zimm/properties.hpp"
#include "../includes/zimm/target.hpp"
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <set>
#include <stack>
#include <string>

namespace fs = std::filesystem;

namespace zimm
{
Project::Project(std::string name, Config config, std::source_location mainFile)
    : m_name(std::move(name)), m_config(std::move(config)),
      m_buildDir(Directory::make(m_config.build_dir)),
      m_installDir(Directory::make(m_config.install_dir)), m_featureDetectionDir(m_buildDir),
      m_compileCommandsPath(File::make(m_config.compile_commands_path)),
      m_mainFilePath(File::make(std::string{mainFile.file_name()}))
{
    fs::create_directories(m_buildDir.path());
    fs::create_directories(m_featureDetectionDir.path());

    // clang-format off
    if (m_config.build_type == "debug")
        add_global_property(CompileFlagProperty{m_config.flags_debug});
    else if (m_config.build_type == "release")
        add_global_property(CompileFlagProperty{m_config.flags_release});
    else if (m_config.build_type == "relwithdebinfo")
        add_global_property(CompileFlagProperty{m_config.flags_relwithdebinfo});
    else if (m_config.build_type == "minsizerel")
        add_global_property(CompileFlagProperty{m_config.flags_minsizerel});
    else
        LOGF("Invalid build type: " << std::quoted(m_config.build_type));
    // clang-format on
}

std::unordered_set<Target *> Project::seach_all_targets() const
{
    std::unordered_set<Target *> seen;
    {
        // check that top level targets unique
        for (Target *t : m_topLevelTargets)
            if (!seen.emplace(t).second)
                LOGF("Duplicate top level target");
        seen.clear();
    }

    std::stack<Target *> stk;
    stk.push_range(m_topLevelTargets);

    while (!stk.empty())
    {
        Target *top = stk.top();
        stk.pop();

        if (seen.contains(top))
            continue;
        seen.insert(top);

        for (auto dep :
             std::views::concat(top->public_dependencies(), top->private_dependencies()))
            if (!seen.contains(dep))
                stk.push(dep);
    }

    return seen;
}

//  Feature detection
bool Project::try_compile(std::string_view source, bool link)
{
    std::set<std::string> includes;
    std::set<std::string> compileFlags;
    std::set<std::string> linkFlags;

    for (const auto &prop : m_globalProperties)
    {
        switch (prop_type(prop))
        {
        case PropertyType::Include:
            includes.insert(std::get<IncludeProperty>(prop).include_path().path().string());
            break;
        case PropertyType::CompileFlag:
            compileFlags.insert(std::string{std::get<CompileFlagProperty>(prop).flag()});
            break;
        case PropertyType::LinkFlag:
            linkFlags.insert(std::string{std::get<LinkFlagProperty>(prop).flag()});
            break;
        default:
            break;
        }
    }

    std::string cxxflags = m_config.cxx_flags;
    for (auto &inc : includes)
        cxxflags += " -I" + inc;
    for (auto &f : compileFlags)
        cxxflags += " " + f;

    // --- write source file ---

    constexpr auto name = "zimm_check";
    auto srcFile = m_featureDetectionDir.file(std::format("{}.cpp", name));
    {
        std::ofstream srcOfs(srcFile.path());
        if (!srcOfs)
            return false;
        srcOfs << source;
    }

    // --- compile ---

    auto objFile = m_featureDetectionDir.file(std::format("{}.o", name));
    auto errFile = m_featureDetectionDir.file(std::format("{}.err", name));

    const auto objStr = objFile.path().string();
    const auto errStr = errFile.path().string();

    std::string compiler = m_config.toolchain_prefix + "g++";

    auto compileCmd = std::format("{} {} -c {} -o {} 2>{}", compiler, cxxflags,
                                  srcFile.path().string(), objStr, errStr);

    if (std::system(compileCmd.c_str()) != 0)
        return false;

    if (link)
    {
        std::string ldflags;
        for (auto &f : linkFlags)
            ldflags += " " + f;

        auto binFile = m_featureDetectionDir.file(name);
        const auto binStr = binFile.path().string();
        auto linkCmd =
            std::format("{} {} {} -o {} 2>>{}", compiler, objStr, ldflags, binStr, errStr);

        if (std::system(linkCmd.c_str()) != 0)
            return false;
    }

    return true;
}

std::optional<std::string> Project::try_run(std::string_view source)
{
    if (!try_compile(source, /*link=*/true))
        return std::nullopt;

    constexpr auto name = "zimm_check";

    auto binFile = m_featureDetectionDir.file(name);
    auto outFile = m_featureDetectionDir.file(std::format("{}.out", name));
    auto errFile = m_featureDetectionDir.file(std::format("{}.err", name));

    const auto binStr = binFile.path().string();
    const auto errStr = errFile.path().string();

    auto runCmd = std::format("{} > {} 2>>{}", binStr, outFile.path().string(), errStr);

    if (std::system(runCmd.c_str()) != 0)
        return std::nullopt;

    std::ifstream outOfs(outFile.path());
    if (!outOfs)
        return std::nullopt;

    std::string result(std::istreambuf_iterator<char>{outOfs}, std::istreambuf_iterator<char>{});
    return result;
}

bool Project::check_header(std::string_view header)
{
    auto src = std::format("#include <{0}>\n"
                           "int main() {{ return 0; }}\n",
                           header);
    return try_compile(std::move(src), /*link=*/false);
}

bool Project::check_function_exists(std::string_view function_name)
{
    auto src = std::format("#ifdef __cplusplus\n"
                           "extern \"C\" {{\n"
                           "#endif\n"
                           "char {0}();\n"
                           "#ifdef __cplusplus\n"
                           "}}\n"
                           "#endif\n"
                           "int main() {{ (void){0}(); return 0; }}\n",
                           function_name);
    return try_compile(std::move(src), /*link=*/true);
}

bool Project::check_symbol_exists(std::string_view symbol, const std::vector<std::string> &headers)
{
    std::string headerBlock;
    for (const auto &h : headers)
        headerBlock += std::format("#include <{}>\n", h);

    auto src = std::format("{}"
                           "int main() {{\n"
                           "    (void)({});\n"
                           "    return 0;\n"
                           "}}\n",
                           headerBlock, symbol);
    return try_compile(std::move(src), /*link=*/false);
}

std::optional<size_t> Project::check_type_size(std::string_view type,
                                               const std::vector<std::string> &headers)
{
    std::string headerBlock;
    for (const auto &h : headers)
        headerBlock += std::format("#include <{}>\n", h);

    auto src = std::format("{}"
                           "#include <cstdio>\n"
                           "int main() {{\n"
                           "    std::printf(\"%zu\\n\", sizeof({}));\n"
                           "    return 0;\n"
                           "}}\n",
                           headerBlock, type);

    auto output = try_run(std::move(src));
    if (!output)
        return std::nullopt;

    return std::stoull(*output);
}

} // namespace zimm
