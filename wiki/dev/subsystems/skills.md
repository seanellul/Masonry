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

## Tier 3 — Thought-only (note: this tier was partially wrong — see correction below)

These skills are referenced inside `Gnome::tickProduction()` / work-mood logic in `src/game/gnome.cpp` (~line 1285 onward). They check `m_job->requiredSkill()` against a hardcoded skill name and add a mood thought. They do not appear in any direct `getSkillLevel("Mining")`-style lookup.

**Correction (T-0016 finding)**: the original audit missed an important code path. The work loop at `gnomeactions.cpp:1758-1763` reads `m_job->requiredSkill()` and applies a duration multiplier:

```cpp
ticks = qMax(10., qMin(1000., ticks - ((ticks / 20.) * current)));
```

So **any job whose `Jobs.SkillID` field is set automatically gets speed-scaling**. The `Jobs` table in `content/db/ingnomia.db.sql` has `SkillID` set on Mining, Woodcutting, Farming, Construction, Horticulture (and many others). That means **Mining, Woodcutting, Farming, and Construction were already speed-scaled all along** — they're not purely thought-only. They produce work faster as the skill goes up; what they were missing was *yield* / *quality* scaling on the produced items.

T-0016 added bonus yield to `CanWork::mineWall` (chance for an extra stone/ore at high Mining). Yield wiring for Woodcutting (logs) and Farming (crops) is deferred — both touch the `Plant` class internals. Construction does not currently have a "yield" concept beyond build success.

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

## Planned redesign (Apr 2026 — supersedes much of the audit above)

The audit's findings drove a follow-up design conversation that produced a concrete cleanup + grouping + cross-training spec. The list below is the **target state**. Tasks T-0015 through T-0021 implement it.

### Removals (4)

- **Horticulture** — duplicate of Farming. Remove.
- **Tinkering** — no workshop, no system, only a mood trigger. Remove.
- **Mechanic** — registered but no consumer. Remove.
- **Caretaking** — functionally identical to Medic. Merge into Medic.

### Skill grouping (10 groups + 3 standalones)

Visual UI optimization in the Population view, no gameplay change at the per-skill level. Each sub-skill keeps its independent XP and level. The group view shows one column per group with a checkbox + the gnome's max sibling-skill level; hover expands to a per-skill breakdown.

| Group | Sub-skills | Theme |
|---|---|---|
| **Earthworking** | Mining, Masonry, Stonecarving | Stone supply chain — gather, build, decorate |
| **Forestry** | Woodcutting, Carpentry, Woodcarving | Wood supply chain |
| **Smithing** | Smelting, Blacksmithing, Metalworking, WeaponCrafting, ArmorCrafting | Metal supply chain |
| **Textiles** | Weaving, Tailoring, Dyeing | Cloth supply chain |
| **Bone & Hide** | Leatherworking, Bonecarving | Animal byproducts |
| **Fine Craft** | Gemcutting, JewelryMaking, GlassMaking, Pottery | Small precious work |
| **Engineering** | Engineering, Machining | Mechanical |
| **Hearth** | Cooking, Brewing, **Butchery** | Food prep — butchery stays distinct (yields meats), grouped here |
| **Field** | Farming, AnimalHusbandry, Fishing | Food production |
| **Magic** | MagicNature, MagicGeomancy | Spellcasting |

Standalones (no group):
- **Hauling** — affects every job (move speed)
- **Construction** — meta build skill
- **Medic** — sole healing skill (post-merge with Caretaking)

Result: 47 skills → ~13 columns in the group view. Individual view (existing) still shows everything for surgical assignment.

### Cross-training XP bonus

When a gnome gains XP in a skill, the grant is multiplied by:

```
xp_gain = base_xp * (1 + (max sibling level / 20) * 0.5)
```

- Novice (no siblings) → multiplier 1.0 (normal)
- One sibling at level 20 → multiplier 1.5 (max +50%)
- Uses `max` not `sum` — depth in any one sibling rewards transfer, no min-max stacking
- Cap at +50% — specialists still matter, generalists are noticeably faster

A Master Bonecarver picking up Pottery learns it +50% faster than a complete novice. A Grandmaster Smith learning a new smithing branch ramps up quickly and meaningfully.

### Skill titles

Derived display label based on top sibling level in any group:

| Top sibling level | Title |
|---|---|
| 0–4 | Novice [Group] |
| 5–9 | Apprentice [Group] |
| 10–14 | Journeyman [Group] |
| 15–19 | Master [Group] |
| 20 | Grandmaster [Group] |

Examples: a gnome with Blacksmithing 18 displays as **Master Smith**. A gnome with Bonecarving 20 displays as **Grandmaster of Bone & Hide** (or sub-skill name — TBD in T-0021). A gnome with multiple level-15+ groups gets a polymath title.

Shown in the gnome info panel header and in the population view name column.

### Wirings (broken skills to actually consume their level)

Tracked under T-0016 (core colony) and T-0017 (easy wirings + Butchery).

| Skill | Should affect |
|---|---|
| Mining | Mining speed, ore yield, rare-ore chance |
| Woodcutting | Felling speed, log count per tree |
| Farming | Tilling/planting/harvest yield |
| Construction | Build speed |
| Medic | Heal rate, treatment outcome |
| AnimalHusbandry | One-line fix at `gnomeactions.cpp:2143` (taming duration); broader: breeding, yield |
| Fishing | Catch rate, fish quality |
| **Butchery** | **Yield % and quality of prepared meats** |

### Combat → stats refactor

Tracked under T-0015 (deferred until you brainstorm the CON/STR/DEX schema).

- **Keep as effect, recast as stat**: Melee → STR, Unarmed → STR, Dodge → DEX
- **Delete**: Ranged, Crossbow, Thrown, Gun, Block, Armor (all dead today)
- 9 skills → ~3 derived stats

### Headcount summary

| Bucket | Count |
|---|---|
| Crafting industries (working, keep) | 22 (Cooking + Butchery + the rest) |
| Core colony skills (broken, fix in T-0016) | 5 (post-merge: Mining, Woodcutting, Farming, Construction, Medic) |
| Easy wirings (T-0017) | 4 (Hauling done; AH + Fishing + Butchery pending) |
| Magic | 2 |
| Combat → stats (T-0015) | 9 → ~3 stats |
| Removals (T-0018) | 4 (Horticulture, Tinkering, Mechanic, Caretaking) |
| **Total today** | **47** |
| **Total after cleanup** | **~36 skills + ~3 combat stats** |

## See also
- [[known-issues]]
- Combat system audit: *(not yet written)*
- `canwork.cpp` — central work/crafting loop
- `gnome.cpp` — thought generation, move speed, equipment
- `src/base/db.cpp` — `Skills` and `Crafts` table loaders
- Tasks: T-0015 (combat refactor), T-0016 (core colony wirings), T-0017 (easy wirings + Butchery), T-0018 (cleanup removals), T-0019 (skill grouping UI), T-0020 (cross-training XP bonus), T-0021 (skill titles)
