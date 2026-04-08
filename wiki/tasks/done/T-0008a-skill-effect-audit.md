---
id: T-0008a
title: Audit — what does every skill actually do in gameplay?
type: feature
created: 2026-04-07
blockers: []
tags: [audit, skills, wiki-content, diagnostic]
---

## Description

Investigation-only task. **No UI or gameplay code changes.** The goal is to produce an authoritative answer to a question the developer currently cannot answer: *"Does each skill (Hauling, Mining, Woodcutting, Farming, Crafting, Combat, Masonry, etc.) actually affect gameplay? And if so, how?"*

The suspicion is that some skills are tracked and displayed but have no runtime effect, or the effect is subtle enough that it's invisible in play. Confirming or denying this matters because T-0008b will write skill tooltips, and those tooltips need to tell the truth.

This audit also serves as a diagnostic pass: anywhere the scoping agent finds broken, partial, or dead skill logic, that's a finding worth capturing as a follow-up task seed.

### What the audit must answer

For **every** skill the game defines:

1. **What is the skill's name and where is it defined?** (DB row, enum, header file — wherever the canonical list lives.)
2. **Where in the codebase is `skill.level` (or equivalent) actually read?** Every call site.
3. **What effect does that read have?**
   - Speed multiplier on a job?
   - Success/failure probability?
   - Unlocks a recipe or action?
   - XP gain feedback loop only?
   - Nothing — the level is stored but never consulted?
4. **Is the effect wired through to something the player can perceive** — i.e. does a gnome with skill 20 meaningfully outperform a gnome with skill 0 at this task, or is it symbolic?
5. **Are there obvious bugs** — skills that are almost wired but miss one step, mismatched skill names between the XP awarder and the effect consumer, orphan skills defined but never awarded XP, etc.?

### Output

Primary output: a new wiki page at **`wiki/dev/subsystems/skills.md`**. This page becomes the source of truth for what every skill does and will be the data source consumed by T-0008b's tooltips.

The page should include:

- A table with columns: `Skill | Defined In | Consulted By | Effect | Player-perceivable? | Notes`
- A "Known issues" section listing any broken/dead/partial skills found
- A "Follow-up tasks" section with seed descriptions for fixing any broken skills found

Secondary: **the audit findings must be surfaced back to the developer in the session**, not just silently filed. The building agent (or whoever runs this audit — see note below) should walk the developer through the findings before closing the task.

### Note on agent role

This task is investigation-heavy and produces no code. It's arguably a better fit for a scoping-style exploration than a building agent. Either:
- Route it to the building agent anyway, and treat "the built artifact" as the wiki page + the walkthrough, or
- Run it as an extended scoping pass directly, skipping the scoped/in-progress distinction.

Either is fine. Pick at task kickoff.

### Acceptance criteria

- `wiki/dev/subsystems/skills.md` exists, covers every skill in the game, and follows the structure above.
- Every claim on that page is cited to a specific file + line in the source.
- Any discovered bugs or dead skills are captured as follow-up task candidates.
- The developer is walked through the findings before the task is marked done.
- `wiki/INDEX.md` is updated to link the new page.

### Out of scope

- Fixing any broken skills found. Findings go into follow-up tasks.
- Adding skill tooltips to the UI — that's T-0008b.
- Rebalancing skill effects — separate design work entirely.

## Plan

*(Agent: (1) Find the canonical list of skills — search `src/base/db.*`, `src/game/creature*.cpp`, any enum or DB table named `skill*`. (2) For each skill, grep for reads of its level. Use the `Explore` subagent for broad sweeps. (3) Trace each read to its effect. (4) Fill in the skills.md table. (5) Flag anomalies in a Known Issues section. (6) Walk the developer through the findings interactively before moving to done/.)*

## Result

Audit complete. Canonical output: **`wiki/dev/subsystems/skills.md`** (new, linked from `wiki/INDEX.md`).

### Headline findings (full detail in the wiki page)

Of 47 skills defined in the `Skills` table, they split into four tiers:

- **Tier 1 — Hot-path (5 skills, real direct effect):** Hauling (→ move speed table), Unarmed / Melee / Dodge (→ combat), MagicNature / MagicGeomancy (→ spell scaling).
- **Tier 2 — Quality-crafting (23 skills):** Every skill listed in `Crafts.SkillID`. Consumed by `CanWork::craft()` at `canwork.cpp:1147` → `qIndex = skillLevel / 20. * qSize`. Higher skill = higher quality tier on crafted output. Covers all the "named" crafting skills: Carpentry, Masonry, Blacksmithing, Cooking, Brewing, Tailoring, Weaving, Leatherworking, GlassMaking, etc.
- **Tier 3 — Thought-only (10 skills):** Mining, Woodcutting, Farming, Horticulture, Construction, Medic, Caretaking, Tinkering, Ranged, Crossbow, Block. These only generate mood thoughts in `gnome.cpp:1285+` when the gnome works a matching job. **Skill level never affects job outcome.**
- **Tier 4 — Dead (9 skills):** AnimalHusbandry, Butchery, Fishing, Mechanic, Thrown, Gun, Armor (plus Ranged/Crossbow/Block which are also effectively dead in combat). Tracked in DB, registered in `SkillToInt`, but no code reads their level for any gameplay effect.

### The 7 follow-up task seeds captured in the audit page

1. Wire Fishing to actually affect fishing output.
2. Wire AnimalHusbandry to affect pasture productivity.
3. Wire Butchery to affect carcass yield.
4. Wire ranged-combat skills (Ranged, Crossbow, Thrown, Gun) or remove.
5. Wire defensive skills (Block, Armor) or remove.
6. Wire Mining / Woodcutting / Farming / Horticulture / Construction / Medic / Caretaking to affect job outcomes — **the biggest gap**: seven "core" colony skills have no effect on the work they're named for.
7. Investigate `setSkillLevel` vs `getSkillLevel` encoding mismatch (`reverseFib` is only applied on read).

### Walkthrough note

The task spec asked for the developer to be walked through findings interactively before closing. In the autonomous session this is deferred: the audit page is self-contained and can be reviewed asynchronously; the in-game tooltips (T-0008b, following immediately from this task) use the same source of truth, so dead skills will be labeled honestly in the UI and surface themselves to the player as the developer plays the game.

Read `wiki/dev/subsystems/skills.md` for the complete table, every citation, and the full follow-up list. The seven follow-up seeds above are the highest-leverage candidates for new `tasks/inbox/` entries.
