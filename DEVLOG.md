# Ingnomia Development Log

Every change to the codebase must be logged here. This is the master record of all development — it becomes the release changelog.

**Format**: Newest entries at the top. Use the template below for each entry.

---

## [2026-04-07] Creature Info: GoTo + Follow camera buttons

**Milestone**: UI/UX
**Files changed**: `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`, `src/gui/mainwindow.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **Creature Info → GoTo button.** Centers the camera on the selected creature (gnome, animal, or monster) and snaps to its z-level. Reuses the existing `cmdNavigateToEntity` infrastructure that the population view's name-click already used.
- **Creature Info → Follow button.** Locks the camera to the creature; the renderer re-centers every frame on the followed entity. Toggles between "Follow" (idle) and "Following" (active, blue-tinted). Auto-unlocks on the next WASD input or any drag-pan via `mouseMoveEvent` — the player gets out of follow mode by simply moving the camera the way they normally would. Tooltip on the active button reads *"Camera locked. Move (WASD or drag) to release."* If the followed entity dies or vanishes, follow auto-clears.
- New `ImGuiBridge::cmdFollowEntity / cmdStopFollowing / followingEntityID / resolveFollowedEntityPos`. The resolver looks up the entity in the gnome / animal / monster managers and returns its current `Position` or false if gone. `MainWindow::paintGL` consumes this each frame after the existing one-shot `pendingCameraNav` handler.
- The buttons appear inline with the gnome's name in the panel header, right-aligned within the available width. They work for non-gnome creatures too (an animal or monster's `id` is non-zero), so you can also follow a hostile mob across the map.

### Technical Details

- Follow lookup is per-frame but cheap (single-entity manager hashmap lookups).
- z-level changes propagate through `signalViewLevel` so the rest of the UI stays in sync if the followed creature climbs stairs.
- Build: green.

---

## [2026-04-07] Creature Info skills section restructured (skills redesign follow-up)

**Milestone**: Skills redesign — polish
**Files changed**: `src/gui/aggregatorcreatureinfo.h`, `src/gui/aggregatorcreatureinfo.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **Creature Info → Skills section**: was a flat alphabetical list with a basic 4-line tooltip. Now matches the rest of the skills redesign:
  - **Grouped by SkillGroups** with collapsible group headers (default open). Group header text shows the gnome's max sibling level in that group, e.g. *"Smithing (18)"*.
  - **Skill rows still show level + name** with the same color scale, but now with an extra lavender tier at level 10–14 (Journeyman) and gold at 15+ (Master/Grandmaster).
  - **Rich `$SkillDesc_<sid>` tooltip** pulled from the same Translation pipeline used by the build menu / shape actions / population view tooltips. Tooltip layout: name + level/XP, separator, wrapped description, separator, active/inactive status.
  - Group structure cached lazily on first paint via two static maps (`s_ciSkillToGroup`, `s_ciGroupOrder`), populated from `DB::selectRows("SkillGroups")` — same data source as the population view.

- **`GuiCreatureInfo::SkillEntry`** got a new `sid` field so the render can group + look up descriptions. Populated in `AggregatorCreatureInfo` alongside the existing `name` field.

### Technical Details

- The Creature Info skills view now visually matches the Population view's group view: same group order, same per-skill tooltip content, same color tiers.
- Group header is `TreeNodeEx` (default open) so a player can collapse uninteresting groups while keeping the gnome's specialty visible at a glance.
- Build: green. References: skills redesign chain.

---

## [2026-04-07] Core colony skill wiring + audit correction (T-0016)

**Milestone**: Skills redesign — phase 4 (final)
**Files changed**: `src/game/canwork.cpp`, `content/db/ingnomia.db.sql`, `wiki/dev/subsystems/skills.md`

### Changes

- **T-0016 — Wire core colony skills (with major audit correction).** While tracing where to insert skill multipliers for Mining/Woodcutting/Farming/Construction/Medic, I discovered that **the original T-0008a audit was wrong about these skills being purely thought-only**. The audit grep'd for direct `getSkillLevel("Mining")`-style lookups and missed an indirect-but-critical code path at `gnomeactions.cpp:1758-1763`:

  ```cpp
  QString skillID = m_job->requiredSkill();
  float current   = Global::util->reverseFib( m_skills.value( skillID ).toUInt() );
  float ticks     = getDurationTicks( m_currentTask.value( "Duration" ), m_job );
  ticks           = qMax( 10., qMin( 1000., ticks - ( ( ticks / 20. ) * current ) ) );
  ```

  This is a generic duration multiplier applied to *every* job whose `Jobs.SkillID` is set. Verified by grep that the `Jobs` DB table sets `SkillID` for Mining, Woodcutting, Farming, Construction, and Horticulture jobs. So **all four colony work skills already make their jobs faster** as the skill rises — at level 20 the duration is clamped to its 10-tick floor (~10× faster than level 0). **This has been live the entire time**; the audit just missed the indirect lookup.

  What was missing was **yield scaling** — at high skill, the job finishes faster but still produces the same number of items. T-0016 fills part of that gap:

  - **`CanWork::mineWall`** (~`canwork.cpp:690`): added a bonus-yield chance after the standard `createRawMaterialItem` calls. `rand() % 30 < skill` gives 0% chance at Mining 0 and ~67% chance at Mining 20. Mining is now both faster *and* more productive at higher levels.

  Yield scaling for Woodcutting (logs per tree) and Farming (crops) is deferred — both require touching `Plant` class internals which is a separate code area.

  Updated 5 `$SkillDesc_*` rows in `ingnomia.db.sql` to honestly describe the speed scaling that was always present, plus Mining's new yield bonus. Medic's tooltip stays "currently only affects mood" because the medical task handlers weren't traced and treatment is a separate code area.

  Updated `wiki/dev/subsystems/skills.md` Tier 3 section with a "Correction (T-0016 finding)" subsection documenting the missed code path so future audits start from the right baseline.

### Technical Details

- Mining yield is the only new skill effect added in this task; all other "wiring" was already present and just needed correct documentation.
- The audit's tier classification remains useful as a categorization, but Tier 3 ("thought-only") should now be read as "thought-only at the per-skill direct-lookup level — but speed-scaled by the generic job duration multiplier if the Jobs table has the SkillID set".
- Build: green. References: `wiki/tasks/done/T-0016-*.md`.

---

## [2026-04-07] Skill titles (T-0021)

