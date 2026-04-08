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

*(Building agent fills in.)*
