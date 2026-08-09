#include <cstdlib>
#include <fstream>
#include <string>
#include <zimm/zimm.hpp>

using namespace zimm;
namespace fs = std::filesystem;

// environment setup to place a dummy library called my_lib at ./my_install/my_lib/[lib, include]
// necessery to test FindPackage
void setup_my_lib();

int main()
{
    auto cfg = make_config();
    Project prj{"ThirdPartyTarget Example", std::move(cfg.value())};
    prj.add_global_property(CompileFlagProperty{"-std=c++20"});

    setup_my_lib();

    auto myLibTpt =
        ThirdPartyTarget::make("my_lib", FindPackageTptStrategy{rel_path("my_install")});
    auto myLib = myLibTpt->assume_static_library("my_lib", "lib/my_lib.a");
    myLib->add_public_property(IncludeProperty{rel_path("my_install/my_lib/include")});

    Directory gtestDir{std::string{prj.build_dir()} + "/googletest"};
    auto gtestTpt = ThirdPartyTarget::make(
        "googletest",
        FetchContentTptStrategy{
            gtestDir,
            git_fetch(gtestDir, "https://github.com/google/googletest.git", "tag v1.17.0"),
            MetaBuildCmd{"cmake -S . -B build"}, BuildCmd{"cmake --build build"}});

    auto gtestLib = gtestTpt->assume_static_library("gtest", "build/lib/libgtest.a");
    gtestTpt->add_public_property(
        IncludeProperty{std::string{gtestDir.path()} + "googletest/include"});
    gtestTpt->add_public_property(
        IncludeProperty{std::string{gtestDir.path()} + "googlemock/include"});

    Directory httplibDir{std::string{prj.build_dir()} + "/cpp-httplib"};
    std::string httplibFetch =
        git_fetch(httplibDir, "https://github.com/yhirose/cpp-httplib.git", "tag v0.52.0");
    auto httpLibTpt = ThirdPartyTarget::make(
        "httplib", FindPackageTptStrategy{},
        FetchContentTptStrategy{httplibDir, httplibFetch, MetaBuildCmd{""}, BuildCmd{""}});
    // auto httpLib = httplibTpt->assume_static_library("httplib", "httplib");
    // TODO: add header only library, or atleast assume_header_only_target
    httpLibTpt->add_public_property(IncludeProperty{std::string{httplibDir.path()}});

    auto app = make_executable("tpt_demo");
    app->add_source(rel_path("main.cpp"));
    app->link_with(private_, myLib.get());
    app->link_with(private_, gtestLib.get());
    add_dependency_rel(app.get(), httpLibTpt.get());

    prj.register_top_level_target(app.get());
    prj.installer().install_binary(app.get());

    generate_build(prj);
}

void setup_my_lib()
{
    fs::path libPath = rel_path("my_install/my_lib/lib/");
    fs::path incPath = rel_path("my_install/my_lib/include/");
    fs::create_directories(libPath.c_str());
    fs::create_directories(incPath.c_str());

    fs::create_directories("tmp");

    std::ofstream srcOfs{"tmp/my_lib.cpp"};
    // clang-format off
    static constexpr auto SRC =
        "#include \"my_lib.hpp\"\n"
        "void my_lib_func() { }";
    // clang-format on
    srcOfs << SRC << std::endl;

    std::ofstream hdrOfs{"tmp/my_lib.hpp"};
    static constexpr auto HDR = "void my_lib_func(void);";
    hdrOfs << HDR << std::endl;

    std::system("g++ tmp/my_lib.cpp -c -o tmp/my_lib.o");
    std::system(std::format("ar rcs {} tmp/my_lib.o", (libPath / "my_lib.a").c_str()).c_str());
    std::system(std::format("cp tmp/my_lib.hpp {}", (incPath / "my_lib.hpp").c_str()).c_str());

    fs::remove_all("tmp");
}
