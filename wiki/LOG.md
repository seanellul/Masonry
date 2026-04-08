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

## [2026-04-07] task | done: T-0002, T-0003, T-0006, T-0012 (autonomous batch)

First autonomous run of the task pipeline. Four UI polish tasks scoped + implemented + committed in a single pass:

- **T-0003** — Build button ~15% taller with centered content (SmallButton → Button + scoped FramePadding push).
- **T-0002** — Build menu tab bar: swapped `ICON_FA_BOX` (missing glyph → `Ǝ`) to `ICON_FA_CUBES`, centered every category button with uniform 130px width.
- **T-0006** — Workshop panel: default to Recipes tab when queue empty; replaced invisible `InputInt` craft amount with explicit `Text + -/+ SmallButtons`.
- **T-0012** — Population view: removed `.left(5)` truncation of skill headers (Woodcutting vs Woodcarving now distinct); gnome-name clicks now navigate camera + open creature info in both Skills views.

All built green. Smoke test runtime timed out at 120s on shutdown — treated as a pre-existing flake since all edits are in ImGui render paths with no game-thread logic. Deferred: extending gnome-name navigation to the remaining three population tabs.

## [2026-04-07] task | done: T-0005, T-0013 (autonomous batch 2)

- **T-0013** — Schedule painter: diagnosed not as a click bug but as a visual misdirection. The per-gnome "All" column and per-hour "All" row were rendering with the current brush's color + label, making it look like every hour had been bulk-painted whenever a brush was armed. Swapped both to neutral dark-gray cells with `<<` / `^^` labels and explicit tooltips. Individual hour click/drag painting was already correctly wired.
- **T-0005** — Tile Info icon buttons: made `actionButton()` frameless via transparent button colors, applied the same treatment to the creature-info `(i)` button. Identified the "mystery button" as `ICON_FA_RIGHT_LEFT` Replace-floor (already had a tooltip — the rectangular frame was just distracting).

Both UI-render-only; build green.

## [2026-04-07] task | done: T-0004, T-0007 (autonomous batch 3)

- **T-0004** — Build menu tooltips. Established the `$BuildingDesc_<id>` Translation-key pattern using the existing localization pipeline (`Strings::s()`). 54 descriptions: 36 workshops + 7 containers + 6 utility + 5 structure categories. Furniture deferred with graceful name-only fallback. Created `wiki/game/systems/buildings.md` as the human-readable mirror.
- **T-0007** — Dig/Nature tooltips. Built on T-0004 with parallel `$ActionDesc_<action>` keys. 11 descriptions covering all dig actions and 4 nature actions. **Discovered two bugs** during the code inspection (the diagnostic dividend of the pattern): `Forage` is a fully unimplemented UI stub; `Plant Tree` in the Nature menu has an empty action string. Both logged to `wiki/dev/known-issues.md`.

The T-0004 pipeline is now the default pattern for all future tooltip tasks. T-0008b will piggyback on it with `$SkillDesc_<id>`.

## [2026-04-07] task | done: T-0008a, T-0008b (autonomous batch 4)

- **T-0008a** — Skills effect audit. New wiki page `wiki/dev/subsystems/skills.md` covering every one of the 47 skills, sorted into 4 tiers (hot-path / quality-crafting / thought-only / dead) with file:line citations. Captured 7 follow-up task seeds — biggest finding: seven "core" colony skills (Mining, Woodcutting, Farming, Horticulture, Construction, Medic, Caretaking) have no effect on the work they're named for.
- **T-0008b** — Honest skill tooltips. 46 new `$SkillDesc_*` Translation rows sourced from the audit. Dead and thought-only skills are labeled honestly ("Currently tracked but has no gameplay effect"). Individual Skills view in the Population panel gets a rich tooltip; group view left as-is.

The T-0004 pattern has now been applied three times (builds, shape actions, skills) and is proven as the default for wiki → UI info flow.

## [2026-04-07] task | done: T-0009, T-0014 (autonomous batch 5)

