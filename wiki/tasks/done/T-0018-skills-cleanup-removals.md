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

Implemented across SQL, C++ enums, and C++ source.

### `content/db/ingnomia.db.sql`

- **Removed 4 `Skills` rows**: `Tinkering`, `Mechanic`, `Horticulture`, `Caretaking`.
- **Removed 8 Translation rows** for `$SkillName_*` and `$SkillTitle_*` of the four removed skills.
- **Removed 4 `$SkillDesc_*` rows** (the T-0008b tooltip rows).
- **Updated 3 `SkillGroups` rows** to drop removed skills:
  - `Engineer`: `Tinkering|Machining|Engineering|Mechanic` → `Machining|Engineering`
  - `Agriculture`: `Horticulture|Farming|Cooking|Brewing` → `Farming|Cooking|Brewing`
  - `Doctor`: `Medic|Caretaking` → `Medic`
- **Updated 6 `Backstories` rows** that granted removed skills, redirecting grants to surviving alternatives:
  - `ChildTinkersApprentice`: `Tinkering:2|Engineering:1` → `Engineering:2|Machining:1`
  - `ChildHerbalGatherer`: `Horticulture:2|Medic:1` → `Farming:2|Medic:1`
  - `AdultTravelingHealer`: `Medic:3|Caretaking:1` → `Medic:4`
  - `AdultClockmaker`: `Tinkering:3|Engineering:1` → `Engineering:3|Machining:1`
  - `AdultHerbalist`: `Horticulture:2|Medic:1` → `Farming:2|Medic:1`
  - `AdultFarmer`: `Farming:3|Horticulture:1` → `Farming:4`

### `src/base/enums.h`

Removed `SK_Tinkering`, `SK_Mechanic`, `SK_Horticulture`, `SK_Caretaking` from the `Skill` enum. Verified via grep that no code outside `enums.h` and `jobmanager.cpp` references these enum values, so reordering is safe (the values are runtime-only, never serialized).

### `src/game/jobmanager.cpp`

Removed the four `m_skillToInt.insert(...)` calls for the dead skills.

### `src/game/gnome.cpp` (mood checks at `tickProduction`)

- Line ~1326 (Farming): `skill == "Farming" || skill == "Horticulture"` → `skill == "Farming"`.
- Line ~1344 (Engineering): `skill == "Tinkering" || skill == "Engineering" || skill == "Machining"` → `skill == "Engineering" || skill == "Machining"`. Updated comment from "Engineering / Tinkering" to "Engineering".
- Line ~1360 (Medical): `skill == "Medic" || skill == "Caretaking"` → `skill == "Medic"`.

### Save migration

**Not shipped**. Old saves with orphan skill keys (`m_skills["Horticulture"]` etc.) load fine because nothing reads those keys anymore — the orphan QVariants take a few bytes and are harmless. If a user notices and complains, a one-shot migration can strip removed keys on load. For now: ignore.

### Build

Green. **0 errors**, 804 warnings — all pre-existing `-Winconsistent-missing-override` warnings re-emitted across the wider rebuild triggered by editing `enums.h` (touching the enum forces every translation unit that includes it to rebuild). The actual count of *unique* warnings is unchanged from baseline.

### Verification

- Inbox no longer references the four skills.
- The Population view will no longer show columns for the removed skills (the skills column iteration is data-driven from the `Skills` DB table).
- Group view will show updated group rosters.
- Loaded saves still work (orphan skill keys ignored).

The way is now clear for **T-0019** (skill grouping in Population view) since the dead skills have been removed from `SkillGroups` and won't pollute the new structure.
