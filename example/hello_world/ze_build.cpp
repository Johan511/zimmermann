#include <zimm/zimm.hpp>

using namespace zimm;

int main()
{
    auto cfg = Config{};
    cfg.install_dir = rel_path("install");
    Project prj{"Hello World", std::move(cfg)};
    prj.add_global_property(CompileFlagProperty{"-std=c++23"});

    auto helloWorld = make_executable("HelloWorld");
    helloWorld->add_property(public_, IncludeProperty{rel_path("include")});
    helloWorld->add_source(rel_path("hello_world.cpp"));

    prj.register_top_level_target(helloWorld.get());

    prj.installer()->install_binary(helloWorld.get());
    prj.installer()->install_headers(Directory{rel_path("include")});

    Tester *tester = prj.tester();
    tester->add_test(helloWorld.get());

    generate_build(prj);
}
