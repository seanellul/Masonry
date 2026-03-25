# Ingnomia Development Log

Every change to the codebase must be logged here. This is the master record of all development — it becomes the release changelog.

**Format**: Newest entries at the top. Use the template below for each entry.

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
