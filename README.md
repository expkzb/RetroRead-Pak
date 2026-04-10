# RetroRead

A game-like dialogue-style reader Pak for TrimUI NextUI.

This repository now contains a working desktop prototype plus two device-facing paths:

- application lifecycle
- scene system
- dialogue-oriented reader scene
- EPUB compiler and cache
- desktop SDL backend
- NextUI input/render/filesystem bridge layers
- TG5040 export scaffolding for TrimUI Brick

## Build in WSL

This project currently builds inside Ubuntu on WSL with the default GNU toolchain.

```bash
sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev libzip-dev
cd /mnt/c/Users/golden/Documents/Developers/nextReading
make
```

Run it with:

```bash
./nextreading
```

Desktop debug controls:

- `Enter` / `Z` / `A`: confirm / next
- `X` / `Space`: fast-forward current sentence
- `T` / `Tab`: toggle auto-play
- `Q` / `[` and `E` / `]`: previous / next chapter
- `P` / `F1`: settings
- `Esc`: back / menu
- arrows: list navigation and previous / next sentence
- `Start + Select` (controller): force quit app

## Build a Pak Layout

To assemble a deployable Pak-style folder:

```bash
cd /mnt/c/Users/golden/Documents/Developers/nextReading
make pak
```

This produces:

```text
dist/RetroRead/
  nextreading
  launch.sh
  manifest.json
  assets/
```

## Build a TG5040 Export

For the shortest hardware path on TrimUI Brick, the preferred route is now:

- keep SDL as the runtime backend
- cross-compile for `TG5040`
- export a Pak-style folder

Scaffold targets are now in place:

```bash
cd /mnt/c/Users/golden/Documents/Developers/nextReading
make tg5040
make export-tg5040
```

If you want a Docker workflow closer to existing TG5040 projects:

```bash
make -f ports/tg5040/Makefile.docker tg5040
make -f ports/tg5040/Makefile.docker export
```

The exported folder layout is:

```text
dist/tg5040/RetroRead.pak/
  nextreading
  launch.sh
  pak.json
  assets/
```

Current caveat: the TG5040 path currently builds with `NEXTREADING_NO_SDL_TTF`, so launch/runtime works but text rendering is temporary and needs a follow-up font path on device.

The TG5040 launch script intentionally runs the SDL path rather than `--nextui`.

See [ports/tg5040/README.md](/C:/Users/golden/Documents/Developers/nextReading/ports/tg5040/README.md) for the expected cross-compilation environment.

`launch.sh` starts the app in `--nextui` mode and seeds the default Brick-sized environment:

- `NEXTUI_SCREEN_WIDTH=1024`
- `NEXTUI_SCREEN_HEIGHT=768`
- `NEXTREADING_HOME`
- `NEXTREADING_BOOKS_PATH`
- `NEXTREADING_CACHE_PATH`
- `NEXTREADING_SAVES_PATH`
- `NEXTREADING_ASSETS_PATH`

## Current NextUI Port Status

- EPUB archive loading is native `libzip`
- Desktop SDL mode remains the fastest way to preview changes
- NextUI mode now has bridge hooks for renderer and input
- `src/platform/nextui/NextUIBindings.cpp` is now the single place to wire real TrimUI Brick / NextUI drawing and button APIs
- The remaining device work is filling those bindings with the actual device-side functions

## Current Brick Hardware Path

- The fastest route to real TrimUI Brick hardware is now the TG5040 SDL export path
- `ports/tg5040/` contains the first cross-build and packaging scaffold
- The remaining work is providing a real TG5040 toolchain/sysroot and validating the exported Pak on device
