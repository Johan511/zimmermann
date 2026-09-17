#include "../../includes/zimm/zimm.hpp"

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"Feature Detection", std::move(cfg.value())};

    auto app = make_executable("feature_detect");
    app->add_source(rel_file("main.cpp"));

    // --- header checks ---

    if (prj.check_header("cstdio"))
        app->add_private_property(CompileFlagProperty{"-DHAVE_CSTDIO"});

    if (prj.check_header("nonexistent_header_xyz"))
        app->add_private_property(CompileFlagProperty{"-DHAVE_NONEXISTENT_HEADER"});

    // --- function checks ---

    if (prj.check_function_exists("printf"))
        app->add_private_property(CompileFlagProperty{"-DHAVE_PRINTF"});

    if (prj.check_function_exists("nonexistent_func_xyz"))
        app->add_private_property(CompileFlagProperty{"-DHAVE_NONEXISTENT_FUNC"});

    // --- symbol checks ---

    if (prj.check_symbol_exists("O_RDONLY", {"fcntl.h"}))
        app->add_private_property(CompileFlagProperty{"-DHAVE_O_RDONLY"});

    // --- type size checks ---

    if (auto sz = prj.check_type_size("int", {}))
        app->add_private_property(CompileFlagProperty{"-DINT_SIZE=" + std::to_string(*sz)});

    if (auto sz = prj.check_type_size("long long", {}))
        app->add_private_property(CompileFlagProperty{"-DLLONG_SIZE=" + std::to_string(*sz)});

    if (auto sz = prj.check_type_size("void*", {}))
        app->add_private_property(CompileFlagProperty{"-DPTR_SIZE=" + std::to_string(*sz)});

    prj.register_top_level_target(app);
    prj.installer().install_binary(app);
    generate_build(prj);
}
