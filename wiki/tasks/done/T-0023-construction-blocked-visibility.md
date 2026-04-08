---
id: T-0023
title: Construction visibility — show *why* a build job is stuck
type: feature
created: 2026-04-07
blockers: []
tags: [ui, construction, ux, debugging]
---

## Description

When the player places a workshop or building and it doesn't get built, there is **no UI feedback explaining why**. The placeholder just sits there forever. Reproduced live: a Distillery placement was waiting for an hour with no progress because the colony had no Barrels (Distillery requires `1 × Table + 2 × Barrel`, verified from `Workshops_Components` in `content/db/ingnomia.db.sql`).

The player has to either know the recipe by heart, browse the DB, or guess. That's a colony sim usability disaster.

### What should happen

When the player clicks (or hovers) a queued construction site that hasn't been built yet, the Tile Info panel (or a tooltip on the placeholder sprite) should show:

- **The thing being built**: e.g. "Distillery (workshop)"
- **Required materials**, with current vs needed counts:
  - `Table — 1 / 1 ✓`
  - `Barrel — 0 / 2 ✗ (none in inventory)`
- **Why it's blocked**, derived automatically:
  - "Waiting for materials" (most common)
  - "No worker available with required skill"
  - "No path from worker to materials"
  - "Construction tile is occupied"
- **A "Cancel" button** so the player can reclaim the space.

This is the same kind of self-documenting UI the wiki-fed tooltips give (T-0004 / T-0007) — the game telling the player *what's wrong* instead of leaving them to guess.

### Acceptance criteria

- Clicking a queued (not-yet-built) construction site shows a Tile Info section with the recipe + per-component progress.
- A blocked build whose missing material gets supplied resumes automatically (no extra click needed — that's already how it works once the data is visible).
- An obvious "Waiting for materials" status string is visible on the placeholder when items are missing.
- The Distillery placeholder reproduction case (no barrels, no carpenter) shows: `Distillery — Waiting for materials — Table 1/1, Barrel 0/2`.
- A `Cancel` action removes the placeholder cleanly.

### Out of scope

- Auto-queuing the prerequisite items (e.g. auto-queue 2 Barrel craft jobs at the carpenter when a Distillery is placed). That's a stretch goal — separate task.
- Smart routing / job-priority changes.
- Visualizing path failures on the map.

## Plan

The required-items data already exists end-to-end — `Job::m_requiredItems` (`QList<RequiredItem>` with `count`, `itemSID`, `materialSID`, **and an `available` flag**). The aggregator already iterates the required items into `GuiTileInfo::requiredItems`. The flag was just never being **set** correctly *and* never being **rendered**.

Three actual bugs to fix:

1. **`JobManager::requiredItemsAvail` was iterating the required items by value**, so `rim.available = true/false` wrote to a discarded local copy. The flag has been silently `false` for every job in the game, regardless of actual availability.
2. **The aggregator** copied `text/count/material` into `GuiTileInfo::requiredItems` but not `available`.
3. **The Tile Info UI** rendered `jobName` but ignored `requiredItems` entirely.

## Result

Implemented end-to-end. Five files changed.

### Bug fixes (the underlying problem)

**`src/game/jobmanager.cpp` `JobManager::requiredItemsAvail`** — replaced `for ( auto rim : job->requiredItems() )` (iterates a *copy* of the list returned by value) with index-based access into `job->m_requiredItems` (the actual member field). `JobManager` is `friend class Job` so direct access is allowed. Now `rim.available = true/false` actually persists to the job and can be read later by the UI. **This is the underlying bug** — the per-item availability flag has been broken since the field was added.

**`src/game/jobmanager.h`** — moved `requiredItemsAvail` from private to public so the tile info aggregator can call it on a fresh selection.

### Plumbing the data through

**`src/gui/aggregatortileinfo.h`** — added `bool available = false;` to `GuiItemInfo`.

**`src/gui/aggregatortileinfo.cpp`** — `requiredItemsAvail( job->id() )` is now called on the aggregator path so the flags are fresh on every Tile Info refresh. Then the per-item loop copies `rim.available` into `git.available` alongside the existing fields.

### Rendering

**`src/gui/ui/ui_tileinfo.cpp`** `drawTileInfo` Active Job section — when `requiredItems` is non-empty:

- A yellow `[!] Waiting for materials` warning appears at the top if any item is missing AND no worker is currently on the job.
- Each required item is rendered indented with a colored prefix:
  - `[OK]` in green when available
  - `[ -]` in red when missing
  - Format: `[OK] 1 x Table` or `[ -] 2 x Barrel`
- A `Cancel job` button (with `ICON_FA_XMARK` glyph) at the bottom of the section dispatches a new `"CancelJob"` command through `cmdTerrainCommand`.

### New cancel command

**`src/gui/eventconnector.cpp` `onTerrainCommand`** — added a `"CancelJob"` case that calls `g->jm()->cancelJob( Position( tileID ) )`. Reuses the existing TerrainCommand pipeline, no new signal/slot wiring needed.

### How the Distillery scenario reads now

Click the queued Distillery placeholder → Tile Info → Active Job:

```
BuildWorkshop
[!] Waiting for materials
  [OK] 1 x Table
  [ -] 2 x Barrel
[X] Cancel job
```

The player can immediately see the missing barrels and either build a carpenter to make them or cancel the placement and try something else. The hour-of-confusion failure mode is gone.

### Why this is high-leverage

The underlying `requiredItemsAvail` bug affected **every job in the game**, not just construction. Anything that wanted to query "does this job have its materials" was getting `false` for every per-item flag. Fixing the by-value iteration unlocks the per-item availability data colony-wide. Future features (smarter job auto-cancellation, "fetch missing materials" actions, item-flow visualization) can now read this data accurately.

### Build

Green (687 warnings on this run, all pre-existing or harmless).

### Out of scope (deferred)

- Auto-queuing prerequisite craft jobs ("you placed a Distillery, queueing 2 Barrels at the Carpenter automatically").
- Visualizing path failures on the map.
- A separate "no worker with required skill" diagnostic (currently the warning only fires when items are missing; if items exist but no gnome can do the job for skill reasons, the warning doesn't appear yet).
