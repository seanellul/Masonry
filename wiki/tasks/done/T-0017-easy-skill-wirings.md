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

All three wirings live in `src/game/canwork.cpp` and `src/game/gnomeactions.cpp`. Each has a tiny code change at a specific read site:

- **AnimalHusbandry** (`Gnome::actionTameAnimal` at `gnomeactions.cpp:2138`): the function already extracts the gnome's skill into `float current` via `reverseFib`. Just use it: `duration = qMax(20, 200 - int(current * 9))`.
- **Fishing** (`CanWork::fish` at `canwork.cpp:1405`): always produces 1 fish today. Add a `getSkillLevel("Fishing")` read and a bonus-catch chance check.
- **Butchery** (`CanWork::butcherFish` and `CanWork::butcherCorpse`, both around `canwork.cpp:1379`): both produce 1 meat + 1 bone per claimed item with no skill scaling and no quality. Add a shared helper that produces meat with skill-based quality (same `qIndex = skill / 20.0 * qSize` formula as `craft()`) plus a bonus-meat chance.

Hearth group (Cooking, Brewing, Butchery) was confirmed in T-0019's SkillGroups restructure — Butchery's group is correct before this task lands.

## Result

Three wirings, all in `src/game/`.

### AnimalHusbandry — `gnomeactions.cpp:2138` `Gnome::actionTameAnimal`

Replaced the hardcoded `m_totalDurationTicks = 100` with a skill-derived duration:

```cpp
QString skillID      = m_job->requiredSkill();
float current        = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
int duration         = qMax( 20, 200 - int( current * 9 ) );
m_totalDurationTicks = duration;
m_taskFinishTick     = GameState::tick + duration;
```

A novice (level 0) takes ~200 ticks per tame; a master (level 20) takes ~20 ticks. Linear scale.

### Fishing — `canwork.cpp:1405` `CanWork::fish`

Added a bonus-catch chance after the baseline fish is produced:

```cpp
int skill = getSkillLevel( "Fishing" );
if ( skill > 0 )
{
    int roll = rand() % 30;
    if ( roll < skill )
    {
        unsigned int extraID = g->inv()->createItem( m_job->posItemOutput(), "Fish", "GreenFish" );
        g->inv()->setMadeBy( extraID, id() );
    }
}
```

At skill 0: 0% chance for a bonus catch. At skill 20: ~67% chance. Skill XP gain happens via the existing work loop chokepoint at `canwork.cpp:337-338` (`gainSkill(sgv, m_job)` after task completion) and now also gets the T-0020 cross-training multiplier.

### Butchery — `canwork.cpp:1379-1402` `butcherFish` + `butcherCorpse`

Added a shared static helper `butcherProduceMeat` that:
- Computes a quality tier from the gnome's Butchery skill using the same `qIndex = skill / 20.0 * qSize` formula as `craft()`, with the same ±1 die-roll variance.
- Calls `setQuality` on the meat item.
- Has a bonus-meat chance: 0% at skill 0, ~67% at skill 20 (`rand() % 30 < skill`). The bonus meat gets the same quality as the primary.

Both `butcherFish` and `butcherCorpse` now read `getSkillLevel("Butchery")` once per task and call the helper instead of the bare `createItem`. The fish-bone / bone outputs are unchanged (no quality, no bonus — those aren't the user-facing "prepared meats" the user cares about).

### Tooltip updates

Updated `$SkillDesc_*` rows in `ingnomia.db.sql` to drop the "Currently tracked but has no gameplay effect" copy:

- AnimalHusbandry: "Determines how quickly this gnome tames animals. A novice takes ~200 ticks per tame; a master takes ~20."
- Butchery: "Higher skill produces higher quality meat from carcasses, and gives a chance for a bonus meat per kill (up to ~67% at master)."
- Fishing: "Higher skill gives a chance for a bonus catch per fishing trip (up to ~67% at master)."

### Build

Green. The 27 mission switch warnings + the unused `result` warning at line 945 are pre-existing in `gnomeactions.cpp` / `canwork.cpp`, unrelated to this change.

### Out-of-scope reminders

- **Breeding rate / animal yield** for AnimalHusbandry: deferred. The natural extension would be `Pasture::onTick` consulting the assigned shepherd's skill, but Pasture wasn't traced and is a separate task.
- **Catch quality** for Fishing: not implemented. `rand() % 30 < skill` only produces a yield bonus, not a quality bonus on the fish itself. Could be added later.
- **Cooking quality scaling** for the Hearth group's actual Cooking skill: already wired via `craft()` since Cooking is in `Crafts.SkillID`. No work needed there.
