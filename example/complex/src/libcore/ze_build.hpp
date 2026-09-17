#pragma once

#include "zimm/zimm.hpp"

using namespace zimm;

inline auto define_libcore()
{
    auto lib = make_static_library("core");
    lib->add_private_property(CompileFlagProperty{"-fPIC"});
    lib->add_source(rel_file("logging.cpp"));
    return lib;
}
