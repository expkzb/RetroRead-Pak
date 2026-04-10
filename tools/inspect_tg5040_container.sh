#!/usr/bin/env sh
set -eu

echo "TOOLCHAIN_BEGIN"
command -v arm-linux-gnueabihf-g++ || true
command -v arm-linux-gnueabi-g++ || true
command -v aarch64-linux-gnu-g++ || true
echo "TOOLCHAIN_END"

echo "PKGCONFIG_BEGIN"
pkg-config --list-all 2>/dev/null | grep -Ei 'sdl|zip' || true
echo "PKGCONFIG_END"

echo "ENV_BEGIN"
env | grep -E 'PKG_CONFIG|SYSROOT|CC|CXX|SDL' || true
echo "ENV_END"

echo "FS_BEGIN"
find /usr /opt -maxdepth 4 \( -name '*.pc' -o -name '*SDL*' -o -name '*zip*' \) 2>/dev/null | sort | head -n 200
echo "FS_END"
