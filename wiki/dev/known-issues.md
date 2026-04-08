---
title: Known Issues
tags: [dev, tech-debt, bugs]
status: draft
last_updated: 2026-04-07
sources: [~/.claude auto-memory]
---

# Known Issues

Running list of known bugs, tech debt, incomplete systems, and scaling bottlenecks in Masonry. Each entry should eventually graduate into a `tasks/inbox/` file when it's ready to be worked on. This page is seeded from `~/.claude` memory and should be updated whenever a new issue is discovered or an old one is resolved.

## Scaling bottlenecks

### Gnome count scaling — O(n²) social + job search
- **Symptom**: frame time degrades sharply as gnome count grows.
- **Root causes**:
  - Social interaction check is O(n²) across all gnome pairs per tick.
  - Job search cost grows with population and job queue length.
  - Behavior-tree tick has no batching or parallelization.
  - Entire simulation runs single-threaded on the game thread.
- **Status**: not yet ticketed. See [[parallelization]] for the broader plan.

## Incomplete systems

### Forage action is a UI stub
- **Symptom**: The `Forage` button in the Shape → Nature menu enqueues jobs via `cmdSetSelectionAction("Forage")`, but no gnome task function handles them. Jobs sit in the queue forever.
- **Evidence**: `Forage` appears in `src/gui/keybindings.h:85` as `ActionForage` and in `ui_gamehud.cpp:196` in `natureActions[]`, but **no corresponding entry** in `src/game/gnome.cpp`'s `m_taskFunctions` map (compare to `FellTree`, `RemovePlant`, `PlantTree` which are all wired).
- **Discovered**: during T-0007 scoping (Apr 2026).
- **Status**: unticketed. Either implement the task handler or remove the button. In-game tooltip now labels it honestly as "[Not yet implemented]".

### Plant Tree button is a UI stub from the Nature menu
- **Symptom**: The `Plant Tree` button in the Nature menu has an empty action string (`""` in `ui_gamehud.cpp:196` `natureActions[]`). The button is already disabled in code. Tree planting happens only through groves (which have their own issues — see T-0010).
- **Status**: the button shows greyed out so it's not actively misleading, but it's dead weight until either groves are fixed (T-0010) or a direct Plant-Tree action is implemented.

### Military / uniform / squad wiring
- **Symptom**: gnomes assigned to squads never equip their assigned gear.
- **Scope**: the squad → uniform → inventory → equipment pipeline has gaps — uniforms are defined, squads are defined, but the actual equip step does not fire for squad members.
- **Status**: not yet ticketed. Likely needs a focused investigation pass before a task can be scoped.

## UI / UX

### UI overhaul (RimWorld-inspired)
- Ongoing multi-phase effort to replace legacy panels with a RimWorld-style layout.
- Constraint: do not multiply ImGui font sizes by DPR, do not change `io.FontDefault` globally (previous attempts caused layout breakage).
- **Status**: in progress across multiple recent commits (see `DEVLOG.md`).

## Test / automation infrastructure

### 4-layer test + MCP feedback system
- Test controller, test command server, MCP server, and smoke-test tooling exist and are wired for agent development.
- Agents should prefer using the `ingnomia-test` MCP tools (`build_game`, `run_smoke_test`, `take_screenshot`, `game_command`) over manual build/run.
- **Status**: working. Expand coverage incrementally as new features land.

## See also
- [[roadmap]]
- [[parallelization]]
- [[gnome-ai-redesign]]
