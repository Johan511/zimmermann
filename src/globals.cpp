#include "../includes/zimm/logger.hpp"
#include <filesystem>

namespace fs = std::filesystem;
namespace zimm
{

/*
    TODO: deal with this ugly situation where we need
    - ze_build.cpp to be in project root
    - zimm::Project must be create in ze_build.cpp
*/

// global state - updated internally by zimm
fs::path buildDir_;
fs::path installDir_;
fs::path zeBuildCppPath_;
fs::path projectRootDir_;
std::string prjName_;

// clang-format off
// must be accessed only after zimm::Project is initialized
fs::path build_dir() { if(prjName_.empty()) LOGF("Calling build_dir without initializing project"); return buildDir_; }
fs::path install_dir() { if(prjName_.empty()) LOGF("Calling install_dir without initializing project"); return installDir_; }
fs::path ze_build_cpp_path() { if(prjName_.empty()) LOGF("Calling ze_build_path without initializing project"); return zeBuildCppPath_; }
fs::path project_root_dir() { if(prjName_.empty()) LOGF("Calling project_root_dir without initializing project"); return projectRootDir_; }
// clang-format on

} // namespace zimm
