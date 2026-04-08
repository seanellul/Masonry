---
id: T-0019
title: Skill grouping in Population view (10 groups + 3 standalones)
type: feature
created: 2026-04-07
blockers: [T-0018]
tags: [ui, skills, population]
---

## Description

Reduce the Population view's Skills tab from 47 columns (47 skills) to **13 columns** (10 groups + 3 standalones) by clustering related skills under group headers. Sub-skills remain independent at the gameplay level — each skill keeps its own XP and level — but the visual UI shows one column per group with the gnome's max sibling-skill level + a checkbox toggling the whole group.

This is a **visual** change. The data model stays the same. Individual view (existing) still shows all sub-skills for surgical assignment.

### Group structure

| Group | Sub-skills |
|---|---|
| **Earthworking** | Mining, Masonry, Stonecarving |
| **Forestry** | Woodcutting, Carpentry, Woodcarving |
| **Smithing** | Smelting, Blacksmithing, Metalworking, WeaponCrafting, ArmorCrafting |
| **Textiles** | Weaving, Tailoring, Dyeing |
| **Bone & Hide** | Leatherworking, Bonecarving |
| **Fine Craft** | Gemcutting, JewelryMaking, GlassMaking, Pottery |
| **Engineering** | Engineering, Machining |
| **Hearth** | Cooking, Brewing, Butchery |
| **Field** | Farming, AnimalHusbandry, Fishing |
| **Magic** | MagicNature, MagicGeomancy |

Standalones (no group):
- **Hauling** — affects every job, doesn't fit a craft category
- **Construction** — meta build skill
- **Medic** — sole healing skill (post-merge with Caretaking from T-0018)

### Cell rendering

Each cell shows:
- **Checkbox**: any sibling active? Toggles all sub-skills in the group on/off.
- **Number**: max sibling level among the gnome's skills in this group, or `—` if none.
- **Color tint**: the group's accent color (already in `s_groupIndex.groups[].color`).

**Hover tooltip** expands to show every sub-skill in the group with its individual level and XP, plus the gnome's title (from T-0021 once that lands).

```
┌─ SMITHING — Brorvar ──────────────────────┐
│ ✓ Smelting           12                    │
│ ✓ Blacksmithing      18  ← top             │
│ ✓ Metalworking        8                    │
│ ✓ WeaponCrafting     15                    │
│   ArmorCrafting       3                    │
│                                            │
│ Title: Master Smith                        │
└────────────────────────────────────────────┘
```

### Acceptance criteria

- Population view → Skills → Group view shows 13 columns instead of 47.
- Each group cell shows checkbox + max-sibling-level number.
- Hover tooltip shows the per-skill breakdown.
- Group checkbox toggles every sibling at once.
- Individual view (existing) is unchanged and still shows all 47 columns.
- Visual verification via `mcp__ingnomia-test__take_screenshot` of both views.

### Implementation note

The group view code already exists (`drawPopulationPanel` group view, ~line 660 in `ui_sidepanels.cpp`, touched by T-0012). The structure `s_groupIndex.groups` already supports `name`, `color`, and `skillIndices`. The work here is:
1. Define the 10 groups with the right `skillIndices` based on the skill ordering at game load.
2. Verify the existing render path produces the right cell content.
3. Optionally enhance the hover tooltip with the per-skill breakdown.

### Out of scope

- Cross-training XP bonus (T-0020).
- Skill titles (T-0021).
- Any gameplay change at the per-skill level.
- Renaming the existing group structure used elsewhere.

### Dependency on T-0018

This task should land **after T-0018** (cleanup removals) so the group definitions don't reference removed skills. If the order ends up reversed, T-0019's group definitions will need a one-line update to drop the missing skills.

## Plan

*(Scoping: (1) Read the current `s_groupIndex` structure in `ui_sidepanels.cpp` to understand how groups are defined today. (2) Define a constant table of the 10 groups + their member skill IDs. (3) Build the indices at game load by mapping skill IDs to their column index in `gnome.skills`. (4) Verify the cell render handles the new structure. (5) Add the hover tooltip if not already present.)*

## Result

*(Building agent fills in.)*
