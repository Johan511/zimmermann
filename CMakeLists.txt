cmake_minimum_required(VERSION 3.20)
project(zimmermann LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(zimmermann STATIC
    src/project.cpp
    src/properties.cpp
    src/target.cpp
    src/config.cpp
    src/gen_cc.cpp
    src/generate_build.cpp
    src/third_party_target.cpp
)
target_compile_definitions(zimmermann PRIVATE
    ZIMM_BUILDING_LIB
    CMAKE_HOST_OS="${CMAKE_HOST_SYSTEM_NAME}"
    CMAKE_HOST_HARDWARE="${CMAKE_HOST_SYSTEM_PROCESSOR}"
)
target_include_directories(zimmermann PUBLIC includes)

include(GNUInstallDirs)

install(TARGETS zimmermann
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY includes/zimm/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zimm
)

# --- Packaging (CPack) ---
set(CPACK_PACKAGE_NAME "zimmermann")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Zimmermann build system library")
set(CPACK_PACKAGE_VENDOR "Zimmermann")
set(CPACK_PACKAGE_CONTACT "harihara.sn@gmail.com")

set(CPACK_GENERATOR "TGZ;DEB;RPM")

# DEB-specific
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "HHN <harihara.sn@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.34), libstdc++6 (>= 11)")

# RPM-specific
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.34, libstdc++ >= 11")

include(CPack)
