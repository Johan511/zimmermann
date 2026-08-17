#include <zimm/zimm.hpp>

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"Hello World", std::move(cfg.value())};
    prj.add_global_property(CompileFlagProperty{"-std=c++23"});

    auto helloWorld = make_executable("HelloWorld");
    helloWorld->add_property(public_, IncludeProperty{rel_dir("include")});
    helloWorld->add_source(rel_file("hello_world.cpp"));

    prj.register_top_level_target(helloWorld);

    prj.installer().install_binary(helloWorld);
    prj.installer().install("include", rel_dir("include"));

    Tester &tester = prj.tester();
    tester.add_test(helloWorld);

    generate_build(prj);
}
