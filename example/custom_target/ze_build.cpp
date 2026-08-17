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
    auto cfg = make_config();
    Project prj{"Custom Target Example", std::move(cfg.value())};

    // --- Custom target: run adder.py to generate adder.h + adder.cpp ---
    auto gen = make_custom_target<AdderGen>("adder", prj.build_dir());
    gen->add_input(rel_file("adder.py").path().string());
    gen->add_outputs(
        {gen->dir().file("adder.h").path().string(), gen->dir().file("adder.cpp").path().string()});
    gen->add_property(public_, IncludeProperty{gen->dir()});

    // --- Executable ---
    auto app = make_executable("a");
    app->add_sources({rel_file("a.cpp"), gen->dir().file("adder.cpp")});
    add_dependency_rel(app, gen);

    prj.register_top_level_target(app);
    prj.installer().install_binary(app);

    generate_build(prj);
}
