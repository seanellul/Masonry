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

*(Scoping agent: (1) Find the Schedule tab rendering code in `src/gui/ui/` — likely in the same population view file touched by T-0012. Reproduce the bug via `mcp__ingnomia-test__build_game` + take_screenshot to confirm the described symptom. (2) Identify how the Paint buttons currently handle their click. The bug is almost certainly that each Paint button's click handler is directly calling a "set schedule type" function on some default target (the "All" cell?) instead of just setting a local brush-armed state. (3) Introduce a `currentPaintType` state on the schedule panel. Paint buttons set this state only. Hour cells check this state on click. (4) Wire click-and-drag — ImGui's `ImGui::IsMouseDragging` + tracking which cell the cursor is over during the drag. (5) Preserve the "All" column / row / corner bulk-apply semantics — those cells should still work, but only when clicked after a brush is armed, same as individual cells. (6) Add visual feedback for the armed brush on the Paint palette.)*

## Result

*(Building agent fills in after implementation.)*
