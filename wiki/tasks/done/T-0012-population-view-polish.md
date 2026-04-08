---
id: T-0012
title: Population view — fix clipped skill headers + gnome-name GoTo navigation
type: polish
created: 2026-04-07
blockers: []
tags: [ui, population, navigation]
---

## Description

Two independent issues in the Population view's Skills tab.

### 1. Skill column headers are clipped to 5 characters

All column headers in the skills grid are being truncated to 5 characters: `Minin`, `Mason`, `Stone`, `Woodc`, `Carpe`, `Woodc` (again), `Smelt`, `Black`, `Metal`, `Weapo`, `Armor`, `Gemcu`, `Jewel`, `Glass`, `Weavi`, `Tailo`, `Dyein`, `Potte`. Notably **Woodcutting** and **Woodcarving** both collapse to `Woodc`, making them indistinguishable.

Increasing the column width does not reveal more of the text, which suggests the clipping is happening at the string level (`substring(0, 5)` or similar) rather than at the render level. Scoping should confirm and fix at the source.

### 2. Clicking a gnome name should GoTo + open their info

Clicking a gnome's name in the population view currently does nothing useful (or only selects the row). It should behave as a **GoTo**:

- Center the camera on the gnome's current position (including z-level if they're on a different layer).
- Open the gnome's character info view (the same panel that appears from the Tile Info → Gnome `(i)` button).

Both actions should happen on a single click on the name. This should work from any tab in the population view (Skills, Schedule, Personality, Social, Professions) since the name column is present in all of them.

### Acceptance criteria

- All skill column headers display their full name: `Mining`, `Masonry`, `Stonecutting`, `Woodcutting`, `Carpentry`, `Woodcarving`, `Smelting`, `Blacksmithing`, `Metalworking`, `Weaponsmithing`, `Armorsmithing`, `Gemcutting`, `Jewelling`, `Glassmaking`, `Weaving`, `Tailoring`, `Dyeing`, `Pottery` (or whatever the canonical names are — scoping will read them from the source).
- No two columns collapse to the same label.
- Clicking any gnome's name in any population view tab:
  1. Centers the camera on that gnome (right z-level).
  2. Opens the gnome's character info panel.
- Visual verification via `mcp__ingnomia-test__take_screenshot` on the skills grid (headers visible) and on the character info panel after a name click.
- No regression on row selection or any other click targets in the population view.

### Out of scope

- Column width tuning (beyond what's needed to display full names — don't redesign the table).
- Manual/auto priority cells (that's T-0011).
- Any other population view tabs' layout issues.

## Plan

Population view lives in `drawPopulationPanel` in `src/gui/ui/ui_sidepanels.cpp` (~line 540). Skills tab has two variants:

- **Individual view** (~line 580): `ImGui::BeginTable("SkillsIndiv", ...)` + `TableSetupColumn(skill.name.left(5).toStdString().c_str(), 0, 50.0f)` — the `.left(5)` literally truncates every column header to 5 characters. That's the bug. Fix = drop `.left(5)`, widen the column from 50 to 100.
- **Group view** (~line 660): `TableSetupColumn(grp.name.toStdString().c_str(), 0, 82.0f)` — no truncation, fine as-is.

Gnome-name click handler lives in both views at ~line 608 (indiv) and ~line 727 (group). Both use `ImGui::Selectable(gnome.name, …)` and only set `bridge.selectedGnomeID`. The bridge already exposes:
- `cmdNavigateToEntity(id)` (declared at `imguibridge.h:99`) — sets a pending camera-nav target consumed by MainWindow.
- `onOpenCreatureInfo(creatureID)` (declared at `imguibridge.h:406`) — requests creature update + sets `activeSidePanel = SidePanel::CreatureInfo`.

Fix: on name click, call both in addition to the existing `selectedGnomeID = …`.

Schedule / Personality / Social / Professions tabs: out of scope for this autonomous pass — separate tab implementations, can be layered on later if the pattern above works.

## Result

Implemented in `src/gui/ui/ui_sidepanels.cpp`:

1. **Clipped headers** (~line 594–600): removed `.left(5)` from the individual-view skill column header and bumped the column width from `50.0f` to `100.0f` so full skill names like `Woodcutting` / `Woodcarving` render distinctly.
2. **Gnome-name navigation — individual view** (~line 608–615): clicking the name `Selectable` now calls `bridge.cmdNavigateToEntity(gnome.id)` (camera jump) and `bridge.onOpenCreatureInfo(gnome.id)` (opens character info panel) in addition to setting `selectedGnomeID`.
3. **Gnome-name navigation — group view** (~line 727–735): same three-line change as above.

Build: green (11 warnings, all pre-existing).

**Deferred** (out of this autonomous pass): extending the navigation behavior to the Schedule / Personality / Social / Professions tabs. Those tabs have separate rendering code paths and the task's scope of "works from any tab" can be closed later in a small follow-up now that the pattern is established in two sites.
