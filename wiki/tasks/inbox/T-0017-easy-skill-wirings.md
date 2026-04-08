---
id: T-0017
title: Easy skill wirings — AnimalHusbandry, Fishing, Butchery
type: feature
created: 2026-04-07
blockers: []
tags: [skills, gameplay]
---

## Description

Three skill wirings small enough to do together. Each has an existing job path; the skill level just needs to be consulted at one or two read sites.

### 1. AnimalHusbandry — one-line fix

**Status today**: taming code already exists at `Gnome::actionTameAnimal` in `src/game/gnomeactions.cpp:~2120`. Registered as the `"TameAnimal"` behavior in `gnome.cpp:595`. The function already extracts the gnome's skill level via `Global::util->reverseFib(m_skills.value(skillID).toUInt())` into a local `current` variable — and **then never uses it**:

```cpp
QString skillID      = m_job->requiredSkill();
float current        = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
m_totalDurationTicks = 100;       // ← hardcoded
m_taskFinishTick     = GameState::tick + 100;  // ← hardcoded
```

**Fix**: replace the hardcoded duration with a formula using `current`. Suggested:

```cpp
int duration = qMax( 20, 200 - int( current * 9 ) );
m_totalDurationTicks = duration;
m_taskFinishTick     = GameState::tick + duration;
```

A complete novice (level 0) takes 200 ticks; a master (level 20) takes 20 ticks. Real gameplay difference, literally one block of code.

**Eventual extensions** (out of scope here, file as separate tasks): breeding rate, wool/milk/egg yield bonuses.

### 2. Fishing — single read site

**Status today**: `setRequiredSkill("Fishing")` is set on Fishery jobs at `src/game/workshop.cpp:987`. The fishing job handler exists but doesn't consult the level for any outcome.

**Fix**: find the fishing task handler in `canwork.cpp` (likely a `catchFish()` or similar) and add level-based scaling for:
- **Catch rate**: chance per tick of producing a fish
- **Fish quality**: use the existing `qIndex = skillLevel / 20.0 * qSize` pattern from `CanWork::craft()`
- **Optional**: rare fish chance scales with skill

### 3. Butchery — wire to yield % and quality (per user's request)

**Status today**: dead. The Butcher workshop has recipes with `SkillID = 'Butchery'`, but the level isn't read in any butchering-specific code path. The skill stays as a distinct skill (not merged into Cooking) and groups under **Hearth** alongside Cooking and Brewing.

**Fix**: find where butchering produces meat/leather/bone items from a carcass — likely in the Butcher workshop's recipe processing or in `CanWork::craft()` when called for a Butcher job. Add:
- **Yield %**: instead of always producing 100% of recipe output, scale by `yieldFraction = 0.5 + skillLevel / 40.0` → 50% at level 0, 100% at level 20. A novice butcher wastes half the carcass.
- **Quality**: prepared meats (sausages, smoked meat) get the standard `qIndex` quality scaling. This is partially already there via the generic `craft()` path; verify it applies to butchering recipes too.

### Acceptance criteria

- AnimalHusbandry 0 vs 20: taming a captured animal at AH 20 finishes in roughly 1/10 the ticks of AH 0. Verify via `mcp__ingnomia-test__run_ticks`.
- Fishing 0 vs 20: a Fishery operated by skill 20 produces fish noticeably faster than skill 0.
- Butchery 0 vs 20: butchering the same carcass at skill 20 yields measurably more meat units than skill 0. Quality on prepared meats scales with skill.
- Updated `$SkillDesc_AnimalHusbandry`, `$SkillDesc_Fishing`, `$SkillDesc_Butchery` rows in `ingnomia.db.sql` — drop the "Currently tracked but has no effect" copy and describe the real behavior.
- Updated `wiki/dev/subsystems/skills.md` moves these 3 out of "dead" tier.

### Out of scope

- The 5 core colony skills (T-0016).
- Combat refactor (T-0015).
- Removals (T-0018).

## Plan

*(Scoping: (1) For AnimalHusbandry, the fix is the one-line replacement above; verify the formula by reading `actionTameAnimal` in full and confirming `current` is in the expected 0–20 range. (2) For Fishing, find the fishing task handler. (3) For Butchery, find where butchering recipes produce items — confirm whether they go through `CanWork::craft()` (which would already give quality scaling) or have their own path; if their own path, add the yield% formula there. (4) Confirm the Hearth group structure (Cooking, Brewing, Butchery) is reflected in T-0019's group definition before this lands.)*

## Result

*(Building agent fills in.)*
