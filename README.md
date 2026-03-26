# Masonry

A colony simulator where you guide a group of gnomes to build, mine, craft, and survive. Inspired by [Gnomoria](https://store.steampowered.com/app/224500/Gnomoria/) and originally forked from [Ingnomia](https://github.com/rschurade/Ingnomia), Masonry has since been substantially rewritten with a new UI framework, macOS support, modern rendering pipeline, and many new gameplay features.

**[Download latest release](https://github.com/seanellul/Masonry/releases/latest)** (Windows, macOS, Linux)

## Features

- **Colony management** — Direct gnomes to mine, farm, craft, build, and defend
- **3D voxel world** — Fully destructible terrain with multiple z-levels
- **ImGui-based UI** — Modern, responsive interface (replaced legacy Noesis/XAML)
- **macOS support** — OpenGL 4.1 core profile, runs natively on Apple Silicon
- **Post-processing** — Bloom, depth fog, seasonal color grading, vignette
- **Fog of war** — Underground caverns remain hidden until explored
- **Behavior tree AI** — Creatures driven by XML behavior trees

## Build (macOS)

**Dependencies**: Qt5, OpenAL Soft, CMake 3.16+ (all via Homebrew)

```bash
brew install qt@5 openal-soft cmake

cmake -B build \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt@5) \
  -DOPENAL_ROOT=$(brew --prefix openal-soft) \
  -DCMAKE_FIND_FRAMEWORK=LAST

cmake --build build -j$(sysctl -n hw.ncpu)
```

Content tilesheets (`content/tilesheet/`) are required at runtime but not in the repo — copy from an existing Gnomoria or Ingnomia installation.

## Build (Windows/Linux)

```bash
cmake -S . -B build -DQt5_DIR="<path_to_qt5>/lib/cmake/Qt5"
cmake --build build
```

See `CLAUDE.md` for detailed architecture documentation.

## Test Mode

```bash
./build/Masonry --test-mode
./build/Masonry --test-mode --load-save <path> --run-ticks 100 --screenshot out.png
```

## License

[GNU AGPL v3](LICENSE)

## Credits

See [CREDITS.md](CREDITS.md) for attribution to the original Ingnomia project and contributors.
