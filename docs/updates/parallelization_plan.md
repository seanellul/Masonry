# Plan: Parallelize the Ingnomia Game Loop

## Context

The game loop in `src/game/game.cpp:177-237` runs **13 subsystems sequentially on one thread** per tick. On modern multi-core CPUs, only ~5% of available processing power is used. This was the most-requested performance improvement from the Discord community (4,693 suggestions analyzed).

Pathfinding already uses `std::async` internally (`src/base/pathfinder.cpp:156`), but it blocks the main loop with `task.get()`. No other system uses threading. The goal is to parallelize independent systems to dramatically reduce per-tick time and enable faster game speeds / larger maps / more gnomes.

## Current Architecture

```
Game::loop() — single thread, sequential:
  1. World::processGrass()        — reads/writes Tile.vegetationLevel, Tile.floorSpriteUID
  2. processPlants()              — reads/writes World::m_plants
  3. CreatureManager::onTick()    — reads World tiles, writes creature positions
  4. GnomeManager::onTick()       — reads/writes World, Inventory, JobManager (HOT PATH)
  5. JobManager::onTick()         — reads/writes job queues, checks Inventory
  6. StockpileManager::onTick()   — reads/writes Inventory
  7. FarmingManager::onTick()     — reads World, adds Jobs
  8. WorkshopManager::onTick()    — manages workshop state
  9. RoomManager::onTick()        — reads World, manages room state
  10. EventManager::onTick()      — schedules events, triggers migration
  11. MechanismManager::onTick()  — reads Inventory (names), writes Tile sprites/walkability
  12. FluidManager::onTick()      — reads/writes Tile.fluidLevel, reads MechanismManager
  13. NeighborManager::onTick()   — manages external faction state
  14. SoundManager::onTick()      — emits signals only
  15. World::processWater()       — reads/writes Tile.fluidLevel
  16. PathFinder::findPaths()     — already async, but blocks until complete
```

## Key Data Structures

- **Tile** (`src/base/tile.h:162`): 40-byte struct with flags, floor/wall type/material, fluid level, light, vegetation, sprites. Stored as `std::vector<Tile>` flat array in `World`.
- **World** (`src/game/world.h`): owns `m_world` (tile vector), `m_plants`, `m_creaturePositions`. Has `m_updateMutex` already used for tile update tracking.
- **Inventory** (`src/game/inventory.h`): shared item database. Accessed by Gnomes, Jobs, Stockpiles, Workshops, Mechanisms.
- **JobManager** (`src/game/jobmanager.h`): job queues. Gnomes request jobs, farming/workshops add jobs.
- **PathFinder** (`src/base/pathfinder.h`): already has `QMutex m_mutex`, uses `std::async` for parallel A* searches. `PathFinderThread` only reads World tiles (safe concurrent reads).
- **GameState** (`src/base/gamestate.h`): all static fields — tick counter, time, season flags. Read by everything.

## Dependency Analysis

### Safe to parallelize (no shared mutable state between them):
- **Group A — Natural world**: `processGrass()` + `processPlants()` + `processWater()` — all modify different Tile fields (vegetation vs fluid vs plant map)
- **Group B — Infrastructure**: `MechanismManager` + `FluidManager` — Fluid reads mechanism power state, but mechanisms don't read fluids. Can run sequentially within group but parallel to others.
- **Group C — Passive**: `SoundManager` + `NeighborManager` + `RoomManager` + `EventManager` — mostly self-contained state

### Tightly coupled (must be sequential):
- **GnomeManager** ↔ **JobManager** ↔ **Inventory** ↔ **StockpileManager** — gnomes pick up/drop items, claim/complete jobs, all through shared mutable state via `g->` pointer. This is the hot path (~70%+ of tick time).
- **CreatureManager** shares world position state with gnomes

### Already parallel:
- **PathFinder::findPaths()** — dispatches `std::async` tasks per path request, but blocks the main loop at `task.get()` (line 161-163)

## Implementation Plan

### Phase 1: Overlap pathfinding with the next tick (biggest win, lowest risk)

**Problem**: `findPaths()` dispatches async workers then immediately blocks with `task.get()`. Meanwhile, the main thread sits idle.

**Solution**: Make pathfinding non-blocking. Start path workers at the end of tick N, let them run during tick N+1's other work, collect results at the start of tick N+2.

**Files to modify**:
- `src/base/pathfinder.h` — add `dispatchPaths()` and `collectResults()` methods
- `src/base/pathfinder.cpp` — split `findPaths()` into dispatch (launch async) and collect (get results)
- `src/game/game.cpp` — call `dispatchPaths()` at end of tick, `collectResults()` at start of next tick

**Detail**:
```cpp
// pathfinder.h — new methods
void dispatchPaths();   // Launch async workers (non-blocking)
void collectResults();  // Wait for outstanding workers and store results

// game.cpp — new tick structure
void Game::loop() {
    m_pf->collectResults();  // Get paths from previous tick's dispatch

    // ... all existing onTick calls ...

    m_pf->dispatchPaths();   // Launch path workers (runs during next loop iteration)
}
```

**Risk**: Paths are 1 tick stale. Since gnomes already wait multiple ticks for paths (they get `Running` status and retry), this adds at most 1 tick of latency — imperceptible at 50ms/tick.

### Phase 2: Parallel natural world processing

**Problem**: Grass, plants, and water simulation run sequentially but touch independent data.

**Solution**: Run them concurrently using `std::async`.

