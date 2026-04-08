---
id: T-0009
title: Workshop queue — auto-merge identical adjacent jobs + top/bottom reorder buttons
type: feature
created: 2026-04-07
blockers: []
tags: [ui, workshop, queue]
---

## Description

Two improvements to the workshop queue view:

### 1. Auto-merge identical jobs at enqueue time

Clicking **Craft** on a recipe multiple times currently creates multiple identical rows (e.g. five separate `Craft 1 x Plank (0 done)` entries). This is noisy and hard to read.

Instead: when enqueuing a new job, if the **last** entry in the queue matches the recipe being added, increment its count rather than pushing a new row. Five clicks on Plank = one row `Craft 5 x Plank (0 done)`.

**"Matching" definition** (all three must hold):
- Same recipe
- Same mode (`Craft N`, `Until N`, or `Repeat`)
- Same material filter (e.g. `any raw wood` vs `any oak` do **not** merge)

**Scope rules**:
- Only the *last* queue entry is checked. If the user manually enqueued a Chair between two Plank batches, the second Plank batch does **not** merge across the Chair — it starts a new row.
- Merging only applies to **newly enqueued** jobs. Existing saves are **not** collapsed retroactively; a loaded queue keeps whatever structure it had.

### 2. Send-to-top / send-to-bottom buttons

Each queue row currently has `^` (up one), `v` (down one), and `X` (delete). Add two more: **send to top** and **send to bottom**. Suggested icons: double-up and double-down chevrons (`⏫` / `⏬`), or whichever matches the icon library already in use.

The final row layout should be:
```
[send-to-top] [up] [down] [send-to-bottom] [delete]
```

### Acceptance criteria

- Clicking Craft five times on Plank produces one row `Craft 5 x Plank (0 done)`, not five rows.
- Clicking Craft on Plank, then Chair, then Plank again produces three distinct rows (no cross-merge).
- Changing material filter between clicks produces separate rows.
- Existing queued jobs on a loaded save are not collapsed.
- Send-to-top button on any row moves it to position 0.
- Send-to-bottom button on any row moves it to the last position.
- Send-to-top on the top row is a no-op; send-to-bottom on the bottom row is a no-op (don't crash, don't error).
- Count display updates correctly when the merged row's count decrements (as jobs complete).
- Decrementing the count (via the existing `-` / `+` or through completion) never underflows or orphans the row.
- Visual verification via `mcp__ingnomia-test__take_screenshot` on a workshop queue with mixed entries before/after reorder actions.

### Out of scope

- Retroactive merging on save load (explicitly deferred).
- Any other queue UX (drag-reorder, multi-select, etc.).
- Changing how Craft N / Until N / Repeat modes themselves behave.

## Plan

**Convenient finding**: `Workshop::moveJob()` at `src/game/workshop.cpp:532` already implements all four move commands — `Up`, `Down`, `Top`, `Bottom`. The UI side was only wiring `Up` / `Down`. No game-thread changes needed for reorder — just new UI buttons that dispatch the already-supported command strings via `cmdWorkshopCraftJobCommand`.

**Merge implementation** in `Workshop::addJob()` at `src/game/workshop.cpp:476`: before the final `m_jobList.append(cj)`, check whether `m_jobList.last()` matches the incoming `cj` on `craftID` + `mode` + same `materialSID` for every component slot. If so, increment the last entry's `numItemsToCraft` and return. Only the tail is checked — manually inserted jobs between batches break the streak on purpose (per task spec).

**Internal representation stays the same**: merged rows are real `CraftJob` entries with `numItemsToCraft > 1`. The existing `(%d done)` renderer and save serialization already handle this — no data-model surgery, no save migration.

## Result

Implemented in two files:

1. **`src/game/workshop.cpp` `Workshop::addJob()`**: added the merge check just before `m_jobList.append(cj)`. Compares the last queue entry to the incoming job on craftID + mode + per-component materialSID equality. On match, the last entry's `numItemsToCraft` is incremented and the new `cj` is discarded. Only the tail is checked.

2. **`src/gui/ui/ui_sidepanels.cpp` `drawWorkshopPanel()` queue row**: extended the `^` / `v` / `X` button row to `^^` / `^` / `v` / `vv` / `X` (send-to-top / up / down / send-to-bottom / cancel). The two new buttons dispatch the already-supported `"Top"` / `"Bottom"` command strings via `cmdWorkshopCraftJobCommand`. Every button now has a hover tooltip describing its action.

**Save-load behavior**: existing queues on loaded saves are untouched (the merge only runs in `addJob`, which is called on new enqueues). A loaded save's queue keeps whatever structure it had. Per spec.

**No data-model changes**, no new commands, no game-thread logic beyond the one tail-match check. Save format unchanged.

Build: green (48 warnings, all pre-existing).
