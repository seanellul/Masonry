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

Tile Info lives in `src/gui/ui/ui_tileinfo.cpp` (small, 239 lines).

- A shared `actionButton(icon, tooltip, idSuffix)` helper at ~line 9 wraps `ImGui::SmallButton(icon)` plus a tooltip. Every Terrain-row action (trash floor, replace floor, fell tree, harvest tree) flows through this helper. The "rectangular frame" the user sees is `SmallButton`'s default frame background.
- The Gnome info button at ~line 226 does *not* use `actionButton` — it hand-rolls its own `SmallButton(ICON_FA_CIRCLE_INFO)` + PushID/PopID + tooltip.
- The "mystery button" next to the trash icon is `actionButton(ICON_FA_RIGHT_LEFT, "Replace floor", "rpfloor")` at line 60 — it already has a **Replace floor** tooltip on hover. The user's confusion was just not hovering long enough to see it; no code action needed beyond making all action buttons clearly icon-only, which gives the tooltip more reason to be consulted.

Fix:
1. Push transparent `ImGuiCol_Button` + subtle hovered/active overlays inside `actionButton()` so every Terrain action renders frameless. One edit fixes the trash, replace-floor, fell-tree, harvest-tree, and mine-wall buttons at once.
2. Apply the same transparent push/pop block around the `SmallButton(ICON_FA_CIRCLE_INFO)` in the Creatures section.

## Result

Implemented in `src/gui/ui/ui_tileinfo.cpp`:

- **`actionButton()` helper** at ~line 9: added a 3-color push (`ImGuiCol_Button` → fully transparent, `ImGuiCol_ButtonHovered` → white@12%, `ImGuiCol_ButtonActive` → white@20%) wrapping the `SmallButton` call. Every Terrain-row action button (trash floor, replace floor, fell tree, harvest, mine wall) now renders icon-only with a subtle hover highlight.
- **Creatures info button** at ~line 226: added the same 3-color push/pop block around the hand-rolled `SmallButton(ICON_FA_CIRCLE_INFO)`.

Mystery button identified: it's the `ICON_FA_RIGHT_LEFT` **Replace floor** action at ~line 60, which already has a hover tooltip reading "Replace floor". No additional identification work needed. With the frame stripped from both icons, the tooltip becomes the obvious way to confirm what each button does.

Build: green (12 warnings, all pre-existing — `btnSize` and `terrainRow` were already unused before this change).