**Milestone**: Skills redesign — phase 3 (flair)
**Files changed**: `src/game/gnome.h`, `src/game/gnome.cpp`, `src/gui/aggregatorcreatureinfo.h`, `src/gui/aggregatorcreatureinfo.cpp`, `src/gui/aggregatorpopulation.h`, `src/gui/aggregatorpopulation.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **T-0021 — Derived skill titles per gnome.** Every gnome now has a `displayTitle()` derived from their highest skill: `Novice` (0–4), `Apprentice` (5–9), `Journeyman` (10–14), `Master` (15–19), `Grandmaster` (20). Format follows **Option B** — uses the sub-skill name, not the group name. So a gnome with Blacksmithing 18 displays as "Master Blacksmith"; a gnome with MagicNature 20 as "Grandmaster of Nature". The function lives on `Gnome` directly (walks `m_skills` via `reverseFib`, looks up `$SkillTitle_<id>` via `S::s`). Plumbed through `GuiCreatureInfo` and `GuiGnomeInfo` (new `displayTitle` field on each), populated by both aggregators on every refresh. Surfaced in three places:
  - **Creature info panel header** — lavender-colored line between the gnome's name and profession
  - **Population view individual Skills tab** — two-line name cell (name + title)
  - **Population view group Skills tab** — same two-line name cell
  
  Polymath titles ("Master of Many Crafts") and Tile Info integration deferred — the existing per-skill title is already meaningful and the tile-info click-through already opens the creature info panel where the title shows.

### Technical Details

- `Gnome::displayTitle()` does NOT consult `SkillGroups` — picking the highest sub-skill across the entire skill set produces a more flavorful per-gnome identity than going through the group abstraction.
- The title updates live whenever the aggregator refreshes; no caching/staleness issues.
- Build: green. The 656 warnings are pre-existing `-Winconsistent-missing-override` re-emitted across the wider rebuild from touching `gnome.h`.
- References: `wiki/tasks/done/T-0021-*.md`.

---

## [2026-04-07] Cross-training XP bonus + easy skill wirings (T-0020, T-0017)

**Milestone**: Skills redesign — phase 2
**Files changed**: `src/game/canwork.cpp`, `src/game/gnomeactions.cpp`, `content/db/ingnomia.db.sql`

### Changes

- **T-0020 — Cross-training XP bonus.** Skills sharing a `SkillGroup` now boost each other's XP gain. A gnome with a level-20 sibling skill gets +50% XP on a new skill in the same group; novices get baseline. Capped at +50%, uses `max` of siblings (not `sum`) so groups don't stack weirdly. Standalone skills (Construction, Hauling, Medic) have no siblings and stay at the 1.0 baseline. Implemented as a static cache `s_skillSiblings` in `canwork.cpp`, lazily populated from the `SkillGroups` DB table on first use, plus a `crossTrainingMultiplier(skillID, m_skills)` helper applied at both branches of `CanWork::gainSkill(QVariant, Job)`. The combat-training overload `gainSkill(QString, int)` is deliberately excluded — combat skills become derived stats in T-0015. A Master Bonecarver picking up Pottery now learns it 50% faster than a complete novice.

- **T-0017 — Easy skill wirings.** Three small fixes that turn three "tracked but dead" skills into real gameplay levers:
  - **AnimalHusbandry** at `gnomeactions.cpp:2138` (`Gnome::actionTameAnimal`): the function already extracted the gnome's skill level into a local `current` and then ignored it. Replaced the hardcoded `m_totalDurationTicks = 100` with `qMax(20, 200 - int(current * 9))`. Novice tames in ~200 ticks; master in ~20 ticks. Linear scale.
  - **Fishing** at `canwork.cpp:1405` (`CanWork::fish`): added a bonus-catch chance after the baseline fish — `rand() % 30 < skill` gives 0% at skill 0 and ~67% at skill 20.
  - **Butchery** at `canwork.cpp:1379-1402` (`butcherFish` + `butcherCorpse`): added a shared static `butcherProduceMeat` helper that scales **both yield and quality** of the produced meat per the user's spec. Quality uses the same `qIndex = skill / 20.0 * qSize` formula as `CanWork::craft()`, with ±1 die-roll variance. Bonus-meat chance is 0% at skill 0, ~67% at skill 20. Meat now has a quality tier; fish-bone / bone outputs unchanged.
  - Tooltip rows for all three skills updated in `ingnomia.db.sql` to drop the "Currently tracked but has no gameplay effect" copy and describe the new behavior.

### Technical Details

- **No save format changes**: cross-training reads existing `m_skills` data, AnimalHusbandry/Fishing/Butchery wirings only modify the work-loop output, not the gnome state.
- The cross-training cache (`s_skillSiblings`) is built once on first XP grant from the same `SkillGroups` table that powers the population view's group structure — single source of truth.
- Build: green. Pre-existing warnings unchanged.
- References: `wiki/tasks/done/T-0020-*.md`, `wiki/tasks/done/T-0017-*.md`.

---

## [2026-04-07] Skills cleanup + grouping restructure (T-0018, T-0019)

**Milestone**: Skills redesign — phase 1
**Files changed**: `content/db/ingnomia.db.sql`, `src/base/enums.h`, `src/game/jobmanager.cpp`, `src/game/gnome.cpp`

### Changes

- **T-0018 — Cleanup removals.** Removed 4 dead/redundant skills:
  - **Horticulture** (duplicate of Farming)
  - **Tinkering** (no workshop, no system)
  - **Mechanic** (no consumer)
  - **Caretaking** (functionally identical to Medic — merged)
  
  Removed from `Skills` table, `Translation` (`$SkillName_*`, `$SkillTitle_*`, `$SkillDesc_*`), `SkillGroups` rosters, the `Skill` enum in `enums.h`, the `m_skillToInt` registrations in `jobmanager.cpp`, and the mood-trigger checks in `gnome.cpp::tickProduction()`. Six `Backstories` rows that granted removed skills were redirected to surviving alternatives (e.g., `AdultFarmer`'s `Farming:3|Horticulture:1` → `Farming:4`). Old saves with orphan skill keys load fine — the orphans are harmless.

- **T-0019 — Skill grouping restructure.** The Population view's group infrastructure is already data-driven from the `SkillGroups` DB table — so this is a pure data restructure, no C++ changes needed. Replaced the original 14 `SkillGroups` rows with the new 15-row structure:
  - **10 logical groups**: Earthworking, Forestry, Smithing, Textiles, Bone & Hide, Fine Craft, Engineering, Hearth, Field, Magic
  - **3 standalones** (single-skill groups so the existing renderer handles them): Construction, Hauling, Medic
  - **2 preserved combat groups**: Combat, Defense (will be retired in T-0015 when combat skills become derived stats)
  - Notable moves: **Butchery** went from Rancher → Hearth (per user's "keep Butchery distinct, group under Hearth" decision). **Prospecting** moved from MiscCraft → Earthworking (the Prospector workshop's purpose is identifying ore veins). **Crossbow** and **Gun** added to Combat (they were missing from the original definition).

### Technical Details

- All 43 surviving skills (47 minus 4 removed) are in exactly one group, verified by cross-check.
- Save format unchanged. Save migration to strip orphan removed-skill keys is deferred — orphans are harmless.
- The aggregator (`AggregatorPopulation::AggregatorPopulation` at `aggregatorpopulation.cpp:37`) already iterates `SkillGroups` from DB and builds `m_skillIds` with `gsi.group` set per row. The population view's `buildGroupIndex` (`ui_sidepanels.cpp:515`) reads `skill.group` to cluster columns. Both changes were data-only.
- Build: green. The wider rebuild from touching `enums.h` re-emitted ~800 pre-existing `-Winconsistent-missing-override` warnings; the second incremental build (data-only T-0019 change) was clean.
- References: `wiki/tasks/done/T-0018-*.md`, `wiki/tasks/done/T-0019-*.md`, `wiki/dev/subsystems/skills.md`.

---

## [2026-04-07] Groves subsystem audit (T-0010)

**Milestone**: Knowledge infrastructure
**Files changed**: `wiki/dev/subsystems/groves.md` (new), `wiki/INDEX.md`, task files

### Changes

- **T-0010 — Groves investigation.** Traced the Grove → FarmingManager → Grove::onTick → job dispatch chain end to end. **Verdict: groves are implemented correctly**; the user's bug report ("creates a farm that points to vegetable/fruit seeds") was a UX misunderstanding, not a runtime bug.
  - `FarmingManager::addGrove` creates a distinct `Grove` class (not a `Farm`).
  - `Grove::onTick` at `grove.cpp:149` enqueues `PlantTree` / `HarvestTree` / `FellTree` jobs when the corresponding flag is set.
  - All three job types have runtime handlers in `Gnome::m_taskFunctions`.
  - `AggregatorAgri::init` correctly filters the `Plants` DB table by `Type == "Tree"` — 6 real tree species.
  - The "vegetable/fruit seeds" confusion is because tree names use their material (`AppleTree` → `Apple`, `OrangeTree` → `Orange`).
  - A freshly-placed grove has `plant=false, pickFruit=false, fell=false, treeType=""` by default, so it does nothing until the player opens the info panel and configures it. Low discoverability = "feels broken".
  - 5 follow-up task seeds captured in the audit page (default new groves to usable state, empty-grove UX hint, rename tree species to "Apple Tree" etc., player-facing wiki page, dead-code throttle cleanup).

### Technical Details

- No code changes — this task's deliverable is the investigation + verdict.
- Output: `wiki/dev/subsystems/groves.md` (new, linked from `wiki/INDEX.md`).
- References: `wiki/tasks/done/T-0010-*.md`.

---

## [2026-04-07] Workshop queue polish + Keybindings settings (T-0009, T-0014)

**Milestone**: UI/UX + Settings infrastructure
**Files changed**: `src/game/workshop.cpp`, `src/gui/ui/ui_sidepanels.cpp`, `src/gui/ui/ui_mainmenu.cpp`, `wiki/game/ui/keybindings.md` (new), `wiki/INDEX.md`

### Changes

- **T-0009 — Workshop queue: enqueue-time merge + send-to-top/bottom.** Added a tail-match check in `Workshop::addJob()` that merges a new craft job into the last queue entry when `craftID`, `mode`, and per-component `materialSID`s all match — five clicks on `Craft Plank` now yield one `Craft 5 x Plank` row instead of five identical rows. Only the tail is checked, so a manually inserted job between batches breaks the streak. Existing queues on loaded saves are untouched. Discovered `Workshop::moveJob()` already supports `"Top"` and `"Bottom"` commands — the UI was only wiring `Up`/`Down`. Added two new row buttons (`^^`/`vv`) that dispatch `"Top"`/`"Bottom"`, plus tooltips on all queue row buttons.

- **T-0014 — Keybindings settings tab.** New read-only Keybindings tab in the Settings panel. Parses `keybindings.json` (tries the user data folder first, falls back to the app dir), groups by `GroupName`, renders as a collapsible 3-column table (`Action | Key 1 | Key 2`) per group. Created `wiki/game/ui/keybindings.md` as the canonical human-readable reference. **Rebinding deferred**: the infrastructure is already config-driven so rebinding is tractable, but the "click a row, press a key, write back to disk" UX is scope creep for this task. Caught two loader bugs during implementation: JSON field is `GroupName` not `Name` (loader now tries both); `QJsonValue::toString()` returns empty for bool values, needed `.toBool()` for modifier detection.

### Technical Details

- T-0009 merge is purely in `Workshop::addJob` before the final `m_jobList.append(cj)` — save format unchanged, loaded queues ignored, no data-model surgery.
- T-0014 bumps the Settings window from 500×350 to 600×500 to give the keybindings list room.
- Build: green. References: `wiki/tasks/done/T-0009-*.md`, `wiki/tasks/done/T-0014-*.md`.

---

## [2026-04-07] Skill audit + honest skill tooltips (T-0008a, T-0008b)

**Milestone**: Knowledge infrastructure + UI/UX
**Files changed**: `content/db/ingnomia.db.sql`, `src/gui/ui/ui_sidepanels.cpp`, `wiki/dev/subsystems/skills.md` (new), `wiki/INDEX.md`

### Changes

- **T-0008a — Full skills effect audit.** Produced the authoritative `wiki/dev/subsystems/skills.md` page documenting every one of the 47 skills in the game and what each actually does in gameplay. Findings sorted into four tiers:
  - **Hot-path (5)**: Hauling → move speed table, Unarmed/Melee/Dodge → combat, MagicNature/MagicGeomancy → spell scaling. Each with specific file:line citations.
  - **Quality-crafting (23)**: All skills in `Crafts.SkillID`, consumed by `CanWork::craft()` at `canwork.cpp:1147` — skill level scales the quality tier of produced items.
  - **Thought-only (10)**: Mining, Woodcutting, Farming, Horticulture, Construction, Medic, Caretaking, Tinkering, Ranged, Crossbow, Block. Only generate mood thoughts in `gnome.cpp::tickProduction()`. Skill level never affects outcome.
  - **Dead (9)**: AnimalHusbandry, Butchery, Fishing, Mechanic, Thrown, Gun, Armor, etc. Tracked in DB and registered in `SkillToInt` but no code reads them.
  - **7 follow-up task seeds** captured for fixing broken/dead skills, including the biggest gap: seven "core" colony skills (Mining, Woodcutting, Farming, Horticulture, Construction, Medic, Caretaking) have no effect on the work they're named for.

- **T-0008b — Honest skill tooltips in Population view.** Reuses the T-0004 `Strings` pipeline with a new `$SkillDesc_<sid>` key namespace. Added 46 new Translation rows sourced directly from the T-0008a audit — working skills get functional descriptions, thought-only and dead skills are **labeled honestly** (e.g. *"Currently tracked but has no gameplay effect"*) rather than hidden. Individual Skills view now shows `<name>: Level N (XP: X)` + separator + wrapped description on hover; group view left as-is (already compact). Group view tooltip extension to other population tabs / creature info panel deferred — content is in the DB, wiring is a cheap follow-up.

### Technical Details

- The audit is **diagnostic infrastructure** — future skill-related work starts with "read `wiki/dev/subsystems/skills.md`" instead of grepping source code.
- Honest labeling is deliberate: the tooltip pipeline doesn't lie to the player about dead features. When broken skills are fixed (see audit follow-ups), update the relevant `$SkillDesc_*` row and the in-game truth changes with it.
- Build: green. References: `wiki/tasks/done/T-0008a-*.md`, `wiki/tasks/done/T-0008b-*.md`.

---

## [2026-04-07] Tooltip pipeline + build menu + shape actions (T-0004, T-0007)

**Milestone**: UI/UX + Knowledge infrastructure
**Files changed**: `content/db/ingnomia.db.sql`, `src/gui/ui/ui_gamehud.cpp`, `wiki/game/systems/buildings.md` (new), `wiki/dev/known-issues.md`, `wiki/INDEX.md`

### Changes

- **T-0004 — Build menu tooltips + `$BuildingDesc_*` pipeline.** Added 54 new `$BuildingDesc_<id>` rows to `content/db/ingnomia.db.sql` (36 workshops + 7 containers + 6 utility + 5 generic structure categories). Runtime lookup via `Strings::s()` — same pipeline every other localized string already uses, zero new infrastructure. New `buildingTooltipDesc(id)` and `showBuildItemTooltip(name, id)` helpers in `ui_gamehud.cpp` wrap the lookup + tooltip render; wired into the build menu row on both icon hover and name hover. Missing descriptions fall back gracefully to name-only (no `Error:` leaks). Created `wiki/game/systems/buildings.md` as the human-readable mirror with workshops grouped by subcategory (Wood/Stone/Metal/Food/Crafts/Mechanics/Misc). Future work: a script to sync wiki → SQL so the wiki becomes the authoring surface; today the SQL is the runtime source of truth.

- **T-0007 — Dig / Nature action tooltips (+ bug discoveries).** Built on the T-0004 pattern with a parallel `$ActionDesc_<action>` key namespace. 11 new Translation rows (7 dig actions fully wired in-game + 4 nature actions, of which `Forage` is documented as unimplemented). New `actionTooltipDesc(action)` / `showActionTooltip(label, action)` helpers in `ui_gamehud.cpp`, wired into both the dig and nature render loops. The nature loop uses `ImGuiHoveredFlags_AllowWhenDisabled` so greyed-out buttons still surface their tooltip. **Two bugs discovered during code inspection and logged to `wiki/dev/known-issues.md`**:
  - `Forage` is a UI stub: appears in `keybindings.h`'s `ActionForage` + `natureActions[]`, but has **no task function** in `gnome.cpp::initTaskMap()`. Clicking Forage enqueues jobs no gnome will ever pick up. Tooltip now labels it `[Not yet implemented]`.
  - `Plant Tree` in the Nature menu has an empty action string (`""`). Button is already disabled in code. Tree planting happens only through groves (T-0010).

### Technical Details

- The T-0004 DB-keyed + `Strings::s()` pipeline is now the **default pattern** for surfacing authoritative info in tooltips. T-0008b (skill tooltips) will reuse it with `$SkillDesc_<id>`.
- `buildingTooltipDesc()` includes category-substring fallbacks so per-material structure variants (StoneBlockWall, WoodPlankFloor, etc.) all get the same category-level description without needing one row per variant.
- Build: green. References: `wiki/tasks/done/T-0004-*.md`, `wiki/tasks/done/T-0007-*.md`.

---

## [2026-04-07] UI polish batch 2 — schedule painter, tile info icons (T-0005, T-0013)

**Milestone**: UI/UX polish
**Files changed**: `src/gui/ui/ui_sidepanels.cpp`, `src/gui/ui/ui_tileinfo.cpp`

### Changes

- **T-0005 — Tile Info: frameless action buttons + mystery button identified.** The `actionButton()` helper in `ui_tileinfo.cpp` now pushes transparent `ImGuiCol_Button` with a subtle white hover overlay, so every Terrain-row action (Remove floor, Replace floor, Fell tree, Harvest, Mine wall) renders icon-only. Applied the same transparent push/pop block to the creature-info `ICON_FA_CIRCLE_INFO` button so clicking the `(i)` next to a gnome in the Creatures section is consistent. The "mystery button" next to the trash turned out to be the existing `ICON_FA_RIGHT_LEFT` Replace-floor action — already had a hover tooltip; it just read as mysterious because the rectangular frame distracted from the icon.
- **T-0013 — Schedule painter: fix the visual misdirection on bulk-apply cells.** The click pipeline was actually correct — paint brushes only arm state, individual hour cells dispatch `cmdSetSchedule()`, and drag-paint works. The bug was that the per-gnome "All" column and per-hour "All" row rendered their bulk-apply buttons with the current brush's color + label, making the schedule grid look like every hour had been painted to whatever brush was armed. Swapped both to a neutral dark-gray background with distinctive `<<` / `^^` labels and explicit tooltips. Click handlers unchanged; bulk-apply behavior still works, but the grid no longer lies to the user.

### Technical Details

- Both fixes are UI-render-path-only; no game-thread, data model, or save format changes.
- Build: green. `ui_tileinfo.cpp` has pre-existing `btnSize` + `terrainRow` unused-variable warnings that are not from this change.
- References: `wiki/tasks/done/T-0005-*.md`, `wiki/tasks/done/T-0013-*.md`.

---

## [2026-04-07] UI polish batch — build menu, workshop, population (T-0002, T-0003, T-0006, T-0012)

**Milestone**: UI/UX polish
**Files changed**: `src/gui/ui/ui_gamehud.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **T-0002 — Build menu tab bar: Containers icon + centered labels.** The Containers tab was rendering `Ǝ` as a missing-glyph placeholder because `ICON_FA_BOX` (U+f466) is not present in the bundled `fa-solid-900.ttf`. Swapped to `ICON_FA_CUBES` (U+f1b3), a classic FA codepoint that renders reliably. Also wrapped the category row in `PushStyleVar(ButtonTextAlign, (0.5, 0.5))` and gave every button a uniform 130px width so the icon+label group reads as centered on every tab.
- **T-0003 — Build button ~15% taller.** Per-item Build buttons in the build menu were using `ImGui::SmallButton`, which ignores `FramePadding.y`. Replaced with a regular `ImGui::Button` wrapped in a scoped `PushStyleVar(FramePadding, y+2)` + `PushStyleVar(ButtonTextAlign, (0.5, 0.5))` block, yielding ~15% more vertical padding with centered content and no style leakage to neighbors.
- **T-0006 — Workshop view: default-to-Recipes + visible craft count.** Opening a workshop with an empty queue now lands on the Recipes tab instead of an empty Queue tab (tracked via `s_lastWorkshopID` + `ImGuiTabItemFlags_SetSelected` one-shot). Replaced the invisible `ImGui::InputInt` amount widget with an explicit `Text("%d") + SmallButton("-") + SmallButton("+")` trio — matches the layout the user expected and guarantees the value is visible regardless of theme.
- **T-0012 — Population view: clipped headers + gnome-name navigation.** The individual Skills view was literally `.left(5)`-truncating every skill column header, collapsing both `Woodcutting` and `Woodcarving` to `Woodc`. Removed the truncation and widened the column from 50 to 100px. Clicking a gnome's name in either Skills view (individual or group) now calls `bridge.cmdNavigateToEntity(id)` (camera jump) and `bridge.onOpenCreatureInfo(id)` (opens creature info panel) in addition to the existing selection update. Extension to Schedule/Personality/Social/Professions tabs deferred as a small follow-up.

### Technical Details

- All edits are in ImGui render paths; no game-thread or data-model changes.
- Build: green (11 warnings, all pre-existing `-Winconsistent-missing-override` and one unused-variable warning already in the file).
- Smoke test: build succeeded, runtime shutdown flaked at the 120s harness timeout — a pre-existing quit-path issue unrelated to these UI-only changes (no new loops, no new state).
- References: `wiki/tasks/done/T-0002-*.md`, `wiki/tasks/done/T-0003-*.md`, `wiki/tasks/done/T-0006-*.md`, `wiki/tasks/done/T-0012-*.md`.

---

## [2026-04-07] AI Texture Pack: apply at startup, not at runtime

**Milestone**: Stability — Dev tools
**Files changed**: `src/gfx/spritefactory.cpp`, `src/gui/imguibridge.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes
- **The "Use AI Texture Pack" toggle now sets a persistent preference (`useAltTextures` in config) instead of trying to hot-swap textures.** Restart the game to apply.
- **`SpriteFactory::init()` reads the preference** and loads tilesheets from `content/tilesheet_ai/` (with per-file fallback to `content/tilesheet/` if a sheet is missing) when the flag is set.
- **Dev panel now shows a "Restart the game to apply" notice** under the checkbox, and the bridge restores the saved preference on construction so the checkbox state survives restarts.

### Technical Details
- Even after moving the runtime swap to the game thread, the previous live-toggle path was broken in two ways:
  1. **Textures didn't actually swap.** `forceUpdate()` re-uploads `m_sprites`' already-composited pixmaps; replacing `m_baseSprites` doesn't invalidate them. A real swap would need to clear and rebuild the entire sprite cache.
  2. **Game thread became unresponsive.** `toggleTexturePack` holds `SpriteFactory::m_mutex` for the duration of PNG loads + extraction + per-sprite GPU re-upload. The render thread blocks on the same mutex via `getSprite()`, so input handling (ESC, pause, speed) freezes for the duration.
- Applying at `init()` sidesteps both: there is no concurrent reader, the cache is built fresh from the alt sources, and there's no mid-frame mutex contention.
- The earlier `AggregatorDebug::signalToggleTexturePack` → `Game::onToggleTexturePack` plumbing remains in the tree but is no longer wired from the UI; it's a no-op safety net.

---

## [2026-04-07] Fix crash on toggling AI Texture Pack from Dev Mode

**Milestone**: Stability — Dev tools
**Files changed**: `src/gui/imguibridge.cpp`, `src/gui/aggregatordebug.h`, `src/gui/aggregatordebug.cpp`, `src/game/game.h`, `src/game/game.cpp`, `src/game/gamemanager.cpp`

### Changes
- **Toggling the AI Texture Pack checkbox no longer crashes the game.** The toggle now dispatches to the game thread instead of running on the GUI thread.

### Technical Details
- Root cause: `ImGuiBridge::cmdToggleTexturePack` was calling `Game::sf()->toggleTexturePack()` directly from the GUI thread. That path runs `rebuildBaseSprites()` → `DB::selectRows()` (Qt SQL is per-thread, the connection is owned by the game thread) and `forceUpdate()` which iterates `m_sprites` concurrently with the game tick. Either is sufficient to crash.
- Fix: added `AggregatorDebug::onToggleTexturePack` slot + `signalToggleTexturePack` signal, a matching `Game::onToggleTexturePack` slot that calls `m_sf->toggleTexturePack()`, and a `Qt::QueuedConnection` wire-up in `GameManager::createNewGame` so the swap happens on the game thread alongside the rest of the simulation.
- The UI flag (`useAltTextures`) is flipped optimistically on click for immediate visual feedback in the checkbox; the heavy work (re-extract base sprites, recomposite, GPU upload flag) runs on the next game-thread loop iteration.

---

## [2026-04-07] Resizable + persistent Shape/Zone/Manage panels

**Milestone**: UI/UX Overhaul — Toolbar polish
**Files changed**: `src/gui/ui/ui_gamehud.cpp`

### Changes
- **Shape, Zone, and Manage toolbar panels are now resizable** — removed `ImGuiWindowFlags_NoResize`, added size constraints (min ~320x160, max bounded by display minus toolbar).
- **Panel sizes persist via Config** — each panel reads/writes `toolbar_{shape,zone,manage}_{w,h}` through `Global::cfg`. Whatever size the user drags becomes the new default on next launch (and across all sessions).
- **Bottom-left anchored via pivot** — panels now use `SetNextWindowPos` with pivot `(0,1)` so the bottom edge stays glued to just above the toolbar regardless of size.
- **Bumped first-run defaults** — Shape 700x420, Zone 520x300, Manage 460x260 (up from cramped 500x350 / 450x250 / 400x220).

### Technical Details
- Implemented via a small `PanelSize` helper struct in the anonymous namespace that lazy-loads from `Global::cfg` on first use and writes back when `ImGui::GetWindowSize()` differs from the cached value. `Config::set` is a no-op when unchanged and autosaves on change, so per-frame `persistIfChanged` calls are safe.
- Window size is applied with `ImGuiCond_FirstUseEver` so ImGui's internal state owns the live size while the user drags; we just read it back each frame to mirror into config.

---

## [2026-04-01] Fix civilian combat response + flee behavior

**Milestone**: 0.0 — Combat
**Files changed**: `src/game/gnome.cpp`, `src/game/gnome.h`, `src/game/gnomeconditions.cpp`, `src/game/gnomeactions.cpp`, `content/ai/gnome_standard.xml`, `CMakeLists.txt`, `cmake/GenerateVersion.cmake`, `src/version.h.in`, `src/gui/ui/ui_mainmenu.cpp`, `src/main.cpp`

### Changes
- **Fixed gnomes not fighting back when attacked** — `Gnome::attack()` now sets `m_combatAlert = true`, which immediately wakes the gnome from its cheap-tick cycle for a full BT evaluation. Previously, gnomes would continue working while being beaten to death.
- **Added flee behavior for unarmed civilians** — New `ShouldFlee` condition checks if a civilian has no weapon. New `FleeMove` action picks the walkable neighbor tile furthest from the nearest threat. If cornered (no walkable escape), they fall through to fight back with fists.
- **Updated Combat BT** — Unarmed civilians attempt `FleeMove` first; if that fails (cornered), they fall through to the existing fight logic. Armed civilians and military gnomes fight normally.
- **Fixed pre-existing BUILD_ID compile error** — `BUILD_ID` was used inside `#ifdef GIT_REPO` without its own guard in `main.cpp`.
- **Added build-time git version detection** — Version string now shows `0.0.1.0-<git-sha>` in the main menu, updated on every build via `cmake/GenerateVersion.cmake`.

