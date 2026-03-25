# Ingnomia Development Roadmap

Built from community feedback (4,693 suggestions), developer discussions (2,567 dev messages), bug reports (6,066 messages, 407 bug threads), version history (60 releases, v0.2.3→v0.8.10), and codebase analysis.

**Total Discord data analyzed: 15,893 messages across 4 channels.**

## Guiding Principles

1. **Fix foundations before adding features** — performance and stability unlock everything else
2. **Each milestone should produce a playable, noticeably improved game**
3. **Leverage what's already built** — needs system, skills, mechanisms, pathfinding parallelism all exist in some form
4. **Community's top 10 most-requested features drive the order**

---

## Milestone 0: Stabilize & Modernize (Foundation)

*Goal: A clean, buildable, performant base to develop on.*

### 0.1 — Build System & Platform
- [ ] Verify clean CMake build on macOS (OpenGL 4.1 shaders already ported — commit `389d8fa`)
- [ ] Verify Linux build still works
- [ ] Document build steps for all 3 platforms in a single BUILD.md

### 0.2 — Critical Bug Fixes
From 6,066 bug report messages (407 bug threads, 310 potentially still open). These are the **recurring crash/stability patterns** that must be fixed first:

| Bug Pattern | Reports | Date Range | Details |
|-------------|---------|------------|---------|
| **Floors disappearing / construction holes** | 7 | 2018–2021 | Built floors spontaneously get holes. Tower floors "suddenly had holes" while running. Persists across save/load |
| **Gnomes get stuck/trapped** | 7 | 2018–2021 | Gnomes stand idle forever. "Standing with face in a corner for two days." Restart sometimes fixes. Related to floor removal trapping |
| **UI crash on scroll/click in workshops** | 4 | 2018–2020 | Crash when scrolling workshop job list while crafting. Crash when rapidly clicking craft items. Scrolling stockpile windows |
| **Save corruption / overwrite** | 3 | 2018–2020 | Same kingdom name overwrites saves. OneDrive sync issues. Save files broken after certain operations |
| **Job cancellation not working** | 3 | 2019–2020 | Cancel job has no effect on some building jobs. Jobs remain in queue after cancellation |
| **Negative thirst → crash** | 2 | 2019 | Gnome thirst goes negative. Opening gnome window when thirst is 0 crashes the game. Thirst displays 0/100 |
| **Rotated workshop bugs** | Recurring | 2018–2021 | Rotated workshops won't craft. Placement issues with rotated buildings |
| **Game speed → lag/freeze** | Recurring | 2019–2020 | High game speed causes periodic slowdowns. Related to gnomes stuck on tasks causing cascading pathfinding |

**Bug report stats**: 68% had dev response, 24% confirmed fixed, 310 potentially still open.
Full details: `docs/bug-reports/bug_report_summary.md`

### 0.3 — Performance: Known Bottlenecks
These were identified by **ext3h** in the dev channel and are low-hanging fruit:

| Issue | Root Cause | Fix |
|-------|-----------|-----|
| Enemy spawn lag spikes | All gnomes pathfind simultaneously | ext3h had this "almost solved" — stagger/batch path requests |
| Farm/grove job queries | O(gnomes × tiles) complexity | Index available farm jobs, reduce to O(gnomes + tiles) |
| Stockpile item selection | Scanning 100k+ items for one slot | Spatial index or priority queue for unstored items |
| Item/creature proliferation | 50k eggs, 2k chickens | Enforce pasture caps (already in code), add item decay/composting |
| Game speed freeze | Stuck gnomes cause cascading pathfinding | Fix stuck gnome detection first (0.2), then optimize path batching |

### 0.4 — Performance: Game Loop Parallelization
*(See `docs/updates/parallelization_plan.md` for full technical plan)*

- [ ] **Phase 1**: Non-blocking pathfinding — dispatch at end of tick, collect at start of next
- [ ] **Phase 2**: Parallel natural world — grass/plants/water on separate threads
- [ ] **Phase 3**: Parallel passive systems — mechanisms/fluids/sound/rooms alongside gnome pipeline

**Expected impact**: 20-40% tick time reduction, enabling faster game speeds and larger populations.

### 0.5 — Dead Gnome Cleanup
ext3h flagged: "all references to the dead Gnome still left in the world need to be cleaned up. Ownerships, carried items, jobs, etc." Workshop assignments were specifically broken on death.

- [ ] Audit all systems that hold gnome references (Jobs, Workshops, Inventory, Military)
- [ ] Implement proper cleanup cascade on gnome death

---

## Milestone 1: Core QoL — "Make What Exists Feel Good"

*Goal: Address the most-requested quality-of-life features. No new game systems — polish existing ones.*

