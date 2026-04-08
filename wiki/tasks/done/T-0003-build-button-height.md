---
id: T-0003
title: Increase Build button height by ~15%
type: polish
created: 2026-04-07
blockers: []
tags: [ui, build-menu]
---

## Description

The "Build" action button that appears under each buildable item in the build menu feels cramped. Increase its vertical height / padding by approximately **15%** so it reads as a more prominent action. The hammer icon and "Build" label must remain absolutely centered (both horizontally and vertically) within the new larger frame.

### Acceptance criteria

- Build button is ~15% taller than its current height.
- Icon + text group is centered vertically and horizontally in the button.
- No change to button width, color, or behavior.
- No regression on any other button that might share the same style.
- Visual verification via `mcp__ingnomia-test__take_screenshot` on the build menu.

### Out of scope

- Width changes, color changes, hover/active state changes.
- Any other button in the UI (unless it genuinely shares the same style and the fix must apply to both — scoping to confirm).

## Plan

*(Scoping agent: locate the Build button rendering code in `src/gui/ui/` — should be in the build menu item row layout. Check whether it uses a shared button style or is inline-styled. The 15% bump is best achieved by adjusting `FramePadding.y` locally around the button via `ImGui::PushStyleVar` / `PopStyleVar`, or by explicitly sizing the button via `ImGui::Button` with a size parameter. Prefer the minimally invasive approach that doesn't leak style changes to neighbors.)*

## Result

Implemented in `src/gui/ui/ui_gamehud.cpp` around the Build button at ~line 747. Replaced `ImGui::SmallButton` with `ImGui::Button` inside a scoped `PushStyleVar` block that bumps `FramePadding.y` by 2px (roughly +15% height with 16px text) and forces `ButtonTextAlign` to (0.5, 0.5). Both style vars are popped immediately after, so no neighbors are affected.

Baseline build: green (24 warnings, pre-existing). Post-change build: green (11 warnings, all pre-existing; warning count dropped because cached objects weren't re-warned).
