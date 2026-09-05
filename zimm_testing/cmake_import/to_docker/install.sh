#!/usr/bin/env bash
set -euo pipefail

dnf -y --setopt=install_weak_deps=False --setopt=keepcache=True install \
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
    vtk-devel \
    hwloc-devel \
    InsightToolkit-devel \
    gdcm-devel \
    libminc-devel \
    ceres-solver-devel \
    bullet-devel