### 1.1 — Event & Notification Log (#11, #19)
The most universally wanted feature. "Find cow dead, want to know why."

- [ ] Create `EventLog` system — structured log of game events (deaths, combat, traps, migration)
- [ ] "Gnome is trapped" detection using existing region IDs (`RegionMap::checkConnectedRegions`)
- [ ] HUD notification popup for critical events (death, trapped, combat)
- [ ] Scrollable event log panel in UI

### 1.2 — Stockpile UX Overhaul (#5, #57, #58, #59)
Biggest source of frustration across all suggestions.

- [ ] Auto-accept new item types when stockpile filter is set to "all" of a category
- [ ] "For trade" stockpile designation — mark stockpiles, filter in trade window
- [ ] Stockpile limit: count items across materials ("max 200 statuettes" regardless of wood type)
- [ ] Copy/paste stockpile settings between stockpiles (clarify UX)
- [ ] Drag-and-drop stockpile priority ordering

### 1.3 — Workshop Production Limits (#48)
`craft-to` mode was added in v0.4.0 ("jobs suspend when number reached") and workshop assignment was added in v0.6.1. But the community still wanted "craft until N **in stockpile**" which is different from "craft N total."

- [ ] Add "craft until N in stockpile" mode — check Inventory count before starting repeat, not just count crafted
- [x] ~~Assign gnomes to workshops~~ — done in v0.6.1

### 1.4 — HUD & UI Improvements (#1, #2, #12, #14)
- [ ] Stock counters on main HUD (food/drink/materials count) — like Gnomoria
- [ ] Battle recap screen after combat (8⭐ — injuries, deaths, equipment summary)
- [ ] Gnome job counter next to name in population list
- [ ] Z-level keyboard shortcut (not just scroll wheel)

---

## Milestone 2: Gnome Depth — "Make Gnomes Feel Alive"

*Goal: Complete the gnome behavior systems that are partially implemented.*

### 2.1 — Happiness/Mood System (#17, #89, #91)
The needs map exists (`creature.h:m_needs`) with Hunger/Thirst/Sleep. Automatons explicitly remove "Happiness" — so the slot exists but is unused.

