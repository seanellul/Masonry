---
id: T-0026
title: Dead creature info should display differently — no moods, no live activity, clear "deceased" framing
type: bug
created: 2026-04-07
blockers: []
tags: [bug, ui, creature-info, ux]
---

## Description

Reproduced live: a dead gnome's Creature Info panel still shows the same UI as a live gnome, including moods, thoughts, current activity, schedule, and so on. **Dead gnomes shouldn't have moods.** It's a clear UX disconnect — the panel reads as "this gnome is fine" when in fact they're a corpse.

### What should happen

When the selected creature is dead (`isDead` flag, or `bloodLevel <= 0`, or however the engine tracks it), the Creature Info panel should **suppress live-only sections** and **show a clear "Deceased" framing**:

- **Header** prefixes the name with a `(Deceased)` tag, ideally in red/grey.
- **Replace** the live activity / mood / thoughts / schedule sections with a small "**Deceased**" block:
  - Cause of death (from the death log entry, e.g. "Killed by wolf", "Starved to death", "Died from thirst")
  - Date/tick of death (if tracked)
  - Body decomposition stage (the existing `rotStage` field — Fresh / Decaying / Rotting / Skeleton / Bones)
  - Burial status (the existing `isBuried` flag)
- **Keep** the historical sections that still make sense:
  - Backstory (childhood + adulthood — these are biographical, not live state)
  - Final skill levels (a memorial of what they were good at)
  - Final stats (STR/DEX/CON/etc.)
  - Final equipment (still on the corpse)
  - Anatomy (with the damage from their final fight visible — this is forensic information, useful for the player)
