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

    auto myLibTpt = ThirdPartyTarget::make("my_lib", FindPackageTptStrategy{rel_dir("my_install")});
    auto myLib = myLibTpt->assume_static_library("my_lib", "lib/my_lib.a");
    myLib->add_public_property(IncludeProperty{rel_dir("my_install/my_lib/include")});

    Directory gtestDir = prj.build_dir().subdir("googletest");
    auto gtestTpt = ThirdPartyTarget::make(
        "googletest",
        FetchContentTptStrategy{
            gtestDir,
            git_fetch(gtestDir, "https://github.com/google/googletest.git", "tag v1.17.0"),
            MetaBuildCmd{"cmake -S . -B build"}, BuildCmd{"cmake --build build"}});

    auto gtestLib = gtestTpt->assume_static_library("gtest", "build/lib/libgtest.a");
    gtestTpt->add_public_property(IncludeProperty{gtestDir.subdir("googletest/include")});
    gtestTpt->add_public_property(IncludeProperty{gtestDir.subdir("googlemock/include")});

    Directory httplibDir = prj.build_dir().subdir("cpp-httplib");
    std::string httplibFetch =
        git_fetch(httplibDir, "https://github.com/yhirose/cpp-httplib.git", "tag v0.52.0");
    auto httpLibTpt = ThirdPartyTarget::make(
        "httplib", FindPackageTptStrategy{},
        FetchContentTptStrategy{httplibDir, httplibFetch, MetaBuildCmd{""}, BuildCmd{""}});
    // auto httpLib = httplibTpt->assume_static_library("httplib", "httplib");
    // TODO: add header only library, or atleast assume_header_only_target
    httpLibTpt->add_public_property(IncludeProperty{httplibDir});

    auto app = make_executable("tpt_demo");
    app->add_source(rel_file("main.cpp"));
    app->link_with(private_, myLib);
    app->link_with(private_, gtestLib);
    add_dependency_rel(app, httpLibTpt);

    prj.register_top_level_target(app);
    prj.installer().install_binary(app);

    generate_build(prj);
}

void setup_my_lib()
{
    Directory libDir = rel_dir("my_install/my_lib/lib");
    Directory incDir = rel_dir("my_install/my_lib/include");
    Directory tmpDir = Directory::make("tmp");
    fs::create_directories(libDir.path());
    fs::create_directories(incDir.path());
    fs::create_directories(tmpDir.path());

    std::ofstream srcOfs{tmpDir.file("my_lib.cpp").path()};
    // clang-format off
    static constexpr auto SRC =
        "#include \"my_lib.hpp\"\n"
        "void my_lib_func() { }";
    // clang-format on
    srcOfs << SRC << std::endl;

    std::ofstream hdrOfs{tmpDir.file("my_lib.hpp").path()};
    static constexpr auto HDR = "void my_lib_func(void);";
    hdrOfs << HDR << std::endl;

    auto myLibO = tmpDir.file("my_lib.o");
    auto myLibCpp = tmpDir.file("my_lib.cpp");
    auto myLibHpp = tmpDir.file("my_lib.hpp");
    std::system(
        std::format("g++ {} -c -o {}", myLibCpp.path().string(), myLibO.path().string()).c_str());
    std::system(
        std::format("ar rcs {} {}", libDir.file("my_lib.a").path().string(), myLibO.path().string())
            .c_str());
    std::system(
        std::format("cp {} {}", myLibHpp.path().string(), incDir.file("my_lib.hpp").path().string())
            .c_str());

    fs::remove_all(tmpDir.path());
}
