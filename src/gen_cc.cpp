#include "gen_cc.hpp"
#include "../includes/zimm/global.hpp"
#include "../includes/zimm/logger.hpp"
#include <filesystem>
#include <format>
#include <fstream>
#include <string_view>

namespace zimm
{

static std::string json_escape(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                out += std::format("\\u{:04X}", static_cast<unsigned char>(c));
            else out += c;
            break;
        }
    }
    return out;
}

void GenCc::add_entry(std::string directory, std::string file, std::string command)
{ m_entries.emplace_back(std::move(directory), std::move(file), std::move(command)); }

void GenCc::write()
{
    std::ofstream out(build_dir() / "compile_commands.json");
    if (!out) LOGF("Error: could not open compile_commands.json for writing");

    out << "[\n";
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        auto &e = m_entries[i];
        out << "  {\n";
        out << "    \"directory\": \"" << json_escape(e.directory) << "\",\n";
        out << "    \"file\": \"" << json_escape(e.file) << "\",\n";
        out << "    \"command\": \"" << json_escape(e.command) << "\"\n";
        out << "  }";
        if (i + 1 < m_entries.size()) out << ",";
        out << "\n";
    }
    out << "]\n";
}

__attribute__((weak)) std::string zimm_install_path();
GenCc::GenCc()
{
    // add ze_build.cpp itself to the compile_commands too
    /*
        TODO: we need some way to figure out the compile command the user used to compile zimm
        For now we are simply guessing it to allow for clangd completions
    */
    std::string zeBuildCpp = ze_build_cpp_path();
    std::string zeBuildCppDir = std::filesystem::path{zeBuildCpp}.parent_path().string();

    std::string zimmIncludePath = (std::filesystem::path{zimm_install_path()} / "include").string();
    std::string compileCmdGuess = std::format("g++ -I{} ze_build.cpp -std=c++23", zimmIncludePath);

    add_entry(std::move(zeBuildCppDir), std::move(zeBuildCpp), std::move(compileCmdGuess));
}

} // namespace zimm
