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

*(Scoping: (1) Find the XP grant function — likely a `gainXP` / `addSkillXP` method on `Creature` or `Gnome`. The audit noted this wasn't traced; this is the first step. (2) Read every call site to confirm it's a single chokepoint. (3) Build the `SkillGroup` lookup table (shared with T-0019). (4) Add the multiplier. (5) Plan the verification — set a save's gnome to known sibling levels, run ticks on a job, check XP delta.)*

## Result

*(Building agent fills in.)*
