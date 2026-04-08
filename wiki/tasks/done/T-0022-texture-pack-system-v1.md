---
id: T-0022
title: Texture Pack system v1 — multi-pack discovery, metadata, settings UI
type: feature
created: 2026-04-07
blockers: []
tags: [graphics, modding, settings, sprites]
---

## Description

Generalize the v0 single-hardcoded-path texture pack (`content/tilesheet_ai/` toggled by a bool) into a real Minecraft-style multi-pack system. Texture packs are the path to letting artists skin Masonry however they want and seeding a community modding/art layer.

### Goals

- **Pack discovery**: scan `content/texturepacks/` at startup; each subdirectory containing `pack.json` is a pack.
- **Pack metadata**: every pack has a `pack.json` with `id`, `name`, `author`, `version`, `description`, optional `preview` thumbnail.
- **Multi-pack load order**: config holds an ordered list of active pack IDs. Earlier packs in the list override later ones; missing files cascade down to the next pack and ultimately to the default `content/tilesheet/`.
- **Settings → Texture Packs tab**: a player-facing UI listing all discovered packs with enable/disable, reorder (↑/↓), preview thumbnail, name/author/description, and a clear "restart to apply" warning.
- **Migrate the v0 pack** at `content/tilesheet_ai/` into `content/texturepacks/ai/tilesheet/` with a `pack.json`.
- **Restart-required is fine** — same as v0. The runtime swap path was already proven unsafe.

### Both pack styles supported automatically

Texture packs replace files at known paths. Whether a pack ships **palette-friendly base atlases** (designed to tint well via the existing `createSprite("Chair", { material })` material composition) or **high-fidelity hand-painted atlases** (that look great without much tinting) is a choice the pack author makes. The engine doesn't care — it just loads `gnomes.png` (etc.) from the highest-priority pack that contains it. Same code path, different art philosophy.

### Acceptance criteria

- `content/texturepacks/ai/` exists with `pack.json` + `tilesheet/` (the migrated v0 pack); `content/tilesheet_ai/` is gone.
- A player who drops a new directory `content/texturepacks/foo/` with a `pack.json` + `tilesheet/foo.png` sees "foo" in the Settings → Texture Packs list without any other setup.
- Toggling a pack on, restarting, and reopening the game shows the pack's textures in-game.
- Multiple active packs in the list cascade per-file: pack A's `gnomes.png` wins over pack B's `gnomes.png`, but pack B's `terrain.png` wins if pack A doesn't ship one.
- Default `content/tilesheet/` is the bottom of the cascade.
- Config persists `activeTexturePacks` as an ordered list.
- "Restart to apply" warning visible whenever changes are pending.

### Out of scope

- Pack-author tooling (a GUI to build a pack).
- Pack download / browse / install from a community gallery.
- Per-file mods (e.g. "replace just this one chair sprite") below the atlas level.
- Hot-swap during play.

## Plan

1. **Pack model**: `struct TexturePackInfo { QString id, name, author, version, description, dirPath, previewPath; }`. Free functions `discoverTexturePacks()` and `getTexturePackPath(packID, filename)` in a small `texturepacks.h/cpp`.
2. **Migration**: `git mv content/tilesheet_ai content/texturepacks/ai/tilesheet`, create `content/texturepacks/ai/pack.json`.
3. **Config**: introduce `activeTexturePacks` (default `[]` = default only). Keep `useAltTextures` for one release as a back-compat shim — if true and the new key is unset, treat as `["ai"]`.
4. **`SpriteFactory::init()`**: replace the v0 single-alt-path branch with a generalized loop over `activeTexturePacks`, then fall back to `content/tilesheet/`. Per-file fallback per pack — same shape as v0 generalized to N packs.
5. **Settings UI**: new "Texture Packs" tab in `drawSettings()` (alongside Keybindings from T-0014). For each discovered pack: thumbnail (or placeholder), name + author + version, description, [active checkbox], [↑] [↓] reorder buttons. Shows the resolved load order at the bottom + restart warning.
6. **Persist + load**: settings UI writes back to `activeTexturePacks` config; SpriteFactory reads it on next startup.
7. **Documentation**: `content/texturepacks/README.md` with the pack format spec, expected filenames, and how the override chain works.

## Result

Implemented end-to-end. Texture packs now work like Minecraft: drop a directory in `content/texturepacks/<id>/`, restart, configure in Settings.

### Files added

- **`content/texturepacks/README.md`** — pack format spec, layout, file override semantics, two-style guidance (palette-friendly vs high-fidelity).
- **`content/texturepacks/ai/pack.json`** — metadata for the migrated v0 AI pack.
- **`content/texturepacks/ai/tilesheet/`** — 25 atlas PNGs, migrated via `git mv` from `content/tilesheet_ai/`.
- **`src/gfx/texturepacks.h`** — `TexturePackInfo` data model + free functions `discoverTexturePacks()` and `resolveTilesheetPath()`.
- **`src/gfx/texturepacks.cpp`** — implementation. Discovery scans `content/texturepacks/`, parses each `pack.json`, returns a sorted list. Resolver iterates active pack IDs and falls back to `content/tilesheet/`.

### Files modified

- **`src/gfx/spritefactory.cpp`**: `init()` now calls `resolveTilesheetPath(activePacks, tilesheet)` instead of the v0 hardcoded `tilesheet_ai/` branch. Reads `activeTexturePacks` from config; back-compat shim treats `useAltTextures: true` + missing list as `["ai"]` for one release.
- **`src/gui/ui/ui_mainmenu.cpp`**: new **Texture Packs** tab in `drawSettings()`. Lazy-loads the discovered pack list + active list once on tab open. Per-pack row: active checkbox, name in gold + version + author, ↑/↓ reorder buttons (only when active), wrapped description. Bottom row: load order summary (`pack-a → pack-b → default`), yellow "Restart to apply" warning when dirty, "Save" button that writes `activeTexturePacks` to config.

### How a pack author ships a new pack today

1. `mkdir content/texturepacks/myskin`
2. `mkdir content/texturepacks/myskin/tilesheet`
3. Drop your atlas PNGs in `tilesheet/` (any subset; missing files cascade)
4. Write a `pack.json` next to `tilesheet/`
5. Restart Masonry → it shows up in Settings → Texture Packs
6. Toggle on, restart, see it in-game

No build step, no engine changes, no recompile.

### Both pack styles work transparently

Per the design discussion: pack authors choose between palette-friendly base atlases (designed to tint via `createSprite("Chair", { material })`) and high-fidelity hand-painted atlases (that look great without much tinting). The engine doesn't distinguish — it just loads the highest-priority `gnomes.png` from the active pack chain. Same code path, different art philosophy. README explains both.

### Build

Green. New `texturepacks.cpp` picked up by the recursive CMake glob after re-configure.

### Out of scope (deferred to v2)

- In-game pack browser / install from a community gallery.
- Hot-swap during play (still unsafe).
- Per-file mods below the atlas level (e.g. "replace just one chair sprite").
- Pack-author tooling (a GUI to build a pack from screenshots).
- Pack signing / sandboxing.

### Known issues

- The reorder buttons use the active list's index — if the same pack appears twice in the active list, behavior would be ambiguous. The toggle path always appends/removes-all so duplicates can't actually arise, but worth noting.
- The settings tab caches the discovered pack list once per session. To pick up newly-dropped packs without restarting the whole game, you'd need to close and reopen the menu — and even then the cache is `static`, so it persists. A "Refresh" button is the obvious extension; deferred since pack authors who restart-after-add will see the change anyway.
