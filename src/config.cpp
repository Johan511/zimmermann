#include "../includes/zimm/config.hpp"
#include <format>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace zimm
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

std::optional<Config> make_config(int argc, char *argv[])
try
{
    Config cfg;
    for (const auto &[key, value] : ArgsParser(argc, argv))
    {
        if (key == "c_flags") cfg.c_flags += value;
        else if (key == "cxx_flags") cfg.cxx_flags += value;
        else if (key == "c_cxx_flags")
        {
            cfg.c_flags += value;
            cfg.cxx_flags += value;
        }
        else if (key == "build_dir") cfg.build_dir = value;
        else if (key == "install_dir") cfg.install_dir = value;
        else if (key == "build_type")
        {
            if (value == "debug")
            {
                cfg.cxx_flags += cfg.flags_debug;
                cfg.c_flags += cfg.flags_debug;
            }
            else if (value == "release")
            {
                cfg.cxx_flags += cfg.flags_release;
                cfg.c_flags += cfg.flags_release;
            }
            else if (value == "relwithdebinfo")
            {
                cfg.cxx_flags += cfg.flags_relwithdebinfo;
                cfg.c_flags += cfg.flags_relwithdebinfo;
            }
            else if (value == "minsizerel")
            {
                cfg.cxx_flags += cfg.flags_minsizerel;
                cfg.c_flags += cfg.flags_minsizerel;
            }
            else
            {
                throw std::invalid_argument(std::format(
                    "invalid_argument: for key='{}', value='{}' not allowed", value, key));
            }
        }
        else if (key == "flags_debug") cfg.flags_debug = value;
        else if (key == "flags_release") cfg.flags_release = value;
        else if (key == "flags_relwithdebinfo") cfg.flags_relwithdebinfo = value;
        else if (key == "flags_minsizerel") cfg.flags_minsizerel = value;
        else if (key == "toolchain_prefix") cfg.toolchain_prefix = value;
        else if (key == "os") cfg.os = value;
        else if (key == "hardware") cfg.hardware = value;
    }
    return cfg;
}
catch (...)
{
    return std::nullopt;
}
} // namespace zimm
