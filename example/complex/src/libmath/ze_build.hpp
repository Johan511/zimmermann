#pragma once

#include "zimm/zimm.hpp"

using namespace zimm;

inline auto define_libmath(StaticLibrary *core)
{
    auto lib = make_static_library("libmath");
    lib->add_source(rel_file("vec3.cpp"));
    lib->link_with_private(core);
    return lib;
}
