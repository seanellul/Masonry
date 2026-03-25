# Ingnomia Development Knowledge Base

Distilled from **2,567 Discord messages** in the dev/modding channel (Nov 2017 – Feb 2026).
137 conversation threads analyzed across 70 contributors.

## Key People

| Who | Role | Focus Areas |
|-----|------|-------------|
| **.roest** | Original developer & creator | Architecture decisions, game mechanics, DB schema |
| **ext3h** | Major contributor | Performance optimization, pathfinding, rendering, build system, behavior trees |
| **arcnor** | Major contributor | Qt removal, cross-platform (macOS), STL migration, renderer abstraction |
| **cravenwarrior** | Active contributor | Linux builds, testing |
| **dinosaure.plz_nofriendrequests** | Active contributor | UI/UX, Noesis XAML, translation system |
| **njoyard** | Contributor | Web-based tools, sprite rendering, data visualization |
| **condac** | Contributor | Sound system implementation |
| **fris0uman** | Contributor | Game mechanics, combat, creature AI |

---

## Architecture Decisions & Context

### Qt Dependency & Removal Effort (2022)

**arcnor** started a major effort to replace Qt containers and signals with STL/C++20:
- Replaced `QSet`, `QHash`, `QMap`, `QVector`, `Q*Pointer` with STL equivalents
- Considered Abseil containers (`absl::btree_set`, `absl::flat_hash_set`) for performance
- Used [palacaze/sigslot](https://github.com/palacaze/sigslot) to replace Qt signal/slot
- **Key challenge**: Thread safety — Qt's `QueuedConnection` provides message-passing between threads. The sigslot replacement doesn't have this, requiring manual mutexing or message queue implementation
- **ext3h** was exploring `std::unique_ptr<std::function<void(void)>>` for a message queue system
- The C++ version was bumped to **C++20** for better STL support

### Rendering Pipeline

- Uses **OpenGL 4.3** compute shaders for world rendering (`worldupdate_c.glsl`)
- **macOS limitation**: Apple deprecated OpenGL, last version is 4.1 — the game "is never gonna work on mac" per .roest (though arcnor's port effort partially addressed this)
- **ext3h** explored Vulkan via bgfx: "once Noesis gets VK support, I might actually consider that again"
- Sprites are 32x32 pixels — .roest: "started with 64x64 but was told by rho (the original gnomoria artist) that 32x32 is easier for artists"
- Some sprite sizes are "semi hard coded" — would take "an afternoon or so to make possible to work with bigger sprites though it probably will still be a multiple of 32"

### UI Framework

- **Noesis GUI** (XAML-based) is the primary UI framework
- Requires a commercial license (free for indie devs under revenue threshold)
- Complex to set up: requires downloading NoesisSDK, building NoesisApp in VS
- **ImGui** was added alongside Noesis as a debug/overlay UI (commit `9c5b6a8`)
- Translation system stores strings in the SQLite database with `$word_something` variables
- .roest acknowledged the translation system needs "a complete overhaul" for proper i18n

### Database / Data Model

- Game data stored in **SQLite database**
- `dbstructs.h` contains struct definitions that mirror DB tables (arcnor: "sort of like a fake ORM")
- **DefNode** objects are constructed from DB data as an intermediate representation for sprites
- Modding works by overriding DB entries — .roest: "just unzip in Documents\My Games\Ingnomia\mods, rename and start modifying"
- Hair color is "semi hardcoded" — random 0-5 index into dye materials table

### Build System

- **CMake** based build
- Dependencies: Qt 5.14+, NoesisSDK, SteamSDK, OpenGL 4.3
- CI auto-builds on GitHub for compilation verification
- Linux builds require "basic knowledge to install prerequisites, run cmake and compile"
- **ext3h** explored vcpkg for dependency management (bgfx packaging)

---

## Known Performance Bottlenecks

Per **ext3h** (the primary performance investigator):

1. **Enemy spawns cause lag spikes** — "all Gnomes start pathfinding simultaneously" when enemies appear. Was "almost solved" as of Nov 2020.

2. **Farm/grove job queries** — `num_gnomes * num_tiles` complexity instead of `num_gnomes + num_tiles`. O(n*m) should be O(n+m).

3. **Stockpile item selection** — "When you have hundred thousand of items outside stockpiles, only a single spot opens up in a stockpile, then choosing the item to store next is excessively expensive."

4. **UI data fetching** — "Some lag when opening a couple of the UIs, when they fetch their data from the sim thread. But that ain't avoidable."

5. **Item proliferation** — Game rules needed to "severely reduce the number of items and creatures in the world. No more 2k chickens and 50k uncollected eggs."

---

## Modding System

### How Mods Work
- Mods live in `Documents/My Games/Ingnomia/mods/`
- They override SQLite database entries
- Config stored in `Documents/My Games/Ingnomia/settings/config.json`
- .roest: "I would advise against modding in automatons, that's a feature that will most definitely be added by myself"
- .roest on early modding: "game is in version 0.55.x and receiving more or less daily updates. Making a mod on something that will most likely break tomorrow is a little optimistic"

### What's Moddable
- Crafting recipes, materials, items (via DB overrides)
- Sprites (32x32 PNG tilesheet)
- Translations (DB string table)
- Workshop definitions
- Styles/themes

### What's NOT Moddable
- Behavior trees (hardcoded in C++)
- UI layout (Noesis XAML, compiled)
- Core game mechanics (job system, pathfinding, combat)
- Sprite sizes (semi-hardcoded at 32x32)

---

## Technical Debt & Known Issues

1. **Dead gnome cleanup** — ext3h: "All references to the dead Gnome still left in the world need to be cleaned up. Ownerships, carried items, jobs, etc." Workshop assignments were specifically broken on death.

2. **Gnomoria legacy data** — .roest: "some stuff in the files is there because I just parsed the original files from gnomoria when I started 3 years ago, I have no idea what the pet rock is for"

3. **Translation system** — Needs complete overhaul. Currently uses DB-based `$word_something` variables. .roest considered but never implemented XLIFF or proper Qt linguist tooling.

4. **Signal/slot thread safety** — arcnor's Qt removal left thread safety as an open problem. Qt's `QueuedConnection` provided free cross-thread message passing; replacement needs manual implementation.

5. **Qt copy-on-write semantics** — ext3h warned: "Qt containers use copy-on-write-after-fork semantics. You can safely send a QList over a signal-slot into another thread" — this behavior is lost when migrating to STL containers.

6. **Behavior tree node naming** — ext3h: "the definition of the Star-suffixed nodes is currently inverse to the definitions in Groot. Maybe we should fix that so we can just point to their documentation."

---

## Combat & AI Development

- **Behavior trees** are the AI system (BT_RESULT return values, `m_behaviorTree->tick()`)
- ext3h worked on "path finding with moving targets in combat" — aggressive path invalidation when target moves
- Creature visibility checks: "checking ahead if the path got blocked, checking if the target has moved"
- Future plans included "checking for stuff visible to him" (line of sight)
- **fris0uman** worked on equipment dropping on death (`Creature::dropEquipment()`)

---

## Cross-Platform / Porting

- **Linux**: Compiles and runs. Build instructions in README. CI builds available.
- **macOS**: .roest said "never gonna work on mac" due to OpenGL 4.3 requirement (macOS caps at 4.1). However, arcnor's no-SFML branch and renderer abstraction work was aimed at enabling macOS support via alternative renderers.
- **Windows**: Primary development platform. Requires MSVC, Qt, NoesisSDK, SteamSDK.
- arcnor's vision: "my end goal was actually to change the renderer as well, so the game could be run on Mac and any other system"

---

## Sound System

- **condac** implemented the sound system
- Approach: "when a gnome starts a task it triggers the sound to be played"
- `playEffect()` checks if sound should be played based on range to the observer
- Range-based audio — sounds far from camera are quieter/suppressed

---

## Topic Distribution

| Topic | Thread Count |
|-------|-------------|
| UI / GUI Framework | 50 |
| Game Mechanics | 47 |
| Bug Fixes / Debugging | 46 |
| Git / Version Control | 43 |
| Database / Data Model | 42 |
| Build System / Compilation | 35 |
| Modding / Data Files | 33 |
| Architecture / Refactoring | 32 |
| Rendering / Graphics | 29 |
| Platform / Portability | 24 |
| Performance / Optimization | 20 |
| Save / Load | 17 |
| World Generation | 12 |
| Pathfinding / AI | 9 |
