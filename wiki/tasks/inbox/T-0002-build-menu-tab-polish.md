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

*(Scoping agent: find the tab bar rendering code in `src/gui/ui/` — likely a build menu panel file. Identify which icon library the other tabs pull from and pick a suitable Containers glyph. For the alignment issue, check whether buttons use fixed width with `ImGui::SameLine()` / manual cursor positioning vs. a proper centered layout — fix via `CalcTextSize` + `SetCursorPosX` or equivalent.)*

## Result

*(Building agent fills in after implementation.)*
