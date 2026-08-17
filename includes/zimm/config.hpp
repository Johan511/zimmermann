#pragma once

#include "path.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace zimm
{

struct Config
{
    std::string c_flags{};   // Inject C flags
    std::string cxx_flags{}; // Inject C++ flags

    // TODO: this seems janky, install/build should be wrt project root, figure out how to manage it
    Directory build_dir{""};
    Directory install_dir{""};
    File compile_commands_path{""};

    std::string flags_debug{};          // flags for debug build
    std::string flags_release{};        // flags for release build
    std::string flags_relwithdebinfo{}; // flags for rel-with-debug-info build
    std::string flags_minsizerel{};     // flags for min size release builds
    std::string build_type{};           // build type

    // prefix used to figure out tools like gcc, g++, ar, ld, ldd, ...
    std::string toolchain_prefix{};

    std::string os{};
    std::string hardware{};

    std::unordered_map<std::string, std::string> misc{};

private:
    friend class std::optional<Config> make_config(int argc, char *argv[]);
    Config() = default;
};

std::optional<Config> make_config(int argc, char *argv[]);
std::optional<Config> make_config();
} // namespace zimm
