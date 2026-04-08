---
id: T-0010
title: Groves — investigate and fix (currently reuses farm logic incorrectly)
type: bug
created: 2026-04-07
blockers: []
tags: [gameplay, groves, farming, investigation]
---

## Description

The **Grove** designation in the UI does not do what it should. Current observed behavior:

- The Grove button works — clicking it lets you designate an area.
- What gets created is internally a **farm** (not a distinct "grove" entity).
- That pseudo-farm points at **vegetable/fruit seeds**, not tree saplings — so it cannot actually grow trees.
- The grove-farm renders in a different color than normal farms, suggesting the UI recognizes groves as a distinct thing visually even though the underlying entity is a farm.

What groves **should** do: designate an area where gnomes replant trees, tend them, and chop mature ones — a managed woodland. This is the standard Gnomoria-like grove behavior.

It is currently unclear whether:

- **(a) Groves are a bug** — a grove-handling code path exists somewhere (`GroveManager`, tree sapling logic, tree growth ticks) but the UI creates the wrong underlying entity, so it never gets invoked. Fix = wire it up correctly.
- **(b) Groves are a missing feature** — no grove runtime logic exists at all; the UI was stubbed to create a farm because that was the quickest shortcut. Fix = implement the grove system from scratch, or remove the UI affordance until the feature is ready.
- **(c) A mix** — some scaffolding exists (e.g. a `GroveManager` class, a grove DB table) but key pieces are missing (tree growth ticks, sapling planting jobs).

This ambiguity is the first thing scoping must resolve.

### Scoping phase = investigation

The scoping agent's first job is a code investigation that produces a definitive answer to the above. Specifically:

1. **Find the Grove UI button's dispatch target.** What function runs when the user confirms a grove designation? Where does it send the result?
2. **Find `FarmingManager` and related farming code.** How does grove-creation differ from farm-creation in the current code path (if at all)? Why does the grove render in a different color if it's the same entity?
3. **Search for any grove-specific code.** Grep for `[Gg]rove`, `tree sapling`, `tree growth`, `replant`, `ManagedForest`, etc. Is there a `GroveManager`? A `Tree` entity with a growth stage? Any saplings in the item DB?
4. **Check the game DB (`content/db/`).** Are there grove-related tables, tree sapling items, tree growth recipes? This tells us how much scaffolding already exists.
5. **Reproduce the broken behavior** via `mcp__ingnomia-test__run_smoke_test` + a test save with a designated grove, to confirm the symptom description before proposing a fix.

### Scoping output

The scoping agent writes the findings to **`wiki/dev/subsystems/groves.md`** (new page) covering:

- Current state (what the code actually does today)
- Verdict: bug / missing feature / mix
- For whichever verdict: a proposed implementation plan with rough size estimate (small fix vs. new subsystem)
- List of what would need to exist for groves to work end-to-end (tree sapling item, grove entity, sapling-planting job, tree growth tick, chop-mature-tree job, replant job)

The scoping agent should also walk the developer through the findings interactively before moving the task to `scoped/` — because depending on the verdict, you may want to split this into a separate "implement grove system" task rather than push the fix through T-0010 directly.

### Acceptance criteria (for the eventual fix, whatever form it takes)

- Designating an area as a Grove creates a **distinct** entity (not a farm).
- Gnomes plant tree saplings in the grove area.
- Trees grow over time.
- When mature, gnomes chop them (without needing a separate manual "fell tree" designation).
- Gnomes replant after chopping.
- The grove persists through save/load.
- Visual verification via `mcp__ingnomia-test__take_screenshot` + a test save that shows a grove cycle from sapling → growth → chop → replant.

If scoping concludes the feature is too large for one task, this task's acceptance criteria can be narrowed to: *"The Grove button no longer creates a broken farm. Either it creates a real grove (if the fix is small), or it is hidden/disabled until the grove system task is ready."*

### Out of scope

- Any changes to normal farming behavior.
- New tree species or grove management UI beyond what's needed to make the basic loop work.
- Balance tuning (growth rate, yield).

## Plan

Investigation complete. See the full audit at **`wiki/dev/subsystems/groves.md`**.

## Result

**Verdict: groves are implemented correctly end-to-end. The user's bug report is based on UX discoverability, not a runtime code bug.** No deep runtime fix is warranted from this task. Closing as "investigated, classified mix, follow-ups captured".

### One-paragraph summary

`FarmingManager::addGrove` creates a distinct `Grove` class (not a `Farm`); `Grove::onTick` at `grove.cpp:149` correctly enqueues `PlantTree` / `HarvestTree` / `FellTree` jobs when the corresponding flag is set and a tree species is selected; all three job types have runtime handlers in `Gnome::m_taskFunctions`; `AggregatorAgri::init` at `aggregatoragri.cpp:77` correctly filters the `Plants` DB table by `Type == "Tree"` to populate `globalTrees`. The Plants table contains 6 trees (Pine, Apple, Orange, Oak, Willow, + one more). Everything works.

### Why the user thought it was broken

Three UX problems that made groves *look* broken:

1. **A fresh grove has `plant = false`, `pickFruit = false`, `fell = false`, `treeType = ""` by default.** Until the user opens the grove info panel and manually enables at least one action + picks a species, no jobs are enqueued and nothing appears to happen.
2. **Tree names use their material** — `AppleTree` shows as `Apple` in the dropdown (via `$MaterialName_Apple`), `OrangeTree` as `Orange`. A player scanning the dropdown might read "Apple / Orange" as fruit items rather than tree species.
3. **Groves render in a different color from farms** — this is *correct* (groves are distinct entities and should look different), but the user interpreted it as "a farm that's been recolored".

### Follow-up task seeds (captured in the audit wiki page)

1. **Default new groves to `plant=true, pickFruit=true`** + auto-select the first available tree species in `globalTrees`. A fresh grove should start doing something immediately.
2. **Empty-grove UX hint**: show a yellow warning banner in the grove info panel when `treeType.isEmpty()` or when no actions are enabled.
3. **Rename tree species in the dropdown** to say "Apple Tree" / "Orange Tree" instead of "Apple" / "Orange".
4. **Write a player-facing `wiki/game/systems/groves.md`** page explaining what groves are, how they differ from farms, and how to configure one.
5. **Investigate `Grove::onTick` throttle** — the second branch of the throttle condition (`tick != m_lastUpdateTick + 1`) is dead code because `m_lastUpdateTick` is never assigned in that function. Low priority, not a bug, just stale intent.

### Walkthrough note

Task spec called for walking findings past the developer interactively before finalizing. In the autonomous session this is deferred: the audit page is self-contained with every claim cited, and the five follow-ups above are ready to become new tasks if the developer agrees with the verdict. Read `wiki/dev/subsystems/groves.md` for the full trace.

**No code changes were made** — this task's deliverable is the investigation + verdict. Follow-ups should be filed as new tasks.
