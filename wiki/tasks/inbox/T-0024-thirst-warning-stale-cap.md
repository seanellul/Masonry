---
id: T-0024
title: Thirst (and hunger) warning thoughts decay out and never re-appear, hiding death progression
type: bug
created: 2026-04-07
blockers: []
tags: [bug, gameplay, mood, needs]
---

## Description

Reproduced live: a colony of gnomes hit very low thirst, displayed the "Dying of thirst" mood thought, then **the thought disappeared and they kept working** — without anyone drinking. They were not safe; they were silently progressing toward death without the warning UI.

### Why it happens

Two things compound:

1. **The death threshold for needs is `-100`, not `0`.** Needs travel from `+150` (well-hydrated) down to `-100` (dead), with `+50` as the neutral baseline. The "0% thirst" the player sees in the gnome info panel is actually only halfway from neutral to dead. From `gnome.cpp:1224-1238`:

   ```cpp
   if ( newVal <= -100.f )
   {
       die();
       log( "Died from thirst." );
   }
   ```

   This is correct gameplay (slow, dramatic death progression) but the player has no way to know `0%` ≠ "about to die".

2. **The warning thought has a stale-cap bug.** From `gnome.cpp:1271`:

   ```cpp
   if ( thirst < 5 ) addThought( "DyingOfThirst", "Dying of thirst", -10, 600, 1 );
   ```

   The `1` is `maxStacks=1`. The thought is added with a 600-tick duration. When the duration expires, the thought is cleaned up. But because the cap is 1, **`addThought` won't add a new instance even if the gnome is still in the critical thirst range**. The player's visible warning vanishes while the gnome is still actively dying. Same code shape applies to hunger and other critical needs — verify and fix all of them.

### Compounded effect

The two bugs combine into a "looks fine, dies offscreen" failure mode:
- T+0: thirst drops to <5, "Dying of thirst" thought appears
- T+600 ticks: thought decays out, UI shows no warning
- T+thousands of ticks: thirst keeps dropping invisibly, eventually reaches -100
- Gnome dies. Player has no idea why.

The user's reported scenario didn't get all the way to death (probably because the gnomes eventually found water in passing) but the *visible warning disappearing* is the alarming part.

### Acceptance criteria

- A gnome with thirst < 5 has the "Dying of thirst" thought visible **continuously** until either thirst rises above the threshold OR they die.
- Same fix applied to other critical-need thoughts (hunger "Starving"; sleep "Exhausted"; verify the full set).
- A new `events`-style alert (top-of-screen toast or similar) fires when a gnome enters a critical need state, and again when they recover or die. The mood thought is necessary but not sufficient — the player needs an active alert for life-threatening states.
- (Stretch) The gnome info panel's "Thirst: X%" display indicates the actual death threshold visually — perhaps a red zone on the bar from -100 to 0 mapped to "0% to 50%" of the bar so the player can see how much real headroom remains.

### Two implementation options

**Option A — fix `addThought` to refresh existing thoughts** (preferred):
- When `addThought` is called with a thought ID that already exists (and the cap is reached), reset the existing instance's duration to the new value instead of refusing.
- This is a one-function fix and benefits every capped thought, not just thirst.
- Risk: might unintentionally extend transient thoughts that were *meant* to age out (e.g. "Working" mood thoughts that should fade).

**Option B — special-case critical need thoughts**:
- Bypass the maxStacks cap for a specific list of thought IDs (`DyingOfThirst`, `Starving`, etc.).
- Less risk of side effects on other thoughts.
- More code surface to maintain.

Recommend Option A unless scoping finds a concrete reason it breaks transient thoughts.

### Out of scope

- Rebalancing the death threshold itself (-100 may be intentional design — slow death is a feature).
- Auto-pause-on-critical-need (could be a separate option).
- Reworking the entire thought system.

## Plan

*(Scoping agent: (1) Read `Gnome::addThought` to understand the current cap behavior. (2) Check whether the cap is global or per-thought-id. (3) Decide between Option A and Option B. (4) For the optional event/alert side: find the existing event toast system and wire a `CriticalNeed` event from `Gnome::tickProduction` when a gnome first enters a critical-need state, and a corresponding `RecoveredFromCriticalNeed` when they leave it. (5) Verify with a save where a gnome's thirst is forced to <5 — the thought should now persist indefinitely.)*

## Result

*(Building agent fills in.)*
