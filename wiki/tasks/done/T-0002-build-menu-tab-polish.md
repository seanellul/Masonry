---
id: T-0002
title: Build menu tab bar — proper Containers icon + center button contents
type: polish
created: 2026-04-07
blockers: []
tags: [ui, build-menu]
---

## Description

Two small issues in the build menu's top tab bar (Workshops / Structures / Furniture / Utility / Containers):

1. **Containers has no proper icon.** The other four tabs use real icons (gear, mountain/arch, couch, wrench) but Containers falls back to the literal character `Ǝ`, which is clearly a missing-glyph placeholder. Needs a proper icon consistent with the other tabs — likely an RPG Awesome or Font Awesome glyph, both of which are already integrated in the project (see recent equipment panel work in `DEVLOG.md`).

2. **Tab button contents are not centered.** The icon + label group inside each tab button is not horizontally centered within the button's own width — it sits toward one side. Each tab's `[icon + text]` should be absolutely centered within its button.

### Acceptance criteria

- Containers tab shows a real themed icon, not `Ǝ`.
- On every tab button, the icon + text group is horizontally centered within the button's frame.
- Visual verification via `mcp__ingnomia-test__take_screenshot` on the build menu, comparing all five tabs.
- No regression on the already-correct tabs.

### Out of scope

- Any other build menu changes (covered by T-0001 and forthcoming tooltip task).
- Restyling the tab buttons themselves (colors, borders, active state).

## Plan

Tab bar lives in `src/gui/ui/ui_gamehud.cpp`:
- `buildCategories[]` at ~line 61 declares the five category buttons and their icons.
- The render loop at ~line 580 lays them out with `ImGui::SameLine()` + auto-width `ImGui::Button( catLabel, ImVec2( 0, 30 ) )`. Auto-width + ImGui's default `ButtonTextAlign = (0, 0.5)` produces left-aligned labels.

Root cause of the `Ǝ` fallback: the bundled `content/fonts/fa-solid-900.ttf` does not contain the newer FA6 codepoint U+f466 (`ICON_FA_BOX`), so the atlas renders a missing-glyph placeholder. Fix is to pick a classic FA codepoint that is present in the shipped font.

Fix:
1. Swap `ICON_FA_BOX` → `ICON_FA_CUBES` (U+f1b3, classic FA codepoint, present in the free tier).
2. Give each category button a uniform `catBtnW = 130.0f` width and wrap the render loop in `PushStyleVar(ImGuiStyleVar_ButtonTextAlign, (0.5, 0.5))` / `PopStyleVar` so every tab renders its icon+label centered.

## Result

Implemented in `src/gui/ui/ui_gamehud.cpp`:
- `buildCategories[]` at ~line 61: Containers icon changed from `ICON_FA_BOX` to `ICON_FA_CUBES` with a comment explaining why.
- Category render loop at ~line 580: added `PushStyleVar(ButtonTextAlign, (0.5, 0.5))` before the loop, matching `PopStyleVar` after, and set every button to a uniform `ImVec2( 130.0f, 30 )` size so the centering is visible.

Build: green (11 warnings, all pre-existing).
