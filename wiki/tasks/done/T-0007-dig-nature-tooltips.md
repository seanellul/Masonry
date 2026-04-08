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

Implemented using the T-0004 tooltip pipeline.

**Code investigation findings** (the diagnostic side of this task):

1. **`Forage` is a UI stub.** Appears in `src/gui/keybindings.h:85` as `ActionForage` and in `src/gui/ui/ui_gamehud.cpp:196` in `natureActions[]`, but there is **no** `m_taskFunctions.insert("Forage", …)` in `src/game/gnome.cpp`. Clicking Forage enqueues jobs that no gnome will ever pick up. Added to `wiki/dev/known-issues.md` and the in-game tooltip labels it honestly as "[Not yet implemented]".

2. **`Plant Tree`** in the Nature menu has an empty action string (`""`). The existing code already disables the button when its action is empty, so it renders greyed out. Tree planting happens only through groves (T-0010). Added to known-issues for tracking.

3. **`Harvest` vs Forage resolved.** Harvest is implemented in `CanWork::harvest()` at `src/game/canwork.cpp:1030`. It picks ripe fruit/leaves from a plant; for one-shot plants (vegetables, flowers) the plant is removed after harvest, for trees and perennials the plant remains. The "confusion" between the two actions is that Forage simply doesn't do anything — there is no overlap to resolve.

4. **All dig actions are properly wired** — `Mine`, `ExplorativeMine`, `RemoveFloor`, `DigStairsDown`, `MineStairsUp`, `DigRampDown`, `DigHole` all flow through `Selection::setAction()` → `JobManager::addJob()` and have corresponding task handlers.

**Implementation**:

1. **`content/db/ingnomia.db.sql`**: added 11 new `$ActionDesc_*` Translation rows in a commented "Shape → Dig / Nature action tooltips (T-0007)" block — 7 dig actions + 4 nature actions. Plant Tree has no row because its button is disabled. Forage's entry openly states it's not implemented.

2. **`src/gui/ui/ui_gamehud.cpp`**:
   - Added static helpers `actionTooltipDesc(action)` and `showActionTooltip(label, action)` alongside the T-0004 helpers.
   - Wired `IsItemHovered() → showActionTooltip(…)` into the dig action render loop (~line 885) and the nature action render loop (~line 908). The nature loop uses `ImGuiHoveredFlags_AllowWhenDisabled` so users can hover the greyed-out Plant Tree and Forage buttons to discover why they don't work.
   - When `action` is empty, `showActionTooltip` appends a `(Not currently implemented.)` line in the tooltip body — a universal fallback beyond the specific Forage entry.

3. **`wiki/dev/known-issues.md`**: added entries for Forage stub and Plant Tree stub under "Incomplete systems".

**Pipeline reuse validated**: this task sits entirely on top of the T-0004 pattern (`Strings` key lookup + `Error:` sentinel check + `BeginTooltip`/`EndTooltip` block). No new infrastructure. T-0008b (skill tooltips) can follow the same pattern with a `$SkillDesc_<id>` key namespace.

Build: green (11 warnings, all pre-existing).
