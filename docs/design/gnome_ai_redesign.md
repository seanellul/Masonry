# RFC: Gnome AI & Job System Redesign

> **Status**: Draft v2 — revised after cross-game analysis and architecture review
> **Goal**: Scale gnome colonies to 1000 without degrading tick performance
> **Prior art studied**: Dwarf Fortress, RimWorld, Songs of Syx, Factorio, Oxygen Not Included
> **Output**: Implementation blueprint for phased delivery with per-phase benchmarking

---

## 1. Problem Statement

Benchmarks show gnome tick cost scales non-linearly:
- 7 gnomes: 25us/gnome (1ms total)
- 50 gnomes: 26us/gnome (2.3ms total)
- 100 gnomes: **43us/gnome** (8ms total) — non-linear jump
- 200 gnomes: 30us/gnome (7.3ms total) — budget-capped, not all gnomes tick

At 100+ gnomes, the per-gnome cost nearly doubles. The game becomes unresponsive at 200+ because the 5ms GnomeManager budget means many gnomes skip their tick entirely.

**Root causes** (ordered by impact):
1. **Every gnome runs a full BT tick every game tick**, even when walking a known path or sleeping
2. **Idle gnomes scan all jobs** — O(skills x jobTypes x jobs) per idle gnome per tick
3. **Social interactions are O(n^2)** — 19,900 pair checks at 200 gnomes
4. **No differentiation between expensive and cheap work** — a gnome mid-stride costs the same as one making a decision

**Target**: 1000 gnomes at <5ms total gnome tick time (~5us amortized per gnome).

## 2. Current Architecture

### 2.1 The Tick Pipeline (game.cpp:180)
```
collectPaths()          <- gather async pathfinding results from last tick
grass/water/plants      <- parallel (std::async)
creatureManager.onTick  <- sequential, 2-5ms adaptive budget
gnomeManager.onTick     <- sequential, 5ms hard budget
jobManager.onTick       <- process returned/completed jobs
stockpileManager.onTick
farmingManager.onTick
workshopManager.onTick
passive systems         <- parallel (rooms, mechanisms, fluids, neighbors, sound)
events.onTick
dispatchPaths()         <- launch async pathfinding for next tick
```

### 2.2 How a Gnome Gets a Job (Pull Model)
```
Gnome BT ticks -> "am I idle?" -> YES ->
  gnomeactions.cpp:574: jm->getJob(m_skillPriorities, m_id, m_position)
    -> for each skill in priority order:
      -> for each job type matching that skill:
        -> for each job of that type (sorted by priority):
          -> check: isWorked? isCanceled?
          -> check: requiredItemsAvail?
          -> check: requiredToolExists?
          -> check: isReachable (region check)?
          -> if all pass -> CLAIM IT
    -> if nothing found -> m_jobCooldown = 100 ticks (~10 seconds idle)
```

**Cost**: O(skills x jobTypes x jobs) per idle gnome per tick. With 15 skills and hundreds of jobs, this is thousands of checks per gnome.

### 2.3 Pathfinding (Already Parallelized)
- Non-blocking dispatch/collect split across ticks
- Requests with shared goals are folded together
- Runs on async threads via `std::future`
- This is NOT the bottleneck — it's well-optimized

### 2.4 Social Interactions (gnomemanager.cpp:820)
- `processSocialInteractions()`: O(n^2) nested loop over all gnome pairs
- Runs every 600 ticks (~1 in-game hour)
- At 200 gnomes = 19,900 pair checks
- Already has proximity filter (Manhattan distance <= 5) and 5% random chance, but the outer loop is still O(n^2)
- Opinion decay loop iterates ALL stored opinions every 14,400 ticks

### 2.5 Time Budgets
- GnomeManager: 5ms hard cap. Processes gnomes sequentially from `m_startIndex`, wraps around.
- CreatureManager: adaptive 2-5ms based on creature count.
- Neither has per-entity budgets — early gnomes in the list get ticked more often than later ones.

---

## 3. Proposed Design

### 3.1 Bucket-Staggered Tick Updates

**Current**: Every gnome runs a full BT tick every game tick, capped by a 5ms time budget. At 200+ gnomes, many are simply skipped.
**Proposed**: Gnomes are assigned to N tick buckets. Each game tick, one bucket gets full evaluation; all others get a minimal "cheap tick."

This is the single highest-impact optimization. It's the core technique behind Songs of Syx handling 1000+ settlers, and the approach Dave Mark advocates in his GDC "Architecture Tricks" talks.

#### How It Works

```
N_BUCKETS = 10  (tunable — higher = more latency, lower cost per tick)

On gnome creation:
  gnome.bucket = gnome.id % N_BUCKETS

Each game tick:
  activeBucket = GameState::tick % N_BUCKETS

  for each gnome:
    if gnome.bucket == activeBucket:
      gnome.fullTick()     // BT evaluation, need checks, job decisions
    else:
      gnome.cheapTick()    // advance movement, decrement timers only
```

