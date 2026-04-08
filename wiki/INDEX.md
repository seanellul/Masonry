# Masonry Wiki — Index

> Read this first. Catalog of every wiki page. Drill into entries by following links. For rules, see [SCHEMA.md](./SCHEMA.md). For history, see [LOG.md](./LOG.md).

## Game Wiki

### Lore
- [identity](./game/lore/identity.md) — Masonry's core identity, tone, and worldbuilding pillars.
- [visual-identity](./game/lore/visual-identity.md) — Logo, sandstone palette, pixel-art style rules, image-gen learnings.

### Systems
- [buildings](./game/systems/buildings.md) — Complete catalog of workshops, structures, furniture, utility, and containers with function descriptions (mirrors `$BuildingDesc_*` runtime tooltips).
- *(pending: farming, jobs, military, pathfinding, stockpiles, economy)*

### Creatures
*(stubs pending — gnomes, animals, monsters, automatons)*

### UI
- [keybindings](./game/ui/keybindings.md) — All keyboard shortcuts, grouped. Reference for the in-game Settings → Keybindings tab.
- *(pending: main HUD, side panels, tile info, main menu)*

## Dev Knowledge Base

### Top-level
- [roadmap](./dev/roadmap.md) — Development roadmap and milestones (migrated from `docs/updates/development_roadmap.md`).
- [known-issues](./dev/known-issues.md) — Current bugs, tech debt, scaling bottlenecks.

### Architecture
*(stubs pending — threading model, aggregator pattern, event connector, rendering pipeline, database)*

### Subsystems
- [sprite-generation](./dev/subsystems/sprite-generation.md) — AI sprite generation tools, workflows, experiments (migrated from `docs/ai_sprite_generation.md`).
- [skills](./dev/subsystems/skills.md) — **Audit**: what every skill actually does in gameplay. 5 hot-path, 23 quality-crafting, 10 thought-only, 9 dead. Authoritative source for skill tooltips.

### Decisions (ADRs)
- [gnome-ai-redesign](./dev/decisions/gnome-ai-redesign.md) — Gnome AI/behavior-tree redesign proposal.
- [parallelization](./dev/decisions/parallelization.md) — Parallelization plan for the simulation loop.

## Tasks

Kanban lives in `tasks/`. Read the folder to see current state:
- [inbox/](./tasks/inbox/) — raw ideas
- [scoped/](./tasks/scoped/) — planned, ready to build
- [in-progress/](./tasks/in-progress/) — active work
- [blocked/](./tasks/blocked/) — waiting on prerequisites
- [done/](./tasks/done/) — archived (DEVLOG is downstream)

Agent prompts for the task lifecycle:
- [PROMPT_INTAKE_AGENT](./tasks/PROMPT_INTAKE_AGENT.md)
- [PROMPT_SCOPING_AGENT](./tasks/PROMPT_SCOPING_AGENT.md)
- [PROMPT_BUILDING_AGENT](./tasks/PROMPT_BUILDING_AGENT.md)
- [PROMPT_FIXING_AGENT](./tasks/PROMPT_FIXING_AGENT.md)

## Raw sources
See `raw/` — immutable source material. Do not modify.

## Outputs
See `outputs/` — generated reports, comparisons, analyses from queries.
