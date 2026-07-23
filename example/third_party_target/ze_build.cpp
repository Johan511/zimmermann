#include "../../includes/zimm/zimm.hpp"

using namespace zimm;

struct AdderGen
{
    static constexpr std::string_view name() { return "adder_gen"; }

    static std::string cmd(std::span<const std::string> inputs,
                           std::span<const std::string> /* outputs */)
    {
        // inputs[0] is the absolute path to adder.py.
        // Run it with argument 1 to generate add_1.
        return "python3 " + std::string(inputs[0]) + " 1";
    }
};

int main()
{
    auto cfg = Config{};
    cfg.install_dir = rel_path("install");
    Project prj{"Third-Party Target Example", std::move(cfg)};

    // --- Custom target: run adder.py to generate adder.h + adder.cpp ---
    auto gen = make_custom_target<AdderGen>("adder");
    gen->add_input(rel_path("adder.py"));
    gen->add_output(gen->dir().make("adder.h"));
    gen->add_output(gen->dir().make("adder.cpp"));
    gen->add_property(public_, IncludeProperty{std::string{gen->dir().path()}});

    // --- Executable ---
    auto app = make_executable("a");
    app->add_source(rel_path("a.cpp"));
    app->add_source(gen->dir().make("adder.cpp"));
    add_dependency_rel(app.get(), gen.get());

    prj.register_top_level_target(app.get());
    prj.installer()->install_binary(app.get());

    generate_build(prj);
}
