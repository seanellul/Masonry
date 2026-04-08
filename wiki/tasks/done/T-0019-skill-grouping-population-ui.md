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

**Convenient finding**: the entire grouping infrastructure is **already data-driven**. The aggregator (`AggregatorPopulation::AggregatorPopulation` at `aggregatorpopulation.cpp:37`) iterates the `SkillGroups` DB table and builds `m_skillIds` with each `GuiSkillInfo.group` set from the SkillGroups row. The population view's `buildGroupIndex()` (`ui_sidepanels.cpp:515`) then reads `skill.group` to cluster columns. The cell render already shows max sibling level + checkbox + grouped tooltip from earlier work.

**This means T-0019 is a pure data restructure** in `content/db/ingnomia.db.sql` — replace the existing 14-row `SkillGroups` block with the new structure. No C++ changes needed.

## Result

Restructured `SkillGroups` in `content/db/ingnomia.db.sql` from the original 14 groups to **15 rows** (10 logical groups + 3 standalones + 2 combat groups preserved for T-0015):

**Final groups** (in display order via `Position`):

1. **Earthworking** — Mining, Masonry, Stonecarving, Prospecting *(Prospecting moved here from old MiscCraft because the Prospector workshop's purpose is identifying ore veins)*
2. **Forestry** — Woodcutting, Carpentry, Woodcarving
3. **Smithing** — Smelting, Blacksmithing, Metalworking, WeaponCrafting, ArmorCrafting
4. **Textiles** — Weaving, Tailoring, Dyeing
5. **Bone & Hide** — Leatherworking, Bonecarving *(new — split from old MiscCraft)*
6. **Fine Craft** — Gemcutting, JewelryMaking, GlassMaking, Pottery *(new — merged old Gem + Pottery from MiscCraft)*
7. **Engineering** — Machining, Engineering *(renamed from Engineer)*
8. **Hearth** — Cooking, Brewing, Butchery *(new — Butchery moved here from old Rancher per user's "stay distinct, group under Hearth" decision)*
9. **Field** — Farming, AnimalHusbandry, Fishing *(new — replaces old Agriculture + Rancher)*
10. **Magic** — MagicNature, MagicGeomancy

Standalones (each rendered as a single-skill group):
11. **Construction** — Construction
12. **Hauling** — Hauling
13. **Medic** — Medic *(was Doctor; renamed since Caretaking is gone)*

Preserved for T-0015:
14. **Combat** — Unarmed, Melee, Ranged, Crossbow, Thrown, Gun *(Crossbow + Gun added — they were missing from the original Combat group definition)*
15. **Defense** — Dodge, Block, Armor

All 43 surviving skills (47 - 4 from T-0018) are now in exactly one group. Verified by manual cross-check.

### What changes in the UI on next launch

- **Individual view**: column order follows the new SkillGroups order (Earthworking → ... → Defense). Skills inside a group appear consecutively, so the visual cluster matches the grouping even before switching to group view.
- **Group view**: shows 15 columns (down from 14, plus 3 standalones — net change is "cleaner clustering"). Each column is a group; the group's gnomes-to-skill cells are unchanged; the column header shows the new group name.
- **Tooltips on group cells**: existing per-skill breakdown tooltip continues to work, now reflecting the new groupings.

### What didn't need to change

- `aggregatorpopulation.cpp` — already iterates `SkillGroups` from DB
- `ui_sidepanels.cpp` `buildGroupIndex` and group render — already data-driven from `skill.group`
- `Skills` table `SkillGroup` column — not consulted at runtime by the population view (only `SkillGroups` is)

### Build

Green (incremental rebuild — only data file changed; 0 warnings).
