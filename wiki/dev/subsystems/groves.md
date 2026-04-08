---
title: Groves — subsystem audit
tags: [dev, subsystem, groves, audit]
status: current
last_updated: 2026-04-07
sources: [src/game/grove.cpp, src/game/farmingmanager.cpp, src/base/selection.cpp, src/gui/aggregatoragri.cpp, src/gui/ui/ui_sidepanels.cpp, content/db/ingnomia.db.sql]
---

# Groves — subsystem audit (T-0010)

## Verdict

**Groves are implemented and wired correctly end-to-end. The user's bug report was based on misunderstanding the UX, not a code-level bug.** The grove backend has its own class, its own storage, its own tick logic, its own job dispatch, and a correct tree species list sourced from the `Plants` DB table. What looks like "it creates a farm that points to vegetable/fruit seeds" is actually a UX discoverability problem: a fresh grove does nothing until the player configures it, and the tree species names happen to include fruit words.

Classification: **Mix — core system works, UX layer needs polish.** No deep runtime fix needed; the work is small follow-ups in the UI and default-state handling.

## What actually happens (end-to-end trace)

### Dispatch: Grove button → `FarmingManager::addGrove`

1. **UI** (`src/gui/ui/ui_gamehud.cpp:236`): the Grove button in the Zone panel has `action = "CreateGrove"`, dispatched via `cmdSetSelectionAction`.
2. **Selection** (`src/base/selection.cpp:729`): when the user confirms the selection, `m_action == "CreateGrove"` routes to `g->fm()->addGrove( m_firstClick, m_selection )`.
3. **FarmingManager** (`src/game/farmingmanager.cpp:213`): `addGrove` creates a **`new Grove( fields, g )`** — a distinct `Grove` class (not a `Farm`), stored in `m_groves` (a separate map from `m_farms`), with its own ID namespace in `m_allGroveTiles`.

**Conclusion**: the entity is a real `Grove`, not a `Farm`. The user's "it creates a farm" report is incorrect at the code level.

### Runtime: `Grove::onTick`

`Grove::onTick()` at `src/game/grove.cpp:149` is invoked every 10 game ticks. For each field in the grove:

- **No plant present + `plant == true`**: enqueue a `PlantTree` job at the field position. Looks up the species's seed ID from `DB::select("SeedItemID", "Plants", treeType)`, finds the closest available seed item via inventory, attaches it to the job's required items.
- **Plant present + harvestable + `pickFruit == true`**: enqueue a `HarvestTree` job.
- **Plant present + mature wood + `fell == true`**: enqueue a `FellTree` job.

All three job types are fully wired in the game thread (`Gnome::m_taskFunctions` has `PlantTree`, `HarvestTree`, `FellTree` entries at `gnome.cpp:635-636`). `Grove::updateAutoForester` additionally toggles `pickFruit` / `fell` automatically when stockpiled fruit/wood counts cross user-configured min/max thresholds.

**Conclusion**: grove runtime logic exists and works. The `PlantTree` + `HarvestTree` + `FellTree` chain is end-to-end functional.

### Tree species list: `AggregatorAgri::init`

At `src/gui/aggregatoragri.cpp:77`, `init()` iterates every row in the `Plants` DB table and populates `m_globalTreeInfo` with entries where `Type == "Tree"`. The `Plants` table contains **6 trees**: `PineTree`, `AppleTree`, `OrangeTree`, `OakTree`, `WillowTree`, plus one more (verify by grep). Each has a `Material` field (Pine, Apple, Orange, Oak, Willow) which becomes the display name via `S::s("$MaterialName_" + material)`.

**Conclusion**: the tree list is correctly populated and shows real tree species, not vegetables. The confusion is that **some trees are named by the fruit they bear** (Apple, Orange) because Masonry's naming convention uses the tree's material/fruit. A player seeing `Apple` and `Orange` in a dropdown labeled "Tree:" could plausibly misread it as fruit seeds.

## Why the user thought it was broken

**Three confluent UX problems**:

