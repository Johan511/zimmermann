#pragma once

#include <filesystem>
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
    std::string build_dir{std::filesystem::current_path().string()};
    std::string install_dir{(std::filesystem::current_path() / "install").string()};
    std::string compile_commands_path{std::filesystem::path{build_dir} / "compile_commands.json"};

    std::string flags_debug{""};          // flags for debug build
    std::string flags_release{""};        // flags for release build
    std::string flags_relwithdebinfo{""}; // flags for rel-with-debug-info build
    std::string flags_minsizerel{""};     // flags for min size release builds
    std::string build_type{"debug"};      // build type

    // prefix used to figure out tools like gcc, g++, ar, ld, ldd, ...
    std::string toolchain_prefix{""};

    std::string os{"Linux"};
    std::string hardware{"x86_64"};

    std::unordered_map<std::string, std::string> misc{};
};

std::optional<Config> make_config(int argc, char *argv[]);
} // namespace zimm
