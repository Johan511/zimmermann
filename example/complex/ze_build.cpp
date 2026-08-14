#include "src/libcore/ze_build.hpp"
#include "src/app/ze_build.hpp"
#include "src/libmath/ze_build.hpp"
#include "src/libnetwork/ze_build.hpp"

#include <print>

using namespace zimm;

int main()
{
    auto cfg = make_config();
    Project prj{"Complex Example", std::move(cfg.value())};

    auto libcore = define_libcore();
    auto libmath = define_libmath(libcore.get());
    auto libnetwork = define_libnetwork(libcore.get());
    auto myapp = define_myapp(libcore.get(), libmath.get(), libnetwork.get());

    prj.add_global_property(IncludeProperty{rel_path("include")});
    prj.add_global_property(CompileFlagProperty{"-std=c++23"});

    prj.register_top_level_targets({myapp.get(), libnetwork.get()});

    prj.installer().install_binary(myapp.get());
    prj.installer().install_lib(libnetwork.get());
    prj.installer().install_headers(Directory{rel_path("include")});

    generate_build(prj);
}
