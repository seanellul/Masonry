---
id: T-0011
title: RimWorld-style manual work priorities in Population view
type: feature
created: 2026-04-07
blockers: []
tags: [ui, population, priorities, gameplay]
---

## Description

Replace the current enabled/disabled checkmark per gnome × skill with a **RimWorld-style numeric priority grid**. Each gnome × work type cell holds either a priority number (`1` = highest, higher numbers = lower priority, blank = disabled) or is empty. Clicking a cell cycles through priorities; right-clicking (or shift-clicking) clears it. The player uses this grid to tell each gnome which work types they should focus on and in what order.

This is the standard RimWorld interaction model. Reference: the Work tab in RimWorld's population screen.

### Work types vs skills — design note for scoping

Masonry's Skills tab currently shows one column per **skill** (Mining, Masonry, Stonecutting, Woodcutting, Carpentry, Woodcarving, Smelting, Blacksmithing, Metalworking, Weaponsmithing, Armorsmithing, Gemcutting, Jewelling, Glassmaking, Weaving, Tailoring, Dyeing, Pottery — plus presumably Hauling, Cooking, etc. elsewhere).

RimWorld's priorities apply to **work types** (categories of jobs), not individual skills. Scoping must decide — by reading the existing Masonry job/skill system — whether:

- (a) Work priorities are **per-skill** (one priority per skill column). Simpler; matches current grid layout.
- (b) Work priorities are **per-category** (Hauling, Construction, Crafting, Mining, etc.), and the grid columns change accordingly.
- (c) Masonry already has a concept of "work type" distinct from "skill" that should be used.

Whichever answer, the existing Professions tab may already have the right concept — scoping should check it first and reuse whatever's there rather than inventing a new taxonomy.

### Manual priority UI (must-have)

- Grid layout matching RimWorld's Work tab: rows = gnomes, columns = work types.
- Each cell is clickable:
  - Left click cycles `blank → 1 → 2 → 3 → 4 → blank`
  - Right click (or shift-click) clears the cell to blank
- Priority `1` is visually distinct from `4` (color or weight) so the grid is scannable at a glance.
- Priorities persist across save/load.
- The job system honors priorities: a gnome with priority 1 on Mining and 3 on Hauling will always prefer a mining job over a haul job when both are available.
- Gnomes with a blank priority for a work type will **not** do that work type at all.

### Auto priority toggle (conditional — see below)

A global **Manual / Auto** toggle above the grid:

- **Manual mode** (default): the priorities the user sets are used as-is.
- **Auto mode**: the game assigns priorities based on a default ruleset (see below). The user's manual values are either overridden or grayed out while Auto is active.

#### Auto ruleset

User has confirmed: **copy RimWorld's default behavior**. That resolves into two pieces:

**Column order (left-to-right priority / tiebreaker order):**

```
1.  Firefighting
2.  Patient (seeking treatment)
3.  Doctor (treating others)
4.  Bed Rest
5.  Basic (flick, door opening, etc.)
6.  Warden
7.  Handle
8.  Cook
9.  Hunt
10. Construct
11. Grow
12. Plant Cut
13. Mine
14. Smith
15. Tailor
16. Art
17. Craft
18. Haul
19. Clean
20. Research
```

**Masonry mapping** — user directive: **do not introduce any work types Masonry does not already have.** Use the RimWorld list purely as a *reference for ordering the work types that already exist in Masonry*.

- If Masonry has no Firefighting, Warden, Doctor, Patient, Bed Rest, or Research systems, those rows are simply skipped. Do not stub them.
- For work types Masonry *does* have, use the RimWorld list's left-to-right order to place them in the grid. Example: if Masonry has Hauling, Mining, Construction, Cooking, and Crafting, their column order follows RimWorld's relative ordering (Cook → Construct → Mine → Craft → Haul).
- For Masonry-specific work types not in the RimWorld list (Glassmaking, Gemcutting, Jewelling, Pottery, Dyeing, Weaving, Stonecutting, Woodcutting, Woodcarving, Smelting, Metalworking, Weaponsmithing, Armorsmithing, etc.), insert them at sensible positions adjacent to their closest RimWorld analog (mostly grouped with "Smith"/"Craft"/"Art"). Scoping surfaces the proposed ordering to the user for sign-off before writing code.

Scoping's output includes the final column-order list of Masonry's actual work types, and the user signs off on it before any code is written.

**Numeric auto values (RimWorld out-of-the-box rule):**

- Every gnome × work type the gnome is *capable of* → priority `3`.
- Work types the gnome is *incapable of* → blank.
- Passion / skill-level bumps are **out of scope** for this task — if Masonry has no passion system, the rule stays flat at 3. A follow-up task can layer passion or skill-based auto-bumping later.

When the user toggles **Auto**, the grid is recomputed from this rule. Manual values are preserved (not destroyed) when switching to Auto, so toggling back to Manual restores what the user had.

### Data model considerations

- The priority value is a new per-gnome per-work-type field. Scoping must decide where it lives: a new table, a JSON blob on the gnome/creature row, a std::map serialized with the save, etc.
- Save format compatibility: loading an old save without priorities should default every gnome × work type to a reasonable value (likely priority 3 — RimWorld's default) rather than blank (which would disable all work).
- The job scheduler (`JobManager` or equivalent) needs to consult priorities when picking the next job for an idle gnome.

### Acceptance criteria

- Population view Skills (or new Work) tab shows a numeric priority grid per RimWorld's pattern.
- Left click cycles priorities 1→4→blank; right-click clears.
- Priority `1` is visually distinct from `4`.
- Priorities persist across save/load; old saves migrate gracefully with a sensible default.
- Gnomes pick jobs in priority order; blank = refuses that work type.
- Manual/Auto toggle exists. Auto behavior follows Path 1 or Path 2 above depending on user input at kickoff.
- Visual verification via `mcp__ingnomia-test__take_screenshot` on the new grid with mixed priorities set.
- Behavioral verification: set one gnome to priority 1 Mining + blank Hauling, another gnome to the inverse, run ticks via `mcp__ingnomia-test__run_ticks`, confirm each gnome does their assigned work and not the other's.

### Out of scope

- Schedule tab changes (work priorities are separate from the time-of-day schedule).
- Global pause/suspend work.
- Rebalancing existing skill effects (T-0008a handles that investigation).

## Plan

*(Scoping agent: (1) Decide per-skill vs per-category grid layout by reading the existing job system and Professions tab. (2) Map the RimWorld column-order list (see "Auto ruleset" above) to Masonry's actual work types, drop entries with no underlying system, insert Masonry-specific work types at sensible positions, and get user sign-off on the final ordering before writing code. (3) Design the data model and save migration. (4) Design the grid UI cell interaction. (5) Wire the job scheduler to consult priorities + the new column tiebreaker order. (6) Propose the full implementation sequence.)*

## Result

*(Building agent fills in after implementation.)*
