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

*(Scoping agent: (1) Find the population view rendering code in `src/gui/ui/`. (2) Identify where the column headers are generated and where the 5-char truncation is happening. Check both the header string source (likely a skill enum or DB query) and the render path. The fix is almost certainly removing a `substr(0, 5)` or equivalent. (3) Locate the gnome-name cell render code. Wire an `ImGui::Selectable` or button + click handler that dispatches a "center camera on entity + open info panel" action. Check whether a "center on entity" helper already exists — the Tile Info panel's `(i)` button and the test controller probably both have something reusable. (4) Verify this works across all five population view tabs without duplicating the click handler in five places — lift it into the shared row renderer.)*

## Result

*(Building agent fills in after implementation.)*
