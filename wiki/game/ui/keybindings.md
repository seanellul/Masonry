---
title: Keybindings
tags: [game, ui, controls, reference]
status: current
last_updated: 2026-04-07
sources: [keybindings.json, src/gui/keybindings.cpp, src/gui/ui/ui_mainmenu.cpp]
---

# Keybindings

Canonical reference for every keyboard shortcut in Masonry. The in-game **Settings → Keybindings** tab (added in T-0014) shows the same information, read directly from `keybindings.json`.

**Source of truth**: `keybindings.json` at the project root (the shipped default) and `<dataFolder>/settings/keybindings.json` at runtime (the user's override). The runtime loader is `KeyBindings::update()` in `src/gui/keybindings.cpp`.

**Rebinding today**: edit the JSON file and restart. In-game rebinding is a future feature.

## Navigation

| Action | Key |
|---|---|
| Scroll left | `A` |
| Scroll right | `D` |
| Scroll up | `W` |
| Scroll down | `S` |
| Rotate camera clockwise | `,` |
| Rotate camera counterclockwise | `.` |
| Z-level down | `-` |
| Z-level up | `]` |
| Zoom in | `-` |
| Zoom out | `+` |

## Game

| Action | Key(s) |
|---|---|
| Quick save | `F5` |
| Quick load | `F8` |
| Toggle fullscreen | `F` |
| Open gnome list | `G` |
| Toggle walls | `H` |
| Toggle axles overlay | `Q` |
| Open last action window | `L` |
| Toggle pause | `Space` or `P` |
| Rotate selection (while placing) | `R` |
| Menu button 1–0 | `1` `2` `3` `4` `5` `6` `7` `8` `9` `0` |

## Debug / Dev Tools

| Action | Key |
|---|---|
| Open bug report window | see JSON |
| Open log window | see JSON |
| Open debug window | see JSON |
| Print debug | see JSON |
| Toggle debug overlay | see JSON |
| Toggle debug mode | see JSON |
| Reload shaders | see JSON |
| Reload CSS | see JSON |

## Shape actions

Dispatched via `cmdSetSelectionAction`. These are the keyboard shortcuts that arm a specific shape action without opening the shape menu.

| Action | Dispatched command |
|---|---|
| Mine | `Mine` |
| Dig hole | `DigHole` |
| Explorative mine | `ExplorativeMine` |
| Remove ramp | `RemoveRamp` |
| Remove floor | `RemoveFloor` |
| Remove plant | `RemovePlant` |
| Mine stairs up | `MineStairsUp` |
| Dig stairs down | `DigStairsDown` |
| Dig ramp down | `DigRampDown` |
| Create room | `CreateRoom` |
| Create stockpile | `CreateStockpile` |
| Create grove | `CreateGrove` |
| Create farm | `CreateFarm` |
| Create pasture | `CreatePasture` |
| Create no-pass zone | `CreateNoPass` |
| Build wall / replace wall / fancy wall / wall+floor | `BuildWall` / `ReplaceWall` / `BuildFancyWall` / `BuildWallFloor` |
| Build floor / replace floor / fancy floor | `BuildFloor` / `ReplaceFloor` / `BuildFancyFloor` |
| Build scaffold | `BuildScaffold` |
| Build fence | `BuildFence` |
| Build workshop | `BuildWorkshop` |
| Build stairs | `BuildStairs` |
| Build ramp / ramp corner | `BuildRamp` / `BuildRampCorner` |
| Cut clipping | `CutClipping` |
| Build item | `BuildItem` |
| Plant tree | `PlantTree` *(dispatched but not implemented from the menu — see known-issues)* |
| Fell tree | `FellTree` |
| Forage | `Forage` *(dispatched but not implemented — see known-issues)* |
| Harvest tree | `HarvestTree` |
| Remove designation | `RemoveDesignation` |
| Deconstruct | `Deconstruct` |
| Cancel job | `CancelJob` |
| Raise priority | `RaisePrio` |
| Lower priority | `LowerPrio` |
| Magic nature — speed growth | `MagicNatureSpeedGrowth` |
| Magic geomancy — reveal ore | `MagicGeomancyRevealOre` |

## Notes

- The exact key assignments for Debug / Shape actions may vary — the in-game Keybindings tab (Settings → Keybindings) reflects the user's current file.
- `Forage` and `Plant Tree` from the Nature menu are known to be UI stubs with no runtime implementation — see [[known-issues]].
- This page is not auto-generated; when new key bindings are added to `keybindings.json`, update both the JSON and this page. A future task can build a sync script.

## See also
- [[known-issues]]
- `src/gui/keybindings.cpp` — `KeyBindings::update()` reads the JSON at startup
- `src/gui/ui/ui_mainmenu.cpp` — `drawSettings()` → Keybindings tab
