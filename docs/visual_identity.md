# Masonry Visual Identity Guide

## Brand Overview

Masonry is a Gnomoria-inspired colony simulator with isometric pixel art visuals. The visual identity should feel **medieval, underground, warm, and hand-built** — reflecting the game's core loop of mining stone, building structures, and managing a gnome colony.

## Logo System

### Icon (Logomark)
- **Element**: Isometric stone brick cube with the letter "M" carved/engraved into the front-left face
- **Accent**: Small pixel pickaxe leaning against the base
- **Background**: Subtle warm ambient glow on dark/black
- **File**: `generated_imgs/masonry_icon.png`
- **Use**: App icon, favicon, social media avatar, small-format branding

### Title Lockup (Wordmark + Icon)
- **Composition**: [M-cube] + "ASONRY" reads as "MASONRY"
- **The cube IS the M** — it replaces the first letter, linking icon and wordmark
- **Text style**: Front-facing bold pixel block letters, stone brick texture with mortar lines
- **File**: `generated_imgs/masonry_title_lockup.png`
- **Use**: Title screen, Steam capsule art, website header, marketing materials

### Standalone Title
- For contexts where the cube icon is too complex, use "MASONRY" in the same stone brick pixel font style without the cube substitution

## Color Palette

### Primary (Sandstone)
| Name | Hex (approx) | Use |
|------|-------------|-----|
| Sandstone Light | `#E8C878` | Brick highlights, top faces |
| Sandstone Mid | `#C8A050` | Primary brick fill |
| Sandstone Dark | `#A07830` | Brick shadows, side faces |
| Mortar | `#604020` | Mortar lines, deep crevices |
| Engraving | `#503018` | Carved text, deep shadows |

### UI Theme (Medieval — Default)
| Name | Hex (approx) | Use |
|------|-------------|-----|
| Background | `#1E1A14` | Window backgrounds |
| Frame | `#2E2618` | Input fields, panels |
| Accent Gold | `#BF9933` | Checkmarks, sliders, active elements |
| Text Parchment | `#EBE1C7` | Primary text |
| Text Disabled | `#8C8068` | Disabled/secondary text |
| Border | `#594830` | Panel borders |

### Background
| Name | Hex | Use |
|------|-----|-----|
| Void Black | `#000000` | Logo backgrounds, fog-of-war |
| Deep Teal | `#0A1510` | Scene vignette edges |

## Typography

- **Logo text**: Custom pixel block letters — each letter constructed from visible stone brick tiles with mortar lines
- **In-game UI**: System pixel font (loaded via ImGui), warm parchment color (`#EBE1C7`)
- **Style**: No anti-aliasing, visible pixels, 16-bit era aesthetic

## Art Style Guidelines

### Do
- Use **isometric perspective** for 3D elements (cubes, tiles, structures)
- Use **front-facing** for text/titles (better readability)
- Keep a **limited color palette** (5-6 shades per material)
- Maintain **visible pixels** — no smoothing or anti-aliasing
- Use **warm torchlight/amber** as the primary light source color
- Let scenes **fade to black** at edges (fog-of-war style)

### Don't
- Don't use photorealistic or hyper-detailed rendering
- Don't use gradients (except subtle torch glow)
- Don't include concept art or illustrated scenes that don't match in-game visuals
- Don't use cool/blue tones as primary (reserve for Nordic theme variant or ice/water)
- Don't add characters unless they match the actual in-game gnome sprite style

## Prompt Templates for Image Generation

### Icon Generation
```
Pixel art game icon. Isometric stone brick cube in warm golden-brown sandstone
with visible mortar lines. Letter "M" carved into the front face. Small pixel
pickaxe leaning against base. Subtle warm ambient glow. Pure black background.
Crisp 16-bit pixel art, visible pixels, limited warm palette, no anti-aliasing.
```

### Title Generation
```
Pixel art game logo on pure black background. [M-cube icon] followed by
"ASONRY" in bold front-facing pixel block letters with warm golden-brown stone
brick texture and dark mortar lines. 3D shadow on bottom-right of letters.
Crisp 16-bit pixel art, no anti-aliasing. Warm sandstone palette throughout.
```

### Style Reference Keywords
`pixel art, 16-bit, isometric, stone brick, sandstone, warm golden-brown,
dark mortar lines, medieval fantasy, colony sim, no anti-aliasing, visible pixels,
limited palette, black background, torch glow`

## File Inventory

| File | Description | Status |
|------|-------------|--------|
| `generated_imgs/masonry_icon.png` | Stone cube "M" icon with pickaxe | Current best |
| `generated_imgs/masonry_title_lockup.png` | Cube-M + ASONRY title (polished) | Current best |
| `generated_imgs/masonry_title_lockup_v1.png` | Cube-M + ASONRY title (pre-polish) | Backup |
| `content/icon.png` | Legacy gnome icon (Ingnomia era) | To be replaced |
| `generated_imgs/archive/` | Previous iterations | Reference only |