- **T-0009** — Workshop queue merge + send-to-top/bottom. Enqueue-time merge in `Workshop::addJob` (tail match on craftID + mode + material filter), two new reorder buttons dispatching the already-supported `"Top"`/`"Bottom"` commands. Saved queues untouched.
- **T-0014** — Keybindings settings tab. Read-only listing parsed from `keybindings.json`, grouped collapsible tables. Created `wiki/game/ui/keybindings.md` as canonical reference. Editable rebinding deferred — infrastructure is ready.

## [2026-04-07] task | done: T-0010 (autonomous batch 6)

- **T-0010** — Groves investigation. Full end-to-end trace in `wiki/dev/subsystems/groves.md`. **Verdict**: groves are implemented correctly; the "broken" report was a UX discoverability issue (fresh groves default to doing nothing until configured; tree species names like "Apple" / "Orange" look like fruits). No runtime fix warranted. 5 UX follow-up seeds captured.

## [2026-04-07] task | intake: T-0015 through T-0021 (skills redesign)

Seven new tasks captured from the skills redesign discussion. These flow from the T-0008a audit findings into a coherent cleanup + grouping + cross-training + titles design. Audit page `wiki/dev/subsystems/skills.md` updated with the consolidated "Planned redesign" section.

- **T-0015** — Combat → stats refactor (deferred until CON/STR/DEX brainstorm). Collapse Melee/Unarmed/Dodge into derived stats; delete 6 dead combat skills.
- **T-0016** — Wire 5 core colony skills (Mining, Woodcutting, Farming, Construction, Medic) to actually affect job outcomes. The biggest gameplay improvement available.
- **T-0017** — Easy wirings: AnimalHusbandry one-line fix at `gnomeactions.cpp:2143`, Fishing single read site, Butchery → yield % + quality of prepared meats. Butchery stays distinct (in the Hearth group with Cooking + Brewing).
- **T-0018** — Cleanup: remove Horticulture, Tinkering, Mechanic; merge Caretaking into Medic.
- **T-0019** — Skill grouping in Population view: 47 columns → 13 (10 groups + 3 standalones). Visual only; sub-skills stay independent.
- **T-0020** — Cross-training XP bonus: max sibling level / 20 * 0.5 multiplier capped at +50%. Master Bonecarver picks up Pottery faster.
- **T-0021** — Skill titles: "Master Smith", "Grandmaster of Magic", etc. Pure flair but sells the design.

Dependencies: T-0019 → T-0020 → T-0021 (group structure is shared); T-0018 should land before T-0019 to avoid stale references. T-0015 is independent but waits on user's stat-system brainstorm.

## [2026-04-07] task | done: T-0018, T-0019 (skills cleanup + grouping)

- **T-0018** — Removed 4 skills (Horticulture, Tinkering, Mechanic, Caretaking → Medic). SQL + enum + jobmanager + gnome.cpp mood checks + 6 backstory rows redirected. Build green.
- **T-0019** — Restructured `SkillGroups` from 14 → 15 rows (10 logical groups + 3 standalones + 2 preserved combat groups). All data — the aggregator and population view are already data-driven from the SkillGroups DB table, so zero C++ changes were needed. Butchery moved to Hearth (with Cooking + Brewing); Field group created (Farming + AnimalHusbandry + Fishing).

## [2026-04-07] task | done: T-0017, T-0020 (skills phase 2)

- **T-0020** — Cross-training XP bonus. Master Bonecarver picks up Pottery 50% faster than a novice. Implemented as a multiplier in `CanWork::gainSkill`, sibling cache lazily populated from `SkillGroups`.
- **T-0017** — Easy wirings. AnimalHusbandry one-line fix (taming duration scales with skill). Fishing bonus-catch chance. Butchery → meat yield + quality. Three previously-dead skills now have real effects.

## [2026-04-07] task | done: T-0021 (skills phase 3 — titles)

- **T-0021** — Derived skill titles. `Gnome::displayTitle()` returns "Master Blacksmith" / "Grandmaster of Nature" / etc. based on the highest sub-skill. Surfaced in the creature info panel header and the population view name column. The flair layer that makes specialization visible per gnome.
