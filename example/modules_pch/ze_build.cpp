#include "zimm/zimm.hpp"

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"modules_pch", std::move(cfg.value())};

    auto app = make_executable("modules_pch");

    // C++20 module interface unit: declares `export module math`.
    app->add_module_unit(ModuleUnit{
        rel_file("math.cppm"),
        "math",        // module name
        {}             // imports: none
    });

    // A second module that imports `math` — exercises topological ordering
    // (extra is built only after math's BMI exists).
    app->add_module_unit(ModuleUnit{
        rel_file("extra.cppm"),
        "extra",
        {"math"}       // imports math
    });

    app->add_source(rel_file("main.cpp"));
    app->set_precompiled_header(rel_file("common.hpp"));

    // Module units need -std=c++20 (or newer) and a module-aware compiler.
    prj.add_global_property(CompileFlagProperty{"-std=c++20"});

    prj.register_top_level_target(app);
    prj.installer().install_binary(app);

    generate_build(prj);
}
