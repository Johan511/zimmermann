#pragma once

#include "zimm/zimm.hpp"

#include <initializer_list>

using namespace zimm;

inline auto define_myapp(StaticLibrary *core, StaticLibrary *math, SharedLibrary *network)
{
    auto app = make_executable("myapp");
    app->add_source(rel_path("main.cpp"));
    app->link_with(private_, core);
    app->link_with(private_, math);
    app->link_with(private_, network);
    return app;
}