**Full tick** (~10-20us): runs the behavior tree, evaluates needs, checks pending jobs, makes decisions. This is what every gnome does today.

**Cheap tick** (~1-2us): advances position along a cached path, decrements work/cooldown timers, checks interrupt flags. No BT traversal, no need evaluation, no job search.

#### Cost Model

| Gnomes | Full ticks/tick | Cheap ticks/tick | Estimated total |
|--------|----------------|-----------------|----------------|
| 200 | 20 | 180 | 20 x 15us + 180 x 2us = 0.66ms |
| 500 | 50 | 450 | 50 x 15us + 450 x 2us = 1.65ms |
| 1000 | 100 | 900 | 100 x 15us + 900 x 2us = 3.3ms |

Compare to current: 200 gnomes = 7.3ms. This alone is a ~4-10x improvement.

#### Latency Guarantee

With 10 buckets, a gnome's full tick fires every 10 game ticks. At normal speed (~10 ticks/second), that's once per second. This is imperceptible for:
- Job pickup (gnome appears to "think" for up to 1 second — natural)
- Need evaluation (hunger/thirst don't change meaningfully in 10 ticks)
- Social checks (already run every 600 ticks)

**Exception — immediate wake**: Certain events bypass the bucket and force a full tick next game tick:
- High-priority job pushed (priority >= 8)
- Critical need threshold crossed (starvation, dehydration)
- Combat alert / alarm
- Arrived at path destination

This is implemented via a `m_forceFullTick` flag checked before the bucket test.

#### Cheap Tick Implementation

```cpp
void Gnome::cheapTick(quint64 tickNumber)
{
    // 1. Advance movement along cached path
    if (m_state == GnomeState::MOVING && !m_path.empty()) {
        m_pathIndex++;
        Position newPos = m_path[m_pathIndex];
        // Check walkability of next tile (terrain may have changed)
        if (g->w()->isWalkable(newPos)) {
            Position oldPos = m_position;
            m_position = newPos;
            move(oldPos);  // update sprite, spatial hash
        } else {
            m_state = GnomeState::IDLE;  // path blocked, needs full tick
            m_forceFullTick = true;
        }
        if (m_pathIndex >= m_path.size()) {
            m_state = GnomeState::ARRIVED;
            m_forceFullTick = true;  // needs decision
        }
    }

    // 2. Decrement work timer
    if (m_state == GnomeState::WORKING) {
        m_workTimer--;
        if (m_workTimer <= 0) {
            m_forceFullTick = true;  // work complete, needs decision
        }
    }

    // 3. Check interrupt flag
    if (m_interruptFlag) {
        m_forceFullTick = true;
    }
}
```

### 3.2 Sleep/Wake for Stable States

**Concept**: Gnomes in fully stable states are removed from the tick list entirely. Inspired by Factorio's entity sleep system, where idle inserters/assemblers cost zero CPU until woken by an inventory change.

#### Sleep-Eligible States

| State | Sleep? | Wake trigger |
|-------|--------|-------------|
| Sleeping in bed | Yes | Sleep timer expires, alarm, need |
| Eating at table | Yes | Eat timer expires, alarm |
| Idle, no pending jobs | Yes | Job pushed to pending queue |
| Waiting for path result | Yes | Path result arrives (checked in collectPaths) |
| Dead/incapacitated | Yes | Never (removed from list eventually) |

#### Implementation

```
m_sleepingGnomes: QVector<Gnome*>   // not ticked at all
m_activeGnomes: QVector<Gnome*>     // ticked via bucket system

wake(gnome):
  remove from m_sleepingGnomes
  add to m_activeGnomes
  gnome.m_forceFullTick = true

sleep(gnome):
  remove from m_activeGnomes
  add to m_sleepingGnomes
```

Wake triggers are event-driven — they fire from the system that caused the change:
- `JobManager::pushJobToGnome()` calls `wake(gnome)` if gnome is sleeping
- `PathFinder::collectPaths()` calls `wake(gnome)` when a path result arrives for a sleeping gnome
- Need timers are checked once every 100 ticks for sleeping gnomes (cheap batch scan) as a safety net

#### Expected Impact

At any given time with 500 gnomes:
- ~60-150 sleeping (nighttime: up to 300)
- ~30 eating/drinking
- ~20-50 idle with no jobs
- = **100-500 gnomes costing zero** per tick

Combined with bucket staggering on the remaining active gnomes, the effective per-tick cost is very low.

### 3.3 Push Model: Jobs Find Gnomes (with Spatial Filtering)

**Current**: Every idle gnome scans every job every tick.
**Proposed**: When a job is created, it's pushed to the nearest eligible gnomes.

#### Skill-to-Gnome Registry

```
m_gnomesBySkill: QMultiHash<QString, unsigned int>
  "Mining"  -> [gnome_42, gnome_17, gnome_88]
  "Masonry" -> [gnome_42, gnome_55]
  ...
```

Updated when gnomes gain/lose/enable/disable skills. Automatons are included — they participate in the same registry with their subset of skills.

#### Spatially-Filtered Push

When `addJob()` is called:

```
1. Get job's required skill
2. Get job's position -> spatial cell (using the shared spatial grid, see 3.5)
3. Filter m_gnomesBySkill[skill] to gnomes in same/adjacent cells
4. If < 3 nearby eligible gnomes: expand to 2-ring, then 3-ring
5. Push to at most K=5 nearest eligible gnomes (sorted by distance)
6. If zero eligible nearby: add to global overflow pool
```

**Cost**: O(K) per job creation — constant, regardless of total gnome count.

This solves the "pending queue grows huge" problem from v1 of this RFC. With only 5 gnomes receiving each job, queues stay small naturally without needing an arbitrary cap.

#### When a Gnome is Idle

1. Check `m_pendingJobs` — O(1) pop from front (priority-sorted)
2. Validate: is the job still available? Reachable? Items available?
3. If valid -> claim and start working
4. If invalid -> pop next, repeat (max 3 attempts per tick to bound cost)
5. If queue empty -> enter fallback state (see 3.6 Spatial Fallback Search)

#### Edge Cases

**No gnome has the required skill:**
- Job stays in the global pool, unclaimed
- Job board UI shows it as "No eligible workers"
- If a gnome later enables that skill, the registry updates and pending overflow jobs for that skill get pushed
- Periodic sweep (every 120 ticks): check overflow pool for jobs with zero eligible gnomes -> toast notification to player

**Job canceled while in pending queue:**
- Job has a `canceled` flag. When the gnome pops it, checks and skips. Lazy invalidation — no need to actively remove from queues.

**Multiple gnomes receive the same job:**
- First gnome to pop and claim it wins. Others see `isWorked=true` and skip.
- The spatial push means usually only 3-5 nearby gnomes have it — minimal wasted validation.

**Save/load:**
- Don't serialize pending queues or the skill->gnome index. Reconstruct both on load: iterate all unclaimed jobs and all gnomes, rebuild registry, run one distribute pass. Takes <1ms even at 1000 gnomes / 5000 jobs.

**Job cooldown:**
- With push model + sleep/wake: no cooldown needed. Idle gnome with empty queue -> sleeps -> wakes when a job is pushed. The 100-tick cooldown was a band-aid for expensive scanning. Remove it.

### 3.4 Gnome State Machine

**Current**: Every gnome runs the full BT tick every frame, regardless of what they're doing.
**Proposed**: Gnomes have explicit states that determine tick cost and bucket behavior.

```
                  pending job       validated         path arrived
  +---------+  ------------->  +-----------+  ---->  +----------+  ---->  +---------+
  |  IDLE   |                  | THINKING  |         | WAIT_PATH|         | MOVING  |
  +---------+  <-------------  +-----------+  <----  +----------+         +---------+
       ^        no jobs             |          no path                        |
       |                            |                                         |
       |                            v                                    arrived
       |                      +-----------+                                   |
       |  job done / aborted  | SLEEPING  |  (bed/eating/idle-no-jobs)        v
       | <------------------  +-----------+                              +---------+
       |                                                                 | WORKING |
       | <------------------------------------------------------------   +---------+
                                   job done / aborted
```

| State | Tick type | Cost | Description |
|-------|-----------|------|-------------|
| **IDLE** | Full only | ~2us | Pop pending queue, transition to THINKING or sleep |
| **THINKING** | Full only | ~5-20us | Validate job, request path. Max 3 validations per tick. |
| **WAIT_PATH** | Sleep | ~0us | Waiting for pathfinder. Woken by collectPaths(). |
| **MOVING** | Cheap | ~1-2us | Advance along cached path. No BT. |
| **WORKING** | Cheap | ~2-3us | Decrement work timer. No BT. |
| **SLEEPING** | Sleep | ~0us | In bed / eating / idle with no jobs. Event-woken. |

**Key insight**: MOVING and WORKING are the most common states, and they only need cheap ticks. The BT is only needed at decision points (IDLE, THINKING), which happen infrequently per gnome.

#### Interruption Protocol

When a priority >= 8 job arrives in a gnome's pending queue:
1. Set `m_interruptFlag = true` on the gnome
2. If sleeping: wake immediately
3. At next cheap tick (MOVING/WORKING): gnome sees flag, sets `m_forceFullTick = true`
4. At next full tick: abort current job (mark as unworked, return to pool), transition to THINKING for the new job
5. The aborted job goes back to the overflow pool and gets re-pushed

**Carrying items when interrupted:**
- Drop items at current position (or return to stockpile if within 5 tiles)
- The interrupted job's item requirements are released (`inJob = 0`)

**Needs (hunger, thirst, sleep):**
- `evalNeeds()` runs during full ticks only (every 10 game ticks via bucket). Needs don't change meaningfully in 10 ticks.
- If a critical need triggers, it overrides the current state -> gnome seeks food/drink/bed
- For sleeping gnomes: needs are checked in the batch safety-net scan (every 100 ticks)

### 3.5 Shared Spatial Grid

Multiple systems need spatial lookups. Rather than maintaining separate grids, use one shared spatial hash:

```
Cell size: 16x16 tiles (tunable — smaller = more precise, more overhead)
Grid: QHash<uint32_t, QVector<unsigned int>>   // cellID -> entity IDs

cellID = (x / 16) + (y / 16) * (dimX / 16) + (z) * (dimX / 16) * (dimY / 16)
```

Updated in `Gnome::move()` — when a gnome changes position, update its cell membership. This is O(1) per move.

**Consumers:**
- Social interactions (3.7): only check pairs in same/adjacent cells
- Job push (3.3): find nearest eligible gnomes
- Spatial fallback search (3.6): find nearby unclaimed jobs
- Future: combat targeting, room occupancy checks

Jobs also register in the grid when created and deregister when claimed/canceled.

### 3.6 Spatial Fallback Search

When a gnome's pending queue is empty and it needs to find work:

1. Check global "urgent" list first (priority >= 8 jobs) — always checked regardless of distance
2. Search the spatial grid outward from gnome's cell: own cell -> adjacent -> 2-ring -> 3-ring
3. For each cell, check unclaimed jobs matching gnome's active skills
4. Stop when a valid job is found or search radius exceeds 5 rings (~80 tiles)
5. If nothing found: gnome enters IDLE/wander, will sleep after 30 ticks of idleness

**Far-away low-priority jobs**: They wait until a gnome finishes nearby work and the search expands, or the player raises priority. This is good gameplay — it encourages building stockpiles and workshops near work sites.

### 3.7 Social Interactions: Spatial + Bounded

**Current**: O(n^2) all-pairs check with Manhattan distance filter.
**Proposed**: Two-tier system using the shared spatial grid.

#### Tier 1: Ambient Social (O(n), no tracking)

Environmental morale bonuses that don't track individual relationships:
- "Ate in dining room with others" -> +mood (check: other gnomes in same room)
- "Slept in shared barracks" -> small +mood (or -mood if overcrowded)
- "Worked near a friend" -> +mood (uses Tier 2 data if a relationship exists)

These are checked during full ticks (bucketed), cost ~1us per gnome, and provide the *feeling* of social simulation without per-pair computation.

#### Tier 2: Personal Relationships (O(n x R), bounded)

Individual opinion tracking, but bounded:
- Use the shared spatial grid: only check pairs in same/adjacent cells
- Max **R=15 stored relationships per gnome** — when full, weakest opinion (closest to 0) is evicted
- Social memory capped at 5 entries per relationship (oldest evicted)
- Run every 600 ticks (unchanged), 5% chance per eligible pair (unchanged)

**Complexity**: With the spatial grid, the inner loop only checks gnomes in nearby cells. At 500 gnomes on a 200x200 map with 16x16 cells: ~156 cells, ~3.2 gnomes per cell average. Each gnome checks ~9 cells (own + 8 adjacent) = ~29 gnomes. Total work: 500 x 29 / 2 = ~7,250 pair checks (vs. 124,750 for O(n^2)).

With the 5% random chance: ~362 actual interaction computations per sweep. Very cheap.

**Opinion decay**: Only iterates R=15 relationships per gnome = O(n x 15) = O(n). At 1000 gnomes: 15,000 operations vs. the current unbounded approach.

### 3.8 Role System & Skill Caps

**Current**: Gnomes can have all ~15 skills active simultaneously.
**Proposed**: Hard cap of 4 active work skills + 1 "Flex" slot per gnome, organized by roles.

#### Why Hard Cap (not soft)

A soft cap with diminishing returns was considered but rejected:
- It doesn't solve the performance problem — a gnome with 15 skills at -50% speed still appears in 15 skill buckets in the push registry
- Hidden penalties confuse players ("why is my gnome slow?")
- RimWorld's hard work-type toggles are well-understood by colony sim players

A hard cap provides a **guaranteed performance bound**: max 5 skill lookups in the push registry per gnome, max 5 skill types in the fallback search. This is a 3x improvement over 15 skills, and it's not probabilistic.

#### How It Works

**4 Active Slots**: The gnome's primary work skills. Fully eligible for pushed jobs.

**1 Flex Slot**: Can be any skill. The gnome only checks Flex-slot jobs when:
- Pending queue is empty AND
- Idle for > 30 ticks (not in the hot path)

This provides an escape valve for players who want some generalist capability without degrading the fast path.

**Default Role Templates** (customizable starting points):
- **Laborer**: Hauling, Mining, Woodcutting, Stonecutting
- **Crafter**: Masonry, Carpentry, Cooking, Brewing
- **Farmer**: Farming, Animal Husbandry, Herbalism, Foraging
- **Scholar**: Medicine, Machining, Engineering, Enchanting
- **Soldier**: (combat skills are separate and don't count against the 4+1 cap)

Players can reassign freely within the cap. Changing a skill triggers a registry update and overflow pool rescan.

**No gnome has a needed skill:**
- Toast notification: "No gnome can do [Mining] -- assign one!"
- Job board highlights skill-orphaned jobs
- Player must actively assign — no auto-assignment (preserves player agency)

---

## 4. Resolved Questions

These were open in v1 of this RFC. Resolved through architecture review and cross-game analysis.

**Thread safety for push model:**
Not an issue. `addJob()` runs during `jobManager.onTick()` / `workshopManager.onTick()`. Gnome ticks run during `gnomeManager.onTick()`. These are sequential phases in `Game::loop()`. No synchronization needed in the current architecture. If gnome ticks are ever parallelized (not proposed here), the push queue would need to be lock-free — but that's a future concern.

**Optimal pending queue size:**
With spatial filtering (K=5 nearest gnomes per job), queues stay naturally small. No hard cap needed — monitor in benchmarks, add a soft cap of 10 if queues grow unexpectedly.

**BT compatibility:**
The state machine wraps the BT, not replaces it. When a gnome is MOVING/WORKING, the BT is suspended (RUNNING on an action node). When interrupted or arriving, call `haltAllChildren()` on the BT root (already implemented in `bt_nodesequence.cpp`). BT resumes from the top on the next full tick. The XML behavior trees don't change.

**Automaton interaction:**
Automatons participate in the same push registry with their subset of skills (typically hauling + one craft). Same bucket/stagger system. No special handling needed.

**Priority gaming:**
Don't limit priority slots. If >50% of active jobs are priority 8+, show a UI hint: "Many urgent jobs -- gnomes can't prioritize effectively." The spatial push handles this fine because each gnome only sees nearby high-priority jobs, not all of them.

---

## 5. Survival Checks in Cheap Tick and Sleep

The current `Gnome::onTick` runs several survival-critical checks every tick:
- `checkFloor()` — tile destroyed beneath gnome (returns NOFLOOR)
- `m_anatomy.setFluidLevelonTile()` — drowning in rising water
- Anatomy death status — bleeding out, poison, etc.

These **must** run even during cheap ticks — a gnome walking over a floor that gets mined out must fall immediately, not 10 ticks later. They're individually cheap (<100ns each), so include them in `cheapTick()`:

```cpp
void Gnome::cheapTick(quint64 tickNumber)
{
    // Survival-critical checks (always run, ~200ns total)
    if (checkFloor()) { m_result = CreatureTickResult::NOFLOOR; return; }
    m_anatomy.setFluidLevelonTile(g->w()->fluidLevel(m_position));
    if (m_anatomy.statusChanged()) {
        if (m_anatomy.status() & AS_DEAD) { die(); return; }
    }

    // ... movement, timers, interrupts as before
}
```

For **sleeping gnomes**, these checks run in the safety-net batch scan (every 100 ticks). This is acceptable because:
- A gnome sleeping in a bed is in a stable location — floors don't normally get mined under beds
- If fluid rises to bed level, 100 ticks (~10 seconds) is fast enough to wake and respond
- If a more aggressive guarantee is needed, sleeping gnomes can register their position and wake on tile-change events (see Phase D implementation)

## 6. Multi-Step Jobs and Hauling

Most jobs are not a single move-then-work. The common pattern is:

```
Haul job:     move to item → pick up → move to destination → drop off
Craft job:    move to item → pick up → move to workshop → work (timed) → produce
Build job:    move to stockpile → pick up material → move to build site → work (timed) → place
```

Hauling alone is typically 40-60% of all active jobs. The state machine must handle multi-step sequences efficiently.

### Solution: Job Step Queue

Each job, when claimed, produces a **step queue** — a flat list of atomic actions:

```
Step { type: MOVE_TO, target: Position }
Step { type: PICK_UP, itemID: unsigned int }
Step { type: MOVE_TO, target: Position }
Step { type: WORK,    duration: int ticks }
Step { type: DROP,    itemID: unsigned int }
```

The state machine processes steps sequentially:
- MOVE_TO steps → gnome enters MOVING state (cheap tick)
- PICK_UP / DROP steps → instant (one full tick, ~2us)
- WORK steps → gnome enters WORKING state (cheap tick with timer)

When a step completes, `m_forceFullTick = true` to process the next step. But if the next step is another MOVE_TO, the full tick just loads the new path and immediately returns to MOVING (cheap tick). The gnome only stays in "full tick" mode for decision-heavy transitions.

**This is analogous to RimWorld's "Toil" system** — jobs are decomposed into atomic toils, and the pawn advances through them without re-evaluating the full think tree between toils.

### Compatibility with BT

The step queue is generated by the BT action node when it starts. Existing BT action nodes like `GetJob`, `MoveToJob`, `Work` already encode this logic — the step queue just makes it explicit and extractable so the state machine can drive it during cheap ticks.

During Phase F implementation, each BT action that involves movement or timed work will need a `generateSteps()` method that produces the step queue. The BT still validates and initiates the job; the state machine just executes the steps cheaply.

## 7. Combat Response During Cheap Tick

The BT evaluates `AlarmTree` and `Combat` subtrees *above* the job subtree. A gnome under attack must respond immediately — waiting up to 10 game ticks for a bucket full tick is potentially lethal.

### Solution: Combat Alert Flag

Separate from the job interrupt flag, a `m_combatAlert` flag triggers immediate wake:

```cpp
// In cheapTick():
if (m_combatAlert) {
    m_forceFullTick = true;
    m_state = GnomeState::IDLE;  // force full BT re-evaluation from root
    return;
}
```

**What sets the combat alert:**
- `CreatureManager` or combat system detects a hostile within N tiles of a gnome → sets `m_combatAlert = true`
- Alarm bell activation → sets `m_combatAlert` on all gnomes (already exists as alarm system)
- Gnome takes damage → sets own `m_combatAlert`

For sleeping gnomes: combat alert calls `wake(gnome)` immediately, same as a high-priority job push.

The full tick then runs the BT from the root, which will hit `AlarmTree` / `Combat` before `Jobs`, producing the correct combat behavior.

## 8. Container Strategy for Active/Sleeping Split

`QVector::remove()` from middle is O(n). With frequent wake/sleep transitions, this becomes a hot path.

### Solution: Swap-and-Pop

```cpp
void GnomeManager::sleepGnome(int index)
{
    Gnome* gnome = m_activeGnomes[index];
    // Swap with last element, then pop — O(1)
    m_activeGnomes[index] = m_activeGnomes.last();
    m_activeGnomes.removeLast();
    m_sleepingGnomes.append(gnome);
    gnome->m_isSleeping = true;
}

void GnomeManager::wakeGnome(Gnome* gnome)
{
    // Sleeping list doesn't need ordered removal — swap-and-pop
    int idx = m_sleepingGnomes.indexOf(gnome);  // O(n) but sleeping list is small
    if (idx >= 0) {
        m_sleepingGnomes[idx] = m_sleepingGnomes.last();
        m_sleepingGnomes.removeLast();
    }
    m_activeGnomes.append(gnome);
    gnome->m_isSleeping = false;
    gnome->m_forceFullTick = true;
}
```

Bucket assignment is by gnome ID, not by position in the vector, so reordering the active list doesn't affect bucket membership.

**Alternative for sleeping gnomes**: maintain a `QSet<unsigned int>` of sleeping gnome IDs for O(1) `indexOf` equivalent. Wake becomes O(1) lookup + O(1) remove. Trade-off: slightly higher memory overhead, but irrelevant at 1000 gnomes.

## 9. Work Timer Extraction from BT

The state machine proposes a `m_workTimer` decremented during cheap ticks. Currently, work duration is controlled inside individual BT action nodes, each with their own timer logic.

### Solution: BT Actions Set a Shared Timer

When a BT action node enters its "working" phase (mining a block, crafting an item, building a wall), it writes to a shared field on the gnome:

```cpp
// In BT action node (e.g., MineAction::tick()):
if (startingWork) {
    gnome->m_workTimer = calculateMiningDuration(skill, material);
    gnome->m_state = GnomeState::WORKING;
    return BT_RESULT::RUNNING;
}
```

The cheap tick decrements `m_workTimer`. When it hits zero, `m_forceFullTick = true`. The next full tick resumes the BT, which sees the timer expired and completes the action.

**Compatibility**: This requires modifying each BT action that has a timed work phase to write `m_workTimer` instead of managing its own internal timer. There are approximately 8-10 such actions (mine, build, craft, farm, butcher, etc.). This is the main integration work for Phase F.

**Actions without timers** (instant actions like pick-up, drop-off) don't use this mechanism — they complete in a single full tick.

## 10. Remaining Open Questions

1. **Bucket count tuning**: N=10 is the starting point. Fewer buckets = lower latency but higher per-tick cost. Need to benchmark N=5, 10, 15, 20 with 500+ gnomes to find the sweet spot. May want different N for different game speeds.

2. **Spatial grid cell size**: 16x16 is a reasonable default. Smaller cells = more precise spatial queries but more overhead in cell updates. Benchmark with 8x8, 16x16, 32x32.

3. **Sleep safety net frequency**: Sleeping gnomes get a batch needs-check every 100 ticks. Is this frequent enough? Need to verify against hunger/thirst drain rates. Current hunger decrement is 0.075 per in-game minute — 100 ticks at normal speed is ~10 seconds, so ~0.0125 hunger drain while sleeping. This is negligible, so 100 ticks is likely fine.

4. **Flex slot interaction with push**: Should Flex-slot jobs be pushed to the gnome at all, or only discovered via fallback search? Pushing would be simpler but adds noise to the pending queue. Recommendation: fallback-only, keeps the fast path clean.

5. **Migration from current BT**: The state machine needs to intercept BT control flow. Specifically, the BT's `GetJob` action node and movement nodes need to be aware of the new states. This is the highest-risk integration point — needs a detailed sub-design before implementation. See section 6 (multi-step jobs) and section 9 (work timer extraction) for the integration strategy.

---

## 11. Benchmark Targets

| Metric | Current (200 gnomes) | Target (500) | Target (1000) |
|--------|---------------------|-------------|---------------|
| Total gnome tick time | 7.3ms | <2ms | <5ms |
| us per gnome (full tick) | 30-43 | <15 | <15 |
| us per gnome (cheap tick) | N/A | <2 | <2 |
| Amortized us per gnome | 30-43 | ~4 | ~5 |
| Job search (idle gnome) | ~100us (full scan) | <2us (queue pop) | <2us |
| Social tick | O(n^2) = 19,900 pairs | O(n) ~7,250 pairs | O(n) ~14,500 pairs |
| Max gnomes before degradation | ~100 | 500+ | 1000+ |
| Gnomes sleeping (typical) | 0 | 100-200 | 200-400 |
| Full ticks per game tick | 200 (all) | ~50 | ~100 |

---

## 12. Benchmark Scenarios

Each phase must be benchmarked against **all four** standard scenarios to avoid optimizing for one case while regressing another.

**Scenario 1 — Active Colony (daytime, full workload):**
All gnomes awake, most have active jobs. Mix of mining, hauling, crafting, building. ~10% idle at any time (normal churn). Tests: overall tick time, job acquisition latency, cheap tick cost.

**Scenario 2 — Night Cycle (sleep-heavy):**
60-80% of gnomes sleeping, guards patrolling, a few crafters working. Tests: sleep/wake overhead, sleeping gnome count, safety-net scan cost.

**Scenario 3 — Mass Idle (early game / no jobs):**
Many gnomes with no pending jobs. Tests: fallback search cost, idle-to-sleep transition rate, wake responsiveness when jobs appear.

**Scenario 4 — Combat Stress:**
Alarm triggered, all gnomes interrupted. Mass wake from sleep, mass BT re-evaluation, pathfinding spike. Tests: wake latency, forceFullTick storm handling, peak tick time.

For each scenario, measure at 100, 200, 500, and 1000 gnomes using `--test-mode --run-ticks N` with the `num_gnomes` parameter.

## 13. Regression Detection

Optimizations must not change simulation correctness. After each phase, run a correctness comparison:

1. Run old and new systems with identical seeds at **50 gnomes** (where old system has no budget-skipping)
2. After 1000 ticks, compare:
   - Gnomes alive count (must match)
   - Total jobs completed (within 5% — bucket latency causes minor ordering differences)
   - Gnomes with critical needs unmet (must be zero in both)

3. **Behavioral smoke tests** (automated via test mode):
   - "No gnome starves with food available" — run 2000 ticks with food stockpile, assert zero starvation deaths
   - "All mining jobs complete" — designate 20 tiles, run until all mined or 5000 ticks, assert completion
   - "Combat response" — spawn hostile, assert gnomes engage within 20 ticks (2 bucket cycles)
   - "Sleep/wake cycle" — advance through night, assert gnomes sleep; create job, assert wake within 5 ticks

These run as part of the existing `--test-mode` infrastructure.

## 14. Implementation Phases

Phases are ordered by impact-to-effort ratio. Independent phases can be developed in parallel.

### Phase A: Bucket-Staggered Ticks
**Depends on**: Nothing
**Impact**: Immediate 4-10x throughput improvement
**Risk**: Low — additive change, existing behavior preserved for full-tick gnomes

- Add `m_bucket` field to Gnome
- Split `Gnome::onTick` into `fullTick()` and `cheapTick()`
- Modify `GnomeManager::onTick` to dispatch based on bucket
- Add `m_forceFullTick` flag and wake triggers
- Implement cheap tick: path advancement, work timer decrement, interrupt check
- Benchmark: compare 200-gnome tick time before/after

### Phase B: Push Model with Spatial Filtering
**Depends on**: Phase E (shared spatial grid)
**Impact**: Eliminates job search cost entirely
**Risk**: Medium — new data structure, changes job assignment flow

- Add `m_gnomesBySkill` registry to JobManager
- Modify `addJob()` to push to K=5 nearest eligible gnomes
- Add `m_pendingJobs` queue to Gnome
- Add overflow pool and periodic sweep (every 120 ticks)
- Modify BT `GetJob` action to pop from pending queue instead of scanning
- Remove 100-tick job cooldown
- Benchmark: measure idle-gnome job acquisition time

### Phase C: Social Spatial Hash + Bounded Relationships
**Depends on**: Phase E (shared spatial grid)
**Impact**: Fixes O(n^2) social, bounds storage growth
**Risk**: Low — localized change in gnomemanager.cpp

- Replace nested loop in `processSocialInteractions()` with spatial grid lookup
- Add ambient social tier (room-based morale bonuses)
- Cap stored relationships at R=15 per gnome (evict weakest)
- Cap social memories at 5 per relationship (evict oldest)
- Bound opinion decay loop to O(n x R)
- Benchmark: social tick time at 200, 500, 1000 gnomes

### Phase D: Sleep/Wake System
**Depends on**: Phase A (bucket infrastructure)
**Impact**: Removes 30-50% of gnomes from tick entirely
**Risk**: Medium — must ensure all wake triggers are correctly wired

- Split gnome lists: `m_activeGnomes` / `m_sleepingGnomes`
- Implement sleep conditions (bed, eating, idle-no-jobs, wait-path)
- Wire wake triggers in JobManager, PathFinder, need system, alarm system
- Add safety-net batch scan for sleeping gnomes (every 100 ticks)
- Benchmark: measure active gnome count across different game states

### Phase E: Shared Spatial Grid
**Depends on**: Nothing
**Impact**: Enables Phases B, C, F
**Risk**: Low — new utility, no existing code modified

- Implement spatial hash: `QHash<uint32_t, QVector<unsigned int>>`
- Cell size 16x16 tiles, 3D-aware (includes z-level in cell ID)
- Update hook in gnome movement code
- Job registration/deregistration in grid
- Unit test: verify cell membership correctness after gnome movement

### Phase F: Gnome State Machine
**Depends on**: Phase A (cheap/full tick split), Phase B (pending queue)
**Impact**: Formalizes state transitions, enables MOVING/WORKING optimization
**Risk**: Medium-high — integrates with BT, most architectural change

- Add `GnomeState` enum and `m_state` field
- Implement state transition logic
- Integrate with BT: IDLE/THINKING run BT, MOVING/WORKING bypass it
- Wire `haltAllChildren()` for interruptions
- Implement interruption protocol (priority >= 8 jobs)
- Benchmark: per-state tick costs match targets

### Phase G: Role System & Skill Caps
**Depends on**: Phase B (push registry benefits from bounded skills)
**Impact**: Bounds registry size, improves gameplay
**Risk**: Low — mostly UI work, skill system already exists

- Add 4+1 Flex skill cap to gnome data
- Create default role templates
- Build role assignment UI
- Add "No eligible workers" toast notification system
- Wire skill changes to registry updates and overflow rescan
- Benchmark: fallback search cost with 5 skills vs 15

### Phase H: Spatial Fallback Search
**Depends on**: Phase B (push as primary), Phase E (spatial grid)
**Impact**: Optimizes the edge case when pending queue is empty
**Risk**: Low — fallback path, not critical

- Implement expanding-ring search using spatial grid
- Urgent job list (priority >= 8) checked first
- Max search radius of 5 rings (~80 tiles)
- Idle gnome sleeps after 30 ticks of failed search
- Benchmark: fallback search cost vs current full scan

---

## 15. Parallel Development Plan

```
Week 1-2:  Phase E (spatial grid) + Phase A (bucket stagger)  [parallel]
Week 3-4:  Phase B (push model) + Phase C (social)            [parallel, both need E]
Week 5-6:  Phase D (sleep/wake) + Phase F (state machine)     [parallel, both need A]
Week 7:    Phase G (roles/skills) + Phase H (fallback search)  [parallel]
Week 8:    Integration testing, benchmark suite, tuning
```

Each phase should be benchmarked before and after using the existing test infrastructure (`--test-mode --run-ticks N`). The `num_gnomes` parameter in `new_game` allows testing at different scales.

---

## 16. Industry References

| Game | Technique we're adopting | Their scale |
|------|------------------------|-------------|
| **Songs of Syx** | Bucket staggering, spatial job assignment, aggregate social | 1000+ settlers |
| **Factorio** | Sleep/wake for idle entities, rail movement | Millions of entities |
| **RimWorld** | Region-based reachability, work priority UI, toil system | ~30 colonists (our BT is similar to their toil chain) |
| **Dwarf Fortress** | Connectivity pre-checks (we already have this via regions) | ~200 dwarves (limited by single-thread) |
| **Dave Mark (GDC)** | Bucket updates, spatial LOD, temporal LOD | Theoretical framework |