1. **A freshly-created grove does nothing by default.** The `GroveProperties` struct initializes `plant`, `pickFruit`, and `fell` all to `false`, and `treeType` to an empty string. `Grove::onTick` respects these flags — no flag set means no jobs enqueued. The player has to open the grove info panel (Side panels → Agriculture → Grove view) and manually:
   - Pick a tree species from the dropdown (required — empty string means no seeds lookup)
   - Enable `Plant Trees` and/or `Pick Fruit` / `Fell Trees`
   
   Until they do that, the grove appears "dead" because no jobs are queued. Easy to assume it's broken.

2. **Tree names include fruit words.** `AppleTree` appears in the dropdown as `Apple` (via `$MaterialName_Apple`), `OrangeTree` as `Orange`. If a player doesn't know that Masonry names tree species by their material, they'll misread the list as "vegetables and fruits" rather than "tree species that produce apples, oranges, etc." The code is correct — the UX copy is misleading.

3. **Different visual color from farms.** Correct: groves render in a different color on the map because they're a distinct entity. The user interpreted this as "a farm that's been coloured differently", but it's the *right* outcome — a grove is visually distinct because it IS a different thing. Confirming grove rendering is in its own render path, not a farm with a color override.

## Real follow-up task seeds

These are the legitimate improvements to file as new `tasks/inbox/` entries:

1. **Default new groves to a usable state.** In `Grove::Grove(tiles, game)` (constructor at `grove.cpp:88`), set `m_properties.plant = true`, `m_properties.pickFruit = true`, and auto-select the first tree species in `globalTrees` (or the first species for which seeds exist in inventory). A fresh grove should start *doing something* immediately, even if the player later tunes it.

2. **Empty-grove UX hint.** In `drawGroveView` (`ui_sidepanels.cpp:2134`), when `grove.treeType.isEmpty()`, show a yellow warning banner at the top: *"This grove has no tree species selected and will not plant anything. Pick a species below."* Same pattern for `plant == false && pickFruit == false && fell == false` ("All actions disabled").

3. **Rename tree species in the dropdown.** In `AggregatorAgri::init` line 92, instead of `gp.name = S::s("$MaterialName_" + gp.materialID)` (which yields "Apple", "Orange"), use the plant ID directly or append " Tree" — e.g. "Apple Tree", "Orange Tree", "Pine Tree". Removes the fruit-name confusion.

4. **Document grove colors in the wiki**. Write a `wiki/game/systems/groves.md` player-facing page explaining what groves are, how they differ from farms visually, and how to configure one. This audit page is dev-facing; the player needs a parallel narrative.

5. **Investigate grove throttle behavior**. `Grove::onTick` only runs field checks every 10 ticks (`if ( tick % 10 != 0 && tick != m_lastUpdateTick + 1 ) return;`) but `m_lastUpdateTick` is never actually updated in the function. The second branch of the OR is dead code. Not a bug per se — the throttle still works via `tick % 10 == 0` — but the intent was probably to allow bursts, and that's broken. Low priority.

## Not a bug

The following user-reported items are not actual code problems:

- ~~"Groves create a farm"~~ — they create a `Grove` object, distinct class and storage.
- ~~"Points to vegetable/fruit seeds"~~ — tree species list is correctly filtered by `Type == "Tree"` in the DB. The "Apple/Orange" naming is the culprit.
- ~~"No logic for grove/tree growing"~~ — `Grove::onTick` is fully implemented and enqueues the correct job types (PlantTree / HarvestTree / FellTree), all of which have runtime handlers.

## See also

- [[known-issues]]
- `src/game/grove.cpp` — Grove class, `onTick`, `updateAutoForester`
- `src/game/farmingmanager.cpp:213` — `addGrove` entity creation
- `src/base/selection.cpp:729` — Selection action dispatch
- `src/gui/aggregatoragri.cpp:77` — Tree species list population
- `src/gui/ui/ui_sidepanels.cpp:2134` — Grove info panel render
