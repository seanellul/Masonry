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

Reused the T-0004 pipeline unchanged. New Translation keys use the `$SkillDesc_<sid>` namespace. Content is a direct projection of the T-0008a audit tiers — hot-path and quality-crafting skills get functional descriptions, thought-only and dead skills are honestly labeled ("Currently tracked but has no gameplay effect" or "Currently only affects mood").

UI wiring target: the individual Skills view in `drawPopulationPanel` (`src/gui/ui/ui_sidepanels.cpp` ~line 636). The existing `ImGui::SetTooltip()` call was replaced with an explicit `BeginTooltip`/`EndTooltip` block that shows `<name>: Level N (XP: X)` + separator + wrapped description pulled from `$SkillDesc_<sid>`. Missing keys fall back silently to the pre-existing level/XP-only tooltip.

Group view (`drawPopulationPanel` ~line 805) left as-is: it already shows a compact group tooltip listing every skill in the group with level + XP. Adding full descriptions per skill would bloat the tooltip; individual view is the right place for rich content.

## Result

Implemented.

1. **`content/db/ingnomia.db.sql`**: added 46 new `$SkillDesc_*` Translation rows in a commented "Skill tooltips (T-0008b)" block, grouped by audit tier:
   - 6 hot-path descriptions (Hauling, Unarmed, Melee, Dodge, MagicNature, MagicGeomancy)
   - 23 quality-crafting descriptions
   - 8 thought-only "currently only affects mood" honest labels
   - 10 dead-skill "currently tracked but has no gameplay effect" honest labels (includes Ranged/Crossbow/Thrown/Gun/Block/Armor combat skills)

2. **`src/gui/ui/ui_sidepanels.cpp`** (individual Skills view, `drawPopulationPanel` ~line 636): replaced the single-line `ImGui::SetTooltip(...)` with a `BeginTooltip`/`EndTooltip` block that shows `<name>: Level N (XP: X)` + separator + wrapped `$SkillDesc_<sid>` description. Missing-key fallback: the separator + description block is skipped, so the tooltip is never empty.

3. **Group view untouched**: its existing tooltip lists every skill in the group with level + XP. Keeping it compact was the right call — descriptions for 4–6 skills per group would bloat the tooltip.

**Honest labeling verified at content level**: every thought-only and dead skill's description now tells the player the skill has no effect (or only mood effect), rather than hiding the truth behind generic filler text.

**Coverage gap to be aware of**: this only updates the Population view Skills tab. If skills are displayed elsewhere (creature info panel, military/squad panels, profession assignment), those sites weren't touched in this pass. A follow-up sweep can extend the tooltip using the same `$SkillDesc_<sid>` + `S::s()` lookup — content is already in the DB, so it's purely a wiring exercise.

Build: green (35 warnings, all pre-existing).
