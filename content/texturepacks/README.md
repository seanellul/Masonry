# Masonry Texture Packs

Drop a directory in here and Masonry will pick it up as a texture pack on next startup. Toggle packs on/off in **Settings → Texture Packs**.

## Layout

Each pack is a directory with this shape:

```
content/texturepacks/
  <pack-id>/
    pack.json           ← required: metadata
    preview.png         ← optional: 256×256 thumbnail shown in Settings
    tilesheet/          ← required: PNG atlases that override the defaults
      gnomes.png
      terrain.png
      furniture.png
      ...
```

The `<pack-id>` is the directory name. Keep it lowercase, no spaces, no punctuation.

## pack.json schema

```json
{
    "id": "ai",
    "name": "AI Generated",
    "author": "seanellul",
    "version": "0.1",
    "description": "Cozy AI-generated tilesheets for a softer, painterly aesthetic.",
    "preview": "preview.png"
}
```

| Field | Required | Notes |
|---|---|---|
| `id` | yes | Should match the directory name |
| `name` | yes | Player-facing display name |
| `author` | yes | Pack author or studio name |
| `version` | yes | Free-form, e.g. `"0.1"` or `"2024-04-07"` |
| `description` | recommended | One or two sentences explaining the style |
| `preview` | optional | Path (relative to the pack dir) to a thumbnail PNG |

## How the file override chain works

A pack only needs to ship the files it wants to override. **Missing files cascade through the active load order and ultimately fall back to the default `content/tilesheet/`**. So a "Fancy Gnomes" pack can ship one file (`gnomes.png`) and inherit everything else.

Example with two active packs (`fancy-gnomes` first, `winter` second):

| Atlas | `fancy-gnomes/tilesheet/` | `winter/tilesheet/` | `tilesheet/` (default) | Loaded from |
|---|---|---|---|---|
| `gnomes.png` | ✓ | | ✓ | fancy-gnomes |
| `terrain.png` | | ✓ | ✓ | winter |
| `furniture.png` | | | ✓ | default |

Earlier in the load order = higher priority. The Settings UI lets you reorder packs.

## Expected atlas filenames

These are the atlases the engine will look for. Ship any subset; missing ones cascade.

```
animals.png                  food_drink_ingredients.png   mushroom_biome_zygs.png       seasonalslopes.png
automatons.png               furniture.png                mushrooms.png                 terrain.png
default.png                  gnomes.png                   plants.png                    traps_mechanism.png
goblin.png                   magic.png                    seasonaldetails.png           weapons-armour-UI-large.png
mobs.png                     multicreatures.png           seasonalgrass.png             weapons_armour.png
multitrees.png               windmill-32x36.png           windmill.png                  workshops.png
```

## Two valid pack styles

Both work with the same file-replacement system — pack authors choose their philosophy:

### Palette-friendly base atlases

Ship base sprites designed to be **tinted at runtime** by the engine's material composition (`createSprite("Chair", { "Oak" })` reads the chair pixels and applies the oak material color). One chair sprite produces N visually-distinct chairs from N materials. Smaller pack file size, more variety per artist hour.

### High-fidelity hand-painted atlases

Ship more detailed, higher-quality art that **looks great without much tinting**. Larger pack file size, more authorial control over each tile, more uniform style. Material tinting still applies but contributes less because the base art is already distinctive.

You can also mix both within one pack — palette-friendly for furniture, hand-painted for gnomes, etc.

## Restart required

Texture pack changes take effect on the **next startup**. Runtime swap is unsafe (sprite cache, GL upload, and SpriteFactory mutex are all owned by the game thread). The Settings UI shows a yellow "Restart to apply" warning whenever there are pending changes.
