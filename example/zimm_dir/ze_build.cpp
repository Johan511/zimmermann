#include <zimm/zimm.hpp>
#include <filesystem>
#include <iostream>

using namespace zimm;

int main()
{
    // Verify zimm_dir() returns the install path
    std::string dir = zimm_dir();
    namespace fs = std::filesystem;

    if (dir.empty())
    {
        std::cerr << "FAIL: zimm_dir() returned empty string" << std::endl;
        return 1;
    }
    if (!fs::exists(fs::path{dir} / "include" / "zimm" / "zimm_dir.hpp"))
    {
        std::cerr << "FAIL: zimm_dir() does not point to a zimmermann install: " << dir
                  << std::endl;
        return 1;
    }

    Project prj{"zimm_dir", Config{.install_dir = "install"};};

    auto exec = make_executable("zimm_dir");
    exec->add_source(rel_path("main.cpp"));

    prj.register_top_level_target(exec.get());
    prj.installer()->install_binary(exec.get());

    generate_build(prj);
}
