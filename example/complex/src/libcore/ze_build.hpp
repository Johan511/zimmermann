#pragma once

#include "zimm/zimm.hpp"

using namespace zimm;

inline auto define_libcore()
{
    auto lib = make_static_library("core");
    lib->add_property(private_, CompileFlagProperty{"-fPIC"});
    lib->add_source(rel_path("logging.cpp"));
    return lib;
}
