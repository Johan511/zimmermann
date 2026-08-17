#pragma once

#include "zimm/zimm.hpp"

#include <initializer_list>

using namespace zimm;

inline auto define_myapp(StaticLibrary *core, StaticLibrary *math, SharedLibrary *network)
{
    auto app = make_executable("myapp");
    app->add_source(rel_file("main.cpp"));
    app->link_with(private_, {core, math, network});
    return app;
}
