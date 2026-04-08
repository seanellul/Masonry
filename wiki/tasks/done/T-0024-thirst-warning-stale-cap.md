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

Implemented Option A — `addThought` now refreshes the soonest-to-expire existing instance instead of refusing to re-add when the cap is hit.

### Code

`src/game/gnome.cpp` `Gnome::addThought` (around line 689): the previous early-return on `stacks >= maxStacks` now scans the existing thoughts for matching instances, picks the one with the lowest `ticksLeft`, and resets its `ticksLeft` to the new `durationTicks`. This is a 15-line block change that benefits **every capped thought** in the game, not just thirst.

```cpp
if ( stacks >= maxStacks )
{
    int minIdx = -1;
    int minTicks = INT_MAX;
    for ( int i = 0; i < m_thoughts.size(); ++i )
    {
        if ( m_thoughts[i].id == id && m_thoughts[i].ticksLeft < minTicks )
        {
            minTicks = m_thoughts[i].ticksLeft;
            minIdx = i;
        }
    }
    if ( minIdx >= 0 )
    {
        m_thoughts[minIdx].ticksLeft = durationTicks;
    }
    return;
}
```

### Behavior change

Before:
1. Gnome's thirst hits < 5 → "Dying of thirst" thought added with 600 ticks remaining
2. After 600 ticks → thought naturally expires and is removed
3. Gnome still at thirst < 5, addThought called again → returns early because the cap is 1
4. **Visible warning permanently gone** even though gnome is still dying

After:
1. Gnome's thirst hits < 5 → "Dying of thirst" thought added with 600 ticks remaining
2. addThought called again next frame → cap hit, refresh soonest-expiring instance to 600 ticks
3. Thought stays visible indefinitely as long as the gnome is in the critical state
4. As soon as thirst rises above the threshold, addThought stops being called, the existing thought naturally expires after its remaining duration, and the warning fades out — the desired UX

### Why every capped thought benefits

The fix is in the shared chokepoint. So:
- "Starving" stays visible while hunger < threshold (whatever that threshold is)
- "Exhausted" stays visible while sleep < threshold
- "Mental break" warnings stay visible
- Trait-driven thoughts that were previously aging out behind the cap now refresh

Side effect to watch: thoughts that **should** age out (e.g. "Productive day" capped at 5 stacks, intended to fade after a long-enough productive session) will now have their soonest-expiring instance refreshed if `tickProduction` keeps adding it. Most "transient" thoughts use higher max stacks (3–5) and re-add infrequently, so the impact should be minimal — but worth a watch in playtest.

### What this doesn't fix (deferred to a follow-up)

- The **death threshold confusion**: "0% thirst" still doesn't read as "halfway to death" because the player has no visual indicator of the -100 → +150 range. A red zone on the thirst bar from 0% to 50% (mapping to -100 → 0) would communicate the actual danger zone. Filed as a UI improvement candidate, not part of this fix.
- A separate **critical-need event/alert** (top-of-screen toast when a gnome enters a life-threatening state). Mood thoughts are still passive UI; an active alert is a stronger signal. Filed as a future enhancement.

### Build

Green. 45 warnings, all pre-existing.

