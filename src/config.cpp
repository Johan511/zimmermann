#include "../includes/zimm/config.hpp"
#include "args_parsers.hpp"
#include "zimm/logger.hpp"
#include <format>
#include <meta>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace meta = std::meta;
namespace fs = std::filesystem;

static constexpr size_t MAX_CONFIG_FILE_DEPTH = 10;

namespace zimm
{

std::string_view resolve_host_os()
{
#ifndef CMAKE_HOST_OS
#error CMAKE_HOST_OS not defined;
#else
    return CMAKE_HOST_OS;
#endif
}

std::string_view resolve_host_hardware()
{
#ifndef CMAKE_HOST_HARDWARE
#error CMAKE_HOST_HARDWARE not defined;
#else
    return CMAKE_HOST_HARDWARE;
#endif
}

consteval bool is_append_field(meta::info info)
{
    static constexpr auto appendFieldsList = {^^Config::c_flags, ^^Config::cxx_flags};
    for (const auto field : appendFieldsList)
        if (info == field) return true;
    return false;
}

void apply_arg(Config &cfg, std::string key, std::string value)
{
    bool matched = false;
    template for (constexpr auto member : std::define_static_array(
                      meta::nonstatic_data_members_of(^^Config, meta::access_context::current())))
    {
        constexpr auto memberName = meta::identifier_of(member);
        if (memberName != key) continue;

        if constexpr (is_append_field(member))
        {
            cfg.[:member:] += std::move(value);
            matched = true;
        }
        else if constexpr (member != ^^Config::misc)
        {
            cfg.[:member:] = typename[:meta::type_of(member):]{std::move(value)};
            matched = true;
        }
        else LOGE("Invalid key: misc is being ignored");
    }

    if (!matched) cfg.misc[std::string{key}] = value;
}

void apply_args(Config &cfg, std::ranges::range auto keysAndValues, int depth)
{
    static_assert(std::is_same_v<std::ranges::range_value_t<decltype(keysAndValues)>,
                                 std::pair<std::string, std::string>>);
    if (depth >= MAX_CONFIG_FILE_DEPTH)
        throw std::runtime_error(std::format("TODO: please write error"));

    for (auto &&[key, value] : std::move(keysAndValues))
    {
        if (key == "args_xml") apply_args(cfg, parse_xml_args(value), depth + 1);
        else if (key == "args_yaml" || key == "args_yml")
            apply_args(cfg, parse_yaml_args(value), depth + 1);
        else apply_arg(cfg, std::move(key), std::move(value));
    }
}

std::optional<Config> make_config(int argc, char *argv[])
try
{
    Config cfg;

    // Set defaults
    cfg.build_dir = Directory{fs::current_path()};
    cfg.install_dir = Directory{fs::current_path().parent_path() / "install"};
    cfg.flags_debug = "-g";
    cfg.flags_release = "-O3 -DNDEBUG";
    cfg.flags_relwithdebinfo = "-O3 -g -DNDEBUG";
    cfg.flags_minsizerel = "-Os -DNDEBUG";
    cfg.build_type = "relwithdebinfo";
    cfg.os = resolve_host_os();
    cfg.hardware = resolve_host_hardware();

    apply_args(cfg, parse_cli_args(argc, argv), 0);

    // make sure the ordering of these depenedent defaults is correct
    // dependent defaults (defaults which depend on other variables)
    if (cfg.compile_commands_path.path().empty())
        cfg.compile_commands_path = cfg.build_dir.file("compile_commands.json");

    return cfg;
}
catch (...)
{
    return std::nullopt;
}

std::optional<Config> make_config() { return make_config(0, nullptr); }

} // namespace zimm
