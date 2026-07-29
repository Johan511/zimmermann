#pragma once

#include <string>
#include <vector>

namespace zimm
{

struct CCEntry
{
    std::string directory;
    std::string file;
    std::string command;
};

class GenCc
{
    std::vector<CCEntry> m_entries;

public:
    GenCc();
    void add_entry(std::string directory, std::string file, std::string command);
    void write();
};

} // namespace zimm