**Files to modify**:
- `src/game/game.cpp` — wrap grass/plants/water in async tasks

**Detail**:
```cpp
// In Game::loop(), replace sequential calls with:
auto grassFuture = std::async(std::launch::async, [this]{ m_world->processGrass(); });
auto waterFuture = std::async(std::launch::async, [this]{ m_world->processWater(); });
processPlants();  // Can run on main thread

grassFuture.get();
waterFuture.get();

// Then proceed with creature/gnome ticks
```

**Safety**: `processGrass()` writes `vegetationLevel` + `floorSpriteUID`. `processWater()` writes `fluidLevel` + `pressure` + `flow`. These are different fields on the Tile struct — no data race. `processPlants()` writes to `World::m_plants` (a separate map) — also independent.

**Files to verify no overlap**:
- `src/game/world.cpp:593` — `processGrass()` implementation
- `src/game/world.cpp:772` — `processWater()` implementation

### Phase 3: Parallel passive systems

**Problem**: Sound, neighbors, rooms, events, mechanisms run sequentially but are mostly independent.

**Solution**: Group them into an async batch that runs parallel to the main gnome/job/stockpile pipeline.

**Files to modify**:
- `src/game/game.cpp` — restructure tick to overlap passive systems with active systems

**Detail**:
```cpp
// Launch passive systems in parallel with the gnome pipeline
auto passiveFuture = std::async(std::launch::async, [this, tick, sc, dc, hc, mc]{
    m_roomManager->onTick(tick);
    m_eventManager->onTick(tick, sc, dc, hc, mc);
    m_mechanismManager->onTick(tick, sc, dc, hc, mc);
    m_fluidManager->onTick(tick, sc, dc, hc, mc);
    m_neighborManager->onTick(tick, sc, dc, hc, mc);
    m_soundManager->onTick(tick);
});

// Main thread handles the hot path
m_creatureManager->onTick(tick, sc, dc, hc, mc);
m_gnomeManager->onTick(tick, sc, dc, hc, mc);
m_jobManager->onTick();
m_spm->onTick(tick);
m_farmingManager->onTick(tick, sc, dc, hc, mc);
m_workshopManager->onTick(tick);

passiveFuture.get();  // Sync before next tick
```

**Risk — MechanismManager**: Reads `Inventory::itemSID()` (read-only string lookups) and writes `World` tile sprite/walkability. The gnome pipeline also writes World tiles (gnome movement). **Mitigation**: `World` already has `m_updateMutex` — mechanism tile writes go through `setWallSpriteAnim()` and `setWalkable()` which should use this mutex. Verify this is the case, or add mutex protection.

**Risk — EventManager**: Calls `signalEvent` which emits Qt signals. Qt signals are thread-safe when using `Qt::QueuedConnection` (already the case — see `gamemanager.cpp:259`).

**Risk — FarmingManager**: Calls `g->m_jobManager->addJob()` during beehive tick. This would race with gnomes consuming jobs on the main thread. **Mitigation**: Either protect `JobManager::addJob()` with a mutex, or move `FarmingManager` back to the main thread (it's cheap anyway).

### Phase 4 (future): Gnome-level parallelism

This is the highest-reward but highest-risk phase. Each gnome's `onTick()` runs a behavior tree that can call:
- `g->pf()->getPath()` — already mutex-protected
- `g->inv()->...` — Inventory reads/writes (item pickup, drop, consume)
- `g->jm()->...` — JobManager (claim job, return job)
- `g->w()->getTile()` — World tile reads (pathfinding, floor checks)

**Approach**: Partition gnomes into batches, process batches on thread pool, with mutexes around Inventory and JobManager access points. This is a larger change and should be done after Phases 1-3 prove stable.

**NOT in scope for this plan** — listed for future reference.

## Files to Modify

| File | Changes |
|------|---------|
| `src/base/pathfinder.h` | Add `dispatchPaths()` / `collectResults()` methods |
| `src/base/pathfinder.cpp` | Split `findPaths()` into non-blocking dispatch + collect |
| `src/game/game.cpp` | Restructure `loop()` to overlap pathfinding, natural world, and passive systems |
| `src/game/world.cpp` | Verify `processGrass()` and `processWater()` don't share Tile fields |
| `src/game/mechanismmanager.cpp` | Verify/add mutex protection for tile writes |
| `src/game/farmingmanager.cpp` | Verify `addJob()` calls — may need mutex or stay on main thread |

## Verification

1. **Build**: `cmake --build build` — must compile cleanly
2. **Basic sanity**: Launch game, create a new world, let it run for 5+ in-game days. Verify:
   - Gnomes pathfind correctly (no freezing, no teleporting)
   - Grass grows, water flows, mechanisms animate
   - No crashes or hangs (race conditions often manifest as intermittent crashes)
3. **Performance**: Add timing around the tick loop phases:
   ```cpp
   QElapsedTimer phaseTimer;
   phaseTimer.start();
   // ... phase ...
   qDebug() << "Phase X:" << phaseTimer.elapsed() << "ms";
   ```
   Compare total tick time before/after. Expect 20-40% improvement from Phases 1-3.
4. **Stress test**: Increase game speed to max, run for 20+ in-game years. Watch for:
   - Memory leaks (growing RSS)
   - Deadlocks (game freezes)
   - Data corruption (gnomes in walls, items disappearing)
5. **Thread sanitizer**: Build with `-fsanitize=thread` (if compiler supports it) to detect data races automatically
