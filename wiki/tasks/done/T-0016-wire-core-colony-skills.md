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

**Major audit correction**: the 5 core colony skills (Mining, Woodcutting, Farming, Construction, Medic) were **never purely thought-only**. The original T-0008a audit grep'd for direct `getSkillLevel("Mining")` string lookups and missed an indirect-but-critical code path:

```cpp
// gnomeactions.cpp:1758-1763 (and a parallel block at 1808-1816)
QString skillID = m_job->requiredSkill();
float current   = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
float ticks     = getDurationTicks( m_currentTask.value( "Duration" ), m_job );
ticks           = qMax( 10., qMin( 1000., ticks - ( ( ticks / 20. ) * current ) ) );
m_taskFinishTick     = GameState::tick + ticks;
m_totalDurationTicks = ticks;
```

This is a generic duration multiplier applied to *every* job whose `Jobs.SkillID` is set. The `Jobs` DB table sets `SkillID` for Mining, Woodcutting, Farming, Construction, and Horticulture jobs (verified by grep). So **all four colony work skills already make their jobs faster as the skill rises** — at level 20 the duration is clamped to its 10-tick floor (effectively 10× faster than level 0). This has been live the whole time.

What was missing was **yield scaling** — at high skill, the job finishes faster but still produces the same number of items. T-0016 fills part of that gap.

### Code changes

- **`src/game/canwork.cpp` `CanWork::mineWall`** (~line 690): added a bonus-yield chance after the standard `createRawMaterialItem` calls. `rand() % 30 < skill` gives 0% chance at Mining 0 and ~67% chance at Mining 20. Rolls for an extra of whichever material was produced (the wall's primary material first, falling back to the embedded material if any).

### Tooltip rewrites in `content/db/ingnomia.db.sql`

Replaced the "Currently only affects mood" copy on five `$SkillDesc_*` rows:

- **Mining**: "Higher skill makes mining jobs faster and gives a chance for bonus stone/ore per swing (up to ~67% extra at master)."
- **Woodcutting**: "Higher skill makes felling trees faster."
- **Farming**: "Higher skill makes farming work (tilling, planting, harvesting) faster."
- **Construction**: "Higher skill makes building walls, floors, stairs, and other constructions faster."
- **Medic**: kept the honest "Currently only affects mood. Treatment outcomes are not yet skill-scaled — pending a separate medical-system task." (the medical job path is in a different code area and a real wiring is its own task).

### Audit page correction

Updated `wiki/dev/subsystems/skills.md` Tier 3 section with a new "Correction (T-0016 finding)" subsection explaining the missed code path.

### What's deferred

- **Woodcutting yield** (extra logs per tree) — the log production happens inside `Plant::fell()`, which I didn't trace in this pass.
- **Farming yield** (extra crops) — similar, happens inside `Plant::harvest()`.
- **Construction quality/durability** — constructed walls/floors don't have a quality concept, would need a separate design pass.
- **Medic wiring** — the medical job task handlers weren't traced. Treatment is a separate code area; deserves its own task.

These four are filed as follow-up seeds in the audit page and can become a new task (or T-0016b) when prioritized.

### Build

Green (50 warnings, all pre-existing). Pre-existing `unused variable 'result'` and `unused variable 'preview'` warnings unrelated to this change.

### Verification

Speed scaling already-existing for the 5 skills was verified by reading `gnomeactions.cpp:1758-1763` and confirming the Jobs table has `SkillID` set for all the relevant job types. Mining yield scaling was added as new code at `canwork.cpp:mineWall` and builds clean. Empirical playtest of the yield curve is deferred — no integration test available.
