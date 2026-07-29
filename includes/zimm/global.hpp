#pragma once

#include <filesystem>

namespace zimm
{

std::filesystem::path build_dir();
std::filesystem::path install_dir();
std::filesystem::path ze_build_cpp_path();
std::filesystem::path project_root_dir();

} // namespace zimm
