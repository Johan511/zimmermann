#pragma once

#include "zimm/zimm.hpp"

using namespace zimm;

inline auto define_libnetwork(StaticLibrary *core)
{
    auto lib = make_shared_library("network");
    lib->add_source(rel_file("socket.cpp"));
    lib->add_public_dependency(core);
    return lib;
}
