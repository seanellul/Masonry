---
title: Skills — effect audit
tags: [dev, subsystem, skills, audit]
status: current
last_updated: 2026-04-07
sources: [src/game/creature.cpp, src/game/canwork.cpp, src/game/gnome.cpp, content/db/ingnomia.db.sql]
---

# Skills — effect audit (T-0008a)

Authoritative answer to "what does each skill actually do in gameplay?" Every claim on this page is grounded in a specific source location. This page is the data source for the in-game skill tooltips (T-0008b).

## TL;DR

Masonry defines **47 skills** (see `Skills` table in `content/db/ingnomia.db.sql` starting near line 5000+). They split into **four tiers** of actual effect:

| Tier | Count | Description |
|---|---|---|
| **Hot-path** | 5 | Directly read by string in core simulation loops — real, measurable gameplay impact. |
| **Quality-crafting** | 23 | Listed in `Crafts.SkillID` and consumed by `CanWork::craft()` → scales the quality tier of produced items. |
| **Thought-only** | 10 | Never read for gameplay outcomes — only generate mood thoughts when a gnome works a job of that category. |
| **Dead** | 9 | Defined in DB and registered in `m_skillToInt`, but not read anywhere for gameplay effect. Some may still accumulate XP as the gnome works related jobs. |

## Tier 1 — Hot-path (real effects)

These skills are consulted by string in the core simulation and have direct, observable gameplay effects.

| Skill | Effect | Source |
|---|---|---|
| **Hauling** | Move speed — the gnome's base `m_moveSpeed` is set by a SQL query against the `MoveSpeed` table keyed on the Hauling skill level. Higher Hauling = faster movement for every job, not just hauling. | `src/game/gnome.cpp:1870` in `Gnome::updateMoveSpeed()` |
| **Unarmed** | Attack skill used for unarmed combat and for the base attack value when wielding a weapon. | `src/game/canwork.cpp:616`, `src/game/creature.cpp:1113`, `src/game/gnome.cpp:182`, `src/game/monster.cpp:325/527` |
| **Melee** | Left/right hand weapon attack skill. Used both for attacks and for target-preference heuristics (AI prefers higher-Melee targets). | `src/game/canwork.cpp:587/607`, `src/game/gnomeactions.cpp:2280` |
| **Dodge** | Defensive combat roll — consulted when a gnome, monster, or animal is attacked. Higher Dodge = higher chance to avoid hits. Also used in target-preference. | `src/game/animal.cpp:1603`, `src/game/gnome.cpp:1959`, `src/game/monster.cpp:436`, `src/game/gnomeactions.cpp:2284` |
| **MagicNature / MagicGeomancy** | Spell radius and effect magnitude — when a spell's DB-configured radius is `HalfSkill` or `Skill`, the actual radius is computed from the caster's required skill. PlantGrowth spell effect duration also scales with the required skill. | `src/game/canwork.cpp:1370/1374/1454` |

## Tier 2 — Quality-crafting (indirect effects via `CanWork::craft()`)

All skills listed in the `Crafts.SkillID` column. A job's `requiredSkill` is set from the recipe's `SkillID`; `CanWork::craft()` reads the gnome's level in that skill and computes a quality tier for the produced item:

```cpp
// src/game/canwork.cpp:1147
float skillLevel = getSkillLevel( m_job->requiredSkill() );
int qSize = DB::numRows( "Quality" );
int qIndex = skillLevel / 20. * qSize;
// ... ± 1 tier on a die roll ...
g->inv()->setQuality( itemID, qIndex );
```

So higher skill = higher quality output on every item this skill crafts. The effect is silent in the UI but durable in value (quality affects trade price, gifts, and room value).

**The 23 crafting skills** (verified by cross-referencing every distinct `SkillID` in `Crafts` INSERT rows):

`ArmorCrafting`, `Blacksmithing`, `Bonecarving`, `Brewing`, `Carpentry`, `Cooking`, `Dyeing`, `Engineering`, `Gemcutting`, `GlassMaking`, `JewelryMaking`, `Leatherworking`, `Machining`, `Masonry`, `Metalworking`, `Pottery`, `Prospecting`, `Smelting`, `Stonecarving`, `Tailoring`, `WeaponCrafting`, `Weaving`, `Woodcarving`

## Tier 3 — Thought-only

These skills are referenced **only** inside `Gnome::tickProduction()` / work-mood logic in `src/game/gnome.cpp` (~line 1285 onward). They check `m_job->requiredSkill()` against a hardcoded skill name and add a mood thought. They do **not** affect job speed, yield, accuracy, or any other observable outcome.

