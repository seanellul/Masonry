# Plan: Parallelize & Optimize the Ingnomia Game Loop

## Performance Laws

These principles govern all simulation code. New features must comply before merge.

### Law 1: Measure Before Optimizing
Every optimization must start with profiling data, not assumptions. The tick profiler (`[TICK N]` log lines in `game.cpp`) exists for this purpose. If a subsystem doesn't register on the profiler, it doesn't need optimization — focus elsewhere.

### Law 2: O(active), Not O(world)
Subsystems must scale with the number of *active entities*, not the total world size. A 200×200×200 map with 10 gnomes should tick as fast as a 100×100×100 map with 10 gnomes. Concrete rules:
- **Never iterate all tiles per tick.** Use dirty-sets, spatial indices, or active-entity lists.
- **Never scan all items to find one.** Use indexed lookups (by type, position, or status).
- **Never pathfind across the entire map per gnome.** Use hierarchical/cached pathfinding.

### Law 3: Tick Budget
Each tick has a time budget based on game speed:
- **Normal speed**: 50ms budget (20 ticks/sec)
- **Fast speed**: 5ms budget (200 ticks/sec)

Any single subsystem exceeding 30% of the budget is a regression. The tick profiler reports per-subsystem costs — use it to catch regressions early.

### Law 4: Parallelize Reads, Serialize Writes
Multiple subsystems can read shared state concurrently (World tiles, Inventory items). Writes to shared state must be serialized or use independent data regions. The existing pattern in `AggregatorRenderer` (parallel read-only tile aggregation) is the model.

### Law 5: No Idle Cores
When a tick takes longer than budget, the CPU should be saturated — not one core at 100% and seven at 0%. Use the parallelization phases below to overlap independent work across cores.

---

## Phase 0: Full Audit Results (2026-03-24)

### Profiling Setup

- **Save**: `content/test_saves/smoke_test` — 130×100×100 world (1.3M tiles), 1 gnome, wild animals, 259 items
- **Hardware**: Apple M1 Pro (10 cores), 16 GB RAM
- **Method**: `QElapsedTimer` per subsystem in `game.cpp`, logged every 50 ticks
- **Finding**: All 16 subsystems complete in **1ms total**. The `QTimer` interval (50ms normal / 5ms fast) is the actual frame-rate limiter, not CPU work. Parallelization only matters once subsystem work exceeds the timer budget at scale.

### Tick Breakdown (steady-state, 1 gnome)

| Subsystem | Time (ms) | Notes |
|-----------|-----------|-------|
| CreatureManager::onTick | 1–9 | Wild animals running behavior trees (see audit below) |
| All other subsystems | 0 | Sub-millisecond with 1 gnome, 259 items |
| **QTimer sleep** | **~49** | The real bottleneck — game idles between ticks |

---

### Audit 1: World Processing (Grass / Water / Plants)

**Verdict: Already well-designed. No O(world) scans.**

| Function | Iterates | Complexity | Status |
|----------|----------|------------|--------|
| `processGrass()` | `m_grassCandidatePositions` (frontier set) | O(frontier perimeter) | Good — active-set pattern |
| `processWater()` / `processWaterFlow()` | `m_water` (tiles with fluid) | O(wet tiles × 7 neighbors) | Good — active-set pattern |
| `processPlants()` | `m_plants` map (tiles with plants) | O(plant count) | Good — entity-based |

**Minor fixes identified:**
- `m_water` uses `std::set` (O(log n) per op) — should be `std::unordered_set` (O(1)) for large floods
- Type inconsistency: `m_grass` is `QSet<Position>` while `m_grassCandidatePositions` is `QSet<unsigned int>` — unify to `unsigned int`

---

### Audit 2: CreatureManager — The 1-9ms Spike

**Root cause: "Animals: 0" is misleading.** The kingdom info counts only pasture-managed animals (`FarmingManager::countAnimals()`). `CreatureManager::m_creatures` holds **wild animals** from world generation — the test save has wild creatures all running behavior trees every tick with no time-slicing.

**Scaling bombs found:**

| Pattern | Location | Complexity | Impact |
|---------|----------|------------|--------|
| `creaturesAtPosition()` | `creature.cpp:721` (move) | O(m_creatures) linear scan | Called on every transparent creature move |
| `gnomesAtPosition()` | `gnomemanager.cpp:309` | O(gnomes) linear scan | Called by every monster on move attempt |
| `actionFindPrey` | `animal.cpp:844` | O(animals_of_prey_type) | Per predator per tick, with region check per candidate |
| `generateAggroList()` | `monster.cpp:172` | O(gnomes) | Every tick when monster aggro list is empty |
| No time-slicing | `creaturemanager.cpp:82-84` | — | Throttle code is commented out — all creatures run every tick |

