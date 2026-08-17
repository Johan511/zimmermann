#include "gen_cc.hpp"
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

void GenCc::add_entry(Directory directory, File file, std::string command)
{
    m_entries.emplace_back(std::move(directory), std::move(file), std::move(command));
}

void GenCc::write(std::ofstream &out)
{
    if (!out) LOGF("Error: could not open compile_commands.json for writing");

    out << "[\n";
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        auto &e = m_entries[i];
        out << "  {\n";
        out << "    \"directory\": \"" << json_escape(e.directory.path().string()) << "\",\n";
        out << "    \"file\": \"" << json_escape(e.file.path().string()) << "\",\n";
        out << "    \"command\": \"" << json_escape(e.command) << "\"\n";
        out << "  }";
        if (i + 1 < m_entries.size()) out << ",";
        out << "\n";
    }
    out << "]\n";
}
} // namespace zimm
