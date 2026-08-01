#include "../includes/zimm/project.hpp"
#include "../includes/zimm/properties.hpp"
#include "../includes/zimm/target.hpp"
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <set>
#include <stack>
#include <string>

namespace fs = std::filesystem;

namespace zimm
{
Project::Project(std::string name, Config config, SourceLoc mainFile)
    : m_name(std::move(name)), m_config(std::move(config)),
      m_featureDetectionDir(m_config.build_dir), m_installer(m_config.build_dir),
      m_mainFilePath(mainFile.file_name())
{
    fs::create_directories(m_config.build_dir);
    fs::create_directories(fs::path{m_featureDetectionDir.path()});
}

std::unordered_set<Target *> Project::seach_all_targets() const
{
    std::unordered_set<Target *> seen;
    {
        // check that top level targets unique
        for (Target *t : m_topLevelTargets)
            if (!seen.emplace(t).second) LOGF("Duplicate top level target");
        seen.clear();
    }

    std::stack<Target *> stk;
    stk.push_range(m_topLevelTargets);

    while (!stk.empty())
    {
        Target *top = stk.top();
        stk.pop();

        if (seen.contains(top)) continue;
        seen.insert(top);

        for (auto dep : top->dependencies())
            if (!seen.contains(dep)) stk.push(dep);
    }

    return seen;
}

//  Feature detection
bool Project::try_compile(std::string source, bool link)
{
    namespace fs = fs;
    auto work_dir = fs::path{m_featureDetectionDir.path()};

    std::set<std::string> includes;
    std::set<std::string> compile_flags;
    std::set<std::string> link_flags;

    for (auto &pobj : m_globalProperties)
    {
        const Property &prop = *pobj;
        switch (prop.type())
        {
        case PropertyType::Include:
            includes.insert(std::string{static_cast<const IncludeProperty &>(prop).include_path()});
            break;
        case PropertyType::CompileFlag:
            compile_flags.insert(
                std::string{static_cast<const CompileFlagProperty &>(prop).flag()});
            break;
        case PropertyType::LinkFlag:
            link_flags.insert(std::string{static_cast<const LinkFlagProperty &>(prop).flag()});
            break;
        default:
            break;
        }
    }

    std::string cxxflags = m_config.cxx_flags;
    for (auto &inc : includes) cxxflags += " -I" + inc;
    for (auto &f : compile_flags) cxxflags += " " + f;

    // --- write source file ---

    constexpr auto name = "zimm_check";
    auto src_path = work_dir / std::format("{}.cpp", name);
    {
        std::ofstream src_file(src_path);
        if (!src_file) return false;
        src_file << source;
    }

    // --- compile ---

    auto obj_path = work_dir / std::format("{}.o", name);
    auto err_path = work_dir / std::format("{}.err", name);

    std::string compiler = m_config.toolchain_prefix + "g++";

    auto compile_cmd = std::format("{} {} -c {} -o {} 2>{}", compiler, cxxflags, src_path.string(),
                                   obj_path.string(), err_path.string());

    if (std::system(compile_cmd.c_str()) != 0) return false;

    if (link)
    {
        std::string ldflags;
        for (auto &f : link_flags) ldflags += " " + f;

        auto bin_path = work_dir / name;
        auto link_cmd = std::format("{} {} {} -o {} 2>>{}", compiler, obj_path.string(), ldflags,
                                    bin_path.string(), err_path.string());

        if (std::system(link_cmd.c_str()) != 0) return false;
    }

    return true;
}

std::optional<std::string> Project::try_run(std::string source)
{
    namespace fs = fs;

    if (!try_compile(std::move(source), /*link=*/true)) return std::nullopt;

    auto work_dir = fs::path{m_featureDetectionDir.path()};
    constexpr auto name = "zimm_check";

    auto bin_path = work_dir / name;
    auto out_path = work_dir / std::format("{}.out", name);
    auto err_path = work_dir / std::format("{}.err", name);

    auto run_cmd =
        std::format("{} > {} 2>>{}", bin_path.string(), out_path.string(), err_path.string());

    if (std::system(run_cmd.c_str()) != 0) return std::nullopt;

    std::ifstream out_file(out_path);
    if (!out_file) return std::nullopt;

    std::string result(std::istreambuf_iterator<char>{out_file}, std::istreambuf_iterator<char>{});
    return result;
}

bool Project::check_header(std::string header)
{
    auto src = std::format("#include <{0}>\n"
                           "int main() {{ return 0; }}\n",
                           header);
    return try_compile(std::move(src), /*link=*/false);
}

bool Project::check_function_exists(std::string function_name)
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

bool Project::check_symbol_exists(std::string symbol, std::vector<std::string> headers)
{
    std::string header_block;
    for (const auto &h : headers) header_block += std::format("#include <{}>\n", h);

    auto src = std::format("{}"
                           "int main() {{\n"
                           "    (void)({});\n"
                           "    return 0;\n"
                           "}}\n",
                           header_block, symbol);
    return try_compile(std::move(src), /*link=*/false);
}

std::optional<size_t> Project::check_type_size(std::string type, std::vector<std::string> headers)
{
    std::string header_block;
    for (const auto &h : headers) header_block += std::format("#include <{}>\n", h);

    auto src = std::format("{}"
                           "#include <cstdio>\n"
                           "int main() {{\n"
                           "    std::printf(\"%zu\\n\", sizeof({}));\n"
                           "    return 0;\n"
                           "}}\n",
                           header_block, type);

    auto output = try_run(std::move(src));
    if (!output) return std::nullopt;

    return std::stoull(*output);
}

} // namespace zimm
