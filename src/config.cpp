#include "../includes/zimm/config.hpp"
#include "zimm/logger.hpp"
#include <format>
#include <iomanip>
#include <meta>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace meta = std::meta;
namespace fs = std::filesystem;

namespace zimm
{
namespace
{
struct ArgsParser
{
    int argc;
    char **argv;

    ArgsParser(int argc, char *argv[]) : argc(argc), argv(argv) {}

    struct iterator
    {
        using value_type = std::pair<std::string_view, std::string_view>;

        const ArgsParser &m_args;
        int m_pos{0};
        value_type m_current;

        void next()
        {
            if (++m_pos >= m_args.argc) return;

            std::string_view arg(m_args.argv[m_pos]);
            if (auto eq = arg.find('='); eq != std::string_view::npos)
                m_current = {arg.substr(0, eq), arg.substr(eq + 1)};
            // TODO: fix `throw;` with something more meaningful like `throw
            // std::invalid_argument(...)`
            else throw;
        }

    public:
        iterator(const ArgsParser &args) : m_args(args) { next(); }
        const value_type &operator*() const { return m_current; }
        iterator &operator++()
        {
            next();
            return *this;
        }
        bool operator==(std::default_sentinel_t) const { return m_pos >= m_args.argc; }
    };

    iterator begin() const { return iterator(*this); }
    std::default_sentinel_t end() const { return {}; }
};

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
} // namespace

consteval bool is_append_field(meta::info info)
{
    static constexpr auto appendFieldsList = {^^Config::c_flags, ^^Config::cxx_flags};
    for (const auto field : appendFieldsList)
        if (info == field) return true;
    return false;
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

    for (const auto &[key, value] : ArgsParser(argc, argv))
    {
        bool matched = false;

        template for (constexpr auto member :
                      std::define_static_array(meta::nonstatic_data_members_of(
                          ^^decltype(cfg), meta::access_context::current())))
        {
            constexpr std::string_view memberName = meta::identifier_of(member);
            if (memberName != key) continue;

            if constexpr (is_append_field(member)) cfg.[:member:] += value;
            else if constexpr (member != ^^Config::misc)
            {
                if constexpr (std::is_same_v<std::remove_cvref_t<decltype(cfg.[:member:])>,
                                             Directory>)
                    cfg.[:member:] = Directory{std::string{value}};
                else if constexpr (std::is_same_v<std::remove_cvref_t<decltype(cfg.[:member:])>,
                                                  File>)
                    cfg.[:member:] = File{std::string{value}};
                else cfg.[:member:] = value;
            }
            else LOGF("Invalid key: misc is being ignored");

            matched = true;
        }

        if (!matched) cfg.misc[std::string{key}] = value;
    }

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
