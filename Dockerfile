# SPDX-License-Identifier: GPL-3.0-or-later
# Ubuntu 22.04 dynamic build + AppImage packaging
#
# Strategy: Build heat_ctl and day_ctl AppImages with named pipe video (no GStreamer)
# Dependencies: Abseil → Protobuf 29.2 → RE2 → protovalidate-cc (vendored) → app
# Note: IXWebSocket is fetched automatically via CMake FetchContent

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# ==============================================================================
# Install build dependencies
# ==============================================================================
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    cmake \
    git \
    ca-certificates \
    wget \
    curl \
    pkg-config \
    python3 \
    file \
    patchelf \
    desktop-file-utils \
    imagemagick \
    # Java for ANTLR4 (required by CEL-C++ which protovalidate-cc will build)
    default-jdk \
    # Dynamic libraries
    libssl-dev \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

# ==============================================================================
# Upgrade CMake to 3.24+ (protovalidate-cc requires 3.24+, Ubuntu 22.04 has 3.22)
# ==============================================================================
RUN cd /tmp && \
    wget https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.tar.gz && \
    tar -xzf cmake-3.28.3-linux-x86_64.tar.gz && \
    cp -r cmake-3.28.3-linux-x86_64/bin/* /usr/local/bin/ && \
    cp -r cmake-3.28.3-linux-x86_64/share/* /usr/local/share/ && \
    rm -rf /tmp/cmake-3.28.3*

# ==============================================================================
# Build Abseil (required by Protobuf 29.2)
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch 20240722.0 https://github.com/abseil/abseil-cpp.git && \
    cd abseil-cpp && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -DABSL_BUILD_TESTING=OFF \
          -DABSL_PROPAGATE_CXX_STD=ON \
          -DBUILD_SHARED_LIBS=ON && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    cd / && rm -rf /tmp/abseil-cpp

# ==============================================================================
# Build Protobuf 29.2 dynamically (matches our pre-compiled protos)
# ==============================================================================
RUN cd /tmp && \
    wget https://github.com/protocolbuffers/protobuf/releases/download/v29.2/protobuf-29.2.tar.gz && \
    tar -xzf protobuf-29.2.tar.gz && \
    cd protobuf-29.2 && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -Dprotobuf_BUILD_TESTS=OFF \
          -Dprotobuf_BUILD_SHARED_LIBS=ON \
          -Dprotobuf_ABSL_PROVIDER=package && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    cd / && rm -rf /tmp/protobuf-29.2*

# ==============================================================================
# Build RE2 (required by protovalidate-cc and CEL-C++)
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch 2024-07-02 https://github.com/google/re2.git && \
    cd re2 && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -DBUILD_SHARED_LIBS=ON \
          -DRE2_BUILD_TESTING=OFF && \
    cmake --build build -j$(nproc) && \
    cmake --install build && \
    ldconfig && \
    cd / && rm -rf /tmp/re2

# ==============================================================================
# Build protovalidate-cc with VENDORING (will vendor CEL-C++ internally)
# ==============================================================================
RUN cd /tmp && \
    git clone --depth 1 --branch v1.0.0-rc.2 https://github.com/bufbuild/protovalidate-cc.git && \
    cd protovalidate-cc && \
    cmake -B build -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCMAKE_CXX_STANDARD=17 \
          -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
          -DCMAKE_CXX_FLAGS="-fPIC" \
          -DBUILD_SHARED_LIBS=ON \
          -DPROTOVALIDATE_CC_ENABLE_TESTS=OFF \
          -DPROTOVALIDATE_CC_ENABLE_CONFORMANCE=OFF \
          -DPROTOVALIDATE_CC_ENABLE_INSTALL=OFF \
          -DPROTOVALIDATE_CC_ENABLE_VENDORING=ON && \
    cmake --build build -j$(nproc) && \
    cp -v build/libprotovalidate_cc.so /usr/local/lib/ && \
    find build/_deps -name "*.so" -o -name "*.so.*" | xargs -I {} cp -v {} /usr/local/lib/ && \
    mkdir -p /usr/local/include && \
    cp -rv buf /usr/local/include/ && \
    cp -rv build/gen/proto_cc_protovalidate/buf/validate/*.pb.h /usr/local/include/buf/validate/ && \
    cp -rv build/_deps/cel_cpp-src/common /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/eval /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/base /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/internal /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/runtime /usr/local/include/ && \
    cp -rv build/_deps/cel_cpp-src/extensions /usr/local/include/ && \
    find build/_deps/cel_cpp-build -type d -name "cel" -exec cp -rv {} /usr/local/include/ \; && \
    ldconfig && \
    cd / && rm -rf /tmp/protovalidate-cc

# ==============================================================================
# Setup workspace and build heat_ctl
# ==============================================================================
WORKDIR /build-heat

COPY CMakeLists.txt.dynamic CMakeLists.txt
COPY src ./src
COPY include ./include
COPY jettison_proto_cpp ./jettison_proto_cpp

RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCAMERA_TYPE=HEAT \
          .. && \
    make -j$(nproc) VERBOSE=1 && \
    make install

RUN ldd /usr/local/bin/heat_ctl && \
    /usr/local/bin/heat_ctl --help || echo "Note: heat_ctl requires hostname argument"

# ==============================================================================
# Setup workspace and build day_ctl
# ==============================================================================
WORKDIR /build-day

COPY CMakeLists.txt.dynamic CMakeLists.txt
COPY src ./src
COPY include ./include
COPY jettison_proto_cpp ./jettison_proto_cpp

RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=MinSizeRel \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          -DCAMERA_TYPE=DAY \
          .. && \
    make -j$(nproc) VERBOSE=1 && \
    make install

RUN ldd /usr/local/bin/day_ctl && \
    /usr/local/bin/day_ctl --help || echo "Note: day_ctl requires hostname argument"

# ==============================================================================
# Create AppImages
# ==============================================================================

# Install linuxdeploy
RUN cd /tmp && \
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage && \
    chmod +x linuxdeploy-x86_64.AppImage && \
    ./linuxdeploy-x86_64.AppImage --appimage-extract && \
    mv squashfs-root /opt/linuxdeploy && \
    ln -s /opt/linuxdeploy/AppRun /usr/local/bin/linuxdeploy && \
    cd /

# Copy AppImage assets
WORKDIR /appimage
COPY .github/appimage-assets/ ./assets/

# Create heat_ctl AppImage
RUN mkdir -p AppDir-heat/usr/bin && \
    cp /usr/local/bin/heat_ctl AppDir-heat/usr/bin/ && \
    linuxdeploy --appdir AppDir-heat \
        --desktop-file=assets/heat-ctl.desktop \
        --icon-file=assets/heat-ctl.svg \
        --output appimage && \
    ls -lh *.AppImage

# Create day_ctl AppImage
RUN mkdir -p AppDir-day/usr/bin && \
    cp /usr/local/bin/day_ctl AppDir-day/usr/bin/ && \
    linuxdeploy --appdir AppDir-day \
        --desktop-file=assets/day-ctl.desktop \
        --icon-file=assets/day-ctl.svg \
        --output appimage && \
    ls -lh *.AppImage

# ==============================================================================
# Export stage - both AppImages
# ==============================================================================
FROM scratch AS export
COPY --from=0 /appimage/*.AppImage /

# ==============================================================================
# Alternative: Runtime stage for regular docker (non-buildx)
# ==============================================================================
FROM ubuntu:22.04 AS runtime
COPY --from=0 /appimage/*.AppImage /
CMD ["ls", "-lh", "/"]
