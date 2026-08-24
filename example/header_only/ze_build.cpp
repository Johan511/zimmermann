#include <zimm/zimm.hpp>

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"Header Only Example", std::move(cfg.value())};
    prj.add_global_property(CompileFlagProperty{"-std=c++23"});

    auto greeter = make_header_only_library("greeter");
    greeter->add_public_property(IncludeProperty{rel_dir("include")});

    auto app = make_executable("header_only_demo");
    app->add_source(rel_file("main.cpp"));
    app->link_with(public_, greeter);

    prj.register_top_level_target(app);

    prj.installer().install_binary(app);
    prj.installer().install("include", rel_dir("include"));

    generate_build(prj);
}