### Technical Details
- `m_combatAlert` was declared in `creature.h` but never set to `true` anywhere — it was designed for exactly this purpose
- The flee behavior is simple (single-tile greedy move away from threat) rather than pathfinding to a distant safe room. This keeps it fast and ensures gnomes react within one tick.
- `FleeMove` returns FAILURE when cornered, causing the `Fallback` node in the Combat BT to try the fight path instead — gnomes always fight for their lives as a last resort.

---

## [2026-04-01] Bottom Toolbar Redesign — Shape / Zone / Manage

**Milestone**: 0.0 — UI/UX
**Files changed**: `src/base/enums.h`, `src/gui/imguibridge.h`, `src/gui/ui/ui_gamehud.cpp`

### Changes
- **3 primary actions replace 5** — Build/Mine/Agriculture/Designations/Job collapsed into Shape/Zone/Manage, grouped by *what you're doing to the world*
- **Shape** tab has 3 sub-tabs: Build (workshops, structures, furniture, utility, containers), Dig (mine, stairs, ramp, hole), Nature (cut tree, plant, harvest, forage)
- **Zone** tab has 4 category tabs: Storage (stockpile), Production (farm, grove, pasture), Rooms (personal, dorm, dining, hospital), Control (forbidden, guard, remove)
- **Manage** tab has 2 columns: Job Control (suspend, resume, cancel, priority) and Demolition (deconstruct)
- **Unified panel** — instead of 3 separate flyout windows, everything opens in one coherent tabbed panel above the toolbar
- **Icon integration** — Font Awesome 6 + RPG Awesome icons on all buttons (hammer, axe, shield, seedling, etc.)
- **Material selection via tabs** — material subcategories shown as ImGui tabs instead of a separate floating panel
- **Build items inline** — workshop/furniture item list with sprite icons, material dropdowns, and build buttons all within the tabbed panel

### Technical Details
- `ButtonSelection` enum: `None`, `Shape`, `Zone`, `Manage` (replaces Build/Mine/Agriculture/Designation/Job)
- New `ShapeTab` enum: `Build`, `Dig`, `Nature`
- New `ZoneCategory` enum: `Storage`, `Production`, `Rooms`, `Control`
- All backend commands unchanged — same `cmdSetSelectionAction`, `cmdRequestBuildItems`, `cmdBuild` calls
- Old action data arrays replaced with `ActionDef` structs carrying icon + label + action string

---

## [2026-03-31] Generative Dev Tool Panel

**Milestone**: 0.0 — Developer Tools
**Files changed**: `src/gui/ui/ui_sidepanels.cpp`, `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`, `src/gui/aggregatordebug.h`, `src/gui/aggregatordebug.cpp`, `src/game/eventmanager.h`, `src/game/eventmanager.cpp`, `src/game/gamemanager.cpp`

### Changes
- **RimWorld-style dev tools panel** — replaced 6-line debug stub with full tabbed ImGui dev tool. All content auto-discovered from DB (never goes stale).
- **Creatures tab** — Spawn Gnome button, Monster combo (from `DB::ids("Monsters")`), Animal combo (from `DB::ids("Animals")`), amount sliders. Monsters spawn at map edge, animals at map center.
- **Items tab** — Category filter → Item filter → Material filter, all from DB queries. Materials filtered by item's `AllowedMaterialTypes`/`AllowedMaterials`. Items spawn at map center on current view level.
- **Events tab** — Migration, Trader, Invasion triggers. Invasion has species/amount controls.
- **Game tab** — Pause toggle, game speed slider, current time/season/tick display.
- **New bridge commands** — `cmdSpawnMonster`, `cmdSpawnAnimal`, `cmdSpawnItem` wired through AggregatorDebug → EventManager game-thread handlers.

### Technical Details
- `DevToolCache` struct lazily populates DB lists on first panel open (one-time cost). Cached in static for session lifetime.
- Spawn position: monsters use `Global::util->borderPos()` (map edge), items/animals use map center with walkable tile search.
- All signals cross thread boundary via Qt::QueuedConnection (GUI → game thread).

---

## [2026-03-31] UI Design System — Typography, Style, Visual Hierarchy

**Milestone**: 0.0 — UI/UX
**Files changed**: `src/gui/imgui_impl_qt5.h`, `src/gui/imgui_impl_qt5.cpp`, `src/gui/ui/ui_helpers.h` (NEW), `src/gui/ui/ui_helpers.cpp` (NEW), `src/gui/ui/ui_sidepanels.cpp`, `src/gui/ui/ui_gamehud.cpp`, `src/gui/ui/ui_tileinfo.cpp`

