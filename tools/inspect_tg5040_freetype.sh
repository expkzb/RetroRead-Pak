#!/usr/bin/env sh
set -eu

echo "PKG_FREETYPE_BEGIN"
pkg-config --cflags freetype2 2>/dev/null || true
pkg-config --libs freetype2 2>/dev/null || true
echo "PKG_FREETYPE_END"

echo "FILES_BEGIN"
find /opt/aarch64-nextui-linux-gnu -maxdepth 7 -type f \( -name 'ft2build.h' -o -name 'freetype2.pc' -o -name 'libfreetype*.so*' \) 2>/dev/null | sort | head -n 100
echo "FILES_END"
