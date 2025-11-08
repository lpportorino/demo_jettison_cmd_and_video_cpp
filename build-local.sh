#!/bin/bash
# DEPRECATED: This script is deprecated in favor of Docker-based builds
# Use ./scripts/build.sh instead for proper dependency management

echo "============================================"
echo "WARNING: This script is deprecated!"
echo "============================================"
echo ""
echo "This local build script may fail due to protobuf version"
echo "incompatibilities and other dependency issues."
echo ""
echo "Please use the Docker-based build instead:"
echo "  ./scripts/build.sh"
echo ""
echo "Press Ctrl+C to cancel, or Enter to continue anyway..."
read

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Check if submodule is initialized
if [ ! -f "jettison_proto_cpp/README.md" ]; then
    echo "Initializing git submodule..."
    git submodule update --init --recursive
fi

# Build heat_ctl (HEAT camera)
echo "Building heat_ctl..."
mkdir -p build-heat
cd build-heat
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCAMERA_TYPE=HEAT \
      -DENFORCE_CHECKS=OFF \
      ..
make -j$(nproc)
cd ..

echo ""
echo "✓ heat_ctl built: build-heat/heat_ctl"
echo ""

# Build day_ctl (DAY camera)
echo "Building day_ctl..."
mkdir -p build-day
cd build-day
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCAMERA_TYPE=DAY \
      -DENFORCE_CHECKS=OFF \
      ..
make -j$(nproc)
cd ..

echo ""
echo "✓ day_ctl built: build-day/day_ctl"
echo ""
echo "Usage:"
echo "  ./build-heat/heat_ctl sych.local"
echo "  ./build-day/day_ctl sych.local"
echo ""
