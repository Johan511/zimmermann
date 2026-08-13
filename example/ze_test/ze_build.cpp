#include <zimm/zimm.hpp>

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"ZeTest", std::move(cfg.value())};
    prj.add_global_property(CompileFlagProperty{"-std=c++23"});

    auto demo = make_executable("ze_test");
    demo->add_source(rel_path("main.cpp"));
    prj.register_top_level_target(demo.get());
    prj.installer().install_binary(demo.get());

    auto testPass = make_executable("test_pass");
    testPass->add_source(rel_path("test_pass.cpp"));
    prj.register_top_level_target(testPass.get());

    auto testArgs = make_executable("test_args");
    testArgs->add_source(rel_path("test_args.cpp"));
    prj.register_top_level_target(testArgs.get());

    Tester &tester = prj.tester();
    tester.add_test(testPass.get());
    tester.add_test(testArgs.get(), "hello", 42);

    generate_build(prj);
}
