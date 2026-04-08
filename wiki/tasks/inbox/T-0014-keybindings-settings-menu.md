---
id: T-0014
title: Settings — keybindings / hotkeys menu
type: feature
created: 2026-04-07
blockers: []
tags: [ui, settings, keybindings, discoverability]
---

## Description

There is currently no in-game way to see what Masonry's keyboard shortcuts are. The player (and the developer) has to read the source code or guess. Add a **Keybindings** section to the Settings menu that lists every bound key alongside what it does.

### Scope

**MVP (must-have):**
- A new **Keybindings** / **Hotkeys** section accessible from the Settings menu.
- A scrollable, grouped list of every currently-bound key showing:
  - The key (or key combination, e.g. `Shift+Space`)
  - The action it triggers (human-readable name)
  - A one-line description of what the action does
- Grouped by category — camera, designation, UI toggles, game speed, debug, etc. Scoping determines the categories based on what actually exists in code.
- Read-only. Listing only; no editing. This alone delivers 80% of the value.

**Stretch (if scoping finds it's cheap):**
- Click a row to rebind the key to a new press.
- Save rebinds to the existing config file (if one exists) or add a keybindings section to it.
- Restore defaults button.

Whether the stretch is in scope depends entirely on how keybindings are currently stored:
- **If there's already a config-file-driven keybinding system** (e.g. a JSON/INI section loaded by `Config`), editing is likely cheap and should be included.
- **If bindings are hardcoded in C++ input handlers** (e.g. a switch statement in `MainWindow::keyPressEvent`), editing is a meaningful refactor and should be deferred to a follow-up task. The MVP menu can still read from the hardcoded list via a manually-maintained registry.

Scoping decides which applies and proposes accordingly.

### Scoping output as wiki content

This task has a useful side effect: regardless of editable vs read-only, the scoping agent must enumerate every keybinding in the code to populate the list. That enumeration should also be written to **`wiki/game/ui/keybindings.md`** as a canonical reference page, linked from `wiki/INDEX.md`. The in-game menu can then be considered a projection of that wiki page — or, ideally, generated from the same source.

This matches the pattern from T-0004 (build menu tooltips) and T-0008a (skills audit): the wiki is the authoritative source, the UI surfaces it.

### Acceptance criteria

- Settings menu has a new Keybindings section.
- Every currently-active keybinding in the game is listed with key, action name, and description.
- Bindings are grouped into sensible categories.
- A new wiki page `wiki/game/ui/keybindings.md` documents all bindings and is linked from `wiki/INDEX.md`.
- If the stretch (editable bindings) is in scope: clicking a row lets the user rebind, changes persist across restarts, restore-defaults works.
- Visual verification via `mcp__ingnomia-test__take_screenshot` of the Keybindings settings panel.

### Out of scope

- Adding new keybindings (this task only surfaces existing ones).
- Controller/gamepad bindings.
- Accessibility / remapping to accommodate non-QWERTY layouts (beyond whatever the existing input system handles).
- Keybinding conflict detection (can be a follow-up if editable lands).

## Plan

*(Scoping agent: (1) Find the input handling code — likely `src/gui/MainWindow.cpp`'s `keyPressEvent` and any other `QKeyEvent` handlers. Grep for `Qt::Key_` to enumerate every bound key. (2) Check `src/base/config.cpp` and any config files (`settings.json` or similar) to see whether keybindings are already stored in a config or hardcoded. (3) Find the existing Settings menu in `src/gui/ui/` to see where the new section plugs in. (4) Decide MVP-only vs MVP+editable based on findings. (5) Write `wiki/game/ui/keybindings.md` as a parallel deliverable. (6) Propose implementation sequence.)*

## Result

*(Building agent fills in after implementation.)*
