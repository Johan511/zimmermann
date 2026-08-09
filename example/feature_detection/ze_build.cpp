#include "../../includes/zimm/zimm.hpp"

using namespace zimm;

int main()
{
    auto cfg = Config{};
    Project prj{"Feature Detection", std::move(cfg)};

    auto app = make_executable("feature_detect");
    app->add_source(rel_path("main.cpp"));

    // --- header checks ---

    if (prj.check_header("cstdio"))
        app->add_property(private_, CompileFlagProperty{"-DHAVE_CSTDIO"});

    if (prj.check_header("nonexistent_header_xyz"))
        app->add_property(private_, CompileFlagProperty{"-DHAVE_NONEXISTENT_HEADER"});

    // --- function checks ---

    if (prj.check_function_exists("printf"))
        app->add_property(private_, CompileFlagProperty{"-DHAVE_PRINTF"});

    if (prj.check_function_exists("nonexistent_func_xyz"))
        app->add_property(private_, CompileFlagProperty{"-DHAVE_NONEXISTENT_FUNC"});

    // --- symbol checks ---

    if (prj.check_symbol_exists("O_RDONLY", {"fcntl.h"}))
        app->add_property(private_, CompileFlagProperty{"-DHAVE_O_RDONLY"});

    // --- type size checks ---

    if (auto sz = prj.check_type_size("int", {}))
        app->add_property(private_, CompileFlagProperty{"-DINT_SIZE=" + std::to_string(*sz)});

    if (auto sz = prj.check_type_size("long long", {}))
        app->add_property(private_, CompileFlagProperty{"-DLLONG_SIZE=" + std::to_string(*sz)});

    if (auto sz = prj.check_type_size("void*", {}))
        app->add_property(private_, CompileFlagProperty{"-DPTR_SIZE=" + std::to_string(*sz)});

    prj.register_top_level_target(app.get());
    prj.installer().install_binary(app.get());
    generate_build(prj);
}
