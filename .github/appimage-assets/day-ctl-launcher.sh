#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Launcher script for Day Camera Control AppImage

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set library path to use bundled libraries
export LD_LIBRARY_PATH="${SCRIPT_DIR}/../lib:${LD_LIBRARY_PATH}"

# Execute the binary with all arguments
exec "${SCRIPT_DIR}/day_ctl" "$@"
