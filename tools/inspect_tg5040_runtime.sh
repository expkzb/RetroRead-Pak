#!/usr/bin/env sh
set -eu

BIN_PATH="ports/tg5040/build/nextreading"
if [ ! -f "$BIN_PATH" ]; then
  echo "missing-binary:$BIN_PATH"
  exit 1
fi

echo "NEEDED_BEGIN"
readelf -d "$BIN_PATH" | grep NEEDED || true
echo "NEEDED_END"

echo "SYSROOT_HINT_BEGIN"
env | grep -E '^SYSROOT=|^PKG_CONFIG_SYSROOT_DIR=' || true
echo "SYSROOT_HINT_END"

for root in \
  "${SYSROOT:-}" \
  "${PKG_CONFIG_SYSROOT_DIR:-}" \
  "/opt/aarch64-nextui-linux-gnu/aarch64-nextui-linux-gnu/libc" \
  "/opt/arm-linux-gnueabihf/arm-linux-gnueabihf/libc"
do
  if [ -n "$root" ] && [ -d "$root/usr/lib" ]; then
    echo "LIB_ROOT:$root/usr/lib"
    ls "$root/usr/lib" | grep -E 'libSDL2|libzip|libfreetype|libpng|libbz2|libz|libharfbuzz|libbrotli|libglib' || true
    break
  fi
done
