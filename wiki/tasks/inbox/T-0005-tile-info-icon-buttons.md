---
id: T-0005
title: Tile Info panel — icon-only buttons + clarify mystery button
type: polish
created: 2026-04-07
blockers: []
tags: [ui, tile-info]
---

## Description

The Tile Info panel has a few small button presentation issues:

1. **Gnome info button should be icon-only.** In the Creatures section, next to `Gnome: <name>`, the circle-`i` info icon currently sits inside a rectangular button frame. The rectangular frame should be removed — render just the `i` icon, clickable, with no button background.

2. **Terrain trash button should be icon-only.** In the Terrain section, next to `Floor: <material>`, there is a trash/delete button (which I assume removes/deconstructs the floor). It also sits in a rectangular frame that should be removed — just the trash icon, clickable, no frame.

3. **Mystery button next to the trash icon is unclear.** Immediately right of the trash button in Terrain, there is another small button whose purpose is not obvious from the UI. Scoping should identify what it does and then either:
   - Give it a distinct icon that communicates its function, and/or
   - Add a hover tooltip describing it (aligns with the T-0004 tooltip direction), and/or
   - Remove it if it is dead/legacy.

### Acceptance criteria

- Gnome info button: no background frame, icon is clickable, behavior unchanged.
- Terrain trash button: no background frame, icon is clickable, behavior unchanged.
- Mystery button: its function is documented (ideally as a one-line tooltip on hover) and its icon is recognizable. If it's dead code, it's removed.
- Visual verification via `mcp__ingnomia-test__take_screenshot` of the Tile Info panel with a tile selected that has terrain + a workshop + a gnome on it.
- No regression on other icon buttons in the Tile Info panel.

### Out of scope

- Redesigning the Tile Info panel layout.
- Changing the spacing/size/ordering of these buttons beyond removing the frames.
- Any work on Tile Info sections other than Terrain and Creatures.

## Plan

*(Scoping agent: find the Tile Info panel rendering code in `src/gui/ui/` — likely a `ui_tileinfo.cpp` or similar. Identify the calls producing the Gnome info button and the Terrain trash button. In ImGui, the fix is typically replacing `ImGui::Button` (which draws a frame) with `ImGui::ImageButton` configured with no background, or with an invisible button wrapping an image draw (`InvisibleButton` + `GetWindowDrawList()->AddImage`). Alternatively use `PushStyleColor(ImGuiCol_Button, 0)` + friends around the button. For the mystery button: trace the code that creates it, identify what action it dispatches, report back, and propose an icon + tooltip.)*

## Result

*(Building agent fills in after implementation.)*
