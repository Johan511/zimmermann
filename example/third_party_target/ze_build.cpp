#include <string>
#include <zimm/zimm.hpp>

using namespace zimm;

// import cmake package as zimm::ThirdPartyTarget
SharedLibrary *define_program_options();

// fetch and build from source
StaticLibrary *define_gtest(Project &prj);

// fetch header only library from source
HeaderOnlyLibrary *define_httplib(Project &prj);

int main()
{
    auto cfg = make_config();
    Project prj{"ThirdPartyTarget Example", std::move(cfg.value())};
    prj.add_global_property(CompileFlagProperty{"-std=c++20"});

    SharedLibrary *po = define_program_options();
    StaticLibrary *gtest = define_gtest(prj);
    HeaderOnlyLibrary *httpLib = define_httplib(prj);

    auto app = make_executable("tpt_demo");
    app->add_source(rel_file("main.cpp"));

    app->link_with_public(po);
    app->link_with_public(gtest);
    app->add_public_dependency(httpLib);

    prj.register_top_level_target(app);
    prj.installer().install_binary(app);

    generate_build(prj);
}

SharedLibrary *define_program_options()
{
    FindCmakePackageTptStrategy boostStrategy{"COMPONENTS program_options"};
    ThirdPartyTargetManifest boostManifest = boostStrategy.attempt("Boost");
    auto po = boostManifest.shared_lib("Boost::program_options");
    return po;
}

StaticLibrary *define_gtest(Project &prj)
{
    Directory gtestDir = prj.build_dir().subdir("googletest");

    auto fetchCmd = git_fetch(gtestDir, "https://github.com/google/googletest.git", "tag v1.17.0");
    FetchContentTptStrategy strat = FetchContentTptStrategy{
        gtestDir, fetchCmd, MetaBuildCmd{"cmake -S . -B build"}, BuildCmd{"cmake --build build"}};

    ThirdPartyTarget *gtestTpt = ThirdPartyTarget::make("googletest", strat);

    auto gtestLib = gtestTpt->assume_static_library("gtest", "build/lib/libgtest.a");
    gtestLib->add_public_property(IncludeProperty{gtestDir.subdir("googletest/include")});
    gtestLib->add_public_property(IncludeProperty{gtestDir.subdir("googlemock/include")});

    return gtestLib;
}

HeaderOnlyLibrary *define_httplib(Project &prj)
{
    Directory httplibDir = prj.build_dir().subdir("cpp-httplib");

    std::string fetchCmd =
        git_fetch(httplibDir, "https://github.com/yhirose/cpp-httplib.git", "tag v0.52.0");

    // no meta build or build step
    auto strat = FetchContentTptStrategy{httplibDir, fetchCmd, MetaBuildCmd{}, BuildCmd{}};

    ThirdPartyTarget *httpLibTpt = ThirdPartyTarget::make("httplib", strat);
    HeaderOnlyLibrary *httpLib = httpLibTpt->assume_ho_library("httplib");
    httpLib->add_public_property(IncludeProperty{httplibDir});

    return httpLib;
}