- [ ] Implement happiness calculation from: room quality, food quality, relationships, recent events
- [ ] Happiness affects work speed, migration rate, kingdom worth
- [ ] "Dead friends" mood penalty (#91 — 3⭐)
- [ ] Gnome needs panel in UI showing all current needs/mood

### 2.2 — Designated Movement Zones (#30)
"No pass designation" was added in v0.4.7 with a checkbox to let gnomes ignore it (v0.4.8). But the community wanted a fuller zone system.

- [x] ~~Basic no-pass zone~~ — done in v0.4.7
- [ ] Zone types: civilian allowed, military only, forbidden (expand beyond simple no-pass)
- [ ] Per-gnome zone permissions (beyond the single "ignore no pass" checkbox)
- [ ] Emergency override when gnome is trapped

### 2.3 — Medical System Improvements (#31, #37)
Anatomy/wound system exists (`anatomy.cpp`). Hospital/bandage code exists.

- [ ] Medics bring bandages TO injured gnomes (not other way around)
- [ ] Triage priority (most injured first)
- [ ] Basic disease system using existing anatomy framework

### 2.4 — Food & Farming QoL (#28, #39, #41, #46)
- [ ] Food exclusion list — reserve items from gnome consumption
- [ ] Groves include existing trees (check grove system)
- [ ] Separate seed vs food items
- [ ] "Paint" mode for bulk farm/grove/pasture crop changes

---

## Milestone 3: Combat & World — "Make the World Dangerous"

*Goal: Flesh out combat, enemies, and world dynamics.*

### 3.1 — Combat UI & Feedback (#12, #13)
- [ ] Combat overview screen (gnome stats left, enemies right)
- [ ] Battle recap after fights end
- [ ] Combat notifications in event log

### 3.2 — Enemy Diversity (#33, #34)
- [ ] Different enemy types: ranged, armored, trap-disarming, flying (over walls)
- [ ] Monsters that dig upward if player avoids digging deep
- [ ] Enemy spawn scaling with kingdom worth

### 3.3 — World Dynamics (#67, #68, #70, #71)
- [ ] Difficulty presets for game creation (easy/normal/hard)
- [ ] Improved water physics (fix premature evaporation from Gnomoria)
- [ ] Day/night effects on work speed and enemy behavior

---

## Milestone 4: Automation & Late Game

*Goal: Give players systems to manage a large, established fortress.*

### 4.1 — Event-Triggered Mechanisms (#93)
Mechanism system exists (`mechanismmanager.cpp`). Controls for mechanisms were added in v0.8.6. Need to extend with event-based triggers.

- [x] ~~Basic mechanism controls~~ — done in v0.8.6
- [ ] Connect mechanisms to game events (raid → close doors, alarm → activate traps)
- [ ] Configurable triggers via UI

### 4.2 — Automaton Progression (#56)
Automatons were added in v0.7.0 with fuel system. Controls added in v0.8.6. Currently single-tier.

- [x] ~~Basic automatons with fuel~~ — done in v0.7.0
- [ ] Tier system: basic (hauling only) → advanced (crafting)
- [ ] Prevent golem cheese (automatons shouldn't fully replace gnomes)

### 4.3 — Magic & Religion (#85, #88)
No code exists for either. New system development.

- [ ] Religion: temple buildings, prayer activity, divine favor accumulation
- [ ] Magic: progression tree, spells unlocked via research/favor, mana source infrastructure

---

## Milestone 5: Modding & Community

*Goal: Let the community extend the game.*

### 5.1 — Modding API (#95)
Currently mods can only override SQLite DB entries.

- [ ] Document current modding capabilities thoroughly
- [ ] Expose behavior trees as loadable XML/JSON (#96 — BehaviorTree.CPP was suggested)
- [ ] Mod manager UI in game
- [ ] Mod load order and conflict detection

### 5.2 — Translation System Overhaul
.roest acknowledged it needs "a complete overhaul." ext3h suggested using Qt linguist tooling or XLIFF.

- [ ] Proper i18n framework (string extraction, XLIFF or similar)
- [ ] Community translation contribution pipeline

---

---

## Bug Report Overview

From the #bug-reports channel (6,066 messages, 2018–2022):

| Category | Total Reports | Key Themes |
|----------|--------------|------------|
| UI / Display | 166 | Crashes on scroll/click, missing info, display glitches |
| Save / Load | 139 | Corruption, overwrite by same name, OneDrive conflicts |
| Building / Construction | 129 | Floors disappearing, rotated buildings broken, scaffolding issues |
| Jobs / Tasks | 95 | Jobs stuck, cancel not working, gnomes ignoring available jobs |
| World / Terrain | 86 | Water behavior, trees blocking, light level issues |
| Stockpile / Storage | 82 | Filter confusion, hauling to deleted stockpiles, item not found |
| Crafting / Workshop | 80 | Rotated workshops, repeat jobs breaking, ingredient detection |
| Crash / Freeze | 72 | Workshop scroll, negative thirst, large map generation |
| Farming / Animals | 71 | Grove issues, animal breeding overflow, harvest timing |
| Pathfinding | 46 | Stuck gnomes, trapped by floor removal, idle despite available jobs |
| Performance | 42 | High speed lag, large stockpile slowdown, enemy spawn spikes |
| Combat / Military | 28 | Squad assignment, equipment not dropped on death |

**Cross-reference with feature requests**: Many bugs and features overlap. For example:
- "Gnomes get stuck" (bug) → "Gnome is trapped notification" (feature #19) → "Designated movement zones" (feature #30)
- "Job cancellation broken" (bug) → "Production limits on workshops" (feature #48)
- "Stockpile filter confusion" (bug) → "Stockpile UX overhaul" (feature #5)
- "Equipment not dropped on death" (bug) → "Dead gnome cleanup" (Milestone 0.5)

Fixing the bugs often naturally leads into implementing the requested features.

Full bug details: `docs/bug-reports/bug_report_summary.md`
Raw data: `docs/bug-reports/bug_reports_raw.json`

---

## What NOT to Build

These were discussed and explicitly rejected or deprioritized by the dev team:

| Suggestion | Why Not |
|-----------|---------|
| Adventurer mode | .roest: "not planned, even if I'd say I do it at some point, that point will be far in the future" |
| Multiple bases | .lld: "double the stuff running, and for what benefit?" |
| Microtransactions | Running joke in Discord |
| Walking on impossible tiles | .roest: "walking on impossible tiles is a bad workaround" |
| MySQL database | Overkill — SQLite is appropriate for a single-player game |

---

## Priority Summary

```
 NOW   Milestone 0: Fix foundations (build, perf, cleanup)
  |    Milestone 1: Core QoL (notifications, stockpiles, workshops, HUD)
  |    Milestone 2: Gnome depth (mood, zones, medical, farming QoL)
  |    Milestone 3: Combat & world (enemies, difficulty, world dynamics)
  |    Milestone 4: Late game (automation, magic, religion)
  v    Milestone 5: Modding & community (API, translations)
LATER
```

Each milestone builds on the previous. Milestone 0 is purely technical — it makes everything after it faster and more stable. Milestone 1 produces the most visible player-facing improvements for the least new code. Milestones 2-3 add gameplay depth. Milestones 4-5 are endgame content and extensibility.
