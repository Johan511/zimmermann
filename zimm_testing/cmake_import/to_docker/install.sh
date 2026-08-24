#!/usr/bin/env bash
set -euo pipefail

DNF_OPTS="-y --setopt=install_weak_deps=False --setopt=keepcache=True"

dnf $DNF_OPTS install \
    gcc gcc-c++ cmake ninja-build git python3 python3-pyyaml \
    fmt-devel \
    spdlog-devel \
    gtest-devel gmock-devel \
    catch2-devel \
    google-benchmark-devel \
    eigen3-devel \
    glm-devel \
    boost-devel \
    flatbuffers-devel flatbuffers-compiler \
    abseil-cpp-devel \
    grpc-devel \
    SFML-devel \
    glfw-devel \
    raylib-devel \
    Box2D-devel \
    qt6-qtbase-private-devel \
    opencv-devel \
    poco-devel \
    tbb-devel \
    llvm-devel \
    clang-devel \
    CGAL-devel \
    hpx-devel \
    opencascade-devel \
    hwloc-devel \
    InsightToolkit-devel \
    vxl-devel \
    gdcm-devel \
    libminc-devel \
    ceres-solver-devel \
    bullet-devel


# vtk requirements
dnf $DNF_OPTS install \
    vtk-devel \
    vtk-qt python3-vtk vtk-java vtk-testing \
    qt6-qtbase-devel qt6-qtdeclarative-devel \
    openslide-devel cli11-devel PEGTL-devel fast_float-devel \
    cgnslib-devel netcdf-devel libogg-devel libtheora-devel \
    libharu-devel proj-devel pugixml-devel jsoncpp-devel json-devel \
    lz4-devel xz-devel double-conversion-devel openxr-devel openvr-devel \
    mesa-libGL-devel libglvnd-devel libX11-devel libXcursor-devel \
    utf8cpp-devel hdf5-devel sqlite-devel python3-devel \
    freetype-devel libxml2-devel zlib-devel libpng-devel \
    libjpeg-turbo-devel libtiff-devel expat-devel
