---
id: T-0015
title: Combat skills → stats refactor (deferred for design)
type: feature
created: 2026-04-07
blockers: []
tags: [skills, combat, refactor, design-pending]
---

## Description

Combat skills don't fit alongside job skills like Mining or Carpentry. The audit (T-0008a) found 9 combat-related skills, of which only 3 do anything (`Melee`, `Unarmed`, `Dodge`); the other 6 (`Ranged`, `Crossbow`, `Thrown`, `Gun`, `Block`, `Armor`) are decorative.

The user's preferred direction: **collapse combat skills into derived combat stats linked to gnome attributes** — e.g. `CON → health`, `STR → melee/unarmed`, `DEX → dodge`. Detail TBD in a separate brainstorm.

This task is **deferred until that brainstorm happens**. It's filed here so the work isn't lost.

### What this task will eventually do

- Define the new combat-stat schema (CON/STR/DEX or whatever the brainstorm produces).
- Replace `getSkillLevel("Melee")` / `Unarmed` / `Dodge` reads in `canwork.cpp:587-626`, `creature.cpp:1113`, `gnome.cpp:182/1959`, `monster.cpp:325/436/527`, `gnomeactions.cpp:2280/2284` with stat-derived equivalents.
- Delete the 6 dead combat skills entirely (`Ranged`, `Crossbow`, `Thrown`, `Gun`, `Block`, `Armor`) — strip from `Skills` DB table, `m_skillToInt`, `gnome.cpp:1308` mood check, and Population view rendering.
- Migrate save files: dropping a skill column from `m_skills` is safe because `getSkillLevel` returns -1 for missing keys, but verify no save-load path crashes.
- Update `wiki/dev/subsystems/skills.md` to reflect the new state.

### Dependencies

- **Brainstorm needed**: the gnome stat schema (CON/STR/DEX) doesn't exist yet. Need to design what attributes a gnome has, how they grow, and how each combat-relevant stat derives from them.
- Should be done **after** T-0019 (skill grouping) so the population view's group structure is in place — this task removes columns rather than rearranging them.

### Out of scope

- Reworking actual combat math (hit chance formulas, damage curves) — that's a separate combat-balance task.
- Adding ranged combat as a real system — currently no ranged code exists at all; that's also separate.
- Animal/monster combat skill paths (those use the same `getSkillLevel("Unarmed")` calls and would need the same migration).

## Plan

*(Scoping needed after the design brainstorm. At minimum: the new schema, the migration plan for existing saves, and a list of every read site that needs updating.)*

## Result

*(Building agent fills in.)*