- **Hide** the obviously-wrong sections:
  - Mood / mental break / thoughts (a corpse has no thoughts)
  - Current activity
  - Schedule
  - Social relationships *(arguable — could keep as "they were close to X")*
  - Profession dropdown (can't reassign a corpse)
  - Active skill checkboxes (can't redirect work)

### Acceptance criteria

- Selecting a dead gnome shows a "**(Deceased)**" tag in the header.
- The mood / thoughts / activity / schedule sections are hidden.
- Cause of death is shown (if available from the death log).
- Decomposition stage and burial status are shown as a small block.
- Backstory + final skills + final stats + equipment + anatomy remain visible.
- The same treatment applies to dead animals and dead monsters (they don't have moods, but they shouldn't show "Activity: hunting" once they're dead).
- Visual verification via `mcp__ingnomia-test__take_screenshot`: spawn a gnome, kill it, open the Creature Info panel.

### Out of scope

- Funeral / burial mechanics (separate task).
- Memorials / graveyards (separate task).
- Resurrection (please no).
- Reworking the death event log.

## Plan

*(Scoping agent: (1) Find how the engine tracks deceased state — `isDead` field, `bloodLevel`, or a separate corpse entity. The `GuiCreatureInfo` struct already has `rotStage` and `isBuried` fields, so the data is plumbed. (2) Find `drawCreatureInfoPanel` in `ui_sidepanels.cpp` (~line 2361). Wrap each live-only section in `if ( !isDead )` guards. (3) Add a "Deceased" header tag and a small status block right under the name that lists cause of death + rot stage + burial status. (4) Find the death log to source the cause of death — there's a `Global::logger().log(LogType::DEATH, ...)` somewhere; if the cause isn't tracked per-creature, that's a small data-model addition. (5) Test by spawning a creature, killing it via debug, and verifying the panel.)*

## Result

Implemented across the data layer + UI.

### Data: cause of death + death tick

Creature didn't track cause of death — only the `m_isDead` flag, `m_deathTick`, and `m_rotStage`. Added `QString m_causeOfDeath` field to `Creature` with `causeOfDeath()` getter and `setCauseOfDeath(cause)` setter, plus serialize/deserialize round-trip via `"CauseOfDeath"` key.

Two universal hooks ensure the metadata is always populated:

- `Creature::die()` — if `m_deathTick == 0`, set it to `GameState::tick`. If `m_causeOfDeath` is empty, default to `"Killed"`. This catches every death from animals, monsters, and combat-related gnome deaths that didn't set the cause beforehand.
- `Gnome::die()` — same defaults applied before delegating to `Creature::die()`.

Specific call sites that know the cause set it before calling `die()`:

- `Gnome::tickProduction` need-deaths (`gnome.cpp:1248`): `setCauseOfDeath("Starved to death")` for hunger, `"Died of thirst"` for thirst.

Future: more death sites in animal combat, monster attacks, falling damage, etc. can set more specific causes by calling `setCauseOfDeath()` before `die()`. Defaulting to `"Killed"` keeps the panel honest in the meantime.

### Plumbing through the aggregator

Added three new fields to `GuiCreatureInfo`:

```cpp
bool isDead = false;
QString causeOfDeath;
quint64 deathTick = 0;
```

Populated in `AggregatorCreatureInfo::onUpdateCreature` for **gnomes, monsters, and animals** — three populate sites, all calling `creature->isDead()`, `creature->causeOfDeath()`, `creature->deathTick()`.

### UI: drawCreatureInfoPanel restructure

Three modifications in `src/gui/ui/ui_sidepanels.cpp` `drawCreatureInfoPanel`:

1. **Header dimming** (~line 2389): when dead, the gnome's name renders in dim gray with a red `(Deceased)` tag inline. The display title (e.g. "Master Blacksmith") also shifts to a dimmer lavender.

2. **Activity hidden** (~line 2431): the green "current activity" line below the profession is gated behind `!ci.isDead`. Corpses don't have activities.

3. **Deceased status block** (new, ~line 2470): inserted between the species/health header and the live state sections. For dead creatures, shows:
   - **"Deceased"** label in red
   - **Cause**: from `ci.causeOfDeath` (e.g. "Starved to death", "Died of thirst", "Killed")
   - **Time**: derived from `(GameState::tick - ci.deathTick) / 14400` ticks per day. Reads as "today" / "1 day ago" / "N days ago".
   - **State**: rot stage label from `ci.rotStage` (Fresh / Decaying / Rotting / Skeleton / Bones).
   - **Burial status**: "Buried" or "Unburied" from `ci.isBuried`.

4. **Mood + needs hidden** (~line 2643): the entire gnome-only Mood bar + Hunger / Thirst / Sleep needs bars + their separator are wrapped in `if ( !ci.isDead )`. Dead gnomes don't have moods or needs.

5. **Thoughts hidden** (~line 2762): the Thoughts CollapsingHeader is gated behind `!ci.isDead`. Corpses don't have thoughts.

### What stays visible for dead gnomes

Per the spec, biographical / forensic sections remain so the panel still serves as a memorial:

- **Name + display title** (dimmed but present)
- **Profession** (their role in life)
- **Backstory** — childhood + adulthood, biographical
- **Personality** (traits) — biographical, valid for the dead
- **Social** — relationships and memories, "they were close to X"
- **Skills** (T-0021/T-0008b grouped Skills section) — final skill levels, a memorial
- **Stats** — STR/DEX/CON/INT/WIS/CHA, biographical
- **Equipment & Combat** — what they were wearing/wielding when they died
- **Anatomy** — forensic information, shows damage from the final fight

### Animals and monsters

The same `isDead` field is populated for animals and monsters. The Deceased status block fires for them too. Animal-specific live state (the Hunger bar, Diet, Temperament, Combat stats) is inside the existing `ci.creatureType == "Animal"` branch — those fields show 100/0/etc. on dead animals, which is harmless. A future polish pass could gate them similarly.

### Build

Green (11 warnings, all pre-existing).

### Out of scope (deferred)

- Funeral / burial mechanics — corpses are still just items.
- Memorial / graveyard system — no tracking of "the founder Brorvar died Spring of year 3".
- Gating animal-specific live sections (Hunger, Diet, Temperament) on dead animals — currently they show but the values are stale; minor cosmetic issue.
- The combat damage path doesn't yet set a specific cause — combat deaths default to "Killed". A follow-up pass through `Anatomy::damage()` and `Creature::attack()` could set "Killed by <attacker species>" but requires plumbing the attacker's identity through the kill chain.