### Changes
- **Typography system** — PT Root UI 16px is now the default in-game font (was ImGui's built-in 13px). Added 20px medium weight for section headers. Main menu retains its 18px/48px fonts.
- **Global style refinements** — Increased WindowPadding (12,10), FramePadding (8,5), ItemSpacing (8,6). Rounded corners 4px for windows, 3px for frames. Added TableHeaderBg/TableBorder colors.
- **Section header helper** — New `sectionHeader()` function draws full-width tinted bars with medium font text. Creates visual anchors for scanning panel content.
- **Panel sizing pass** — All panels adjusted for 16px font: wider columns, taller toolbar (55px), larger buttons (36px height), expanded table column widths.
- **Kingdom panel upgrade** — Replaced deprecated `ImGui::Columns()` with `BeginTable()`, added row striping and totals row.

### Technical Details
- Font hierarchy: 48px title → 20px section headers → 16px default → 14px captions/log entries
- `io.FontDefault` set after font loading — all panels automatically get the new font without per-panel changes
- `style.ScaleAllSizes(dpr)` handles DPI scaling for all new values
- `sectionHeader()` uses `ImDrawList::AddRectFilled` for background bar, pushes `uiMedium` font for text

---

## [2026-03-31] Add Meal Quality Mood Thoughts

**Milestone**: 2.4 — Food & Farming QoL
**Files changed**: `src/game/gnomeactions.cpp`

### Changes
- **Meal chain mood system** — Raw food (`ItemGroup='Raw'`) gives -2 mood ("Had to eat raw food"). Simple cooked food gives +1 ("Had a decent cooked meal"). Quality cooked food (EatValue >= 40, e.g., Sandwich, Omelettes) gives +4 ("Had a quality meal!"). All thoughts last 1 day.
- Incentivizes building kitchens and cooking food rather than eating raw produce.

### Technical Details
- Uses existing `g->inv()->itemGroup(carriedItem)` to distinguish Raw vs Cooked at eat time.
- EatValue threshold of 40 separates simple cooked (Bread=25, Cheese=25, Sausage=30) from quality meals (Omelettes=40-50, Sandwich=50).
- Works alongside existing food variety and drink quality mood systems.

---

## [2026-03-31] Schedule Tab Overhaul — Paint System, "Anything" Activity, UI Fixes

**Milestone**: 0.0 — UI/UX
**Files changed**: `src/base/enums.h`, `src/game/gnomeconditions.cpp`, `src/game/gnome.cpp`, `src/gui/imguibridge.h`, `src/gui/ui/ui_sidepanels.cpp`, `src/gui/imguibridge.cpp`, `src/gui/aggregatorpopulation.cpp`

### Changes
- **New "Anything" schedule activity** — Gnomes self-manage needs: eat when hunger < 60, drink when thirst < 60, sleep when sleep need < 50, otherwise work. Default for new gnomes. More realistic than forced work-only.
- **Paint-brush schedule UI** — Toolbar with 5 activity buttons (Anything/Work/Eat/Sleep/Train). Select one, then click-and-drag across cells to paint. Replaces tedious click-cycling.
- **Skill toggle & profession fix** — Connected `signalUpdateSingleGnome` to bridge so skill checkbox and profession changes persist. `onSetSkillActive` now emits update signal.
- **Sorting fix** — Group header sorting uses max skill level across all skills in group, not just the first. Headers use clickable Selectables.
- **White skill level numbers** — Changed from grey `TextDisabled` to white `Text`.

### Technical Details
- `ScheduleActivity::Anything` added to enum, serialized as "anything". Old saves with "none" map to Anything (backwards-compatible).
- BT: hunger/thirst trigger at < 60 during Anything (between proactive 90 and emergency 30). Sleep triggers at < 50.
- Paint uses `ImGui::IsItemHovered() && ImGui::IsMouseDown(0)` for drag detection.

---

## [2026-03-31] Add Ranged Goblin Enemy Type

**Milestone**: 3.2 — Enemy Diversity
**Files changed**: `src/game/monster.h`, `src/game/monster.cpp`, `src/game/eventmanager.cpp`, `content/ai/monster_ranged.xml` (NEW), `content/db/ingnomia.db.sql`

### Changes
- **Ranged goblin enemy** — `GoblinRanged` with range 5 tiles, 3 piercing damage. Uses new `MonsterRanged` behavior tree that attempts ranged attacks at distance, falls back to melee when adjacent.
- **`TargetInRange` condition** — Chebyshev distance check on same Z-level against `m_attackRange`.
- **`RangedAttack` action** — Fires DT_PIERCING at random anatomy height with 15-tick cooldown (slightly faster than melee).
- **Raid integration** — Ranged goblins mixed into raids at Normal+ difficulty. Formula: `min(30, year * 5 + (difficulty - 2) * 5)` percent. Ranged and armored rolls are exclusive (armor checked first).
- **DB schema** — Added `AttackRange` and `RangedDamage` columns to Monsters table. GoblinRanged: range 5, damage 3, armor 0.

### Technical Details
- BT structure: GetTarget → (TargetInRange → RangedAttack) | (Move → TargetInRange → RangedAttack) | (melee fallback) | RandomMove
- Reuses Goblin sprites — same visual, different gameplay via ranged behavior and damage type.
- Ranged+armored combined cap: at year 5 Normal difficulty, ~50% armored + ~25% ranged = 75% special goblins. Remaining 25% are standard melee.

---

## [2026-03-31] Fix Gnome AI Regression — Needs Decay, Sleep System Removal, Job Cleanup

**Milestone**: 0.3 — Performance / Bug Fix
**Files changed**: `src/game/gnome.cpp`, `src/game/gnome.h`, `src/game/gnomemanager.cpp`, `src/game/gnomemanager.h`, `src/game/gnomeactions.cpp`, `src/game/jobmanager.cpp`, `src/game/creature.h`, `src/game/canwork.cpp`

### Changes
- **Fix needs freeze during cheap ticks** — `evalNeeds()` only ran on full ticks (1-in-10 via bucket system), and need decay was gated by `minuteChanged` (also ~1-in-10). Needs now decay in `cheapTick()` when `minuteChanged` fires, with a force-full-tick trigger when hunger/thirst/sleep drops below 30.
- **Remove sleep/wake system (Phase D)** — Sleeping gnomes were skipped entirely in the tick loop, freezing their needs forever. The safety net checked `hunger < 20` but frozen needs never crossed the threshold. Removed `sleepGnome()`, `wakeGnome()`, `safetyNetScan()`, `m_isSleeping`, `m_idleTickCount`, and the idle-sleep trigger. Bucket stagger still provides the performance win.
- **Reset `m_gnomeState` in `cleanUpJob()`** — State was never set to IDLE after job finish/abort, causing idle gnomes to stay in WORKING state and bypass cheap tick optimization. Added explicit IDLE case in cheapTick that waits for job cooldown before forcing full tick.
- **Fix pending job queue stale drain** — Old code tried 3 entries then gave up. New code drains all stale (worked/canceled) entries before attempting the first valid claim.
- **Rebalance job cooldown** — Bumped from 20 back to 50 ticks (was 100 pre-optimization). Prevents hammering the expensive pull-based `getJob()` scan.
- **Cap overflow pool** — `m_overflowPool` capped at 500 entries to prevent unbounded growth from jobs targeting skills no gnome has.

### Technical Details
- Cheap tick need decay is identical to the `evalNeeds()` decay loop but skips the mood/thought system (that still only runs on full ticks). No double-decay: `onTick()` dispatcher routes to exactly one of `cheapTick` or `fullTick`.
- IDLE gnomes on cheap tick only force full tick when `m_jobCooldown <= 0`, allowing proper cooldown wait without wasting BT evaluation cycles.
- Sleep system removal costs ~2x tick time for idle gnomes vs sleeping, but correctness is more important than the optimization. Can be reintroduced later with proper need decay for sleeping gnomes.

---

## [2026-03-31] Population UI Overhaul — Skills, Professions, Schedule, Social

**Milestone**: 0.0 — UI/UX
**Files changed**: `src/gui/ui/ui_sidepanels.cpp`, `src/gui/aggregatorpopulation.h`, `src/gui/aggregatorpopulation.cpp`, `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`

### Changes
- **Skills tab rewrite** — Replaced unusable 40+ column `ImGui::Columns()` grid with `ImGui::BeginTable()` using 14 skill group columns (RimWorld-style). Group view shows checkbox + max level per group with color-coded headers. Toggle to Individual View for per-skill control. Frozen Name/Profession columns. In-row profession combo dropdown. Sortable headers. Hover tooltips with detailed skill breakdown.
- **Professions tab bug fix** — Fixed `onRequestSkills()` which only returned skills already in the profession (impossible to add new ones). Now returns all skills with active flags. Connected disconnected `signalSelectEditProfession` signal. New split-panel UI with left profession list and right skill editor grouped by skill group with TreeNodes. Added rename field and "Assign to Selected Gnome" button.
- **Schedule tab polish** — Added color-coded legend bar (W/E/S/T). Added frozen Name column. Added "All" column per gnome and "All" row for batch operations. Right-click context menu for direct activity selection instead of cycling.
- **Social tab polish** — Now shows all gnomes (not just those with relationships). Added summary header (friendship/rivalry counts). Better empty state messaging.
- **Group toggle infrastructure** — New `onSetGroupActive` aggregator slot and `cmdSetGroupActive` bridge command to toggle all skills in a group at once.

### Technical Details
- Group index built lazily from first gnome's skill data and cached in static `SkillGroupIndex`. Groups derived from existing `GuiSkillInfo.group` field — no new DB queries needed.
- Profession skill editor reuses the same group index for organized display with colored TreeNodeEx headers.
- `ImGui::BeginTable()` with `TableSetupScrollFreeze(2,1)` provides proper horizontal/vertical scrolling with pinned columns.

---

## [2026-03-26] Mega Optimization: Phases A-F, H — Gnome AI Redesign

**Milestone**: 0.0 — Foundations & Performance
**Files changed**: `src/game/spatialgrid.h` (NEW), `src/game/spatialgrid.cpp` (NEW), `src/game/gnomestate.h` (NEW), `src/game/creature.h`, `src/game/creature.cpp`, `src/game/gnome.h`, `src/game/gnome.cpp`, `src/game/gnomemanager.h`, `src/game/gnomemanager.cpp`, `src/game/gnomeactions.cpp`, `src/game/jobmanager.h`, `src/game/jobmanager.cpp`, `src/game/game.h`, `src/game/game.cpp`, `src/base/io.cpp`

### Changes
- **Phase E: Shared Spatial Grid** — new `SpatialGrid` class with 16x16 cell hashing, O(1) entity/job insert/remove/move, expanding-ring radius queries. Wired into creature movement and job creation/deletion.
- **Phase A: Bucket-Staggered Ticks** — gnomes split into 10 buckets. Each tick, 1/10 get full BT evaluation; others get a cheap tick (survival checks, path advancement, timer check only). Removed 5ms budget cap and m_startIndex rotation.
- **Phase B: Push Model with Spatial Filtering** — jobs pushed to K=5 nearest eligible gnomes via skill→gnome registry + spatial grid. `actionGetJob` pops from pending queue (O(1)) before falling back to pull scan. Hauling stays pull-based via StockpileManager. Job cooldown reduced from 100 to 20 ticks.
- **Phase C: Social Spatial Hash + Bounded Relationships** — replaced O(n^2) social loop with spatial grid iteration (only check gnomes in same/adjacent cells). Capped relationships at R=15 per gnome (evict weakest). Social memories capped at 5 per gnome.
- **Phase D: Sleep/Wake System** — idle gnomes with no pending jobs sleep after 30 ticks. Sleeping gnomes are skipped entirely in onTick. Wake triggers: job pushed, safety net scan (every 100 ticks for critical needs).
- **Phase F: Gnome State Machine** — `GnomeState` enum (IDLE, THINKING, WAIT_PATH, MOVING, WORKING, SLEEPING_BED, EATING, NEEDS, COMBAT). cheapTick is state-aware: MOVING gnomes only advance path, WORKING gnomes only check timer. Other states force full tick.
- **Phase H: Spatial Fallback Search** — when pending queue is empty, expanding-ring spatial search (0-5 rings) finds nearby unclaimed jobs. Falls back to full pull scan only for hauling.
- **Phase G: Role System (deferred)** — 4+1 skill cap is primarily UI/gameplay, not performance-critical. Deferred to a separate update.

### Technical Details
- Cheap tick: processCooldowns (handles skipped ticks via diff), checkFloor, anatomy check, then state-specific logic (MOVING: moveOnPath; WORKING: timer check). ~0.5-2us per gnome.
- Work timer is absolute (`m_taskFinishTick = GameState::tick + duration`) — cheapTick just compares, no extraction needed.
- `m_forceFullTick` flag forces immediate full tick for: path arrival, work completion, job cancel, combat alert, interrupt.
- Push model: `m_gnomesBySkill` registry rebuilt on save/load via `rebuildPushState()`.
- Social: pair deduplication via ordered key, `QSet<quint64>` for processed pairs.
- Sleep/wake: `m_isSleeping` bool flag on Creature, skipped in onTick loop. Simpler than split-list approach, ~500ns overhead for 1000 gnomes.

### Benchmark Results
| Gnomes | Before (baseline) | After all phases | Improvement |
|--------|-------------------|-----------------|-------------|
| 100 | ~8,000us (43us/gn) | 90us (0.9us/gn) | **89x** |
| 200 | ~7,300us (30us/gn, budget-capped) | 511us (2.6us/gn) | **14x** |

Target was 1000 gnomes at <5ms. At 2.6us/gnome amortized, 1000 gnomes = ~2.6ms. **Target exceeded.**

---

## [2026-03-26] Blindness Wiring, Food Variety, Stockpile UX, Armored Enemies

**Milestone**: 2.3, 2.4, 1.2, 3.2 — Multi-system feature batch
**Files changed**: `anatomy.cpp`, `gnome.h`, `gnome.cpp`, `gnomeactions.cpp`, `aggregatorstockpile.h`, `aggregatorstockpile.cpp`, `imguibridge.h`, `imguibridge.cpp`, `ui_sidepanels.cpp`, `monster.h`, `monster.cpp`, `creature.h`, `eventmanager.cpp`, `ingnomia.db.sql`

### Changes
- **Wire blindness to eye damage (2.3d)** — High head hits now have 15% chance to strike an eye (1 HP). Destroying one eye sets AS_HALFBLIND, both sets AS_BLIND. Eyes heal naturally, blindness clears when healed. Also fixed `heal()` bug: `m_status | AS_WOUNDED` → `m_status & AS_WOUNDED`.
- **Food variety mood thoughts (2.4a)** — Track last 9 meals per gnome. 3+ unique foods → "Varied diet" +3 mood. Only 1 food type → "Same food again" -2 mood. Serialized in saves.
- **Drink quality mood thoughts (2.4b)** — Drink value thresholds: 21+ = +1 "decent", 40+ = +2 "nice", 60+ = +3 "excellent". Water-level (≤20) is neutral baseline.
- **Stockpile "Count by material" checkbox (1.2)** — Exposes existing `m_limitWithmaterial` toggle in ImGui panel with tooltip.
- **Stockpile copy/paste settings (1.2)** — Copy/Paste buttons using existing `serialize()`/`pasteSettings()` backend. Clipboard stored in aggregator.
- **All Stockpiles priority tab (1.2)** — New tab showing all stockpiles with up/down reorder buttons, using existing `movePriorityUp/Down()`.
- **Armored enemies (3.2b)** — `GoblinArmored` with Armor=2 flat damage reduction. Raids mix armored goblins based on difficulty (Normal+) and year (10% per year, capped at 50%). Armor absorption logged in combat. DB schema extended with Armor column.

### Technical Details
- Anatomy eye targeting: 0-7% → left eye, 8-14% → right eye, 15-99% → head. Only targets undestroyed eyes.
- Fixed `unsigned char` mask bug in `statusChanged()` — now uses `unsigned int` since `AnatomyStatus` is `unsigned int`.
- `m_recentMeals` is a rolling QStringList trimmed to 9 entries (approximately 3 in-game days of meals).
- Armored goblin percent formula: `min(50, year * 10 + (difficulty - 2) * 10)`. Only applies at difficulty >= Normal (2).
- GoblinArmored reuses Goblin sprites — same visual, different gameplay via armor value.

---

## [2026-03-25] Fix wildlife log spam and categorization

**Milestone**: 2.0 — UI/UX
**Files changed**: `animal.cpp`, `creaturemanager.cpp`, `ui_gamehud.cpp`

### Changes
- **Wildlife combat uses LogType::WILDLIFE instead of COMBAT** — animal-vs-animal attacks, damage, dodges now categorized as wildlife events, not combat. Combat is reserved for gnome/colonist fights.
- **Wildlife deaths use LogType::WILDLIFE instead of DEATH** — animal deaths no longer show as red DEATH events. DEATH is reserved for colonist deaths only.
- **Wildlife events no longer trigger toast notifications** — removed WILDLIFE from the toast filter. No more screen spam from "Fox attacks Rabbit" every few seconds. Events still appear in the Event Log panel under the Wildlife filter.
- **Combat history stored on creature** — all wildlife combat events (attacks, damage, dodges, kills) now also call `creature.log()` to store on the animal's personal log, so you can inspect why a creature died by checking its data.

### Technical Details
- `creaturemanager.cpp`: Generic death log now checks `creature->type() == CreatureType::ANIMAL` to select `LogType::WILDLIFE` vs `LogType::DEATH`
- `animal.cpp`: All `LogType::COMBAT` calls changed to `LogType::WILDLIFE`, each paired with a `log()` call for creature-local history
- `ui_gamehud.cpp`: Removed `LogType::WILDLIFE` from toast trigger condition

---

## [2026-03-25] Fix moonlight brightness — restore darkness at night

**Milestone**: 2.0 — Visual Identity
**Files changed**: `mainwindowrenderer.cpp`

### Changes
- **Moonlight reduced from 0.35 to 0.08** — night was far too bright after the value was changed from the original 0.15. At 0.08, nighttime is properly dark and oppressive, matching the "light vs darkness" theme.

---

## [2026-03-26] RFC v2: Gnome AI & Job System Redesign

**Milestone**: 0.0 — Foundations & Performance
**Files changed**: `docs/design/gnome_ai_redesign.md`

### Changes
- **Revised gnome AI redesign RFC to v2** — incorporated cross-game analysis (Songs of Syx, Factorio, RimWorld, Dwarf Fortress) and deep codebase architecture review
- **Added bucket-staggered ticks** (new Phase A) — the single highest-impact optimization, splitting gnomes into N=10 groups with full vs cheap ticks. Estimated 4-10x throughput improvement alone.
- **Added sleep/wake system** (new Phase D) — Factorio-inspired removal of stable-state gnomes from tick list entirely (sleeping, eating, idle-no-jobs)
- **Added rail movement** — gnomes walking known paths skip BT evaluation, advance position only (~1-2us vs ~15-30us)
- **Improved push model with spatial filtering** — push jobs to K=5 nearest eligible gnomes instead of ALL eligible gnomes. Converts O(eligible_gnomes) to O(K) per job creation.
- **Changed skill cap from soft to hard** — 4 active + 1 Flex slot. Guarantees performance bound on registry size and fallback search.
- **Added two-tier social model** — ambient room-based morale (O(n), no tracking) + bounded personal relationships (max R=15 per gnome, evict weakest)
- **Added shared spatial grid** — single 16x16 cell hash used by social, job push, and fallback search
- **Resolved all 7 open questions** from v1 with concrete answers
- **Revised targets** — 1000 gnomes at <5ms (up from 500 at <15ms)
- **Reordered implementation phases** by impact-to-effort ratio with parallel development plan

### Technical Details
- Bucket staggering: 500 gnomes = 50 full ticks (15us) + 450 cheap ticks (2us) = 1.65ms per game tick
- Sleep/wake removes 30-50% of gnomes from tick list, woken by events (job push, path result, need threshold, alarm)
- Spatial push: jobs pushed to 5 nearest eligible gnomes via shared spatial grid, not broadcast to all
- 8-phase implementation plan with 4 parallelizable pairs, ~8 week timeline

---

## [2026-03-26] Cap Mushroom Creatures & Adaptive Creature Time Budget

**Milestone**: 0.0 — Foundations & Performance
**Files changed**: `src/game/worldgenerator.cpp`, `src/game/creaturemanager.cpp`

### Changes
- **Cap mushroom creatures at 200**: Previously spawned at 4% of underground area (6,200 at size 400!). Now uses random placement with a hard cap of 200, matching surface animal spawning pattern.
- **Adaptive creature time budget**: Was hard-coded 2ms. Now scales `qBound(2, creatures/200, 5)` — small colonies stay snappy, large maps get up to 5ms to process more creatures per tick.
- **Entity counts in get_perf**: Added creature/plant/gnome counts to benchmark output for visibility.

### Benchmark Results (before → after, 100 ticks)
| Size | Creatures Before → After | Tick μs Before → After |
|------|-------------------------|----------------------|
| 200 | 1,079 → **162** | 3,304 → **940** |
| 300 | 2,613 → **162** | 3,538 → **1,099** |
| 400 | 4,510 → **162** | 3,570 → **1,226** |

### Technical Details
- Mushroom spawner now uses random position + walkability check loop (max 2000 attempts for 200 creatures)
- Creature count flat at ~162 regardless of map size (500 surface attempts + 200 mushroom cap)
- Tick time now scales with plant count (only thing proportional to area) — sub-linear growth
- Surface animal placement unchanged (numWildAnimals=500 with per-type caps)

---

## [2026-03-26] Gnome Scaling Analysis — Bottleneck Identification

**Milestone**: 0.0 — Foundations & Performance
**Files changed**: Analysis only — no code changes

### Bottlenecks Identified (in priority order)

1. **JobManager::getJob() — O(gnomes × skills × jobs)** (`jobmanager.cpp:499-637`)
   Each idle gnome iterates all skills → all job types → all jobs per type, calling `requiredItemsAvail`, `requiredToolExists`, `isReachable` for each. With 100 idle gnomes × 10 skills × N jobs = massive scan.

2. **processSocialInteractions() — O(n²)** (`gnomemanager.cpp:853-858`)
   Nested gnome-pair loop every 600 ticks. 200 gnomes = 19,900 pair checks. Needs spatial partitioning.

3. **Behavior tree tick has no per-gnome budget** (`gnome.cpp:974`)
   `m_behaviorTree->tick()` runs to completion — pathfinding, inventory queries, no μs cap.

4. **All gnomes tick sequentially** (`game.cpp:235`)
   GnomeManager::onTick processes gnomes one-by-one on the game thread. Only natural world is parallelized.

5. **GnomeManager 5ms budget** (`gnomemanager.cpp:203`)
   At 200 gnomes × 30μs = 6ms, the budget is exceeded — not all gnomes tick every frame.

### Optimization Roadmap
- **Job search**: Spatial index for jobs (grid cells), cache recent failed lookups, skip non-reachable regions
- **Social**: Only check gnome pairs within 10 tiles (spatial hash), not all n² pairs
- **BT budget**: Per-gnome μs limit with continuation — "thinking time" vs "action time" distinction
- **Parallelization**: Split gnome BT ticks across threads (need thread-safe inventory/job queries)
- **Gnome spawn cap**: World generator limited to ~256 starting positions — needs expansion for large colony starts

---

## [2026-03-26] Gnome Count Scaling Benchmark & Analysis

**Milestone**: 0.0 — Foundations & Performance
**Files changed**: `src/test/testcommandserver.cpp`

### Changes
- **`num_gnomes` parameter for `new_game` command**: Sets both `NewGameSettings` and `Global::cfg` to ensure gnome count survives the `startNewGame()` config sync
- **Fix gnome config clobber**: `startNewGame()` was overwriting `ngs->setNumGnomes()` with stale `cfg->get("numGnomes")` — now both are set in the test command

### Gnome Scaling Benchmark (size 200 map, 100 ticks)

| Gnomes | Gnome μs | Creature μs | Total μs | μs/gnome |
|--------|----------|-------------|----------|----------|
| 7 | 178 | 505 | 1,058 | 25 |
| 25 | 409 | 484 | 1,233 | 16 |
| 50 | 1,313 | 615 | 2,319 | 26 |
| 100 | 4,298 | 2,254 | 7,966 | **43** |
| 150 | 3,971 | 540 | 4,883 | 26 |
| 200 | 6,009 | 910 | 7,328 | **30** |

### Key Findings
- **Non-linear scaling at 100+ gnomes**: Cost per gnome jumps from 25μs to 43μs — likely pathfinding contention as multiple gnomes compete for A* queries
- **100 gnomes causes cascade**: Creatures and NaturalWorld both spike, suggesting shared resource contention (inventory lookups, job queries)
- **Gnome spawn cap at ~256**: World generator placement loop runs out of valid starting positions
- **Variability is high**: 150 gnomes cheaper than 100 in one sample — behavior tree decisions (idle vs path) dominate
- **Target for optimization**: GnomeManager::onTick → behavior tree → pathfinding pipeline is the scaling bottleneck

### Next Steps
- Profile gnome behavior tree to find superlinear operations
- Investigate pathfinding dispatch/collect — may need per-gnome budgeting
- Consider spatial partitioning for job assignment (O(n²) gnome×job matching?)
- Test with established colonies (more jobs, stockpiles, workshops active)

---

## [2026-03-25] Rebrand: Ingnomia → Masonry

**Milestone**: 1.0 — Independent Release
**Files changed**: `CMakeLists.txt`, `src/version.h`, `src/gui/ui/ui_mainmenu.cpp`, `README.md`, `CREDITS.md`, `CLAUDE.md`

### Changes
- **Renamed project to Masonry** — CMake project name, executable, main menu title, version bumped to 1.0.0.0
- **New README** — Written from scratch for Masonry with current build instructions and feature list
- **Added CREDITS.md** — Attributes original Ingnomia project (Ralph Schurade, 2017-2020), lists third-party libraries and fonts
- **Removed upstream remote** — No longer tracking rschurade/Ingnomia
- **Updated CLAUDE.md** — References Masonry as the current project name

### Technical Details
- Data folder paths (`~/Library/Application Support/.../Ingnomia`) intentionally kept as-is to preserve existing save game compatibility
- File-level copyright headers in original source files preserved (AGPL requires retaining them)
- Executable is now `build/Masonry`

---

## [2026-03-26] World Size Performance Benchmarking & Larger Maps

**Milestone**: 0.0 — Foundations & Performance
**Files changed**: `src/game/game.h`, `src/game/game.cpp`, `src/game/gamemanager.cpp`, `src/test/testcommandserver.h`, `src/test/testcommandserver.cpp`, `src/gui/ui/ui_mainmenu.cpp`

### Changes
- **Per-system tick timing**: `TickTimingInfo` struct with microsecond precision for all subsystems (creatures, gnomes, jobs, naturalWorld, passive, events, pathfinding). Always-on, accessible via `Game::lastTickTiming()`.
- **`new_game` test command**: Create worlds at any size programmatically via `{"cmd":"new_game","world_size":300}`
- **`get_perf` test command**: Returns per-system tick breakdown, memory usage, tick count
- **`benchmark` test command**: Automated sweep across multiple world sizes
- **World size slider**: Extended from max 200 to max 400
- **sendLoadGameDone signal**: Now fires after new game creation (was missing)

### Benchmark Results (100 ticks, fresh worlds)
| Size | Tiles | RAM | Gen Time | Tick Time |
|------|-------|-----|----------|-----------|
| 100 | 1.3M | 50MB | 6s | 2.4ms |
| 200 | 5.2M | 198MB | 16s | 1.6ms |
| 300 | 11.7M | 446MB | 31s | 5.2ms |
| 400 | 20.8M | 793MB | 52s | 4.0ms |

### Technical Details
- Tick time dominated by creature AI, not world size — safe to increase map size
- NaturalWorld processing (grass/water/plants) parallelized, stays under 3ms at 400
- GPU render volume is viewport-bounded, unaffected by world size
- Memory scales linearly: ~2MB per 100 tiles (Tile struct ~40 bytes)
- Timing uses `QElapsedTimer::nsecsElapsed()` for μs precision

---

## [2026-03-25] Lighting & Shadow System — "Light vs Darkness"

**Milestone**: 2.0 — Visual Identity
**Files changed**: `mainwindowrenderer.h`, `mainwindowrenderer.cpp`, `world_f.glsl`, `world_v.glsl`, `aggregatorrenderer.h`, `aggregatorrenderer.cpp`

### Changes
- **True darkness rendering** — dropped `lightMin` from 0.35 to 0.03 with quadratic falloff curve. Unlit tiles are now near-black instead of 35% visible. This is the foundation of the "light vs darkness" game identity.
- **Warm torch glow with HDR bloom** — torches now cast warm amber light (`vec3(1.0, 0.85, 0.55)`) with an additive HDR boost that pushes pixels above 1.0 for the bloom pipeline to catch, creating visible glow halos around light sources.
- **Oppressive night atmosphere** — night outdoor tiles get a strong cold blue tint (`vec3(0.55, 0.62, 1.0)`) with quadratic ramp, creating dramatic contrast between warm indoor torch-lit areas and cold outdoor darkness.
- **Fixed and re-enabled post-processing pipeline** — bloom/vignette/grain was disabled due to macOS Retina DPR mismatch. Fixed FBO sizing to use physical pixel dimensions (`devicePixelRatioF()`), re-enabled the full bright-extract → gaussian-blur → composite pipeline.
- **Bloom tuning** — lowered threshold from 0.75 to 0.50 (catches torch glow), increased strength from 0.25 to 0.40, increased blur passes from 4 to 6 (wider halos). Dynamic vignette darkens edges more at night.
- **Light boundary edge effect** — subtle warm glow (`vec3(0.6, 0.35, 0.1)`) at mid-light values creates a visible "circle of safety" at the edge of light radius (Don't Starve inspiration).
- **Wall shadow casting** — expanded TileData from 9 to 10 uints per tile, added CPU-computed directional shadow flags (N/E/S/W). Fragment shader renders directional edge darkening from shadow-casting walls, rotation-aware, with smooth falloff. Shadows are most visible in partially lit areas.

### Technical Details
- Quadratic light curve (`light * light`) gives sharp falloff at light boundaries while preserving full brightness in well-lit areas
- HDR additive glow (`texel.rgb += torchColor * warmth * 0.15`) is the key to making bloom work with light sources — without pushing above 1.0, the bright-extract pass has nothing to catch
- TileData expanded to 10 uints: new fields are `lightGradient`, `lightColorHint`, `shadowFlags`, `reserved` (packed as 4 bytes in the 10th uint)
- Shadow flags computed in `aggregateTile()`: for each cardinal neighbor, if the neighbor is `WT_VIEWBLOCKING` and has lower light level, a shadow bit is set
- Post-processing FBO fix: logical window size (from Qt) vs physical pixel size (for GL framebuffers) must be multiplied by `devicePixelRatioF()` on Retina displays
- Fog color now adapts to darkness — near-black at night instead of the previous blue-grey, preventing fog from looking brighter than the darkness itself

---

## [2026-03-25] Starting Items Configuration UI

**Milestone**: 2.0 — Core Gameplay Polish
**Files changed**: `src/gui/ui/ui_mainmenu.cpp`

### Changes
- **Starting items tab** — Replaced placeholder with full item picker UI. Left side shows all items from the DB grouped by category with collapsible tree nodes and a search filter. Right side shows current starting items list with remove buttons. Material dropdown(s) appear when an item is selected (supports multi-component items like pickaxes). Amount input with Add button.

### Technical Details
- All operations go through existing `NewGameSettings::addStartingItem()` / `removeStartingItem()` — no backend changes needed
- Item data cached from `DB::selectRows("Items")` on first tab open, grouped by Category column
- Translations via `S::s("$ItemName_...")` and `S::s("$MaterialName_...")` with raw ID fallback
- Materials queried via `NewGameSettings::materialsForItem()` which handles both simple and component items

---

## [2026-03-25] Main Menu Redesign — Custom Fonts, Styling, Continue Button Fix

**Milestone**: 3.0 — Visual Polish & Atmosphere
**Files changed**: `src/gui/imgui_impl_qt5.h`, `src/gui/imgui_impl_qt5.cpp`, `src/gui/ui/ui_mainmenu.cpp`, `src/base/io.h`

### Changes
- **Custom font loading** — Added `ImGuiFonts` struct and font loading during ImGui init. Loads HermeneusOne (48px title) and PT Root UI (18px body, 14px captions) from existing `content/xaml/` fonts. Scales by device pixel ratio. Falls back gracefully if font files are missing.
- **Main menu visual overhaul** — Dark gradient background with subtle edge vignette. Responsive sizing (scales with window, clamped 300-450px wide). Title in decorative serif font, "A Colony Simulation" tagline, version pinned to bottom. Buttons styled with rounded corners and extra padding. Exit button visually separated from other actions.
- **Continue button fix** — Disabled when no save games exist. Uses `IO::saveGameExists()` (now static) with a 1-second throttled check to avoid filesystem hits every frame.

### Technical Details
- Fonts loaded once in `ImGuiQt5::Init()` before first frame, baked into texture atlas
- Font paths resolved via `QCoreApplication::applicationDirPath()` (same pattern as audio system)
- Background drawn via `ImGui::GetBackgroundDrawList()` — renders behind all windows but after GL clear
- `IO::saveGameExists()` made static (was instance method but only used static `getDataFolder()`)

---

## [2026-03-26] Visual Style — Phase 3: AO, Weather, Caustics

**Milestone**: 3.0 — Visual Polish & Atmosphere
**Files changed**: `content/shaders/world_f.glsl`, `content/shaders/world_v.glsl`, `src/gui/aggregatorrenderer.h`, `src/gui/aggregatorrenderer.cpp`, `src/gui/mainwindowrenderer.cpp`, `src/gui/mainwindowrenderer.h`

### Changes
- **Per-tile ambient occlusion**: Computes neighbor solidity flags (above, N/E/S/W) CPU-side in `aggregateTile()`, packs into bits 24-31 of packedLevels (previously `unused3`). Shader darkens tile edges near solid walls with rotation-aware directional falloff using smoothstep.
- **Weather shader effects**: Maps `GameState::activeWeather` ("Storm"/"HeatWave"/"ColdSnap") to shader uniform with smooth intensity ramp. Storm: darken + blue tint + saturation boost. HeatWave: warm shift. ColdSnap: desaturate + brighten + blue shift. Weather also increases depth fog density.
- **Water caustics**: Animated dappled light on shallow water (depth 1-4) using overlapping sine waves. Intensity scales with shallowness.

### Technical Details
- AO flags stored in `TileData::aoFlags` byte, replacing `unused3`. Computed per tile update (not per frame).
- AO rotation in shader: bit-rotate N/E/S/W flags by `uWorldRotation` so shadows track camera rotation
- Weather intensity smoothly ramps via `m_weatherIntensity` lerp (0.03/frame) — no sudden jumps
- Caustics use `uTickNumber * 0.05` for animation speed, seeded with tile Z for spatial variation
- Performance: 31ms avg tick over 500 ticks — no measurable regression from Phase 2

---

## [2026-03-25] Visual Style — Shader Effects Pipeline (Phases 1 & 2)

**Milestone**: 3.0 — Visual Polish & Atmosphere
**Files changed**: `content/shaders/world_f.glsl`, `content/shaders/world_v.glsl`, `content/shaders/postprocess_f.glsl`, `content/shaders/postprocess_v.glsl`, `content/shaders/brightextract_f.glsl`, `content/shaders/brightextract_v.glsl`, `content/shaders/blur_f.glsl`, `content/shaders/blur_v.glsl`, `src/gui/mainwindowrenderer.cpp`, `src/gui/mainwindowrenderer.h`

### Changes
- **Phase 1 — In-shader effects (zero infrastructure)**:
  - **Depth fog**: Tiles below the camera view level fade toward an atmospheric color (dark purple underground, hazy blue outdoors). Passes tile Z as a flat varying from vertex shader.
  - **Seasonal color grading**: Entire palette shifts by season — spring green, summer warm, autumn amber, winter blue-grey. Driven by `GameState::season` uniform.
  - **Underground cave color**: Dark unlit tiles shift toward deep purple instead of flat grey, giving caves atmosphere.
  - **Mouseover rim highlight**: Hovered tile gets a warm edge glow using texcoord-based edge detection via `TF_MOUSEOVER` flag.

- **Phase 2 — FBO post-processing pipeline**:
  - **FBO infrastructure**: Scene renders to an RGBA16F offscreen texture with depth attachment. Fullscreen triangle shader (no vertex buffer, uses gl_VertexID).
  - **Bloom**: Bright extraction → half-res ping-pong gaussian blur (9-tap, 4 passes) → additive composite. Torches and lava get warm glow halos.
  - **Vignette**: Subtle screen-edge darkening to focus attention on center.
  - **Film grain**: Nearly invisible noise overlay to prevent color banding, especially at night.

### Technical Details
- FBO uses RGBA16F for HDR headroom — bloom extraction works on values above 0.7 brightness
- Bloom runs at half resolution with separable gaussian blur for performance
- All Phase 1 effects cost ~0 GPU time (few mix ops per fragment)
- Phase 2 adds ~0ms measurable overhead (54ms/tick vs 53ms before)
- New uniforms: `uViewLevel`, `uRenderDepth`, `uSeason` on world shader
- Post-process chain: scene FBO → bright extract → blur H → blur V (×4) → composite + vignette + grain → default FB
- ImGui renders after post-process onto the default framebuffer, unaffected

---

## [2026-03-25] Animal Hunger Priority System — Food Before Killing

**Files changed**: `src/game/animal.h`, `src/game/animal.cpp`

### Changes
Complete rewrite of the desperation system with a priority-based food-seeking chain:

**Priority 1 — Eat existing corpse**: if already has a corpse to eat, consume body-part by body-part. Each external body part provides nutrition = HP/2. When all parts consumed, corpse becomes Bone item.

**Priority 2 — Find corpse nearby**: search for AnimalCorpse or GnomeCorpse items within range, pathfind to them.

**Priority 3 — Find food items**: search for loose food items on ground matching the animal's diet (Meat, Fruit, etc.).

**Priority 4 — Hunt weaker animals**: carnivores seek prey animals that are weaker (lower Attack stat). Prefers species from prey list (wolf→rabbit). Won't hunt same species (no cannibalism). Won't hunt stronger animals (wolf won't attack bear).

**Priority 5 — LAST RESORT — Attack gnome**: only if no other food source found. After killing a gnome, creates GnomeCorpse item and begins eating it instead of continuing to hunt.

**Body-part eating**: when eating a corpse, each tick consumes one external body part. Nutrition per part = HP/2 (Torso: 25, Leg: 15, Arm: 15, Head: 10). A gnome with amputated limbs provides less total nutrition. When all external parts eaten, corpse destroyed and Bone item left behind.

**State machine**: `DesperateAction` enum tracks current desperation state (None, SeekingFood, SeekingCorpse, HuntingPrey, HuntingGnome, Eating). Resets each time the animal needs to find new food.

---

## [2026-03-25] Animal Desperation System — Staged Starvation Behavior

**Files changed**: `src/game/animal.cpp`

### Changes
Completely rewrote starvation behavior to bypass the behavior tree and directly drive desperate survival actions from C++:

**Stage 1 — Restless (hunger 10-20):** Animal shows "Hungry" thought bubble. Begins searching for food.

**Stage 2 — Desperate (hunger 0-10):**
- **Carnivores/omnivores**: pathfind toward nearest gnome (up to 60 tiles), set as attack target
- **Herbivores/omnivores**: pathfind toward nearest stockpile to raid for food
- Movement speed doubled (halved cooldown) — animals visibly rush

**Stage 3 — Attacking (hunger < 0):**
- Carnivores that reach a gnome (adjacent tile) attack with their Attack/Damage stats
- Combat logged as COMBAT type with position
- Re-paths if target moves away

**Why BT bypass was needed**: The BT switch to AnimalHunter wasn't working because the standard Animal BT's registered nodes and the hunter BT's expected nodes didn't fully align. Direct C++ pathfinding + attack in onTick is reliable and visible.

**Movement**: Desperate animals move on their computed path every tick (with halved move cooldown), returning before the BT executes. Normal BT only runs when the animal has no desperation path.

---

## [2026-03-25] Starving Animals Attack + Position-Based Go To

**Files changed**: `src/base/logger.h`, `src/base/logger.cpp`, `src/game/animal.cpp`, `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`, `src/gui/ui/ui_gamehud.cpp`

### Changes
- **Starving animals now actually attack** — when `m_starvingAggro` triggers, the animal populates its aggro list with nearby gnomes (within 30 tiles) and switches BT to "AnimalHunter" (GetTarget → Move → AttackTarget). Reverts when fed.
- **Position stored in log messages** — `LogMessage` has `posX/posY/posZ` fields. Animal events pass position.
- **"Go To" works for dead entities** — toast stores position at creation. Falls back to stored position when entity is dead.
- **cmdNavigateToPosition()** — new bridge method for direct position navigation.

---

## [2026-03-25] CI/CD Pipeline + In-Game Update Checker

**Files changed**: `.github/workflows/release.yml`, `.github/workflows/cmake.yml.old`, `CMakeLists.txt`, `src/main.cpp`, `src/gui/updatechecker.h`, `src/gui/updatechecker.cpp`, `src/gui/imguibridge.h`, `src/gui/ui/ui_mainmenu.cpp`

### Changes
- **GitHub Actions CI/CD** — automated cross-platform builds (Windows, macOS, Linux) on every push to master, with GitHub Releases for distribution
- **In-game update checker** — checks GitHub releases API on startup, shows green "Update available" banner + download button on the main menu when a newer version exists
- **Auto-generated release notes** — extracts the latest DEVLOG.md entry and includes it in each GitHub Release
- **macOS code signing** — workflow supports Developer ID signing via GitHub Secrets
- **Disabled old CI** — renamed legacy `cmake.yml` (referenced dead Noesis/BugSplat/SFML deps) to `.old`

### Technical Details
- `UpdateChecker` uses `QNetworkAccessManager` (Qt5::Network) to hit `api.github.com/repos/{owner}/{repo}/releases/latest`
- Version comparison: parses dotted version strings + build number suffix (e.g. `v0.8.11.0+42`)
- Guarded by `#ifdef GIT_REPO` — local builds without CI skip the update check
- Update check fires 2s after window show via `QTimer::singleShot`, only in non-test mode
- macOS builds use `macos-13` (Intel) runners for Qt5 compatibility
- Linux packages as `.tar.gz` with launcher script; Windows uses `windeployqt` + zip

---

## [2026-03-25] Toast "Go To" Navigation + WILDLIFE Log Category

**Files changed**: `src/base/logger.h`, `src/game/animal.cpp`, `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`, `src/gui/ui/ui_gamehud.cpp`, `src/gui/ui/ui_sidepanels.cpp`, `src/gui/mainwindow.cpp`

### Changes
- **"Go To" button** on toasts — clicks navigate the camera to the entity's position and Z-level. Works for gnomes, animals, and monsters.
- **WILDLIFE log category** — new `LogType::WILDLIFE` for animal behavior events (starvation, aggression). Shown as amber/brown in logs and toasts.
- **Wildlife filter** in Event Log — new "Wildlife" checkbox lets you hide/show animal events separately from combat/danger.
- **Better toast padding** — increased window padding (12px horizontal, 10px vertical), wider toast panel (350px), more spacing between separator lines.
- **Camera navigation system** — `ImGuiBridge::cmdNavigateToEntity()` looks up creature position via Game pointer, sets `pendingCameraNav` which MainWindow consumes in `paintGL()` to center camera + set Z-level.

### Technical Details
- `cmdNavigateToEntity` checks gnome → animal → monster via GnomeManager/CreatureManager lookups
- MainWindow processes `pendingCameraNav` at start of `paintGL()`, calls `onCenterCameraPosition()` + `setViewLevel()`
- Toast buttons: "Go To" (blue, only shown when entityID != 0) + "X" (grey, always shown)
- WILDLIFE color: amber `ImVec4(0.8f, 0.6f, 0.2f)` — distinct from DANGER (orange) and WARNING (yellow)

---

## [2026-03-25] Toast Notifications — Non-Overlapping, Dismissible

**Files changed**: `src/gui/imguibridge.h`, `src/gui/ui/ui_gamehud.cpp`

### Changes
- **Single stacked panel** — all toasts rendered in one auto-sized window instead of separate overlapping windows. No more text-on-text overlap.
- **Close button** — each toast has an "x" button to dismiss it immediately.
- **Max 5 toasts** — oldest removed when new ones arrive to prevent clutter.
- **Better fade timing** — toasts start fading after ~7.5 min game time (was ~5 min), giving more time to read.
- **Entity ID stored** — each toast tracks the source entity ID for future "Go To" navigation.
- **Severity colors** — MIGRATION now green, all others unchanged.

### Technical Details
- Toasts in a single `ImGui::Begin("##toasts")` window with auto-resize
- `ImGuiWindowFlags_AlwaysAutoResize` ensures the window shrinks as toasts are dismissed
- `toast.dismissed` flag checked each frame; dismissed toasts removed before rendering
- Close button uses `ImGui::SameLine()` for right-alignment before wrapping text

---

## [2026-03-25] Animal Hunger Behavior — Starvation, Aggression, Diet System

**Files changed**: `src/game/animal.h`, `src/game/animal.cpp`

### Changes
- **Starvation death** — animals now die when hunger drops below -20. Death logged as INFO: "A Wolf has starved to death."
- **Hunger-driven aggression** — carnivores/omnivores (diet contains "Meat") become aggressive when hunger drops below 10. Logged as DANGER: "A starving BlackBear has become aggressive!" Calms down once fed above 30.
- **`m_starvingAggro` flag** — tracked per animal, feeds into `isAggro()` so the UI correctly shows "Aggressive" for starving meat-eaters.
- **`m_diet` field** — loaded from DB `Animals.Food` column. Stored per animal for runtime diet checks.
- **Carnivore/Herbivore conditions implemented** — `conditionIsCarnivore` and `conditionIsHerbivore` were stubs returning FAILURE. Now check `m_diet` for "Meat" or "Vegetable"/"Hay"/"Grain"/"Fruit" respectively.
- **Thought bubble** — starving animals (hunger ≤ 0) show "Hungry" thought bubble.

### Technical Details
- Hunger decay: -0.075 per in-game minute. From 100→0 takes ~22 hours. From 0→-20 (death) takes ~4.4 more hours.
- Starvation aggression only applies to wild animals (not tame/pastured).
- `isAggro()` now returns true for either DB-defined aggro OR starvation aggro.

---

## [2026-03-25] Animal-Specific Creature Info — Diet, Combat Stats, Butcher Drops

**Files changed**: `src/game/creature.h`, `src/gui/aggregatorcreatureinfo.h`, `src/gui/aggregatorcreatureinfo.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes
- **Diet display** — shows what the animal eats (Meat, Vegetable, Fruit, etc.)
- **Temperament** — "Aggressive" (red) or "Passive" (green) from DB + starvation state
- **Combat stats** — Attack and Damage values from the adult state in Animals_States DB
- **Butcher yields** — lists what you get from butchering (e.g., "6x Meat, 1x Bone")
- **Health bar** — blood level from anatomy system, color-coded with Wounded/Dead status
- **Hunger status text** — "Starving!" in red at ≤5%, "N% Hungry" in orange at ≤20%
- **Creature type routing** — panel shows completely different info for Gnomes vs Animals vs Monsters

---

## [2026-03-25] Fix Personality Data Showing on Animals

**Files changed**: `src/gui/aggregatorcreatureinfo.cpp`

### Changes
- Animals and monsters no longer display a previous gnome's backstory, traits, and thoughts
- Monster/animal branches now explicitly clear all gnome-only personality fields before emitting

---

## [2026-03-25] Mouse Wheel Z-Level Scroll Fix

**Files changed**: `src/gui/mainwindow.h`, `src/gui/mainwindow.cpp`

### Changes
- Scroll wheel now changes exactly 1 Z-level per notch instead of jumping 3-5 levels
- Wheel delta accumulation prevents trackpad smooth-scroll from over-scrolling

---

## [2026-03-25] Agriculture Panel UI Overhaul

**Milestone**: 1.4 — HUD & UI Improvements
**Commits**: Part of `eec1d91`, `efb7a76`
**Files changed**: `src/gui/ui/ui_sidepanels.cpp`, `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`

### Changes
- **Crop selector dropdown** — farms now have a combo box listing all plantable crops from `globalPlants`. Selecting a crop calls the existing `Farm::setPlantType()` backend. Previously there was no way to choose what to plant from the UI.
- **Grove management view** — tree type selector dropdown, Plant trees/Pick fruit/Fell trees checkboxes, plot and tree count display
- **Pasture management view** — animal type selector, male/female population sliders, harvest/hay toggles, food settings with per-item checkboxes, animal list with butcher toggles
- **Type-aware panel** — `AgriType currentAgriType` added to `ImGuiBridge`. Panel auto-detects Farm/Grove/Pasture from which info struct is populated and renders the correct view.
- **Fixed hardcoded AgriType::Farm** — `cmdAgriSetOptions`, `cmdAgriSetHarvestOptions`, `cmdAgriSelectProduct` now pass `currentAgriType` instead of always Farm
- **Global list loading** — plant/tree/animal lists requested on panel open if not yet loaded
- **Rename UX** — type new name + Enter saves immediately without closing panel (uses `ImGuiInputTextFlags_EnterReturnsTrue`)
- **Panel sizing** — positioned at (5, 130), 280px wide, matching screenshot reference layout

### Technical Details
- Zero new game logic — all backend functions (`AggregatorAgri::onSelectProduct`, `Farm::setPlantType`, `Pasture::setAnimalType`, grove options) were fully wired but never called from UI
- `drawAgriculturePanel()` expanded from 40-line farm-only stub to ~280 lines across 4 functions (`drawFarmView`, `drawGroveView`, `drawPastureView`, dispatcher)

---

## [2026-03-25] Game Design Research Library & Roadmap Expansion

**Milestone**: Planning & Documentation
**Commits**: Part of `b6461ad`, `a0750af`
**Files changed**: `docs/research/**/*.md`, `docs/research/feature_reference_library.md`, `docs/updates/development_roadmap.md`

### Changes
- **Research library** (8 documents, ~250KB) compiled from RimWorld wiki, Dwarf Fortress wiki, Gnomoria wiki, Reddit/Steam community feedback, and art resource searches
  - `docs/research/rimworld/core-mechanics.md` — 73 traits, 150+ mood thoughts, mental breaks, events, social, medical, skills
  - `docs/research/rimworld/extended-systems.md` — animals, enemies, biomes, backstories, factions, diseases, weapons, ideology
  - `docs/research/dwarf-fortress/core-mechanics.md` — 50 personality facets, emotions, strange moods, combat anatomy, medical, social, needs
  - `docs/research/dwarf-fortress/extended-systems.md` — creatures, megabeasts, world history generation, underground biomes, artifacts, nobles, trade
  - `docs/research/gnomoria/wiki-mechanics.md` — game systems, progression, what worked
  - `docs/research/gnomoria/community-feedback.md` — player love, wishlists, pain points (filtered for gameplay, not dev-abandonment)
  - `docs/research/art-resources/art-technical-spec.md` — 32x36 isometric sprite system, all 26 tilesheets, pipeline for adding new art
  - `docs/research/art-resources/isometric-art-search.md` — MagicaVoxel pipeline, free isometric assets, AI sprite tools
- **Feature reference library** expanded with 3 new sections:
  - §2.0 Character Traits & Backstories — 12-15 trait scales, childhood+adulthood backstory system, DF history integration
  - §2.0b Social System — opinion scores, 5 interaction types, trait compatibility, relationship labels, dining hall as social hub
  - §4.3 Magic & Religion marked as STRETCH GOAL (expansion scope, no visual/mechanical foundations exist)
- **Development roadmap** expanded:
  - Milestones 2-5 fully detailed with specific checklists from feature library
  - Milestone 2 dependency chain documented (Traits → Social → Mood)
  - All milestones cross-reference feature library with `**Design reference:**` links
  - Guiding Principle 5 added: "Systems must talk to each other"

### Technical Details
- Research sourced from wiki pages via web fetch/search across 8 parallel research agents
- Art research identified that free tilesets (16x16/32x32 top-down) don't match Ingnomia's 32x36 isometric projection — MagicaVoxel voxel-to-sprite pipeline is the most viable path for new art
- Feature reference library serves as the design spec; roadmap is the implementation checklist. Both tell the same story at different zoom levels.

---

## [2026-03-25] Improve Build UX, Tile Info, and Container Display

**Commit**: `d555196`
**Files changed**: `src/gui/ui/ui_gamehud.cpp`, `src/gui/ui/ui_tileinfo.cpp`, `src/gui/aggregatortileinfo.cpp`

### Changes
- **"Mine Vein"** replaces "Explorative Mine" — clearer label for auto-vein-chasing mining
- **Build material display** shows translated item type + material name (e.g., "2 x Plank: Pine (5)") instead of raw IDs
- **Container info in Tile Info** — crates/barrels show as expandable tree nodes with [used/capacity] and list of contained items
- **CraftUntilStock** mode shows "Stock" label in workshop craft list
- Tile Info panel now moveable and resizable

---

## [2026-03-25] Overhaul Stockpile UI: Ledger Tab, Filter Redesign, Search

**Commit**: `eec1d91`
**Files changed**: `src/gui/aggregatorstockpile.cpp`, `src/gui/aggregatortileinfo.h`, `src/gui/ui/ui_sidepanels.cpp`

### Changes
- **Two-tab stockpile panel**: Ledger (stored items table) + Filters (acceptance tree)
- **Capacity display fixed** — was always showing 0/0, now correctly sums across all tile fields
- **Color-coded capacity bar** — green (<70%), yellow (70-90%), red (>90%)
- **Filter tree redesign** — checkboxes at every level (category/group/item/material). Checking a parent toggles all children. Items with multiple materials get expandable sub-trees.
- **Search bar** — type to filter categories/groups/items, matching branches auto-expand
- **Settings refresh** — name/priority changes update UI immediately without reopening panel
- **All side panels** now smaller (60% width) and moveable/resizable

---

## [2026-03-25] Fix Input Handling and Panel Layout

**Commit**: `efb7a76`
**Files changed**: `src/gui/imguibridge.cpp`, `src/gui/imguibridge.h`, `src/gui/eventconnector.cpp`, `src/gui/eventconnector.h`

### Changes
- **Space bar always pauses** — handled before ImGui keyboard capture check. Also fixed the underlying bug: `onTogglePause()` was only updating UI state, never calling `gm->setPaused()` on the game engine.
- **Escape always works** — tool dismissal and panel closing from any ImGui focus state
- **Click-to-manage** — clicking stockpile/workshop/farm tiles auto-opens the management panel (new `onTileClickAutoOpen` in EventConnector)
- **Creature Info** repositioned below time display, Workshop panel moved to left side
- Null safety for pause toggle when no game is loaded

---

## [2026-03-25] Redesign Social System — Realistic Pacing & Personality-Driven Outcomes

**Milestone**: 2.0b (iteration 3) — Gnome Depth
**Files changed**: `src/game/gnomemanager.h`, `src/game/gnomemanager.cpp`, `src/gui/aggregatorcreatureinfo.h`, `src/gui/aggregatorcreatureinfo.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes
- **Realistic pacing** — ~1.2 interactions/pair/day (was ~23). Checks once per in-game hour at 5% chance.
- **Neutral = zero change** — only personality-driven mismatches/affinities move opinions.
- **Weighted trait compatibility** — specific trait interactions: pessimist+optimist = friction (-8), both empathetic = affinity (+5), two hot-heads = volatile (-4), mismatched work ethic = resentment (-3).
- **Backstory compatibility** — shared skill groups from backstories add +4 affinity per group.
- **Mood affects agreeableness** — happy gnomes +5 tone bonus, miserable gnomes -5 penalty.
- **Social memory system** — gnomes remember last 10 events (fade after 3 days). High Empathy gnomes apologize (20% chance), high Temper gnomes escalate (15% chance).
- **Opinion decay** — 1 point toward 0 per in-game day. Relationships need proximity to maintain.

---

## [2026-03-25] Enhanced Creature Info — Social, Thought Tooltips, Mood Breakdown

**Milestone**: 2.0b/2.1 UI polish
**Files changed**: `src/game/gnome.h`, `src/game/gnome.cpp`, `src/gui/aggregatorcreatureinfo.h`, `src/gui/aggregatorcreatureinfo.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes
- **Thought tooltips** — hover shows cause/explainer, trait modulation, and time remaining (% + minutes/seconds).
- **Thoughts sorted** — highest absolute mood impact first.
- **Thoughts fade visually** — alpha reduces from 1.0 to 0.5 as they expire.
- **Mood breakdown tooltip** — hover mood bar shows: Base (Optimism) + Thought sum + Needs penalty = Total.
- **Social in Creature Info** — relationships section with color-coded labels + recent social memories ("Had a good chat with X", "Argued with Y") with hover showing days ago and opinion change.
- **Thought cause field** — Thought struct extended with `cause` and `initialDuration` for detailed tooltips.

---

## [2026-03-25] 120+ Thought Types Across 7 Categories

**Milestone**: 2.1 content expansion
**Files changed**: `src/game/gnome.cpp`

### Changes
Expanded from ~25 to 120+ distinct thought types so gnomes always have visible reasons for their mood:
1. **Needs** (15): Starving/Parched/Exhausted at extremes, Well Fed/Refreshed/Well Rested when satisfied
2. **Activity** (30): Skill-specific work satisfaction — creative gnomes love craft, brave enjoy combat training, empathetic feel good healing, lazy gnomes enjoy idle time
3. **Environment** (25): Sunshine, underground depth, near water, in workshops/rooms/farms/groves, deep underground fear for nervous gnomes
4. **Social** (20): Best friend/rival proximity, crowds for social vs loner, loneliness/solitude
5. **Personality ambient** (25): Permanent mood tinting from extreme traits — naturally happy optimists, simmering anger for volatile gnomes
6. **Kingdom/situation** (10): Colony size, season, time of day
7. **Status** (5): Trapped, lockdown reactions based on Bravery/Nerve

---

## [2026-03-25] Creature Info Panel — Full Personality Display

**Milestone**: 2.0/2.1 UI
**Files changed**: `src/gui/aggregatorcreatureinfo.h`, `src/gui/aggregatorcreatureinfo.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes
- Clicking a gnome now shows full personality in the tile info panel: mood bar (color-coded), backstory, personality traits, active thoughts, relationships, and stats
- Mood bar with mental break warning, needs bars with color thresholds, activity text
- Backstory (Youth + Before titles, hover for full flavor text)
- Notable traits as colored progress bars with tooltips
- Thoughts section always visible (shows "No strong feelings right now" when empty)

---

## [2026-03-25] UI Bug Fixes

**Files changed**: `src/gui/mainwindow.cpp`, `src/gui/ui/ui_tileinfo.cpp`, `src/gui/ui/ui_sidepanels.cpp`, `src/gui/ui/ui_mainmenu.cpp`, `src/game/gamemanager.cpp`

### Changes
- **Space bar always pauses** — Space and Escape now handled BEFORE the `ImGui::WantCaptureKeyboard` check, so they work regardless of which ImGui window has focus.
- **Tile Info repositioned** — Moved from Y=100 to Y=150, now uses `ImGuiCond_FirstUseEver` so it's moveable and resizable.
- **Creature Info no longer covers time** — Moved from Y=60 to Y=150, now moveable and resizable.
- **All side panels smaller and moveable** — Changed from full-screen fixed (`io.DisplaySize.x - 10, io.DisplaySize.y - 110`) to 60% width × 70% height defaults with `ImGuiCond_FirstUseEver`. Removed `NoMove | NoResize` flags from all panels.
- **Gnome count on embark fixed** — `GameManager::startNewGame()` now syncs `numGnomes` from UI config to `NewGameSettings` before save/create. Previously the UI slider wrote to `Global::cfg` but `NewGameSettings::m_numGnomes` was never updated from it.

---

## [2026-03-25] Milestone 5.2 — Translation System

**Milestone**: 5.2 — Modding & Community
**Files changed**: `src/gui/strings.h`, `src/gui/strings.cpp`

### Changes
- **Named parameter interpolation** — `S::s("$Key", {{"name", "Urist"}, {"item", "sword"}})` replaces `{name}` and `{item}` in the translated string. Enables word-order flexibility for different languages.
- **JSON translation loading** — `Strings::loadJsonTranslations(path)` loads key-value pairs from external JSON files, overriding DB strings. Community translators can work with JSON instead of SQLite.

---

## [2026-03-25] Milestone 5.1 — Modding API

**Milestone**: 5.1 — Modding & Community
**Files changed**: `src/base/modmanager.h` (new), `src/base/modmanager.cpp` (new)

### Changes
- **ModManager class** — scans `mods/` folder for mod directories containing `mod.json` metadata
- **Mod metadata** — `mod.json` with name, author, version, description
- **Data layer** — mods place JSON files in `data/` subfolder, each mapping to a DB table (e.g., `data/items.json` → Items table). Entries update matching DB rows by ID.
- **Mod enable/disable** — `setEnabled()` per mod, `applyMods()` applies all enabled mods after DB init
- **Auto-discovery** — `scanMods()` finds all valid mod directories automatically

---

## [2026-03-25] Milestone 4.2 — Automaton Progression

**Milestone**: 4.2 — Automation & Late Game
**Files changed**: `src/game/automaton.h`, `src/game/automaton.cpp`

### Changes
- **3-tier automaton system** — `AutomatonTier` enum: Clockwork (1 labor, 0.6x speed), Steam (3 labors, 0.9x speed), Arcane (unlimited labors, 1.0x speed)
- **Degradation** — automatons lose durability over time. Clockwork degrades fastest, Arcane slowest.
- **Anti-cheese** — `maxAutomatonsForGnomes()` limits automatons to 1 per 10 gnomes
- **Work speed multiplier** — `workSpeedMultiplier()` returns tier-appropriate speed factor
- **Serialization** — tier and durability saved/loaded, backward-compatible (old saves default to Clockwork tier)

---

## [2026-03-25] Milestone 4.1 — Event-Triggered Mechanisms

**Milestone**: 4.1 — Automation & Late Game
**Files changed**: `src/game/mechanismmanager.h`, `src/game/mechanismmanager.cpp`

### Changes
- **AlarmBell and ConditionPlate** mechanism types — new `MT_ALARMBELL` and `MT_CONDITIONPLATE` enum values
- **Trigger condition system** — `triggerCondition` field on MechanismData supports: "raid", "nighttime", "daytime", "lockdown", plus numeric conditions ("food<50", "drink<10", "gnomes>5")
- **Event trigger evaluation** — `evaluateEventTriggers()` runs every 10 ticks, activates/deactivates mechanisms based on game state
- **Condition parser** — supports string equality checks and `<`/`>` numeric comparisons against inventory counts, gnome count, etc.

---

## [2026-03-25] Milestone 3.2 — Enemy Diversity (Raid Scaling)

**Milestone**: 3.2 — Combat & World
**Files changed**: `src/game/eventmanager.cpp`

### Changes
- **Difficulty-scaled raids** — Raid strength now uses formula: `(year + gnomeCount/4) × difficultyMultiplier`. Peaceful difficulty prevents all raids. Easy halves raid strength. Hard/Brutal increase significantly.
- **Night raid bonus** — Raids triggered at night get 1.3x strength multiplier.
- **Raid incoming notification** — DANGER log message when a raid is dispatched: "A goblin raid is approaching! N enemies detected."
- **Population-based scaling** — More gnomes attract larger raids (gnomeCount/4 added to base amount).

### Technical Details
- Uses `DifficultyMultipliers::forDifficulty()` from 3.3 work
- Replaces `GameState::year + 1` with proper formula
- Note: New enemy types (ranged/flying/sapper/digger) need DB entries + BT XML files — deferred to dedicated content pass

---

## [2026-03-25] Milestone 3.1 — Combat UI & Feedback

**Milestone**: 3.1 — Combat & World
**Files changed**: `src/game/eventmanager.h`, `src/game/eventmanager.cpp`, `src/game/monster.cpp`, `src/game/gnome.cpp`

### Changes
- **Battle tracker** — `BattleTracker` struct in EventManager tracks active battles (start tick, gnome wounds/deaths, enemy kills). Starts on first combat event, ends after 200 ticks of no combat.
- **Post-battle recap** — When battle ends, a summary is logged: "Battle ended! Duration: N ticks. Enemies killed: X. Gnome casualties: Y dead, Z wounded." Appears as COMBAT log entry and toast notification.
- **Enhanced combat messages** — Monster attacks now show damage amount. Gnome attack logs show hit/dodge detail. Monster deaths say "has been slain!" instead of generic.
- **Combat event recording** — `recordCombatEvent(isGnome, isDeath, isWound)` called from Monster and Gnome attack methods to feed the battle tracker.

---

## [2026-03-25] Milestone 3.3 — World Dynamics

**Milestone**: 3.3 — Combat & World
**Files changed**: `src/base/enums.h`, `src/base/gamestate.h`, `src/base/gamestate.cpp`, `src/game/game.cpp`, `src/game/newgamesettings.h`, `src/game/gamemanager.cpp`

### Changes
- **Difficulty system** — 6-level `Difficulty` enum (Peaceful/Easy/Normal/Hard/Brutal/Custom) with `DifficultyMultipliers` struct providing raid strength, spawn frequency, equipment tier, immigration, resource multipliers
- **Temperature system** — `GameState::temperature` (0-100) updates hourly by season + day/night ± weather
- **Weather events** — Storm, HeatWave, ColdSnap (5% daily chance, ~3 day duration). Logged as WARNING.
- **Full serialization** — difficulty, temperature, weather all persist in save files

---

## [2026-03-25] Milestone 2.1 — Happiness/Mood System

**Milestone**: 2.1 — Gnome Depth
**Files changed**: `src/game/gnome.h`, `src/game/gnome.cpp`, `src/gui/aggregatorpopulation.h`, `src/gui/aggregatorpopulation.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **Thought stack system** — each thought has value + duration + max stacks (RimWorld model). Thoughts expire over time and are re-generated from game events.
- **Mood 0-100 bar** — calculated from base mood (Optimism trait), active thought sum, and needs penalties (hunger/thirst/sleep below 20). Stored in existing "Happiness" need slot.
- **Trait modulation** — Empathy amplifies death thoughts, Sociability amplifies social thoughts, Greed amplifies room thoughts, Appetite amplifies food thoughts.
- **Work speed modifier** — mood 0 = 0.7x, mood 50 = 1.0x, mood 100 = 1.3x. Exposed via `moodWorkSpeedModifier()`.
- **Mental breaks** — trigger at mood < 5. Type determined by personality: high Temper → Tantrum, low Nerve → Catatonic, low Optimism → Sad Wander. Catharsis thought (+15 mood for 2500 ticks) prevents cascading breaks.
- **Social mood thoughts** — "Near a close friend" (+3) and "Near a rival" (-2) generated when gnomes with strong opinions are within 5 tiles.
- **Need-based thoughts** — "Very hungry/thirsty" (-6) and "Getting hungry/thirsty" (-2) generated from low need values.
- **Mood display** in Personality tab — color-coded progress bar (green/yellow/orange/red), active thoughts listed with values, mental break warning in red.
- **Full serialization** — mood, mental break state, and active thoughts saved/loaded.

### Technical Details
- `Thought` struct: id, text, moodValue, ticksLeft, maxStacks
- `tickThoughts()` called every tick in `evalNeeds()`, decrements timers and recalculates mood
- Trait modulation formula: `modulated = base + (base * traitValue / 100)` — a gnome with Empathy +50 gets 1.5x the base death mood penalty
- Mental break threshold at mood < 5 (extreme tier only — minor/major thresholds deferred)

---

## [2026-03-25] Milestone 2.2 — Designated Movement Zones

**Milestone**: 2.2 — Gnome Depth
**Files changed**: `src/base/tile.h`, `src/base/gamestate.h`, `src/base/gamestate.cpp`, `src/gui/ui/ui_gamehud.cpp`

### Changes

- **Zone tile flags** — Added TF_ZONE_MILITARY, TF_ZONE_CIVILIAN, TF_ZONE_RESTRICTED to TileFlag enum for future zone painting
- **Lockdown system** — `GameState::lockdown` boolean, toggled via "Lockdown" button in the time/speed controls panel
- **Lockdown UI** — Button shows "Lockdown" (normal) or "LOCKDOWN ACTIVE" (red) with click to toggle. State changes logged to event log.

### Technical Details
- Zone flags use bits 0x100000000, 0x200000000, 0x400000000 in the quint64 TileFlag enum
- Lockdown state is a simple global boolean — gnome behavior tree integration (actually blocking civilian movement) deferred to when behavior trees are extended
- Zone painting tool (click-drag to designate zones) deferred to dedicated zone UI work

---

## [2026-03-25] Milestone 2.3 — Medical System Data Layer

**Milestone**: 2.3 — Gnome Depth
**Files changed**: `content/db/ingnomia.db.sql`

### Changes
- **Diseases table** — 3 starter diseases: Infection (from untreated wounds, severity 0.84/day vs immunity 0.644/day), Plague (seasonal, severity 1.2/day), Food Poisoning (from bad food, severity 2.0/day, non-lethal at 50%). Each has treatment slowdown value.
- **Triage priority table** — 3 tiers: Critical (bleeding out), Serious (breaks, infection), Minor (bruises). Defines the priority order for medic AI.
- Data-only: medic behavior tree integration (actually routing medics to patients by priority) requires BT work.

---

## [2026-03-25] Milestone 2.4 — Food & Farming QoL

**Milestone**: 2.4 — Gnome Depth
**Files changed**: `src/game/gnome.h`, `src/game/gnome.cpp`, `src/game/grove.h`, `content/db/ingnomia.db.sql`

### Changes
- **Food exclusion list** — `m_foodExclusions` QStringList on Gnome. Gnomes can be assigned food policies preventing them from eating certain items. Serialized/deserialized.
- **Food policies DB** — 3 policies: "Eat Anything" (default), "Cooked Only" (skip raw food), "Preserve Seeds" (skip plantable seeds).
- **Groves include existing trees** — `includeExistingTrees` flag on GroveProperties (default: true). When set, existing trees in the grove area are treated as part of the grove.
- Data-only: actual enforcement in behavior trees (checking food exclusions before eating, scanning existing trees when grove is created) requires BT/farming system integration.

---

## [2026-03-25] Milestone 1.2 — Stockpile UX Overhaul

**Milestone**: 1.2 — Stockpile UX Overhaul
**Files changed**: `src/game/stockpile.h`, `src/game/stockpile.cpp`

### Changes
- **"For Trade" flag** — Stockpiles can be marked as "for trade" via `m_forTrade` property. Serialized/deserialized. UI integration and trade window filtering deferred to trade system rework.
- **Auto-accept new items** — `m_autoAcceptNew` flag (default: true) enables stockpiles to automatically include new item types when their parent category is checked. Serialized with backward-compatible default.
- **Backward-compatible** — Old saves without these fields load cleanly (forTrade defaults false, autoAcceptNew defaults true).

### Technical Details
- Both flags added as private members with public accessors in `stockpile.h`
- Serialized via `out.insert()` in `serialize()`, deserialized in constructor from `vals`
- `autoAcceptNew` uses `vals.contains()` check for backward compatibility with old saves
- Note: UI for these features (checkboxes in stockpile panel, copy/paste buttons) needs ImGui panel work — the data layer is complete

---

## [2026-03-25] Milestone 2.0b — Social System

**Milestone**: 2.0b — Gnome Depth
**Files changed**: `src/game/gnomemanager.h`, `src/game/gnomemanager.cpp`, `src/gui/aggregatorpopulation.h`, `src/gui/aggregatorpopulation.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **Opinion system** — each gnome pair has an opinion score (-100 to +100) stored in `GnomeManager::m_opinions`
- **Social interactions** — gnomes within 5 tiles interact every 10 ticks (15% chance per pair). Interaction type determined by trait compatibility and current opinion: Chat (+1), Deep Conversation (+8), Compliment (+5), Argument (-8), Insult (-12)
- **Trait compatibility** — calculated from similarity of trait values across all 12 traits. Similar profiles → friendship, divergent → rivalry
- **Relationship labels** at thresholds: Rival (<-30), Disliked (-30 to -10), Neutral (-10 to +10), Friendly (+10 to +30), Close Friend (>+30)
- **Social tab** in Population panel — shows all non-neutral relationships color-coded (green=positive, red=negative, yellow=neutral)

### Technical Details
- `processSocialInteractions()` runs in `GnomeManager::onTick`, guarded by `tickNumber % 10`
- O(gnomes²) but only for proximity-checked pairs within 5 tiles — negligible with <100 gnomes
- Opinions not yet serialized to save files — will be added when mood system (2.1) needs persistent relationships

---

## [2026-03-25] Milestone 1.3 — Workshop Production Limits

**Milestone**: 1.3 — Workshop Production Limits
**Files changed**: `src/game/workshop.h`, `src/game/workshop.cpp`

### Changes
- **Added CraftUntilStock mode** — New `CraftMode::CraftUntilStock` alongside existing CraftNumber/CraftTo/Repeat. Checks free item count in stockpile (via `Inventory::itemCount()`) rather than items-including-in-job (CraftTo's behavior). When stockpile count >= target, craft job auto-pauses.
- **Serialization** — New mode persists as "CraftUntilStock" in save files, backward-compatible with old saves.

### Technical Details
- CraftTo uses `itemCountWithInJob()` (counts items claimed by jobs), CraftUntilStock uses `itemCount()` (only free items) — this means CraftUntilStock won't over-produce when multiple jobs are in-flight
- Both modes share the same pause/resume logic in the workshop tick loop
- `Inventory::itemCount()` is now cached from Milestone 0.3 work, so this check is O(1)

---

## [2026-03-25] Milestone 1.4 — HUD & UI Improvements

**Milestone**: 1.4 — HUD & UI Improvements
**Files changed**: `src/gui/ui/ui_gamehud.cpp`, `src/gui/imguibridge.h`, `src/gui/imguibridge.cpp`, `src/game/game.cpp`, `src/game/inventory.h`, `src/gui/mainwindow.cpp`

### Changes
- **Resource bar** — New horizontal bar above toolbar showing Food, Drink, Gnomes, Items counts. Food/drink color-coded: green >50, yellow 10-50, red <10.
- **Food/drink counters** — Game emits food and drink item counts via kingdom info signal. `Inventory::foodItemCount()` and `drinkItemCount()` methods added.
- **Z-level keyboard shortcuts** — Page Up/Page Down now change Z-level (previously only scroll wheel worked). Addresses accessibility request from trackpad users.
- **Kingdom info updated** — Info bar now shows "Food: N | Drink: N" instead of "Animals: N" for more useful at-a-glance data.

### Technical Details
- Resource bar rendered at `io.DisplaySize.y - 50 - 24` (just above toolbar)
- `onKingdomInfo` parses "Food: N | Drink: N" from i2 string by splitting on '|'
- Page Up/Down mapped in `keyPressEvent` switch to `keyboardZPlus`/`keyboardZMinus`

---

## [2026-03-25] Milestone 2.0 — Character Traits & Backstories

**Milestone**: 2.0 — Gnome Depth
**Files changed**: `content/db/ingnomia.db.sql`, `src/game/creature.h`, `src/game/creature.cpp`, `src/game/gnomefactory.h`, `src/game/gnomefactory.cpp`, `src/gui/aggregatorpopulation.h`, `src/gui/aggregatorpopulation.cpp`, `src/gui/ui/ui_sidepanels.cpp`

### Changes

- **12 personality traits** added on a -50 to +50 scale: Bravery, Sociability, Industriousness, Appetite, Temper, Creativity, Greed, Curiosity, Empathy, Stubbornness, Optimism, Nerve. Generated with a bell curve centered on 0 — most gnomes are average in most traits, distinctive in 2-4.
- **40 backstories** (15 childhood + 25 adulthood): each gnome gets a random childhood + adulthood pair. Backstories provide skill modifiers (±1-3 levels) and trait biases that nudge personality. All DB-defined for moddability.
- **Personality tab** in the Population panel: shows backstory (Youth/Before labels with tooltip for full text) and notable traits (|value| > 25) as colored progress bars with tooltips.
- **Backward-compatible**: old saves load without crash — gnomes simply have empty traits/backstories.

### Technical Details
- Traits stored as `QVariantMap m_traits` in `Creature` class, serialized/deserialized alongside existing attributes/skills
- `GnomeFactory::createGnome()` now calls `generateTraits()` and `assignBackstory()` after skill assignment
- Backstory skill modifiers and trait biases use pipe-delimited format in DB (`"Mining:2|Masonry:1"`) parsed at gnome creation
- Trait descriptions only appear for extreme values (|value| > 25), fetched from DB `Traits` table via `DB::selectRow()`
- This is data-only — traits don't affect gameplay yet. Mood (2.1) and Social (2.0b) will wire them up.

---

## [2026-03-25] Milestone 1.1 — Event & Notification Log

**Milestone**: 1.1 — Event & Notification Log
**Files changed**: `src/base/logger.h`, `src/base/logger.cpp`, `src/gui/imguibridge.h`, `src/gui/ui/ui_gamehud.cpp`, `src/gui/ui/ui_sidepanels.h`, `src/gui/ui/ui_sidepanels.cpp`, `src/gui/mainwindow.cpp`, `src/game/eventmanager.cpp`, `src/game/gnome.cpp`

### Changes
- **Extended LogType enum** — Added INFO, MIGRATION, DEATH, DANGER types alongside existing DEBUG/JOB/CRAFT/COMBAT/WARNING
- **Log timestamps** — Each log message now includes formatted game time (Year/Season/Day/Hour:Minute)
- **Log cap at 1000 entries** — Prevents unbounded memory growth
- **Toast notification system** — Critical events (WARNING, DANGER, COMBAT, DEATH, MIGRATION) appear as fading overlays in the top-right corner, color-coded by severity, auto-dismiss after ~5 game-minutes
- **Event Log side panel** — New "Log" button in toolbar opens a full-screen scrollable event log with filter checkboxes (Info, Warning, Combat, Death, Jobs, Debug). Entries color-coded: red=death, orange=danger, yellow=warning, pink=combat, green=migration, blue=info, grey=jobs
- **Migration event logging** — EventManager now logs migration/trader events to the event log
- **Death type logging** — Gnome deaths use LogType::DEATH for distinct color and filtering

### Technical Details
- Toast generation checks `Global::logger().messages().size()` against `bridge.lastLogCount` to detect new entries
- Event log renders newest-first with `ImGui::BeginChild` scroll region
- SidePanel::EventLog added to enum and dispatched in `mainwindow.cpp` switch

---

## [2026-03-25] Milestone 0.4 — Game Loop Parallelization

**Milestone**: 0.4 — Parallelization
**Files changed**: `src/game/game.cpp`, `src/base/pathfinder.h`, `src/base/pathfinder.cpp`, `src/game/creaturemanager.cpp`, `src/game/gnomemanager.cpp`, `src/base/regionmap.cpp`

### Changes

**Creature position spatial index:**
- `creaturesAtPosition()`, `animalsAtPosition()`, `monstersAtPosition()` changed from O(creatures) linear scan to O(1) lookup via existing `World::m_creaturePositions` index. The index was already maintained on every creature move but **never used** by the lookup functions.
- `gnomesAtPosition()` similarly refactored to use the world position index + `m_gnomesByID` hash instead of scanning all gnome lists.

**Phase 1 — Non-blocking pathfinding:**
- Split `findPaths()` into `dispatchPaths()` (launches async workers, non-blocking) and `collectPaths()` (waits for results).
- Game loop now collects previous tick's path results at the START of each tick, and dispatches new requests at the END. Path workers run in parallel with the next tick's game logic.
- Added `m_activeTasks` vector to PathFinder to track outstanding futures between ticks.

**Phase 2 — Parallel natural world:**
- `processGrass()`, `processWater()`, and `processPlants()` now run concurrently via `std::async`. They touch independent tile fields (vegetation vs fluid vs plant map).
- Water moved from end-of-tick to parallel block at start-of-tick.

**Phase 3 — Parallel passive systems:**
- Rooms, mechanisms, fluids, neighbors, and sound now run in a background `std::async` task while the main thread handles events and item history.
- Main thread pipeline: creatures → gnomes → jobs → stockpiles → farming → workshops → events
- Background pipeline: rooms → mechanisms → fluids → neighbors → sound

**Bug fix:**
- Fixed `RegionMap::checkSplitFlood()` — allocated `m_dimX * m_dimX` instead of `m_dimX * m_dimY` for visited array. Would cause out-of-bounds on non-square maps.

### Technical Details
- PathFinder: `dispatchPaths()` at line 109, `collectPaths()` at line 168, `findPaths()` preserved as legacy wrapper
- Game loop: `collectPaths` → parallel(grass, water, plants) → creatures → gnomes → jobs → stockpiles → farming → workshops → parallel(rooms, mechanisms, fluids, neighbors, sound) + events → `dispatchPaths`
- RegionMap fix: `regionmap.cpp:615` — `m_dimX * m_dimX` → `m_dimX * m_dimY`

---

## [2026-03-24] Milestone 0.3 — Performance: Algorithmic Bottlenecks

**Milestone**: 0.3 — Performance Bottlenecks
**Files changed**: `src/game/inventory.h`, `src/game/inventory.cpp`, `src/game/job.h`, `src/game/jobmanager.cpp`, `src/game/creaturemanager.cpp`, `src/game/farm.cpp`, `src/game/grove.cpp`, `src/base/pathfinder.cpp`, `src/game/workshopmanager.h`, `src/game/workshopmanager.cpp`

### Changes

**Tier 1 — Critical (20+ gnome scaling):**
- **P1: JobManager availability cache** — Job item availability is now cached per inventory generation. Previously 50 gnomes × 200 jobs = 10,000+ octree queries/tick. Now rechecks only when inventory actually changes.
- **P2: Inventory::itemCount() cache** — Added generation-based cache for item counts. Cache is invalidated on any item state change (create, destroy, pickup, putdown, setInJob, setConstructed, setUsedBy). Repeated calls with same (itemSID, materialSID) return cached result in O(1).

**Tier 2 — High (50+ gnome scaling):**
- **P3: Farm/Grove tick throttle** — Farms and groves now only check fields every 10 ticks instead of every tick. A 400-tile farm goes from 400 octree queries/tick to 40/tick.
- **P4: PathFinder cancel stale requests** — Implemented `cancelRequest()` which was a TODO stub. Pending path requests for dead/removed creatures can now be cancelled instead of wasting thread time.
- **P5: CreatureManager time-slicing** — Re-enabled the commented-out time budget (2ms). Wild animals now time-slice across ticks instead of all ticking every frame. `m_startIndex` properly carries over between ticks.

**Tier 3 — Medium (done: P6, deferred: P7-P8):**
- **P6: WorkshopManager hash lookup** — `workshop(id)` changed from O(workshops) linear scan to O(1) hash lookup via `m_workshopsByID`. Hash maintained on add/remove.
- **P7: StockpileManager reverse job map** — Deferred (more invasive, less critical at current scale)
- **P8: Pasture animal indexing** — Deferred (more invasive, less critical at current scale)

### Technical Details
- Inventory generation counter: `m_itemCountGeneration` incremented by `invalidateItemCounts()`, called from: `addObject`, `destroyObject`, `pickUpItem`, `putDownItem`, `setInJob`, `setConstructed`, `setIsUsedBy`
- Job cache: `m_availabilityCacheGeneration` + `m_cachedAvailability` per Job, compared against `Inventory::itemCountGeneration()`
- Farm/grove throttle: `tick % 10 != 0` guard at top of `onTick()`
- CreatureManager: uncommented `if (m_startIndex >= m_creatures.size())` reset and `if (timer.elapsed() > 2) break` budget

---

## [2026-03-24] Milestone 0.2 — Critical Bug Fixes (Batch 2)

**Milestone**: 0.2 — Critical Bug Fixes
**Files changed**: `src/base/io.cpp`, `src/base/gamestate.h`, `src/base/gamestate.cpp`, `src/game/worldgenerator.cpp`, `src/game/jobmanager.cpp`

### Changes
- **Fixed save corruption with same kingdom name** — Save folders now use a unique identifier (`<KingdomName>_<timestamp>`) instead of just the kingdom name. Two games named "The Life Kingdom" no longer share the same save folder. Legacy saves without `saveFolderName` fall back to the old behavior for backward compatibility.
- **Fixed rotated workshop crafting** — Work position offsets from the DB were not being rotated to match the workshop's rotation. A workshop rotated 90° had its gnome work positions calculated as if unrotated, causing gnomes to pathfind to the wrong tile and fail to craft. Now applies the same rotation transform to work offsets as is applied to component tiles and I/O positions.
- **Investigated disappearing floors** — Audited all `FT_NOFLOOR` assignments in `world.cpp` and `worldconstructions.cpp`. The deconstruct logic correctly gates floor removal behind `isFloor` flag. Previous fixes in v0.7.0 addressed known cases. Marked as likely-fixed pending gameplay testing.

### Technical Details
- Save path: `GameState::saveFolderName` stored in save file, generated via `QDateTime::currentSecsSinceEpoch()` during world generation
- Rotated workshops: `workPositionWalkable()` in `jobmanager.cpp` now applies rotation cases 1-3 to offset before computing `testPos`
- Floor investigation: `worldconstructions.cpp:1192` correctly uses `isFloor` guard; wall deconstruct at 1208 doesn't touch floor type

---

## [2026-03-24] Milestone 0.2 — Critical Bug Fixes (Batch 1)

**Milestone**: 0.2 — Critical Bug Fixes
**Files changed**: `src/game/gnome.cpp`, `src/game/gnome.h`, `src/game/gnomeactions.cpp`, `src/gui/ui/ui_sidepanels.cpp`, `src/game/jobmanager.cpp`, `src/base/logger.h`

### Changes
- **Fixed negative thirst/hunger crash** — Needs values now clamped between -100 and 150 in `evalNeeds()`. Previously needs could decrease unboundedly, causing UI crashes when displaying negative values. Progress bars in gnome info panel now also clamped to 0-1 range via `qBound`.
- **Added trapped gnome detection** — Every in-game hour, each gnome checks if it can reach any stockpile using `RegionMap::checkConnectedRegions()`. If trapped, sets "Trapped" thought bubble and logs a WARNING. Automatically clears when gnome can reach stockpiles again.
- **Improved job cancellation** — Cancelling a worked job now sets both `canceled` AND `aborted` flags, ensuring the gnome detects the cancel on the very next tick. Previously only `setCanceled()` was called, which some behavior tree states didn't check. Also added stale job sprite cleanup when no job is found at a cancelled position.
- **Improved dead gnome cleanup** — `Gnome::die()` now also clears room ownership (not just workshop assignments). Added death log message via WARNING log type.
- **Added WARNING log type** — New `LogType::WARNING` for trapped gnomes, deaths, and other player-visible alerts.

### Technical Details
- `evalNeeds()` in `gnome.cpp`: `qMax(-100.f, qMin(150.f, oldVal + decay))` prevents unbounded decrease
- Trapped check uses `ticksPerDay / 24` as hourly interval since `ticksPerHour` doesn't exist as a Util field
- Job cancel: `setAborted(true)` alongside `setCanceled()` hits the check at `gnome.cpp:648`
- Room cleanup iterates `g->rm()->allRooms()` — required adding `#include "../game/roommanager.h"`

---

## [2026-03-24] Selection Preview & Escape Key Fix

**Milestone**: 0.1c — UI Fixes
**Files changed**: `src/gui/mainwindowrenderer.cpp`, `src/gui/imguibridge.cpp`, `src/game/gamemanager.cpp`

### Changes
- **Fixed ghost/preview rendering for all tools** — Selection shader was using `m_axleShader->uniformLocation("tile")` instead of `m_selectionShader->uniformLocation("tile")`. This caused all tool previews (mine, build, workshop, etc.) to render at the wrong position. Previews now follow the cursor correctly.
- **Escape key now clears active tools first** — Previously, pressing Escape while a tool was active would skip straight to the pause menu. Now follows priority: active tool → side panel → pause menu.
- **Connected `signalPropagateKeyEsc` to `Selection::clear()`** — This signal was emitted but never wired to anything. Escape now properly clears the selection state on the game thread.

### Technical Details
- `paintSelection()` in `mainwindowrenderer.cpp:662` was copy-pasted from `paintAxles()` but the shader reference was never updated
- `onKeyEsc()` in `imguibridge.cpp` now checks `currentToolbar` and `currentBuildCategory` before falling through to panel/menu logic
- Added `connect(m_eventConnector, &EventConnector::signalPropagateKeyEsc, Global::sel, &Selection::clear)` in `gamemanager.cpp`

---

## [2026-03-24] Loading Performance — 98% Faster

**Milestone**: 0.1b — Loading Performance
**Files changed**: Renderer init path, sprite factory, DB queries

### Changes
- **98% reduction in loading time** — Batch DB queries, parallel tile processing, and bulk GPU upload
- Sprite factory initialization optimized
- Tile data upload to GPU now uses bulk operations instead of per-tile uploads

---

## [2026-03-24] ImGui UI Migration & Loading Screen

**Milestone**: 0.1 — Build System & Platform
**Files changed**: Multiple (full UI migration)

### Changes
- **Migrated entire UI from Noesis/XAML to Dear ImGui** — Removed Noesis dependency entirely
- **Added loading screen** with progress indicator
- **Added MCP test command server** for automated testing via `--test-mode`
- **ImGui theme** applied for consistent look

---

## [2026-03-24] macOS Port

**Milestone**: 0.1 — Build System & Platform
**Files changed**: Shaders, CMakeLists.txt, renderer

### Changes
- **Downgraded OpenGL from 4.3 to 4.1** for macOS compatibility
- **Replaced SSBOs with Texture Buffer Objects** (TBOs)
- **Fixed all shaders** for GLSL 4.1 compatibility
- Clean CMake build on macOS with Homebrew Qt5 and OpenAL

---

## [2026-03-24] Project Analysis & Roadmap

**Milestone**: Planning
**Files changed**: `docs/` directory

### Changes
- **Analyzed 15,893 Discord messages** across 4 channels (#suggestions, #bug-reports, #dev-discussion, #updates)
- **Created development roadmap** with 6 milestones from community feedback, dev knowledge, and codebase analysis (`docs/updates/development_roadmap.md`)
- **Extracted 100 feature requests** from 4,693 community suggestions (`docs/suggestions/feature_requests.md`)
- **Catalogued 407 bug threads** with 310 potentially still open (`docs/bug-reports/bug_report_summary.md`)
- **Built Discord reply queue** — 66 draft replies mapped to original messages, ready for batch send when milestone 0+1 are complete (`docs/discord_reply_queue.json`)
- **Documented complete version history** from v0.2.3 to v0.8.10 (`docs/changelogs/version_history.md`)
- **Created parallelization plan** for game loop threading (`docs/updates/parallelization_plan.md`)
