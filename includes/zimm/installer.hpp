#pragma once

#include "target.hpp"
#include <initializer_list>
#include <span>
#include <unordered_map>

namespace zimm
{

class Installer
{
    std::unordered_map<std::string /* install subdir */, std::vector<File>> m_installFiles;
    std::unordered_map<std::string /* install subdir */, std::vector<Directory>> m_installDirs;
    std::vector<Target *> m_installTargets;

public:
    Installer() {}

    // file copied into installSubDir
    void install(std::string_view installSubDir, File from)
    {
        // todo: add std::string_view comparator to map
        m_installFiles[std::string{installSubDir}].push_back(std::move(from));
    }

    // directory contents are copied into installSubDir
    void install(std::string_view installSubDir, Directory from)
    {
        // todo: add std::string_view comparator to map
        m_installDirs[std::string{installSubDir}].push_back(std::move(from));
    }

    void install_binary(Executable *exe) { m_installTargets.push_back(exe); }
    void install_lib(SharedLibrary *lib) { m_installTargets.push_back(lib); }
    void install_lib(StaticLibrary *lib) { m_installTargets.push_back(lib); }

    void install_binary(std::initializer_list<Executable *> exes)
    {
        for (auto *e : exes) install_binary(e);
    }

    void install_lib(std::initializer_list<SharedLibrary *> libs)
    {
        for (auto *lib : libs) install_lib(lib);
    }

    void install_lib(std::initializer_list<StaticLibrary *> libs)
    {
        for (auto *lib : libs) install_lib(lib);
    }

    const auto &files() const noexcept { return m_installFiles; }
    const auto &dirs() const noexcept { return m_installDirs; }
    std::span<Target *const> targets() const noexcept { return m_installTargets; }
};

}; // namespace zimm
