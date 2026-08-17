#pragma once

#include "../includes/zimm/path.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace zimm
{

struct CCEntry
{
    Directory directory;
    File file;
    std::string command;
};

class GenCc
{
    std::vector<CCEntry> m_entries;

public:
    void add_entry(Directory directory, File file, std::string command);
    void write(std::ofstream &ofs);
};

} // namespace zimm
