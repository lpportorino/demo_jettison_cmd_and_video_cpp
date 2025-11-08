#!/bin/bash
# DEPRECATED: This script is deprecated
# Use ./scripts/build.sh instead

echo "============================================"
echo "DEPRECATED SCRIPT"
echo "============================================"
echo ""
echo "This script has been replaced by ./scripts/build.sh"
echo ""
echo "The new script provides:"
echo "  - Better Docker buildx support"
echo "  - Proper version management"
echo "  - Cleaner extraction process"
echo "  - Better error handling"
echo ""
echo "Please use instead:"
echo "  ./scripts/build.sh"
echo ""
echo "Redirecting to new script in 3 seconds..."
sleep 3

exec ./scripts/build.sh "$@"
