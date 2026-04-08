---
id: T-0008b
title: Add skill tooltips to the gnome info UI
type: feature
created: 2026-04-07
blockers: [T-0004, T-0008a]
tags: [ui, tooltips, skills, wiki-content]
---

## Description

Add hover tooltips to every skill shown in the gnome creature info panel (and anywhere else skills appear in the UI). Each tooltip explains what the skill does and how higher levels affect gameplay.

### Hard dependencies

- **T-0004** — establishes the tooltip pipeline this task must reuse, not reinvent.
- **T-0008a** — establishes the authoritative source for what each skill actually does. T-0008b's tooltip copy is derived directly from `wiki/dev/subsystems/skills.md`. If the audit says a skill has no gameplay effect, the tooltip must say so honestly (e.g. *"Currently tracked but has no gameplay effect"*) rather than pretend.

### Coverage

- **Primary location**: the gnome creature info panel (recently updated with equipment/combat info per `DEVLOG.md`).
- **Scoping must also find** any other UI locations that display skills — profession assignment screen, gnome list, military squad panel, etc. — and add tooltips in all of them. Consistency matters; players should not see a skill name with a tooltip in one place and without in another.

### Content format

Slightly richer than T-0004/T-0007 because skills benefit from a two-part description:

```
Hauling
Affects how quickly gnomes carry items between stockpiles and job sites.
Higher levels reduce haul time proportionally.
```

- Line 1: bold name
- Line 2: what the skill affects (one line)
- Line 3: how the level matters (one line)

If T-0008a finds a skill has no effect, the tooltip should be honest:

```
Masonry
Currently tracked but has no gameplay effect.
```

### Acceptance criteria

- Every skill displayed in the gnome info panel has a hover tooltip.
- Every other UI location that shows skills also has tooltips (identified during scoping).
- Tooltip content is sourced from `wiki/dev/subsystems/skills.md` via the T-0004 pipeline — no hardcoded copy scattered in `src/gui/`.
- Dead/ineffective skills are labeled honestly.
- Visual verification via `mcp__ingnomia-test__take_screenshot` with cursor on representative skills in each UI location.

### Out of scope

- Fixing broken skills discovered in T-0008a (separate follow-up tasks).
- Rebalancing skill effects.
- Redesigning the skills section of the gnome info panel.

## Plan

*(Scoping agent: blocked on T-0004 and T-0008a. Once both unblock: (1) Find every UI location that displays skills — grep for skill rendering in `src/gui/ui/`. (2) For each, wire tooltips into the T-0004 pipeline. (3) Populate tooltip content from `wiki/dev/subsystems/skills.md`. (4) Verify the honest-labeling behavior for any skills the audit flagged as ineffective.)*

## Result

*(Building agent fills in after implementation.)*
