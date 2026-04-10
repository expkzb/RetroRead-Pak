#!/usr/bin/env sh
set -eu

echo "PKG_BIN_BEGIN"
command -v pkg-config || true
echo "PKG_BIN_END"

echo "PKG_TEST_BEGIN"
pkg-config --cflags sdl2 || true
pkg-config --libs sdl2 || true
pkg-config --cflags libzip || true
pkg-config --libs libzip || true
echo "PKG_TEST_END"

echo "MAKE_VARS_BEGIN"
make -C ports/tg5040 -pn | grep -E '^(PKG_CONFIG|SDL_CFLAGS|SDL_LIBS|ZIP_CFLAGS|ZIP_LIBS|CXX|CXXFLAGS) ?[:?]?=' || true
echo "MAKE_VARS_END"
