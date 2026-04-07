# Masonry Wiki — Schema

This file is the contract between you (the human curator) and any LLM agent operating on this wiki. Agents should read this file and `INDEX.md` at the start of any wiki-touching session. Humans rarely edit wiki pages directly — the LLM writes, the human curates, asks, and approves.

## What this wiki is

A single flat-file knowledge base for **Masonry** (the game, formerly Ingnomia) that serves three purposes simultaneously:

1. **Game wiki** (`game/`) — player/community-facing: systems, creatures, lore, UI. What Masonry *is*.
2. **Dev knowledge base** (`dev/`) — implementation-facing: architecture, subsystems, decisions. How Masonry *works*.
3. **Project management** (`tasks/`) — kanban-by-folder markdown tasks. What's being done next.

Every piece of durable knowledge about Masonry lives here. Ephemeral stuff (chat history, conversation context) does not.

## Folder semantics

| Folder | Owner | Mutable? | Purpose |
|---|---|---|---|
| `raw/` | Human | No — immutable | Source material: clippings, screenshots, transcripts, exported chats, reference images. LLM reads, never modifies. |
| `game/` | LLM | Yes | Player-facing wiki pages. Organized by `systems/`, `creatures/`, `lore/`, `ui/`. |
| `dev/` | LLM | Yes | Dev-facing wiki pages. Organized by `architecture/`, `subsystems/`, `decisions/` (ADR-style), plus `roadmap.md` and `known-issues.md`. |
| `tasks/` | LLM + Human | Yes | One `.md` file per task. Kanban-by-folder: `inbox/`, `scoped/`, `in-progress/`, `blocked/`, `done/`. Status = which folder the file is in. Moves are `git mv`. |
| `outputs/` | LLM | Yes | Generated artifacts from queries (reports, comparisons, slide decks). Can be filed back into `game/`/`dev/` if valuable. |
| `INDEX.md` | LLM | Yes | Content catalog of every wiki page. One line per page. Read this first. |
| `LOG.md` | LLM | Append-only | Chronological record of ingests, queries, lints, major task transitions. `## [YYYY-MM-DD] <type> | <title>` prefix. |
| `SCHEMA.md` | Human (this file) | Rarely | The rules. Update when workflows change. |

## Page conventions

Every wiki page in `game/` or `dev/` follows this shape:

```markdown
---
title: Farming
tags: [system, economy, gameplay]
status: stub | draft | current | stale
last_updated: 2026-04-07
sources: [raw/gnomoria-farming-notes.md, dev/subsystems/farmingmanager.md]
---

# Farming

One-paragraph summary — what this page is about, enough for an agent to decide if it's the right page without reading further.

## <sections...>

## See also
- [[farmingmanager]]
- [[food-economy]]
```

Rules:
- **Summary first**: always a 1-paragraph summary directly under the title. This is what the agent reads when triaging.
- **Backlinks via `[[page-name]]`**: use the basename without extension, case-insensitive. Obsidian-compatible.
- **Dual-track cross-link**: if a topic exists in both `game/` and `dev/`, each page links to the other in "See also".
- **`sources` frontmatter**: list the `raw/` files or other wiki pages the content is derived from. This is the citation trail.
- **`status` frontmatter**: `stub` (placeholder, needs writing), `draft` (in progress), `current` (authoritative), `stale` (known out of date, flagged for re-ingest).

## Workflows

### Ingest

Trigger: human drops a new file into `raw/` and says "ingest this".

1. Read the source end-to-end.
2. Discuss the key takeaways with the human (1–5 bullets) and confirm scope.
3. Identify which existing wiki pages are affected. Update them, touching as many as needed (typically 5–15).
4. If a referenced concept has no page yet, create a `stub` page for it.
5. Update `INDEX.md` with any new pages.
6. Append a log entry: `## [YYYY-MM-DD] ingest | <source filename>` + a 2-line summary + bullet list of touched pages.

### Query

Trigger: human asks a question that the wiki could answer.

1. Read `INDEX.md` first. Do not blindly grep.
2. Drill into the most relevant pages. Follow `[[backlinks]]`.
3. Answer with citations (page paths). If the answer required reading `raw/`, cite those too.
4. If the answer is non-trivial and likely reusable, write it to `outputs/<YYYY-MM-DD>-<slug>.md` and offer to file it back into the wiki.
5. Append a log entry: `## [YYYY-MM-DD] query | <question>` + which pages were consulted.

### Lint

Trigger: human says "lint the wiki" or it's been a while.

Check for:
- **Contradictions** between pages
- **Stale pages** (`status: stale`, or `last_updated` older than N months on a fast-moving topic)
- **Orphans** (pages with no inbound `[[backlinks]]`)
- **Dangling links** (`[[backlinks]]` pointing to non-existent pages)
- **Missing-but-referenced concepts** — candidates for new stub pages
- **Uncited claims** (content with no `sources` entry)

Report findings as a list. Do not auto-fix; let the human decide.

### Task lifecycle

Tasks are markdown files. Status = folder. Transitions are `git mv`.

