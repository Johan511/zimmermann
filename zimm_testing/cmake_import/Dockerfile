# syntax=docker/dockerfile:1
FROM fedora:44

COPY zimm_testing/cmake_import/to_docker/install.sh /tmp/install.sh
RUN --mount=type=cache,target=/var/cache/libdnf5 \
    bash /tmp/install.sh && rm /tmp/install.sh

COPY zimm_testing/cmake_import/to_docker/run_tests.py /opt/harness/run_tests.py
COPY zimm_testing/cmake_import/to_docker/ze_build.cpp /opt/harness/ze_build.cpp
COPY zimm_testing/cmake_import/to_docker/targets.yml /opt/harness/targets.yml
WORKDIR /work

RUN --mount=type=bind,source=.,target=/zimm-src \
 cmake -S /zimm-src -B /tmp/zimm-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/opt/zimm \
 && cmake --build /tmp/zimm-build --parallel "$(nproc)" \
 && cmake --install /tmp/zimm-build \
 && rm -rf /tmp/zimm-build
