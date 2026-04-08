---
id: T-0004
title: Add hover tooltips to build menu items (workshops/structures/furniture/utility/containers)
type: feature
created: 2026-04-07
blockers: []
tags: [ui, build-menu, tooltips, wiki-content]
---

## Description

When hovering over any buildable item in the build menu — across all five tabs (Workshops, Structures, Furniture, Utility, Containers) — a tooltip should appear describing what the item is and what it does. Most players (and the developer) can't tell from the name alone what a `sawmill` does differently from a `carpenter`, or what a `chisel` workstation is for.

This is the first of several "surface authoritative info in the UI" tasks and sets the precedent for how tooltip content flows from the wiki into the game.

### Content format

Each tooltip shows, at minimum:

- **Bold name** (already in the row, but repeated in the tooltip for clarity)
- **One-line function description** — what this thing *does*, not what it *is*. Examples:
  - Carpenter — *Crafts wooden furniture, tools, and workbenches.*
  - Sawmill — *Cuts logs into planks and boards used by other wood workshops.*
  - Barrel — *Stores liquids and brewed goods. Required by breweries.*
  - Chisel — *A stone-cutting workstation. Produces carved stone blocks and masonry components.*

Keep it short. If a one-liner isn't enough, the tooltip can grow later — but the initial pass is strictly name + function line.

Tooltips start off minimal but the pipeline should be designed so that richer content (inputs/outputs, required skills, related items) can be layered in later without rewriting the mechanism.

### Source of truth

**Deferred to scoping.** This is an architecture decision and the scoping agent should investigate before proposing. Options to evaluate:

- **SQLite game DB** (`content/db/`) — Masonry already loads buildings, recipes, and items from SQLite via `DB::init()`. Adding a `tooltip` or `description` column is likely the path of least resistance and integrates naturally with existing loaders.
- **Wiki markdown frontmatter** — each `wiki/game/systems/*.md` or `wiki/game/ui/*.md` page holds its own tooltip in YAML frontmatter, parsed at load time or exported to the DB by a build-step script.
- **Dedicated JSON/YAML file** under `content/` — simple, but yet another source of truth to keep in sync.

The scoping agent should inspect `content/db/` structure, check how buildings currently get their names/metadata loaded, and recommend the minimum-friction approach. **Strong preference**: whatever is picked, the wiki should remain the *human authoring surface* — even if the runtime reads from the DB, the DB should ideally be generated from (or cross-referenced to) the wiki so content lives in one place and the wiki drives the game's self-documentation.

### Trigger

Standard ImGui hover delay tooltip (`ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)` + `ImGui::BeginTooltip()` / `EndTooltip()` or `ImGui::SetTooltip()`). Match the hover delay used elsewhere in the project if one is already established. Tooltip should appear on every row in all five build menu tabs.

### Coverage

This task delivers **the mechanism** plus initial one-line tooltips for **every item across all five tabs**. After the task is done, expanding tooltip content for specific items is ongoing work done via the wiki, not additional engineering.

### Acceptance criteria

- Hovering any row in Workshops, Structures, Furniture, Utility, or Containers tabs produces a tooltip with name + one-line function description.
- Every buildable item in the current build menu has a tooltip (no missing entries).
- Tooltip content is sourced from whatever single-source-of-truth the scoping agent chose — no hardcoded strings scattered through `src/gui/`.
- Visual verification via `mcp__ingnomia-test__take_screenshot` with the mouse hovered over a representative item in each tab.
- Architectural verification: updating a tooltip's text in the source of truth and rebuilding reflects the change in game.

### Out of scope

- Tooltips outside the build menu (dig/nature tooltips are T-0005; skill tooltips are T-0006).
- Rich tooltip content (inputs, outputs, required skills, images) — future expansion.
- Translation/localization.

## Plan

*(Scoping agent: (1) Survey `content/db/` to find where buildings/workshops/furniture/utility/containers are defined and loaded — likely via `DB::init()` / `DB::initStructs()` in `src/base/db.*`. (2) Decide the source of truth (see "Source of truth" above) — recommend strongly for DB if there's already a description column or a natural place for one, otherwise propose the wiki-as-authoring-surface approach. (3) Find the build menu rendering code in `src/gui/ui/` — the file that draws each row is where `ImGui::IsItemHovered` + `BeginTooltip` will be wired. (4) Propose a plan that includes: the storage location, the loader changes (if any), the UI wiring, and the initial content pass covering every item in the five tabs. (5) If the wiki-as-source approach is chosen, design it so the tooltip text for a building lives on its `wiki/game/` page and is discoverable by a simple script or build step.)*

## Result

*(Building agent fills in after implementation.)*
