# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is Masonry

Masonry (formerly Ingnomia) is a Gnomoria-inspired colony simulator written in C++17. It uses Qt5 for windowing/app framework, OpenGL for rendering, and Dear ImGui for the in-game UI (replacing the legacy Noesis/XAML GUI which has been removed). Originally forked from [rschurade/Ingnomia](https://github.com/rschurade/Ingnomia), it has been substantially rewritten.

## Build Commands (macOS)

```bash
# Configure
cmake -B build \
  -DCMAKE_PREFIX_PATH=$(brew --prefix qt@5) \
  -DOPENAL_ROOT=$(brew --prefix openal-soft) \
  -DCMAKE_FIND_FRAMEWORK=LAST

# Build
cmake --build build -j$(sysctl -n hw.ncpu)
```

Dependencies: Qt5, OpenAL Soft, CMake 3.16+ (all via Homebrew). Steam SDK is optional and not required.

Content tilesheets (`content/tilesheet/`) are required at runtime but not in the repo — copy from an existing installation.

## Test Mode

```bash
# Run with test mode (suppresses audio, enables test controller + MCP command server)
./build/Ingnomia --test-mode

# Load a specific save, run N ticks, take screenshot
./build/Ingnomia --test-mode --load-save <path> --run-ticks 100 --screenshot out.png

# MCP test server (tools/mcp_server.py) connects to the test command server on localhost
```

## Architecture

### Threading Model
- **Main/GUI thread**: Qt event loop, OpenGL rendering (`MainWindow`/`MainWindowRenderer`), ImGui drawing
- **Game thread**: `GameManager` runs on a separate `QThread`, owns the `Game` object and all simulation logic

Cross-thread communication uses Qt signals/slots exclusively via `EventConnector`, which brokers between the GUI and game threads.

### Source Layout

- **`src/base/`** — Engine foundations: `Global` (static singleton with game-wide state), `Config`, `DB` (SQLite game database), pathfinding, tile/position types, behavior trees
- **`src/game/`** — Simulation: `Game` (tick loop), `World` (voxel map), creature hierarchy (`Creature` → `Gnome`/`Animal`/`Monster`/`Automaton`), manager classes (`JobManager`, `StockpileManager`, `FarmingManager`, etc.)
- **`src/gfx/`** — Sprite loading and `SpriteFactory`
- **`src/gui/`** — Rendering and UI layer:
  - `MainWindow` (QOpenGLWindow) — input handling, GL lifecycle
  - `MainWindowRenderer` — OpenGL tile/sprite rendering
  - `ImGuiBridge` + `imgui_impl_qt5` — Dear ImGui integration with Qt5/OpenGL
  - `ui/` — ImGui panel implementations (game HUD, main menu, side panels, tile info)
  - `Aggregator*` classes — data adapters that query game state for UI display
  - `EventConnector` — signal/slot hub connecting GUI ↔ game thread
- **`src/test/`** — `TestController` (automated test sequences), `TestCommandServer` (TCP command interface for MCP)

### Key Patterns

- **`Global` static class**: Acts as a service locator — holds pointers to `Config`, `EventConnector`, `Selection`, `NewGameSettings`, and global flags. Access from anywhere via `Global::`.
- **Aggregator pattern**: Each UI domain (tile info, stockpile, workshop, etc.) has an `Aggregator*` class that collects data from the game thread and formats it for the UI. These live in the GUI thread and are owned by `EventConnector`.
- **Creature hierarchy**: `Object` → `Creature` → `Gnome`/`Animal`/`Monster`/`Automaton`, with `CanWork` mixin for gnomes. Behavior trees (XML) drive creature AI.
- **World**: 3D voxel grid (`dimX × dimY × dimZ`), tiles stored flat. `World` class has split implementation files: `world.cpp`, `worldgetters.cpp`, `worldconstructions.cpp`.

### OpenGL / Rendering

OpenGL 4.1 core profile (downgraded from 4.3 for macOS compatibility). Uses Texture Buffer Objects instead of SSBOs. Shaders are in `content/shaders/` as `.glsl` files.

### Database

Game data (items, materials, recipes, etc.) loaded from SQLite databases in `content/db/` via Qt SQL. `DB::init()` and `DB::initStructs()` set up the schema and load data into `DBStruct` types.

## Development Log — MANDATORY

**Every code change must be logged in `DEVLOG.md` at the project root.** This is a hard rule — no exceptions.

After completing any fix, feature, or improvement:
1. Add a new entry at the TOP of `DEVLOG.md` (newest first)
2. Use this format:
```markdown
## [YYYY-MM-DD] Short Description

**Milestone**: X.X — Category
**Files changed**: `file1.cpp`, `file2.h`, ...

### Changes
- **Bold summary of change** — explanation of what and why
- Additional bullet points as needed

### Technical Details
- Implementation specifics that future developers should know
```
3. Reference the relevant milestone from `docs/updates/development_roadmap.md`
4. After logging, check if the fix/feature maps to a Discord reply in `docs/discord_reply_queue.json` and update if needed

This log becomes the release changelog and the source for Discord community replies.

## License

GNU AGPL v3. All contributions must adhere to this license.