```
inbox/        → raw idea, just described
scoped/       → has a plan under ## Plan
in-progress/  → actively being built
blocked/      → waiting on a prerequisite (note in frontmatter)
done/         → implementation finished + DEVLOG entry generated
```

Task file format:

```markdown
---
id: T-0042
title: Fix gnome social O(n²) scaling
type: bug | feature | polish
created: 2026-04-07
blockers: []
---

## Description
<what and why>

## Plan
<written by scoping agent — concrete steps, files to touch, tests>

## Result
<written when marked done — what actually shipped, links to commits>
```

Task IDs are monotonic: next ID = (highest existing ID across all `tasks/**/*.md`) + 1, zero-padded to 4.

#### Agent roles (reusable prompts in `tasks/`)

- **Intake agent** (`tasks/PROMPT_INTAKE_AGENT.md`) — has a conversation to clarify an idea, writes a new file into `inbox/`.
- **Scoping agent** (`tasks/PROMPT_SCOPING_AGENT.md`) — picks an `inbox/` task, writes a `## Plan`, moves to `scoped/`.
- **Building agent** (`tasks/PROMPT_BUILDING_AGENT.md`) — picks a `scoped/` task, moves to `in-progress/`, implements it, commits, moves to `done/`, writes `## Result`, generates the `DEVLOG.md` entry.
- **Fixing agent** (`tasks/PROMPT_FIXING_AGENT.md`) — picks up a task the building agent couldn't finish (marked with a `needs_fixing: true` frontmatter flag in `in-progress/`).

These are adapted from `~/Code/ai-and-automation/claude/project-management/jrpg-engine/PROMPT_*.md` but rewritten to operate on files instead of the Notion API.

### DEVLOG integration

`DEVLOG.md` at the repo root is the public changelog. It is **downstream** of the wiki:

- When a task moves to `done/`, the building agent appends an entry to the top of `DEVLOG.md` using the format defined in `CLAUDE.md`.
- The devlog entry links back to the task file (`wiki/tasks/done/T-0042-*.md`).
- `LOG.md` inside the wiki is the *internal* log (ingests, queries, lints, all task transitions). `DEVLOG.md` is the *external* changelog (only completed work that ships).

## Dual-track rule

Many topics have both a game-facing and dev-facing page. These are separate documents:

- `game/systems/farming.md` — what farming *is* in the game world, from the player's perspective.
- `dev/subsystems/farmingmanager.md` — how `FarmingManager` is implemented, what files it touches, known bugs.

Each links to the other under "See also". Do not merge them. The game wiki should be readable by someone who has never seen the code; the dev wiki should be readable by someone who does not care about the game narrative.

## What NOT to put in the wiki

- Source code (lives in `src/`).
- Build artifacts (`build/`).
- Tooling scripts (`tools/`, `docs/discord_*.py`) — these stay in place; the wiki just links to them from a `dev/subsystems/` page if relevant.
- Content assets (`content/`).
- In-progress conversation state — use Claude Code plan files or tasks, not wiki pages.

## Bootstrapping state

As of 2026-04-07, the wiki was seeded by migrating these files from `docs/`:

| From | To |
|---|---|
| `docs/updates/development_roadmap.md` | `wiki/dev/roadmap.md` |
| `docs/design/gnome_ai_redesign.md` | `wiki/dev/decisions/gnome-ai-redesign.md` |
| `docs/worldbuilding/game_identity.md` | `wiki/game/lore/identity.md` |
| `docs/ai_sprite_generation.md` | `wiki/dev/subsystems/sprite-generation.md` |
| `docs/visual_identity.md` | `wiki/game/lore/visual-identity.md` |
| `docs/updates/parallelization_plan.md` | `wiki/dev/decisions/parallelization.md` |

Everything else in `docs/` remains in place for now. The long-term split is:

- **`docs/` = operational tooling + data pipelines** (computer-owned): `discord_*.py`, `generate.py`, `process_suggestions.py`, `*.json` data files, `Pipfile`, `lib/`, `discord_announcements.md`. These stay put — they are a working Python project, not knowledge.
- **`wiki/` = curated human knowledge** (LLM-maintained): everything narrative, interpretive, or design-oriented.

Knowledge currently living in `docs/` that should eventually migrate to `wiki/` (do this incrementally, on a per-topic basis, not in one bulk move):

| `docs/` path | Eventual home |
|---|---|
| `docs/bug-reports/` | Active ones → `wiki/tasks/inbox/`; historical → `wiki/raw/bug-reports/` |
| `docs/changelogs/` | `wiki/raw/changelogs/` (DEVLOG.md is the live changelog now) |
| `docs/dev-discussion/` | Ingest into `wiki/dev/` pages; originals → `wiki/raw/` |
| `docs/research/` | `wiki/raw/research/` |
| `docs/suggestions/` | `wiki/raw/suggestions/` (feeds the intake pipeline) |
| `docs/updates/gui/`, `docs/updates/notes/` | `wiki/raw/` or ingest into `wiki/dev/` |
| `docs/VERSIONING.md` | `wiki/dev/decisions/versioning.md` |

The rule of thumb: whenever one of these folders becomes relevant to a conversation, ingest it properly at that moment, then remove the originals. Over time `docs/` naturally shrinks to just tooling.
