# Ingnomia Version History

Complete changelog from Discord #updates channel. 60 releases from June 2018 to August 2021.
**200 bug fixes, 134 feature additions, 31 changes** across versions 0.2.3 → 0.8.10.

## Development Timeline

```
2018 Jun-Dec  v0.2.3 → v0.5.4   Core game (32 releases, ~2/week pace)
2019 Jan-Dec  v0.5.5 → v0.7.4   Behavior trees, combat, trading, automatons
2020 Jan-Dec  v0.7.5 → v0.8.6   Open source, major refactor, UI rewrite
2021 Jan-Aug  v0.8.7 → v0.8.10  Final updates (ext3h took over)
2021 Aug →    No releases         Development stalled
```

## Major Milestones

### v0.3.x (Aug–Sep 2018) — Polish & Animals
- Animals: duck, goose, dog, cat, fox, wolf, deer, goat, squirrel, cows
- Short walls, fences, keyboard zoom
- Skill system overhaul (part 1)
- Configurable key bindings
- Stockpile improvements: move top/bottom, craft# countdown, craft-to suspend

### v0.4.x (Sep–Nov 2018) — Underground & Water
- Water flows, fish, cheese
- Dig ramps, mine stairs up
- No-pass designation zones (partially addresses community request #30)
- Wheelbarrows, carry containers
- Auto-generate jobs

### v0.5.x (Nov 2018–Jan 2019) — Trading & AI
- Trading system with seed/animal traders
- Migration (1-5 gnomes per season)
- **Behavior trees for creature AI** — major architecture change
- Basic magic features started
- Job priorities
- Mod support (stylesheet mods)
- Wall paintings, bone furniture

### v0.6.x (Mar–Aug 2019) — Combat & Workshops
- **Assign gnome to workshops** (community request #49 — done!)
- Workshop priority
- Armor/weapon crafting and equipping
- Military settings, squad targets
- Scheduling system
- Stockpile limits
- Hard limit of 1000 creatures per type
- Octree for item management (performance)

### v0.7.x (Nov 2019–Jan 2020) — Automatons & World
- **Automatons** (community request #56 — implemented!)
- Windmill, rivers, ocean
- Neighbor kingdoms and interactions
- Mechanisms, controls for automatons
- Death from drowning
- Camera center on gnome death notification
- Quick save
- Attack/defend combat behaviors
- Water evaporation
- Peaceful mode
- DPI scaling

### v0.8.x (Oct 2020–Aug 2021) — Open Source & Refactor
- **Open sourced under AGPL v3**
- **Switched to CMake build**
- **C++17** (from C++11 with MSVC extensions)
- **Qt 5.14.1**
- **Isolated simulation thread from UI thread** — first threading!
- Removed old Qt UI, replaced with Noesis XAML
- Refactored rendering: compute shaders, split render passes
- Pathfinding fixes (wrong heuristic, moving targets)
- Large code refactor for crashes/memory leaks
- ext3h takes over maintenance (v0.8.9–0.8.10)

## Features Already Implemented (vs Community Requests)

Cross-referencing changelogs with the 100 community feature requests:

| Request | Version | Status |
|---------|---------|--------|
| #30 Designated movement zones | v0.4.7 | **PARTIAL** — "no pass designation" added, but not full zone system |
| #48 Production limits (craft-to) | v0.4.0 | **DONE** — "craft to, jobs suspend when number reached" |
| #49 Assign gnomes to workshops | v0.6.1 | **DONE** — "assign gnome to workshops" |
| #56 Automatons | v0.7.0 | **DONE** — single tier with fuel |
| #65 Hotkeys for actions | v0.3.7 | **DONE** — configurable keybindings |
| #68 Water physics | v0.4.5–v0.7.4 | **DONE** — water flow, evaporation |
| #76 Animal population caps | v0.6.9 | **DONE** — "hard limit of 1000 creatures per type" |
| #89 Happiness affects migration | v0.5.4 | **PARTIAL** — migration exists but happiness doesn't affect it |
| #93 Mechanism controls | v0.8.6 | **DONE** — "controls for mechanisms" |
| #98 Threading | v0.8.0 | **PARTIAL** — sim/UI thread split, but game loop still single-threaded |

## Bugs Fixed Per Version (showing the biggest fix batches)

| Version | Date | Fixes | Notable |
|---------|------|-------|---------|
| v0.6.4 | 2019-06-13 | 17 | Stockpile, workshop, designation fixes |
| v0.6.6 | 2019-07-21 | 11 | Auto craft freeze, trader items, deconstructing |
| v0.7.3 | 2019-12-19 | 11 | Enemy aggression, goblin peaceful bug, haircolor direction bug |
| v0.7.2 | 2019-11-23 | 10 | Automaton, stockpile, workshop fixes |
| v0.6.2 | 2019-06-13 | 10 | Floor sprites, materials, tech gain |
| v0.5.8 | 2019-01-22 | 8 | Tree growth evading, priorities, death/thirst |
| v0.7.5 | 2020-01-15 | 8 | Phase shifting gnomes, item search, floor deconstruct |

## What Was Never Released

Features started but never completed (from changelogs + dev discussion):
- **Magic system** — "started implementing basic magic features" in v0.5.2, never expanded
- **Full combat AI** — attack/defend added in v0.8.7, but no diverse enemy types
- **Translation system** — prepared for translations in v0.7.0, never shipped
- **Mod API** — stylesheet mods in v0.5.3, DB override mods, but no formal API
- **Happiness/mood** — needs system exists but happiness never wired up
- **Multiplayer** — never attempted (wisely)
