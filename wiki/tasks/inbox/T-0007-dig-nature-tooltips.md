---
id: T-0007
title: Dig & Nature menu tooltips (incl. harvest vs forage clarification)
type: feature
created: 2026-04-07
blockers: [T-0004]
tags: [ui, tooltips, wiki-content]
---

## Description

Add hover tooltips to every action in the **Dig** menu and the **Nature** menu. The player (and the developer) cannot tell from the icons alone what each action actually does, and the difference between **Harvest** and **Forage** is specifically confusing.

### Source of truth: the code

Unlike T-0004 where content could come from the game DB, here the content should be **derived from reading the actual implementation** of each action. The scoping agent enumerates every action in both menus, traces the code that runs when the action is dispatched, and writes a one-line function description based on what the code actually does — not what the name suggests. This double-serves as a diagnostic pass: if an action's real behavior diverges from its name, or if two actions overlap more than expected, that's a finding worth flagging.

### Harvest vs forage specifically

The scoping agent should definitively answer: what does each one do, and how do they differ? Expected outcomes, in order of preference:

1. They are meaningfully distinct → write two clear tooltips that make the difference obvious.
2. They overlap heavily but not fully → write tooltips that explain the edge where they differ, and file a follow-up task proposing a consolidation or rename.
3. One of them is dead/redundant → file a follow-up task to remove it.

Do not decide this from the name; decide it from the code.

### Content format

Same as T-0004: bold name + one-line function description. Short. Grow later if needed.

### Dependency on T-0004

This task is blocked on T-0004 because T-0004 establishes the tooltip pipeline (storage location, loader wiring, UI hookup). T-0007 then extends that pipeline to the Dig and Nature menus — it must not invent a second parallel tooltip mechanism. When T-0004 lands, T-0007 scoping should be a much shorter exercise: enumerate actions, derive content from code, plug into the existing pipeline.

### Acceptance criteria

- Every action in the Dig menu has a hover tooltip (name + one-line function description).
- Every action in the Nature menu has a hover tooltip.
- Harvest and Forage tooltips make the distinction between them unambiguous.
- All tooltip content is derived from code inspection, not guessed.
- Any discovered discrepancies (dead actions, name/behavior mismatches, redundant pairs) are surfaced as follow-up task candidates in the scoping report.
- Visual verification via `mcp__ingnomia-test__take_screenshot` with cursor hovered over representative actions in each menu.

### Out of scope

- Changing action behavior.
- Renaming actions (even if scoping finds a better name — file a follow-up).
- Consolidating overlapping actions (even if scoping finds heavy overlap — file a follow-up).

## Plan

*(Scoping agent: blocked on T-0004. Once T-0004 is scoped, investigate: (1) Find the Dig menu and Nature menu rendering code in `src/gui/ui/`. Enumerate every action. (2) For each action, trace the dispatch target in the game thread — likely a `JobManager` / `DesignationManager` / `FarmingManager` method. Read the actual logic. (3) Write the one-line function description from what the code does. (4) For Harvest vs Forage specifically, produce a definitive comparison. (5) Plug into the T-0004 tooltip pipeline — do not create a new one. (6) Scoping report surfaces any discovered anomalies as follow-up task seeds.)*

## Result

*(Building agent fills in after implementation.)*
