---
id: T-0025
title: Body part duplication on damage — dead gnomes have 4 torsos
type: bug
created: 2026-04-07
blockers: []
tags: [bug, anatomy, combat, data-corruption]
---

## Description

Reproduced live: when gnomes were attacked by animals and their torso was damaged, **another torso entry was added to their anatomy** instead of (or in addition to) the existing one being damaged. The user found dead gnomes with **4 torso entries** in the Creature Info anatomy section.

This is a data-corruption-shaped bug — every damage event to a body part is *creating* a new instance instead of mutating the existing one. So a gnome who took 3 torso hits ends up with 4 torsos (the original + 3 damaged copies), all listed in the anatomy panel.

### Acceptance criteria

- A gnome who takes N hits to their torso has exactly **1 torso** in their anatomy after combat (the original, with reduced HP).
- A gnome who takes hits to multiple parts (torso + arm + head) has the original anatomy structure, just with HP reductions on the affected parts.
- Existing dead gnomes in saves with duplicated body parts: either tolerated (they're dead, doesn't matter) or auto-cleaned on load (preferred — strip duplicate entries).
- Verified by spawning a hostile animal next to a gnome via the test controller, letting the gnome take damage, and checking the Creature Info anatomy section after each hit.

### Likely root cause (hypothesis — needs verification)

Somewhere in the damage application path (probably `Creature::attack` or `Anatomy::applyDamage` or similar), the code is calling something like `bodyParts.append(...)` instead of mutating the existing entry in place. Look for `append` / `push_back` / `insert` calls inside the damage handling code, vs the place where anatomy is initially constructed (which would be in `Gnome::init` or a creature factory).

Cross-reference: `GuiCreatureInfo` has a `BodyPartInfo` struct populated from the gnome's anatomy in the aggregator. The duplication happens in the simulation, not the UI — the UI is faithfully showing what's in the gnome's data.

### Out of scope

- Reworking the anatomy/damage system itself.
- Adding healing mechanics (separate task).
- Visualizing damage on the gnome sprite.

## Plan

*(Scoping agent: (1) Find the gnome anatomy initialization — likely in `Gnome::init()` or a body part loader from the DB. (2) Find the damage application code — `Creature::attack`, `bool attack(...)` in `gnome.cpp`/`creature.cpp`, or whatever the attack pipeline ends with. (3) Identify the call site that's appending to the body parts list instead of mutating in place. (4) Fix to mutate in place. (5) Add a save-load migration that strips duplicate body parts (same `name` field) on load — keep the lowest-HP instance since that's the "real" damaged one. (6) Test by spawning a hostile and watching a gnome's anatomy after damage.)*

## Result

*(Building agent fills in.)*
