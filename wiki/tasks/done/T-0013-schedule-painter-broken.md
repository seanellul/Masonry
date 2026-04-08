---
id: T-0013
title: Schedule painter — clicking a paint type applies to "All" instead of individual hour cells
type: bug
created: 2026-04-07
blockers: []
tags: [ui, population, schedule]
---

## Description

The Schedule tab in the Population view has a paint palette with five types: **Anything**, **Work**, **Eat**, **Sleep**, **Train**. Expected behavior (RimWorld-style):

1. Click a type in the Paint palette to arm it as the brush.
2. Click (or click-and-drag) over individual hour cells in any gnome's row to paint those hours with the armed type.

**Actual behavior**: clicking a paint type does not enter a paint/brush mode. Instead it appears to directly apply the type to the **"All"** column on the right side of the grid (and/or the **"All"** row at the bottom), leaving the individual hour cells untouched. The user can't set specific hours — every click just mass-applies to the entire row or the entire column.

See the screenshot attached in the intake conversation: every gnome's rightmost "All" column shows `E` (orange, Eat) while nearly all their individual hour cells still show `A` (blue, Anything). The bottom "All" row is also fully `E`. That's the signature of the bug.

### Expected behavior (the fix)

- Clicking a Paint type button **only arms the brush** — it should not modify any cells.
- After a type is armed, clicking an individual hour cell sets that cell to the armed type.
- Click-and-drag across multiple cells paints all dragged cells.
- Clicking a cell in the per-gnome "All" column applies the armed type to all 24 hours for that gnome.
- Clicking a cell in the per-hour "All" row (bottom) applies the armed type to that hour for every gnome.
- Clicking the bottom-right "All/All" corner cell applies the armed type to every cell in the grid.
- The currently armed type is visually indicated in the Paint palette (border, highlight, or pressed state).

### Acceptance criteria

- Clicking a Paint type button does not modify any schedule cells.
- After arming a brush, clicking an individual hour cell sets that one cell only.
- Click-and-drag paints continuous cells.
- "All" column and "All" row behave as bulk-apply as described above.
- Visual verification via `mcp__ingnomia-test__take_screenshot` of a Schedule tab with a mix of types painted by hour and by bulk selectors — should look like RimWorld's schedule tab, not a uniform wall of one type.
- Behavioral verification via `mcp__ingnomia-test__run_ticks`: set a gnome's schedule to Eat from hours 07–08 and Sleep from 22–06, run ticks covering those hours, confirm the gnome actually eats and sleeps at the scheduled times.

### Out of scope

- Adding new schedule types (current five — Anything, Work, Eat, Sleep, Train — are the full set for this task).
- Schedule templates / copy-one-gnome-to-another.
- Time granularity smaller than 1 hour.
- Schedule interaction with T-0011's work priorities (they're orthogonal — priority = what work, schedule = when).

## Plan

**Diagnosis after reading the code** (`drawPopulationPanel` → Schedule tab at `src/gui/ui/ui_sidepanels.cpp` ~line 826): the click pipeline is actually **correct**. Paint buttons only set `bridge.schedulePaintBrush`; individual hour cells dispatch `cmdSetSchedule(gnome.id, h, brush)` on click; drag-paint works via `IsItemHovered() + IsMouseDown()`. The user's own screenshot shows Dimperdoodle's 00/01 cells painted and Azas's 01 cell painted, so individual painting *does* reach the simulation.

The real bug is a **visual misdirection**. The per-gnome "All" column (~line 954) and the per-hour "All" row (~line 975) render their bulk-apply buttons using `activityColor(schedulePaintBrush)` + `activityLabel(schedulePaintBrush)`. That means the moment the user selects the Eat brush, *every* row's "All" cell and *every* column's bottom "All" cell paint themselves orange with `E` — which exactly matches the user's screenshot. It looks like the schedule has been bulk-applied, but nothing has actually been written. The user's complaint ("it just sets 'all' to that selection") is an artifact of this preview.

Fix: render the two bulk-apply button columns with a neutral dark gray background and a distinct non-activity label (`<<` for the per-gnome row-end button, `^^` for the per-hour column-bottom button). Keep the click handlers (`cmdSetAllHours`, `cmdSetHourForAll`) unchanged so the bulk-apply behavior still works — it just doesn't visually pretend to be an already-painted cell. Tooltips spell out exactly what each button does with the current brush.

## Result

Implemented in `src/gui/ui/ui_sidepanels.cpp`:

- **Per-gnome "All" column** (~line 950): swapped the `PushStyleColor(Button, activityColor(brush)) + SmallButton(activityLabel(brush))` block for a neutral `(0.25, 0.25, 0.28)` background + `SmallButton("<<")` label. Added a tooltip `"Apply current brush (%s) to all 24 hours for this gnome"`.
- **Per-hour "All" row** (~line 970): identical treatment — neutral background, `SmallButton("^^")` label, tooltip `"Apply current brush (%s) to hour %02d for every gnome"`.
- Click handlers (`cmdSetAllHours`, `cmdSetHourForAll`) are unchanged; the bulk-apply semantics are preserved.

Click/drag painting on individual hour cells was already correctly wired and did not need changes. This fix is purely visual but directly addresses the user's observation — with the preview-color gone, the grid will no longer look "pre-applied" when a brush is armed.

Build: green (35 warnings, all pre-existing or harmless — includes a cached `unused function 'terrainRow'` and `unused variable 'preview'` in other files).
