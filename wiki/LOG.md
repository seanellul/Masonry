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
