---
id: T-0018
title: Skills cleanup — remove Horticulture, Tinkering, Mechanic; merge Caretaking into Medic
type: polish
created: 2026-04-07
blockers: []
tags: [skills, cleanup]
---

## Description

Four cleanup operations on the skills list, identified during the T-0008a audit and confirmed in the redesign discussion.

### 1. Remove `Horticulture`

**Why**: duplicate of Farming. The two are checked together in the only place either is referenced (`gnome.cpp:1326`'s mood trigger). Neither is in `Crafts.SkillID`. There's no separate horticulturist workshop or job type.

**Steps**:
- Delete the `INSERT INTO "Skills" ... 'Horticulture'` row from `content/db/ingnomia.db.sql`.
- Remove the `m_skillToInt.insert("Horticulture", ...)` line from `src/game/jobmanager.cpp`.
- Remove the `Horticulture` enum value if it exists (check `jobmanager.h` or wherever `SK_*` is defined).
- Remove the `if ( skill == "Farming" || skill == "Horticulture" )` mood check at `gnome.cpp:1326` — leave just `Farming`.
- Remove the `$SkillDesc_Horticulture` translation row.

### 2. Remove `Tinkering`

**Why**: no workshop, no recipes, no system. Only appears in `gnome.cpp:1344`'s mood trigger alongside Engineering and Machining (which both have real workshops). Tinkering was probably meant to be a "general handyman / fix-it" skill that never got built.

**Steps**:
- Delete the `Skills` row.
- Remove `m_skillToInt` entry.
- Remove the `Tinkering` arm of the `gnome.cpp:1344` mood check (Engineering and Machining stay).
- Remove `$SkillDesc_Tinkering`.

### 3. Remove `Mechanic`

**Why**: registered in `m_skillToInt` but no code reads it anywhere. Pure decoration.

**Steps**:
- Delete the `Skills` row.
- Remove `m_skillToInt` entry.
- Remove `$SkillDesc_Mechanic`.

### 4. Merge `Caretaking` into `Medic`

**Why**: functionally identical — both are referenced only in `gnome.cpp:1360`'s `if ( skill == "Medic" || skill == "Caretaking" )` mood trigger. Pick one canonical name; "Medic" is the more common term in colony sims.

**Steps**:
- Delete the `Caretaking` row from `Skills`.
- Remove the `Caretaking` `m_skillToInt` entry.
- Strip `|| skill == "Caretaking"` from the mood check, leaving just `Medic`.
- Remove `$SkillDesc_Caretaking`.
- T-0016 (core colony wirings) will be the task that wires the merged Medic to actually affect treatment rate.

### Save migration

`m_skills` is a `QMap<QString, QVariant>` keyed by skill ID. Removing a skill means saved gnomes will have orphan keys in their map. `getSkillLevel("Horticulture")` etc. will continue to return data for old saves, but no code will read those keys. **The orphans are harmless** — they take a few bytes each in serialization but are otherwise dead.

If desired, add a save-load migration that strips removed skill keys from `m_skills` on load. **Recommended but optional**: skipping it just means slightly bloated old saves, no functional issue.

### Acceptance criteria

- The four skills no longer appear in the Population view's individual Skills tab.
- The four skills no longer appear in the Settings → Keybindings tab (if any of them were bound — verify).
- A loaded save with old gnomes still loads cleanly (orphan skill keys ignored).
- Updated `wiki/dev/subsystems/skills.md` reflects the removals.
- `$SkillDesc_*` rows for the removed skills are deleted from `ingnomia.db.sql`.
- Build green; smoke test loads a save without crashing.

### Out of scope

- The combat skill removals (Ranged/Crossbow/Thrown/Gun/Block/Armor) — those are in T-0015's combat refactor.
- Wiring Medic to actually do something — that's T-0016.

## Plan

*(Scoping: (1) Find every reference to each of the four skills via grep. (2) Plan the deletion sequence. (3) Decide whether to ship the save-migration step. (4) Verify the Population view's Skills tab handles a missing skill gracefully — it iterates `gnome.skills` so it should, but confirm.)*

## Result

*(Building agent fills in.)*
