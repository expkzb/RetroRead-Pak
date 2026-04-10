# TG5040 Build Notes

This port targets `TG5040`, the platform family used by:

- TrimUI Brick
- TrimUI Smart Pro

For the shortest path to real hardware, this project currently follows the same broad deployment idea as `SDLReader-brick`:

- keep the app as an SDL program
- cross-compile for `TG5040`
- export a self-contained Pak-style folder

## Expected Flow

From the workspace root:

```bash
make tg5040
make export-tg5040
```

This expects a working TG5040 cross-compilation environment that provides:

- a target C++ compiler
- SDL2 headers and libraries for the target
- SDL2_ttf headers and libraries for the target
- libzip for the target
- matching `pkg-config` metadata or equivalent compiler/linker flags

If you prefer a containerized workflow, this repo also includes:

- [Makefile.docker](/C:/Users/golden/Documents/Developers/nextReading/ports/tg5040/Makefile.docker)
- [docker-compose.yml](/C:/Users/golden/Documents/Developers/nextReading/ports/tg5040/docker-compose.yml)

Example:

```bash
cd /mnt/c/Users/golden/Documents/Developers/nextReading
make -f ports/tg5040/Makefile.docker tg5040
make -f ports/tg5040/Makefile.docker export
```

## Output

`make export-tg5040` creates:

```text
dist/tg5040/NextReading.pak/
  nextreading
  launch.sh
  pak.json
  assets/
```

## Launch Model

The TG5040 launch script intentionally runs the SDL path, not `--nextui`.

That keeps the device route aligned with the proven `SDLReader-brick` approach while the optional NextUI-native bridge work continues separately.

## Current Rendering Caveat

The current TG5040 build enables `NEXTREADING_NO_SDL_TTF` because the provided toolchain image does not ship `SDL2_ttf`.

That means the app can launch and run, but text rendering is in a temporary fallback mode. The next device step is adding a real font rendering path (either by providing SDL2_ttf in the sysroot or wiring NextUI-native text drawing).
