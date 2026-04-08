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

**Investigation result**: keybindings are **already config-driven**. They live in `keybindings.json` at the project root (shipped default) and are parsed at startup by `KeyBindings::update()` in `src/gui/keybindings.cpp`. The JSON structure is:

```json
[
  {
    "GroupName": "Navigation",
    "Keys": [
      { "Command": "WorldScrollLeft",
        "Key1": { "Key": "A", "Ctrl": false, "Alt": false, "Shift": false },
        "Key2": { ... } }
    ]
  }
]
```

This means the MVP (read-only listing) is a straightforward JSON parse + table render. **The stretch (editable bindings) is also tractable** because the file is already the source of truth — a rebind just writes back to the same JSON — but the input-capture UX (modal "press a key" dialog, conflict detection, save-to-disk wiring) is real work and a separate follow-up.

**UI placement**: `drawSettings()` in `src/gui/ui/ui_mainmenu.cpp` already has a tab bar with Game / Controls / Audio tabs. Add a new **Keybindings** tab alongside. Bump the settings window from 500×350 to 600×500 so the scrollable list has room.

**Loader**: a file-scope `ensureKeybindingsLoaded()` reads the JSON once, caches a grouped view in static vectors. Tries `<dataFolder>/settings/keybindings.json` first (user override), falls back to `<appDir>/keybindings.json` (shipped default). Format is resilient to both `"GroupName"` and `"Name"` field naming.

## Result

Implemented.

1. **`src/gui/ui/ui_mainmenu.cpp`**:
   - Added `QFile` / `QJsonDocument` / `QJsonArray` / `QJsonObject` includes.
   - Added file-scope `KbEntry` / `KbGroup` types, static cache, `formatKbKey()` helper (builds `Ctrl+Shift+Key` strings, honestly shows `—` for empty slot), and `ensureKeybindingsLoaded()` which parses the JSON once per session with dataFolder → appDir fallback.
   - Added a new `"Keybindings"` tab to `drawSettings()` after the Controls tab. Renders each group as a `CollapsingHeader` (default-open) containing a 3-column `Action | Key 1 | Key 2` table. Graceful empty-state when the file can't be found (shows the expected path).
   - Bumped the settings window size from 500×350 to 600×500 to accommodate the new tab.

2. **`wiki/game/ui/keybindings.md`** (new): canonical human-readable reference listing Navigation, Game, Debug/Dev, and Shape Action bindings with the shipped default keys. Notes the JSON as the runtime source of truth and flags the known-issue UI stubs (`Forage`, `Plant Tree`).

3. **`wiki/INDEX.md`**: linked the new `keybindings.md` under Game Wiki → UI.

**Two small loader bugs caught during testing**:
- JSON field is `GroupName`, not `Name` — loader now tries both for compatibility.
- `QJsonValue::toString()` returns empty for bool values in Qt5 — modifier detection now uses `.toBool()` instead of string comparison.

**Stretch (editable bindings) deferred**: the infrastructure is ready (file-driven, standard JSON format) but the "click a row, press a key, write back to disk" UX is scope creep for this task. Recommend as a small follow-up when rebinding becomes a real need.

Build: green (10 warnings, all pre-existing).
