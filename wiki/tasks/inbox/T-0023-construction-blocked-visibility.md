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

*(Scoping agent: (1) Find the construction placeholder data — likely a `BuildJob` or `Construction` entity in `src/game/`. (2) Identify how the build job currently determines whether it has materials — there's probably a check function that returns "ready" / "needs materials". (3) Find Tile Info's render code (`src/gui/ui/ui_tileinfo.cpp`) and add a section that, when the selected tile has a queued construction, queries that job's required components vs available inventory and renders the per-component status. (4) Add the "why blocked" string by inspecting the job's state. (5) Wire a Cancel button.)*

## Result

*(Building agent fills in.)*
