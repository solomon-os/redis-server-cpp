FROM gcc:16

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build git curl ca-certificates zip unzip tar pkg-config \
    && rm -rf /var/lib/apt/lists/*

# vcpkg — same dependency manager the CodeCrafters runner uses (for asio)
RUN git clone https://github.com/microsoft/vcpkg /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg \
    CMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    CMAKE_GENERATOR=Ninja
# ^ Make Ninja the default generator. C++20 modules are NOT supported by the
# Make generator, so a bare `cmake -B build-docker` must use Ninja.

WORKDIR /app

# Shorthand: `run` inside the container = build + run the server
RUN printf '#!/bin/sh\nexec cmake --build /app/build-docker --target run\n' > /usr/local/bin/run \
    && chmod +x /usr/local/bin/run