**Recommended fixes:**
1. **Enable time-slicing** — uncomment/implement the budget guard (like `GnomeManager`'s 5ms cap)
2. **Spatial index for creature positions** — replace linear scans in `creaturesAtPosition()` and `gnomesAtPosition()` with `QHash<unsigned int, QSet<unsigned int>>` (tileID → creatureIDs)
3. **Throttle `generateAggroList()`** — cache aggro list for N ticks instead of regenerating every tick

---

### Audit 3: Hot Path — Gnome / Job / Inventory / Stockpile / Farm / Workshop

**This is where the real scaling walls live.** Currently invisible at 1 gnome, but will dominate at 20+ gnomes.

#### JobManager::getJob() — The Dominant Bottleneck

Location: `jobmanager.cpp:475–615`

Every idle gnome calls `getJob()` which evaluates:
```
for each skill (up to 40):
  for each priority (10 levels):
    for each job-type matching skill:
      for each job at that priority:
        requiredItemsAvail()  →  octree query per ingredient per material
```

**Complexity: O(gnomes × skills × priorities × jobs × items × materials)**

No cache exists for "does this job have items available" — every idle gnome re-evaluates from scratch. With 50 idle gnomes and 200 pending jobs: 10,000+ octree queries per tick.

**Estimated threshold: ~50 gnomes × 40 pending jobs before exceeding 15ms.**

**Fix:** Job-availability cache, dirty-flagged when items change. Idle gnomes skip octree queries for already-evaluated jobs.

#### Inventory::itemCount() — Uncached Linear Scan

Location: `inventory.cpp:1028`

When `materialSID == "any"`, iterates all items of that type. Called constantly by workshops and farms. No count cache exists.

**Fix:** Maintain `QHash<QString, QHash<QString, int>>` count cache, inc/dec on item create/destroy.

#### Farm/Grove::onTick() — Unthrottled O(farm_tiles) Every Tick

Location: `farm.cpp:175`, `grove.cpp:154`

`getClosestItem()` called for every jobless tile every single tick. A 400-tile farm = 400 octree queries/tick.

**Fix:** Throttle to `hourChanged` or batch one seed-availability check per farm, not per tile.

#### Pasture::onTick() — Hidden O(n²)

Location: `pasture.cpp:240`

Each pasture iterates ALL global animals to find unassigned ones: O(pastures × total_animals).

**Fix:** Maintain "unassigned tame animal" list in `CreatureManager`, drain from it.

#### WorkshopManager — Missing ID Index

Location: `workshopmanager.cpp:111`

`workshop(id)` is a linear scan over `QList<Workshop*>`. Called O(workshops²) per tick.

**Fix:** Replace with `QHash<unsigned int, Workshop*>`.

#### StockpileManager::finishJob() — No Reverse Index

Location: `stockpilemanager.cpp:242, 254`

`finishJob()` and `giveBackJob()` iterate all stockpiles to find the job owner.

**Fix:** Add `QHash<unsigned int, unsigned int>` mapping jobID → stockpileID.

#### Complete Scaling Estimate Table

| Subsystem | Dominant Pattern | Est. Threshold | Priority |
|-----------|-----------------|----------------|----------|
| **JobManager::getJob()** | gnomes × jobs × items × materials | ~50 gnomes × 40 jobs | **Critical** |
| **Farm/Grove::onTick()** | farm_tiles × octree per tick | ~5,000 farm tiles | **High** |
| **Inventory::itemCount()** | linear scan all items of type | ~10,000 items | **High** |
| **WorkshopManager lookup** | O(workshops) per lookup, called O(W²) | ~15 workshops | **Medium** |
| **StockpileManager finish** | O(stockpiles) reverse scan | Worsens with count | **Medium** |
| **Pasture animal assign** | O(pastures × all_animals) | ~20 pastures × 500 animals | **Medium** |
| **Stockpile::getJob()** | 50 items × 2 × stockpile_fields | ~15 large stockpiles | **Medium** |

---

### Audit 4: PathFinder

**Verdict: Smart design with a ticking time bomb in the thread model.**

**What works well:**
- A* with admissible heuristic, optimal paths
- Region map pre-rejects unreachable paths in O(1) (cached) or O(regions) (first time)
- Goal-folding: multiple gnomes targeting the same destination share one A* search
- Gnomes cache paths and walk them step-by-step — no re-pathfinding every tick
- `GnomeManager` has a 5ms budget cap that batches gnome ticks across frames

**What will break at scale:**

| Issue | Location | Impact |
|-------|----------|--------|
| **No thread pool** — `std::async` creates a fresh OS thread per goal group | `pathfinder.cpp:104` | ~0.1ms per thread creation; 20+ groups = multi-ms overhead |
| **`cancelRequest()` is a stub** (TODO comment) | `pathfinder.cpp:39-42` | Dead/reassigned gnomes still run full A* to completion, results discarded |
| **Full cache invalidation on any tile split** | `regionmap.cpp:168` | Digging one tile clears entire region connectivity cache |
| **Game thread blocks on `task.get()`** | `pathfinder.cpp:161-163` | Slowest A* search serializes the entire tick |
| **Latent bug**: `checkSplitFlood` allocates `QVector<bool>(m_dimX * m_dimX)` | `regionmap.cpp:615` | Should be `m_dimX * m_dimY` |
| **No path result caching** | — | Full A* every time a gnome needs a new path |

**Estimated ceiling:** Thread creation overhead bites at ~50-100 gnomes with diverse job targets (~20+ distinct goal groups).

**Recommended fixes (priority order):**
1. **Thread pool** — replace `std::async` with a fixed-size `QThreadPool` or `std::thread` pool
2. **Implement `cancelRequest()`** — kill stale pathfinding jobs when gnomes change goals or die
3. **Incremental cache invalidation** — on tile split, invalidate only affected region pairs, not the entire cache
4. **Non-blocking dispatch** (original Phase 1) — dispatch at end of tick, collect at start of next

---

## Implementation Priority (Revised)

Based on the audit, the order of work should be driven by **when each bottleneck will actually bite**, not theoretical threading gains.

### Tier 1 — Do Now (prevents scaling walls at 20+ gnomes)

| # | Fix | Subsystem | Effort | Impact |
|---|-----|-----------|--------|--------|
| 1 | Job-availability cache (dirty-flagged on item changes) | JobManager | Medium | Eliminates 10,000+ redundant octree queries/tick |
| 2 | Inventory item count cache | Inventory | Small | O(1) instead of O(items) for every `itemCount("any")` |
| 3 | Workshop ID hash index | WorkshopManager | Small | O(1) instead of O(workshops) per lookup |
| 4 | Creature position spatial index | CreatureManager | Small | O(1) instead of O(creatures) for position queries |
| 5 | Enable creature time-slicing | CreatureManager | Small | Budget-cap wild animal ticks like gnomes |

### Tier 2 — Do Before Large Colonies (prevents walls at 50+ gnomes, large farms)

| # | Fix | Subsystem | Effort | Impact |
|---|-----|-----------|--------|--------|
| 6 | Throttle farm/grove tile checks to hourChanged | FarmingManager | Small | 60× reduction in octree queries |
| 7 | Job-to-stockpile reverse index | StockpileManager | Small | O(1) instead of O(stockpiles) per finish |
| 8 | Pathfinder thread pool | PathFinder | Medium | Eliminates OS thread creation overhead |
| 9 | Implement `cancelRequest()` | PathFinder | Small | Stop wasting A* on dead requests |
| 10 | Unassigned-animal list for pastures | FarmingManager | Small | O(unassigned) instead of O(pastures × all_animals) |

### Tier 3 — Do When Tick Budget Exceeded (parallelization)

Only after Tiers 1-2 are done and profiling shows subsystems still exceeding budget:

| # | Fix | Description | Effort |
|---|-----|-------------|--------|
| 11 | Non-blocking pathfinding | Dispatch end of tick N, collect start of tick N+2 | Medium |
| 12 | Parallel natural world | `processGrass` + `processWater` + `processPlants` on async threads | Small |
| 13 | Parallel passive systems | Rooms/events/mechanisms/fluids overlap with gnome pipeline | Medium |
| 14 | Gnome-level parallelism | Partition gnomes across thread pool with mutex guards | Large |

### Tier 4 — Minor Cleanups

| # | Fix | Location | Effort |
|---|-----|----------|--------|
| 15 | `m_water` → `std::unordered_set` | `world.h:97` | Trivial |
| 16 | Fix `checkSplitFlood` bug (`m_dimX * m_dimX` → `m_dimX * m_dimY`) | `regionmap.cpp:615` | Trivial |
| 17 | Incremental region cache invalidation | `regionmap.cpp:168` | Medium |
| 18 | Unify grass sets to `QSet<unsigned int>` | `world.h:94-95` | Trivial |

---

## Renderer Init Optimization (Completed 2026-03-24)

Separate from the tick loop, the renderer initialization path was optimized:

| Optimization | Description | Impact |
|---|---|---|
| **Skip empty tiles** | GPU buffer is pre-zeroed; skip tiles that would write all zeros | Reduces upload count by up to 90% on sparse maps |
| **Bulk GPU upload** | `glMapBuffer` + `memcpy` instead of per-tile `glBufferSubData` | One driver call instead of ~1M |
| **Parallel tile aggregation** | `std::thread` workers process tile ranges concurrently | Uses all CPU cores for initial load |
| **Batch DB queries** | 8 bulk `SELECT *` instead of hundreds of per-sprite queries | ~130ms saved in `SpriteFactory::init` |

**Result**: `onAllTileInfo` dropped to **75ms** on the test save (1.3M tiles, 10 cores).

**Files modified**: `aggregatorrenderer.cpp`, `mainwindowrenderer.cpp`, `mainwindowrenderer.h`, `spritefactory.cpp`

---

## Verification

1. **Tick profiler**: Check `[TICK N]` log lines after each change — no subsystem should exceed 30% of tick budget (Law 3)
2. **Regression test**: `./build/Ingnomia --test-mode --load-save content/test_saves/smoke_test --run-ticks 500` — verify metrics match or improve
3. **Stress test**: Create a save with 20+ gnomes, active farms, stockpiles, workshops. Profile under load.
4. **Thread sanitizer**: Build with `-fsanitize=thread` to detect data races after any threading changes
5. **Watch for**: memory leaks (growing RSS), deadlocks (game freezes), data corruption (gnomes in walls, items disappearing)
