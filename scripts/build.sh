#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Docker-based build script for Jettison Cmd TX (heat_ctl and day_ctl)

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "${SCRIPT_DIR}")"

# Parse arguments
CLEAN=false

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --clean         Remove existing AppImages before building"
    echo "  -h, --help      Show this help message"
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            print_usage
            exit 1
            ;;
    esac
done

# Print header
echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}Jettison Cmd TX - Docker Build${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# Check for required tools
echo -e "${BLUE}[1/5] Checking required tools...${NC}"
for tool in docker git; do
    if ! command -v $tool &> /dev/null; then
        echo -e "${RED}ERROR: $tool is not installed${NC}"
        exit 1
    fi
    echo -e "  ${GREEN}✓${NC} $tool found"
done
echo ""

# Check git submodules
echo -e "${BLUE}[2/5] Checking git submodules...${NC}"
cd "${PROJECT_DIR}"
if [ ! -f "jettison_proto_cpp/jon_shared_data.pb.h" ]; then
    echo -e "${YELLOW}  Submodule not initialized, initializing now...${NC}"
    git submodule update --init --recursive
fi
echo -e "  ${GREEN}✓${NC} Submodules ready"
echo ""

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${BLUE}[3/5] Cleaning old AppImages...${NC}"
    rm -f "${PROJECT_DIR}"/*.AppImage
    echo -e "  ${GREEN}✓${NC} Old AppImages removed"
    echo ""
else
    echo -e "${BLUE}[3/5] Skipping clean (use --clean to remove old AppImages)${NC}"
    echo ""
fi

# Extract version
echo -e "${BLUE}[4/5] Extracting version...${NC}"
if [ ! -f "${PROJECT_DIR}/VERSION" ]; then
    echo -e "${RED}ERROR: VERSION file not found${NC}"
    exit 1
fi
VERSION=$(cat "${PROJECT_DIR}/VERSION" | tr -d '[:space:]')
echo -e "  ${GREEN}✓${NC} Version: ${VERSION}"
echo ""

# Build AppImages with Docker
echo -e "${BLUE}[5/5] Building AppImages with Docker...${NC}"
cd "${PROJECT_DIR}"

echo -e "  ${BLUE}→${NC} Building heat_ctl and day_ctl AppImages..."
echo -e "  ${YELLOW}  This will take several minutes (compiling dependencies)...${NC}"

# Check if buildx is available
if docker buildx version &> /dev/null; then
    # Use buildx with local output - extracts from scratch export stage
    if ! docker buildx build \
        --target export \
        --output type=local,dest=. \
        . 2>&1 | tee build.log; then
        echo -e "${RED}ERROR: Docker build failed${NC}"
        echo -e "${YELLOW}See build.log for details${NC}"
        exit 1
    fi
else
    # Fallback to regular docker build + container extraction
    echo -e "  ${YELLOW}  Using docker build (buildx not available)${NC}"

    if ! docker build --target runtime -t jettison-cmd-tx-appimage:latest . 2>&1 | tee build.log; then
        echo -e "${RED}ERROR: Docker build failed${NC}"
        echo -e "${YELLOW}See build.log for details${NC}"
        exit 1
    fi

    # Extract AppImages from container
    echo -e "  ${BLUE}→${NC} Extracting AppImages from container..."
    CONTAINER_ID=$(docker create jettison-cmd-tx-appimage:latest)

    if ! docker cp "${CONTAINER_ID}:/Jettison_Heat_Camera_Control-x86_64.AppImage" . ; then
        echo -e "${RED}ERROR: Failed to extract heat AppImage${NC}"
        docker rm "${CONTAINER_ID}"
        exit 1
    fi

    if ! docker cp "${CONTAINER_ID}:/Jettison_Day_Camera_Control-x86_64.AppImage" . ; then
        echo -e "${RED}ERROR: Failed to extract day AppImage${NC}"
        docker rm "${CONTAINER_ID}"
        exit 1
    fi

    docker rm "${CONTAINER_ID}" > /dev/null
    echo -e "  ${GREEN}✓${NC} AppImages extracted successfully"
fi

# Rename extracted AppImages
echo ""
echo -e "  ${BLUE}→${NC} Renaming AppImages..."
if [ -f "Jettison_Heat_Camera_Control-x86_64.AppImage" ]; then
    mv Jettison_Heat_Camera_Control-x86_64.AppImage "heat-ctl-${VERSION}-linux-x86_64.AppImage"
    chmod +x "heat-ctl-${VERSION}-linux-x86_64.AppImage"
    echo -e "  ${GREEN}✓${NC} heat-ctl-${VERSION}-linux-x86_64.AppImage"
else
    echo -e "${RED}ERROR: Jettison_Heat_Camera_Control-x86_64.AppImage not found${NC}"
    exit 1
fi

if [ -f "Jettison_Day_Camera_Control-x86_64.AppImage" ]; then
    mv Jettison_Day_Camera_Control-x86_64.AppImage "day-ctl-${VERSION}-linux-x86_64.AppImage"
    chmod +x "day-ctl-${VERSION}-linux-x86_64.AppImage"
    echo -e "  ${GREEN}✓${NC} day-ctl-${VERSION}-linux-x86_64.AppImage"
else
    echo -e "${RED}ERROR: Jettison_Day_Camera_Control-x86_64.AppImage not found${NC}"
    exit 1
fi

echo ""

# Test AppImages
echo -e "  ${BLUE}→${NC} Testing AppImages..."
echo ""
echo -e "    ${BLUE}heat-ctl:${NC}"
./heat-ctl-*.AppImage --help 2>&1 | head -5 || echo -e "    ${YELLOW}(requires hostname argument)${NC}"
echo ""
echo -e "    ${BLUE}day-ctl:${NC}"
./day-ctl-*.AppImage --help 2>&1 | head -5 || echo -e "    ${YELLOW}(requires hostname argument)${NC}"
echo ""

# Success
echo -e "${GREEN}============================================${NC}"
echo -e "${GREEN}✓ Build completed successfully!${NC}"
echo -e "${GREEN}============================================${NC}"
echo ""
echo "AppImages created:"
ls -lh "${PROJECT_DIR}"/*-ctl-*.AppImage
echo ""
echo "To run:"
echo "  ./heat-ctl-${VERSION}-linux-x86_64.AppImage sych.local"
echo "  ./day-ctl-${VERSION}-linux-x86_64.AppImage sych.local"
echo ""