| Skill | Thought generated | Source |
|---|---|---|
| **Mining** | MiningExcitement / MiningWork | `gnome.cpp:1318` |
| **Farming** | FarmingWork / FarmerGourmand | `gnome.cpp:1326` |
| **Horticulture** | FarmingWork / FarmerGourmand | `gnome.cpp:1326` |
| **Woodcutting** | WoodcuttingWork | `gnome.cpp:1356` |
| **Construction** | BuildingWork | `gnome.cpp:1340` |
| **Tinkering** | EngineerWork / TinkeringJoy | `gnome.cpp:1344` |
| **Medic** | HealingOthers / MedicalDuty | `gnome.cpp:1360` |
| **Caretaking** | HealingOthers / MedicalDuty | `gnome.cpp:1360` |
| **Ranged, Crossbow, Block** | EnjoysCombatTraining / DreadsCombat (during training only) | `gnome.cpp:1308` |

Gnomes with a high or low personality trait (industriousness, curiosity, bravery, appetite, empathy) get a bonus or malus mood thought when working these jobs — but skill level itself does nothing.

## Tier 4 — Dead (no consumer)

These skills are defined in `content/db/ingnomia.db.sql` (and most are registered in `JobManager::m_skillToInt`), but **no code reads their level** for any gameplay outcome. They are effectively decoration.

| Skill | Status |
|---|---|
| **AnimalHusbandry** | Set as a job's `requiredSkill` in animal handling code but the level is never consulted — pasture/animal jobs succeed or fail independent of skill. |
| **Butchery** | No level consumer found. |
| **Fishing** | Set as a job's `requiredSkill` for fishery jobs but the level is not read in any fishing handler. |
| **Mechanic** | Registered in `SkillToInt` but no code path reads it. |
| **Thrown** | Registered only. Ranged combat does not consult it — gnomes use Unarmed/Melee only. |
| **Gun** | Registered only. No firearm combat code exists. |
| **Armor** | Monster `m_armorValue` is a stat, not this skill. No code reads the gnome's Armor skill. |
| **MagicNature** / **MagicGeomancy** | *(Listed in Tier 1 — both have real effects via spell scaling. Listed here only as a reminder that other magic variants are not defined.)* |

**Also worth noting**: `Ranged`, `Crossbow`, `Block` technically live in both Tier 3 (thought) and Tier 4 (dead for actual combat) — they only matter during combat training mood, never during actual combat.

## Storage and XP

- Stored on the creature as `QMap<QString, QVariant> m_skills` where the value is raw XP. `getSkillLevel(id)` applies `Global::util->reverseFib(xp)` to convert XP to a level.
- `setSkillLevel(id, level)` writes a raw value directly (not Fib-encoded) — **this is inconsistent with the read path** and may be a bug depending on usage.
- XP award path: not audited in this pass. Most skills accumulate via generic job-completion hooks, but the exact grant function should be confirmed in a follow-up if skill gain balance becomes an issue.

## Follow-up task seeds

These are the concrete bugs / gaps discovered during the audit. Each becomes a candidate `wiki/tasks/inbox/` entry if you decide to act on them:

1. **Wire up `Fishing` skill level** to actually affect fishing output (catch rate, quality, rare fish chance). Currently decorative.
2. **Wire up `AnimalHusbandry`** to affect pasture productivity and animal taming success.
3. **Wire up `Butchery`** to affect yield (meat/leather/bones per carcass).
4. **Wire up `Ranged`, `Crossbow`, `Thrown`, `Gun`** or remove them — currently no ranged combat exists in the simulation despite the UI showing these skills.
5. **Wire up `Block`, `Armor`** or remove them — same as above for defensive skills.
6. **Wire up `Mining`, `Woodcutting`, `Farming`, `Horticulture`, `Construction`, `Medic`, `Caretaking`** to affect job outcomes (speed, yield, success rate) — these currently only change mood. This is the biggest gap: seven "core" colony skills have no effect on the work they're named for.
7. **Check `setSkillLevel` vs `getSkillLevel` encoding consistency** — `getSkillLevel` applies `reverseFib`, `setSkillLevel` does not. If both paths are used on the same skill this will produce inconsistent values.

## See also
- [[known-issues]]
- Combat system audit: *(not yet written)*
- `canwork.cpp` — central work/crafting loop
- `gnome.cpp` — thought generation, move speed, equipment
- `src/base/db.cpp` — `Skills` and `Crafts` table loaders
