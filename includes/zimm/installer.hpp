#pragma once

#include "target.hpp"
#include <unordered_map>

namespace zimm
{

class Installer
{
    Directory m_buildDir;
    std::unordered_map<std::string, std::vector<std::string>> m_installMap;

public:
    Installer(std::string buildDir) : m_buildDir(std::move(buildDir)) {}

    void install_binary(Executable *exe)
    {
        m_installMap["bin"].push_back(m_buildDir.make(std::string{exe->name()}));
    }
    void install_lib(SharedLibrary *lib)
    {
        m_installMap["lib"].push_back(m_buildDir.make("lib" + std::string{lib->name()} + ".so"));
    }
    void install_lib(StaticLibrary *lib)
    {
        m_installMap["lib"].push_back(m_buildDir.make(std::string{lib->name()} + ".a"));
    }
    void install_headers(Directory headersDir, std::string includeSubDir = "")
    {
        m_installMap["include/" + includeSubDir].push_back(std::string{headersDir.path()});
    }

    const auto &map() const noexcept { return m_installMap; }
};

}; // namespace zimm