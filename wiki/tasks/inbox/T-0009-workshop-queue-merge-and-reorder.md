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

*(Scoping agent: (1) Find the workshop job/queue model — likely in `src/game/workshop.*` or similar. Identify where a new job is appended to the queue on a "Craft" click. (2) Add the merge check: compare the incoming job to `queue.back()` by recipe id + mode + material filter; if match, increment count; else push. (3) Find the queue rendering code in `src/gui/ui/` (likely the same file touched by T-0006) and add the two new buttons. (4) Add the reorder actions to the workshop model and wire the buttons. (5) Verify count decrement + row removal still work correctly when a merged row's last job finishes. (6) Think through save/load: confirm existing queue serialization handles a `count > 1` row identically to multiple rows — or, if the internal representation is still "N separate jobs", the merge is purely at the enqueue boundary and each merged row is really N jobs under the hood. Pick whichever is minimally invasive.)*

## Result

*(Building agent fills in after implementation.)*
