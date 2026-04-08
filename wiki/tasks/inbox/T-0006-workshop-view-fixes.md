---
id: T-0006
title: Workshop view — default to Recipes when queue empty + fix missing craft count
type: bug
created: 2026-04-07
blockers: []
tags: [ui, workshop]
---

## Description

The workshop info view has two concrete bugs and some general polish debt.

### 1. Default tab should be Recipes when the queue is empty

When opening a workshop with no queued jobs, the view lands on the **Queue** tab, which shows nothing useful. The user then has to click **Recipes** to do anything. Instead: if the queue is empty, default the active tab to **Recipes**. If the queue has jobs, keep defaulting to **Queue** (so in-progress workshops don't lose the at-a-glance state view).

### 2. Craft N / Until N count number is not visible

In the Recipes tab, the Mode row shows `Craft N` / `Until N` / `Repeat` with a number input between "Repeat" and the `-` / `+` buttons. The number itself does not render — the input field is empty-looking even when a value is set. The `-` / `+` buttons still work, but the user has no feedback on what the current N value is. Widening the field does not help, which suggests the text isn't being drawn rather than being clipped.

Scoping should investigate whether this is:
- A text color issue (value drawn in the same color as the background),
- A widget-state issue (the int is not being bound to the input),
- An ImGui layout/overflow issue (cursor wrong, text drawn offscreen), or
- A recently-introduced regression.

### 3. General "needs TLC" polish (context, not strict scope)

The user has flagged the whole workshop info view as visually inconsistent and cramped. **Not in scope for this task** beyond the two fixes above, but while scoping the fixes, the scoping agent should note any cheap polish wins they spot (misaligned labels, inconsistent spacing, obvious dead widgets) and surface them as candidate follow-up tasks — don't silently fix them.

### Acceptance criteria

- Opening a workshop with an empty queue lands on the Recipes tab.
- Opening a workshop with a non-empty queue still lands on the Queue tab.
- The Craft N / Until N count is visible and updates when `-` / `+` are clicked.
- Visual verification via `mcp__ingnomia-test__take_screenshot`:
  - Workshop with empty queue (should show Recipes active)
  - Workshop with queued job (should show Queue active)
  - Recipes tab with Craft N selected, number visible and responsive to +/-
- Scoping report includes a short list of candidate polish tasks to file separately.

### Out of scope

- Any polish fixes beyond the two listed bugs.
- Broader workshop UX redesign.
- Changing how Craft N / Until N / Repeat modes themselves work.

## Plan

*(Scoping agent: (1) Find the workshop info view in `src/gui/ui/` — probably a `ui_workshop*.cpp` file or a section of `ui_sidepanels.cpp`. Identify where the tab state is initialized when the panel opens. Check whether tab state is persisted across opens or reset — the fix should be a conditional default, not a hard reset. (2) Find the Craft N input widget in the same file. Verify the int source, check whether `ImGui::InputInt` / `InputScalar` is being called correctly and whether a text color or bg color override is hiding the value. Run the game via `mcp__ingnomia-test__build_game` + `take_screenshot` to reproduce before proposing a fix. (3) While in the file, jot down any cheap polish candidates — don't fix them here.)*

## Result

*(Building agent fills in after implementation.)*
