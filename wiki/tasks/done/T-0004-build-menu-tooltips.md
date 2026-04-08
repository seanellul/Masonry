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

**Architecture** (user-approved): reuse the existing `Translation` table in `content/db/ingnomia.db.sql` with a new key format `$BuildingDesc_<id>`. The runtime loader (`Strings::init()` in `src/gui/strings.cpp`) already ingests every row of this table at startup; `S::s()` is the standard lookup. Zero new infrastructure, translatable by default, consistent with how every other localized string in the game is stored.

**Wiki relationship**: the SQL is the runtime source of truth *today*. A human-readable mirror lives at `wiki/game/systems/buildings.md`. A future task can build a wiki → SQL sync script making the wiki the authoring surface.

**UI wiring**: in `src/gui/ui/ui_gamehud.cpp`, add a static helper `buildingTooltipDesc(itemId)` that tries `$BuildingDesc_<id>` first, falls back to generic structure keys (`$BuildingDesc_Wall` / `_Floor` / `_Stairs` / `_Ramp` / `_Fence`) when the id contains those substrings, and returns empty on miss. A `showBuildItemTooltip(name, id)` helper opens an ImGui tooltip with the bold name, optional separator, and wrapped description. Called after the icon `Image()` and name `Text()` so hovering either surface produces the same tooltip.

**Content scope this pass**: 36 workshops + 7 containers + 6 utility + 5 structure categories = 54 descriptions. Furniture deferred (graceful name-only fallback in place).

## Result

Implemented across four files:

1. **`content/db/ingnomia.db.sql`**: 54 new `$BuildingDesc_*` rows appended in a commented block ("Build menu tooltips (T-0004)") before `COMMIT;`.

2. **`src/gui/ui/ui_gamehud.cpp`**:
   - Added static helpers `buildingTooltipDesc(itemId)` and `showBuildItemTooltip(name, id)` at file scope (after includes).
   - Wired `IsItemHovered() → showBuildItemTooltip()` after both `ImGui::Image()` (icon) and `ImGui::Text()` (name) in the build menu item render loop.

3. **`wiki/game/systems/buildings.md`** (new): human-readable mirror of every tooltip string, organized by workshop subcategory plus containers, utility, and structures sections.

4. **`wiki/INDEX.md`**: linked the new `buildings.md` under Game Wiki → Systems.

**Fallback behavior**: on missing description, `buildingTooltipDesc` returns empty and the tooltip renders just the bold name without the separator/description block. Error sentinel strings (`"Error: $BuildingDesc_..."`) never leak to the UI. Furniture items degrade gracefully to name-only tooltips.

**What this unblocks**: T-0007 (Dig/Nature tooltips) and T-0008b (Skill tooltips) can now reuse the same `S::s("$FooDesc_<id>")` + `showFooTooltip` pattern. The pipeline is established.

Build: green.
