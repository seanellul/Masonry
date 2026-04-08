---
id: T-0020
title: Cross-training XP bonus — siblings in the same skill group boost learning rate
type: feature
created: 2026-04-07
blockers: [T-0019]
tags: [skills, gameplay, xp]
---

## Description

A Master Bonecarver picking up Pottery should learn it noticeably faster than a complete novice. Their hands already understand precise crafting work; the transferable knowledge is real. This task implements that as a learning-rate multiplier on the XP grant function.

### The mechanic (one formula)

When a gnome gains XP in a skill, multiply the grant by:

```
xp_gain = base_xp * (1 + (max_sibling_level / 20.0) * 0.5)
```

Where `max_sibling_level` is the gnome's highest skill level among other skills in the **same group** (per T-0019's group structure).

- **No siblings (or all at level 0)** → multiplier = 1.0 (baseline)
- **One sibling at level 10** → multiplier = 1.25 (+25%)
- **One sibling at level 20** → multiplier = 1.5 (+50%, the cap)

### Why these numbers

- **Cap at +50%**: any higher and specialists stop mattering — every gnome converges into a generalist over time. Lower than ~+25% and the bonus is invisible to the player. +50% is "noticeably faster but specialists still mean something".
- **Use `max`, not `sum`**: a gnome with two siblings at level 10 gets the same bonus as one with one sibling at level 10. This prevents weird min-max stacking where filling a group becomes a race. The bonus rewards depth in any one sibling, not breadth across many.
- **Linear**: simpler to teach, simpler to debug, easier to balance later.

### Standalone skills (no group)

`Hauling`, `Construction`, `Medic` have no group and therefore no siblings → no cross-training bonus. This is intentional — they don't need it, and the meta-skills don't slot into a "school" the same way.

### Optional cross-group bonus (defer)

A smaller bonus could apply between unrelated groups too — a Master Smith picks up Stonecarving slightly faster than a complete novice because they understand tools and patience. **Out of scope for this task** — start without it, observe play, add later if needed.

### Implementation

The XP grant function isn't fully traced in the audit (`wiki/dev/subsystems/skills.md` "Storage and XP" notes this). Scoping needs to find it first. Once found, the change is roughly:

```cpp
void Creature::gainSkillXP( const QString& skillID, int baseXp )
{
    int maxSibling = 0;
    if ( const SkillGroup* group = findGroupForSkill( skillID ) )
    {
        for ( const QString& sibling : group->skills )
        {
            if ( sibling == skillID ) continue;
            maxSibling = qMax( maxSibling, getSkillLevel( sibling ) );
        }
    }
    float multiplier = 1.0f + ( maxSibling / 20.0f ) * 0.5f;
    int actualXp = int( baseXp * multiplier );
    // ... existing XP storage update ...
}
```

The `SkillGroup` lookup table is shared with T-0019's group structure — single source of truth, defined once.

### Acceptance criteria

- A gnome with Bonecarving 20 gains Pottery XP at +50% the rate of a gnome with all sibling levels at 0. Verify via `run_ticks` and inspecting `getSkillXP` before and after.
- A gnome with no siblings has unchanged XP gain (no regression).
- Standalone skills (Hauling, Construction, Medic) have unchanged XP gain.
- Updated `wiki/dev/subsystems/skills.md` documents the cross-training rule.
- Updated `$SkillDesc_*` rows for crafting skills mention cross-training.

### Dependency on T-0019

This task uses the group structure defined in T-0019. If T-0019 ships first, the lookup table is reused. If T-0020 ships first by mistake, it'll need a temporary inline group definition that gets refactored when T-0019 lands.

### Out of scope

- Cross-group bonuses.
- Per-skill XP grant rate balancing (separate task).
- Passion-style "this gnome enjoys X" multipliers (separate concept).

## Plan

XP grant happens in **`CanWork::gainSkill`** in `src/game/canwork.cpp`. Two overloads:

- `gainSkill( QVariant skillGain, QSharedPointer<Job> job )` at line 392 — the work-driven path. Two branches: empty `skillGain` (default +1 to job's required skill) and `$Craft` / QVariantMap (lookup gain from `Crafts_SkillGain` table).
- `gainSkill( QString skillID, int gain )` at line 434 — direct skill grant by ID + amount, used by combat training in `gnomeactions.cpp:2291-2294`.

**Apply cross-training to the work-driven path only**. The combat training path is excluded because (a) it has its own deliberate training mechanic with trainer-vs-trainee level checks, and (b) combat skills are slated to become derived stats in T-0015 — no point complicating their XP grant now.

The sibling lookup is built from the `SkillGroups` DB table (the same source T-0019 restructured). Cached lazily on first use into a static `QHash<QString, QStringList> s_skillSiblings`.

## Result

Implemented in `src/game/canwork.cpp`.

### Helper additions

- Added `#include <QHash>`.
- Added file-scope static cache `s_skillSiblings` (`QHash<QString, QStringList>`) and `ensureSkillSiblingsLoaded()` that lazily populates it from `DB::selectRows("SkillGroups")`. For each `SkillGroups` row, the `SkillID` field is split on `|` and each member skill is mapped to a list of its siblings (excluding itself).
- Added `crossTrainingMultiplier(skillID, m_skills)` returning `1.0 + (max_sibling_level / 20.0) * 0.5`, capped at 1.5. Skills with no siblings (Construction, Hauling, Medic — the standalone groups) return 1.0 trivially because their sibling list is empty.

### Hook into `gainSkill( QVariant, Job )`

- **Empty skillGain branch (line 397)**: was `current = m_skills.value(skillID).toFloat() + 1`, now `current = ... + (1.0f * multiplier)`.
- **QVariantMap branch (line 428)**: was `current + gain`, now `current + gain * multiplier`.

Both branches compute the multiplier from the gnome's current `m_skills` map (which holds raw XP values keyed by skill ID).

### Behavior

- A complete novice with no siblings → multiplier 1.0 → unchanged baseline XP gain.
- A gnome with one sibling at level 10 → multiplier 1.25 → +25% XP gain on the new skill.
- A gnome with one sibling at level 20 (cap) → multiplier 1.5 → +50% XP gain (the cap).
- Standalone skills (Construction, Hauling, Medic) → no siblings → always 1.0.
- Combat training XP grants (the `gainSkill(string, int)` overload) are unchanged — they bypass the multiplier, deliberately, until T-0015.

### Performance note

`s_skillSiblings` is a tiny static map (~50 entries × small string lists) populated once at first XP grant. Each `crossTrainingMultiplier` call iterates the sibling list (max ~5 entries per group) and does a `reverseFib` per entry — negligible cost in the work loop.

### Build

Green. The pre-existing `unused variable 'result'` warning in `canwork.cpp:945` is unrelated and not from this change.

### Verification (pending real playtest)

The mechanic should be observable via `mcp__ingnomia-test__run_ticks`: set up a save with a gnome having Bonecarving 20 and Pottery 0, run a Pottery job for N ticks, observe Pottery XP at end — should be +50% above a baseline gnome with no Bone & Hide siblings. Defer the actual measurement to playtest.
