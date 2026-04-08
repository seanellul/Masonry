---
id: T-0021
title: Skill titles — derived display label per gnome based on top group skill
type: feature
created: 2026-04-07
blockers: [T-0019]
tags: [ui, skills, flair]
---

## Description

Give every gnome a derived **title** that grows with their highest skill — "Apprentice Smith", "Master Bonecarver", "Grandmaster of Magic". Pure cosmetic flair, but it's the layer that **sells the whole grouped-skill design** to the player. Without titles the grouping is just a UI optimization. With titles, every gnome has a visible identity that grows as they specialize.

### Title tiers

Based on the gnome's **highest sibling level in any group** (per T-0019's group structure):

| Top sibling level | Tier prefix |
|---|---|
| 0–4 | Novice |
| 5–9 | Apprentice |
| 10–14 | Journeyman |
| 15–19 | Master |
| 20 | Grandmaster |

The group name attaches: `Apprentice Smith`, `Journeyman Forester`, `Master Bonecarver` (or `Master of Bone & Hide`), `Grandmaster of Magic`.

### Title format

**Two options** — pick during scoping:

**Option A: title from group name**
- `Apprentice Earthworker`, `Journeyman Forester`, `Master Smith`, `Grandmaster of Magic`
- Titles are clean and category-level. A gnome with Blacksmithing 18 is a "Master Smith" regardless of which sub-skill is the top one.

**Option B: title from sub-skill name**
- `Apprentice Stonecarver`, `Journeyman Carpenter`, `Master Blacksmith`, `Grandmaster Geomancer`
- Titles are more specific. A gnome with Blacksmithing 18 is a "Master Blacksmith"; a gnome with Metalworking 18 is a "Master Metalworker".

I'd recommend **Option B** — more flavorful, more identity per gnome, more variety in the population view.

### Polymath titles (stretch)

A gnome with **two or more groups at level 15+** could get a special compound title:
- `Master Smith & Stonecarver` (two masteries)
- `Master of Many Crafts` (three+)
- `The Renaissance Gnome` (five+ — extremely rare endgame)

This is pure flair but signals "this gnome is exceptional". Defer if it adds complexity.

### Standalone skills

`Hauling`, `Construction`, `Medic` have no group, but they should still produce titles:
- `Master Hauler` (Hauling ≥ 15)
- `Master Builder` (Construction ≥ 15)
- `Master Medic` (Medic ≥ 15)

### Where titles appear

- **Gnome creature info panel header**: under or beside the gnome's name. Recently touched in equipment-panel work.
- **Population view name column**: shown beside the name as a smaller subtitle, e.g.:
  ```
  Brorvar
  Master Smith
  ```
- **Tile Info → Creatures section**: when hovering or clicking a gnome on the map.

### Acceptance criteria

- Every gnome has a derived title visible in at least the gnome info panel header and the population view.
- Titles update live when a gnome's skills change.
- Standalone skill titles work (Master Hauler etc).
- Visual verification via `mcp__ingnomia-test__take_screenshot` of a populated colony showing varied titles.

### Implementation note

Title derivation is a pure function of the gnome's skills + the group structure from T-0019. No data-model changes, no save migration. Implement as a method on `Gnome` (e.g. `Gnome::displayTitle()`) that runs on demand or is cached on the `GuiGnomeInfo` aggregator output.

### Out of scope

- Custom titles set by the player.
- Title localization (use the existing `$SkillName_*` translation pattern if needed).
- Title-based gameplay effects (a "Master Smith" doesn't get any stat bonus from the title itself — the effect is the underlying skill level).

### Dependency on T-0019

Reuses the group structure. T-0019 must land first, or the title function inlines a temporary group definition that gets refactored.

## Plan

*(Scoping: (1) Pick Option A vs Option B for title format. (2) Decide whether to ship polymath titles in this task or defer. (3) Implement `Gnome::displayTitle()` (or equivalent) using the group structure. (4) Wire the title into the gnome info panel header, population view name cell, and tile info creatures section. (5) Verify the title updates when skills change.)*

## Result

*(Building agent fills in.)*
