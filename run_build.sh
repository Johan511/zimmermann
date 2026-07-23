#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# --- Clean (optional: pass --clean) ---
if [[ "${1:-}" == "--clean" ]]; then
    echo "==> Cleaning build and install directories..."
    rm -rf "$BUILD_DIR" "$INSTALL_DIR"
fi

# --- Configure ---
echo "==> Configuring..."
cmake -S "$PROJECT_ROOT" \
      -B "$BUILD_DIR" \
      -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
      -DCMAKE_BUILD_TYPE=Release

# --- Build ---
echo "==> Building..."
cmake --build "$BUILD_DIR" --parallel "$(nproc)"

# --- Install ---
echo "==> Installing to $INSTALL_DIR..."
cmake --install "$BUILD_DIR"

# --- Package ---
echo "==> Packaging..."
cd "$BUILD_DIR"
cpack -G TGZ
cpack -G DEB
cpack -G RPM
cd "$PROJECT_ROOT"

echo ""
echo "Done. Artifacts:"
echo "  Build:   $BUILD_DIR"
echo "  Install: $INSTALL_DIR"
echo "  Packs:"
echo "    " "$BUILD_DIR"/*.tar.gz
echo "    " "$BUILD_DIR"/*.deb
echo "    " "$BUILD_DIR"/*.rpm
