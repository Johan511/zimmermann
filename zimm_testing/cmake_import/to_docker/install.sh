#!/usr/bin/env bash
set -euo pipefail

dnf -y --setopt=install_weak_deps=False install \
    gcc gcc-c++ cmake ninja-build git python3 \
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
    qt6-qtbase-devel \
    opencv-devel \
    poco-devel \
    tbb-devel \
    llvm-devel \
    clang-devel \
    CGAL-devel \
    hpx-devel \
    opencascade-devel \
    InsightToolkit-devel \
    ceres-solver-devel \
    bullet-devel

dnf -y clean all
