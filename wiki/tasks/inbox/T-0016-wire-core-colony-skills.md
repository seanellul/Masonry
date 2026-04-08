---
id: T-0016
title: Wire 5 core colony skills to actually affect job outcomes
type: feature
created: 2026-04-07
blockers: []
tags: [skills, gameplay, balance]
---

## Description

The single biggest gameplay improvement available from the T-0008a skills audit. Five "core" colony skills currently have **no effect on the work they're named for** — they only generate mood thoughts in `Gnome::tickProduction()`. A gnome with Mining 20 mines at the same speed and yields the same ore as a gnome with Mining 0.

This task wires each of these five skills to actually affect the relevant job's outcome.

### The 5 skills (after Horticulture removal + Caretaking merge per T-0018)

| Skill | Should affect | Read site to add |
|---|---|---|
| **Mining** | Mining speed, ore quantity per swing, rare-ore detection chance | The mine job task handler — find via `m_taskFunctions.insert("Mine", …)` in `gnome.cpp` |
| **Woodcutting** | Felling speed, log count per tree | `Gnome::fellTree()` task handler |
| **Farming** | Tilling speed, planting speed, harvest yield, possibly crop quality | `Farm::onTick`, farming job handlers in `canwork.cpp` |
| **Construction** | Build job speed, possibly durability/quality of constructed walls/floors | `BuildWall`/`BuildFloor`/etc. job handlers in `canwork.cpp` |
| **Medic** *(post-merge with Caretaking)* | Heal rate, treatment success, infection chance | Medical job handlers — need to find where wounds are treated |

### Suggested formula pattern

Reuse the existing `CanWork::craft()` quality scaling pattern as a template — `getSkillLevel(requiredSkill) / 20.0` is the natural normalized 0–1 multiplier. Suggested per-skill effects:

- **Speed**: `duration_ticks = base * (1.5 - skill / 40.0)` → 50% faster at level 20, 50% slower at level 0
- **Yield**: `extra_yield_chance = skill / 40.0` → 50% chance for extra item at level 20
- **Quality** (where applicable): same formula as crafting (`qIndex = skill / 20.0 * qSize`)

These are starting points — actual numbers should be tuned during scoping after looking at job tick rates.

### Acceptance criteria

- A gnome with Mining 20 finishes a mine job measurably faster than a gnome with Mining 0 (verify via `mcp__ingnomia-test__run_ticks` + game state inspection).
- Same for Woodcutting on a fell-tree job.
- Same for Farming on tilling/planting/harvesting cycles.
- Same for Construction on a build job.
- Same for Medic on a treatment job (if a save with wounded gnomes is available).
- Updated `wiki/dev/subsystems/skills.md` moves these 5 skills out of the "thought-only" tier into the appropriate effect tier with citations.
- Updated `$SkillDesc_*` Translation rows in `ingnomia.db.sql` — drop the "Currently only affects mood" prefix and describe the real effect.

### Out of scope

- Rebalancing existing crafting skill effects (the 22 quality-crafting skills are out of scope here).
- The combat skill refactor (T-0015).
- Wiring `AnimalHusbandry`, `Fishing`, `Butchery` — those are T-0017.

## Plan

*(Scoping agent: (1) For each of the 5 skills, find the corresponding job task handler in `src/game/`. (2) Identify the place where the job's tick rate or completion threshold is set. (3) Propose a `getSkillLevel(requiredSkill)` multiplier formula and where to insert it. (4) For Mining/Woodcutting/Farming, also consider yield — find the `inv->createItem` call inside the task handler and add a chance-based extra-yield branch. (5) For Construction, decide whether quality is meaningful (constructed walls don't currently have a quality concept — may want to defer that aspect). (6) Verify save-format compatibility — the changes are pure consumption of existing skill data, no new fields. (7) Build a small reproducible test plan for each: load a save, set skill to 0 vs 20, run N ticks, measure outcome.)*

## Result

*(Building agent fills in.)*
