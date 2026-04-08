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

*(Scoping agent: this task's scoping phase is a full investigation. Output = `wiki/dev/subsystems/groves.md` + a verdict + a proposed plan (or a proposed task split). Walk the developer through findings interactively before finalizing. Do not start coding until the verdict is confirmed with the developer.)*

## Result

*(Building agent fills in after implementation.)*
