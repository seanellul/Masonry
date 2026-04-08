---
id: T-0001
title: Fix build menu icon rendering (size, background, color)
type: polish
created: 2026-04-07
blockers: []
tags: [ui, build-menu, sprites]
---

## Description

The build menu's icon rendering has three visible problems, most severe on the **Furniture**, **Utility**, and **Containers** tabs:

1. **Icons too small.** Furniture/Utility/Containers icons render at roughly half the size of Workshops icons. A chair or crate icon is ~32px while a carpenter workshop is ~80px. They should be at least doubled so they match the visual weight of the Workshops tab.

2. **Opaque black background.** Every icon in the build menu sits on a solid black square, which clashes hard with the warm brown panel background. This is visible on workshops too but is much more jarring on the smaller tabs where the black frame dominates the sprite. The background should be transparent so the icon floats on the panel.

3. **Monochrome rendering on small sprites.** On Furniture/Utility/Containers, icons render as white silhouettes (see `chair`, `crate`, `barrel`, `sack`). Workshops on the same panel render in full color (carpenter, sawmill). This looks like the small icons come from a different sprite source or take a different rendering path — possibly fallback/stub sprites, possibly a color-stripping step in the icon pipeline.

All three problems likely share a root cause in whatever codepath generates the build menu's icon previews, so they should be investigated and fixed together.

### Acceptance criteria

- Furniture, Utility, and Containers icons render at the same visual size as Workshops icons (roughly 2× current).
- No black square background on any build menu icon — icon sits on the panel with a transparent background.
- Small icons render in full color, consistent with workshop icons.
- Visual verification via `mcp__ingnomia-test__take_screenshot` + `compare_screenshots` on the Workshops, Furniture, Utility, and Containers tabs.
- No regression on the Workshops tab.

### Out of scope

- Redesigning the build menu layout.
- Replacing any sprite art.
- Tooltips on these icons (tracked separately under a future task).

### Reference

- Screenshots attached in the intake conversation show: Workshops (good size, black bg, color); Containers (small, black bg, monochrome); Furniture chair icon (small, black bg, monochrome).

## Plan

*(Scoping agent: investigate why Furniture/Utility/Containers icons take a different rendering path than Workshops. Identify the icon-generation codepath in `src/gui/` — likely somewhere in the build menu UI code, an aggregator, or `SpriteFactory`. Determine whether the black background is baked into the sprite, drawn by an ImGui frame, or comes from a clear-color in an offscreen framebuffer. Determine why small icons lose color. Propose a fix that addresses all three symptoms at their shared root cause rather than patching each separately.)*

## Result

*(Building agent fills in after implementation.)*
