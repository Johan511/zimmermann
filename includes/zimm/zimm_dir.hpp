#pragma once

#include "path.hpp"
#include <filesystem>
#include <source_location>

/*
    Must not be included anywhere in the zimm project
    only the final zimm.hpp should include this at the ends

    Why? If we include it anywhere in the zimm project and use zimm_dir()
    It evaluates to the path of the header when that .cpp file was compiled to .o

    We do not want that, we don't want it to be in any zimmermann built compilataion unit
    It must only be included in
*/

#ifdef ZIMM_BUILDING_LIB
#error "zimm_dir.hpp must not be included in any zimmermann library translation unit. \
It may only reach user code through the final zimm.hpp. Do not include it (or zimm.hpp) from src/."
#endif

namespace zimm
{

__attribute__((weak)) Directory zimm_dir()
{
    namespace fs = std::filesystem;
    auto loc = std::source_location::current();
    return Directory{fs::path{fs::absolute(loc.file_name())} // "install/zimm/include/<file>"
                         .parent_path()                      // "install/zimm/include/"
                         .parent_path()                      // "install/zimm/"
                         .parent_path()};                    // "install"
}

} // namespace zimm
