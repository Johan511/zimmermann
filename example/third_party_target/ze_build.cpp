#include <iostream>
#include <string>
#include <zimm/zimm.hpp>

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"ThirdPartyTarget Example", std::move(cfg.value())};
    prj.add_global_property(CompileFlagProperty{"-std=c++20"});

    // find_package(Boost CONFIG REQUIRED COMPONENTS program_options) runs under the hood;
    // the strategy materializes an assumed target per imported target the config defines.
    FindCmakePackageTptStrategy boostStrategy{"COMPONENTS program_options"};
    ThirdPartyTargetManifest boostManifest = boostStrategy.attempt("Boost");
    ThirdPartyTarget *boostTpt = boostManifest.tpt();

    // get the Library* to link by name from the populated dependents
    auto depByName = [](const ThirdPartyTarget *tpt, std::string_view name) -> Library *
    {
        for (Target *d : tpt->dependents())
            if (d->name() == name) return dynamic_cast<Library *>(d);
        return nullptr;
    };
    // Boost::program_options — its Boost::headers dep's include dirs ride along transitively
    auto po = depByName(boostTpt, "program_options");

    Directory gtestDir = prj.build_dir().subdir("googletest");
    ThirdPartyTarget *gtestTpt = ThirdPartyTarget::make(
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
    ThirdPartyTarget *httpLibTpt = ThirdPartyTarget::make(
        "httplib", FindPackageTptStrategy{},
        FetchContentTptStrategy{httplibDir, httplibFetch, MetaBuildCmd{""}, BuildCmd{""}});
    httpLibTpt->add_public_property(IncludeProperty{httplibDir});

    auto app = make_executable("tpt_demo");
    app->add_source(rel_file("main.cpp"));
    app->link_with(private_, po);
    app->link_with(private_, gtestLib);
    add_dependency_rel(app, httpLibTpt);

    prj.register_top_level_target(app);
    prj.installer().install_binary(app);

    generate_build(prj);
}
