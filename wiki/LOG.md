# Masonry Wiki — Log

Append-only chronological log. Newest entries at the bottom. Format: `## [YYYY-MM-DD] <type> | <title>` where `<type>` ∈ {`bootstrap`, `ingest`, `query`, `lint`, `task`}.

---

## [2026-04-07] bootstrap | Wiki scaffold created

Set up the wiki skeleton at `wiki/` per plan `zazzy-dreaming-walrus.md`.

- Created folder structure: `game/{systems,creatures,lore,ui}`, `dev/{architecture,subsystems,decisions}`, `tasks/{inbox,scoped,in-progress,blocked,done}`, `raw/`, `outputs/`.
- Wrote `SCHEMA.md`, `INDEX.md`, `LOG.md`.
- Migrated 6 files from `docs/`:
  - `docs/updates/development_roadmap.md` → `dev/roadmap.md`
  - `docs/design/gnome_ai_redesign.md` → `dev/decisions/gnome-ai-redesign.md`
  - `docs/worldbuilding/game_identity.md` → `game/lore/identity.md`
  - `docs/ai_sprite_generation.md` → `dev/subsystems/sprite-generation.md`
  - `docs/visual_identity.md` → `game/lore/visual-identity.md`
  - `docs/updates/parallelization_plan.md` → `dev/decisions/parallelization.md`
- Created `dev/known-issues.md` from `~/.claude` memories (gnome scaling, military gaps, UI overhaul, test infrastructure).
- Added 4 agent prompts to `tasks/` adapted from `~/Code/ai-and-automation/claude/project-management/jrpg-engine/`, rewritten for file-based kanban.
- Updated `CLAUDE.md` with wiki entrypoint + adjusted DEVLOG workflow.

## [2026-04-07] task | intake: T-0001 Fix build menu icon rendering (size, background, color)

Furniture/Utility/Containers tabs render icons at ~half the size of Workshops, on an opaque black background, and as monochrome white silhouettes. Likely shared root cause in the build menu icon codepath. Scoping to investigate the rendering path and propose a unified fix.

## [2026-04-07] task | intake: T-0002 Build menu tab bar — proper Containers icon + center button contents

Containers tab shows `Ǝ` as a missing-glyph fallback; needs a real icon from RPG Awesome/Font Awesome. Tab buttons also have off-center icon+label content.

## [2026-04-07] task | intake: T-0003 Increase Build button height by ~15%

Per-item Build action button is cramped. Bump vertical padding ~15%, keep content centered.

## [2026-04-07] task | intake: T-0004 Add hover tooltips to build menu items

Hover tooltips across all five build tabs, format = bold name + one-line function description. Source of truth deferred to scoping (evaluate SQLite DB vs. wiki-as-authoring-surface). First of several wiki→UI info tasks; sets the precedent for how authoritative info flows from the wiki into the game.

## [2026-04-07] task | intake: T-0005 Tile Info panel — icon-only buttons + clarify mystery button

Remove button frames from Gnome info (i) and Terrain trash icons. Identify and clarify (icon + tooltip) the unknown button next to the Terrain trash.

## [2026-04-07] task | intake: T-0006 Workshop view — default to Recipes when queue empty + fix missing craft count

Empty-queue workshops should open on Recipes tab. Craft N / Until N count number is not rendering. Scoping to also jot down cheap polish candidates (general "needs TLC") as follow-up task seeds.

## [2026-04-07] task | intake: T-0007 Dig & Nature menu tooltips

Blocked on T-0004. Tooltip content derived from code (not guessed). Harvest vs Forage distinction resolved definitively via code inspection. Pipeline must reuse T-0004.

## [2026-04-07] task | intake: T-0008a Audit — what does every skill actually do in gameplay?

Investigation-only. Output = new wiki page `wiki/dev/subsystems/skills.md` documenting every skill, its effects, and any broken/dead logic found. Doubles as a diagnostic pass. Walk developer through findings before closing.

## [2026-04-07] task | intake: T-0008b Add skill tooltips to the gnome info UI

Blocked on T-0004 + T-0008a. Content sourced from the skills audit page. Dead/ineffective skills must be labeled honestly in their tooltips rather than hidden.

## [2026-04-07] task | intake: T-0009 Workshop queue — auto-merge identical jobs + top/bottom reorder

Enqueue-time merging (match last entry on recipe + mode + material filter). Two new reorder buttons (send to top, send to bottom). No retroactive merge on load.

## [2026-04-07] task | intake: T-0010 Groves — investigate and fix

Grove designation currently creates a broken farm pointing at vegetable seeds. Unclear whether groves are a bug or a missing feature. Scoping phase is an investigation producing `wiki/dev/subsystems/groves.md` + a verdict + proposed fix size. Walk developer through findings before finalizing plan.

## [2026-04-07] task | intake: T-0012 Population view — fix clipped skill headers + gnome-name GoTo

Skill column headers are truncated to 5 chars (Woodcutting and Woodcarving both show as `Woodc`). Clicking a gnome name should center camera on the gnome and open their character info panel.

## [2026-04-07] task | intake: T-0011 RimWorld-style manual work priorities

Numeric priority grid (1–4 + blank) per gnome per work type. Manual + Auto both in scope. Auto rule: everyone gets priority 3 on work types they're capable of, blank otherwise. Column order derived from RimWorld's default work tab, mapped to Masonry's actual work types only (do not introduce work types Masonry does not have). User signs off on final column ordering before any code is written.

## [2026-04-07] task | intake: T-0013 Schedule painter — paint type applies to "All" instead of individual cells

Clicking a Paint type button in the Schedule tab mass-applies to the "All" column/row instead of arming a brush. Fix: introduce brush-armed state, wire hour cells to check it on click, add click-and-drag, preserve "All" bulk-apply semantics for post-arm clicks.

## [2026-04-07] task | intake: T-0014 Settings — keybindings / hotkeys menu

Add a Settings section listing every keybinding. MVP = read-only; editable is stretch depending on whether bindings are already config-driven or hardcoded. Scoping also writes `wiki/game/ui/keybindings.md` as canonical reference.
