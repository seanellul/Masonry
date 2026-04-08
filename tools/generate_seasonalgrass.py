#!/usr/bin/env python3
"""
Procedurally generate content/tilesheet_ai/seasonalgrass.png.

Strategy:
  1. Use the ORIGINAL seasonalgrass.png as the shape source (alpha mask per
     cell) and as the palette source (unique colors per season).
  2. For each of the 24 ACTIVE cells (6 variants × 4 seasons at cols 0/4/7/10),
     generate a new dithered fill using 2D value noise keyed by (variant, season),
     mapping noise value → palette index sorted by luminance.
  3. Shading: blend the per-pixel noise with the source cell's own luminance so
     the tile retains the "darker at the rim, brighter on top" gradient that
     makes it read as an isometric diamond.
  4. Write the output to content/tilesheet_ai/seasonalgrass.png with the full
     416×1116 dimensions, magenta (#FF00FF) background, and every non-active
     cell left as pure magenta (the engine keys on magenta).

Usage:
    python3 tools/generate_seasonalgrass.py
"""
import math
import random
from pathlib import Path

from PIL import Image


# ---------------------------------------------------------------- palettes
#
# Hand-picked, intentionally distinct from the originals. Each palette is
# sorted darkest → brightest (5 colors = 5 luminance bands).
#
PALETTES = {
    "Spring": [
        (26, 64, 28),    # deep forest shadow
        (52, 112, 44),   # cool moss green
        (86, 164, 58),   # vivid grass
        (140, 196, 72),  # sunlit green
        (198, 226, 110), # highlight lime
    ],
    "Summer": [
        (58, 82, 30),    # dry shadow olive
        (98, 124, 42),   # warm olive
        (148, 166, 58),  # ochre-green
        (196, 200, 82),  # sun-bleached yellow-green
        (236, 226, 132), # straw highlight
    ],
    "Autumn": [
        (72, 40, 18),    # burnt umber shadow
        (136, 72, 28),   # rust
        (186, 112, 38),  # pumpkin
        (218, 156, 58),  # amber
        (244, 208, 108), # dry gold
    ],
    "Winter": [
        (120, 138, 174), # cold steel shadow
        (172, 190, 220), # icy blue
        (216, 228, 246), # pale frost
        (238, 246, 254), # snow white
        (255, 255, 255), # specular
    ],
}

# Accent "feature" pixels scattered sparsely inside each cell.
# (color, weight) pairs — higher weight = more frequent.
ACCENTS = {
    "Spring": [
        ((246, 226, 90), 1),    # yellow buttercup
        ((244, 146, 180), 1),   # pink flower
        ((200, 110, 200), 1),   # purple flower
    ],
    "Summer": [
        ((226, 196, 64), 2),    # dry tuft
        ((130, 80, 30), 1),     # twig
    ],
    "Autumn": [
        ((102, 30, 18), 2),     # dark leaf litter
        ((252, 220, 120), 1),   # fallen petal
    ],
    "Winter": [
        ((60, 92, 140), 1),     # blue ice crack
        ((255, 255, 255), 2),   # sparkle
    ],
}

REPO = Path(__file__).resolve().parent.parent
SRC = REPO / "content" / "tilesheet" / "seasonalgrass.png"
DST = REPO / "content" / "tilesheet_ai" / "seasonalgrass.png"

CELL_W, CELL_H = 32, 36
MAGENTA = (255, 0, 255, 255)

# Active cells: 6 variants × 4 seasons, with season→column mapping
VARIANTS = [1, 2, 3, 4, 5, 6]
SEASONS = [("Spring", 0), ("Summer", 4), ("Autumn", 7), ("Winter", 10)]


# -------------------------------------------------------------------- utils

def is_magenta(rgba):
    r, g, b, a = rgba
    return r > 240 and b > 240 and g < 16


def luma(rgb):
    r, g, b = rgb[:3]
    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def extract_cell(src, row, col):
    return src.crop((col * CELL_W, row * CELL_H,
                     col * CELL_W + CELL_W, row * CELL_H + CELL_H))


def cell_mask_and_palette(cell):
    """
    Return:
      mask: set of (x, y) opaque (non-magenta) pixels
      palette: list of unique RGB colors sorted darkest → brightest
      lum_map: { (x, y): normalized luminance in [0, 1] } within mask
    """
    px = cell.load()
    mask = set()
    colors = {}
    for y in range(CELL_H):
        for x in range(CELL_W):
            rgba = px[x, y]
            if is_magenta(rgba) or rgba[3] < 16:
                continue
            mask.add((x, y))
            rgb = rgba[:3]
            colors[rgb] = colors.get(rgb, 0) + 1

    palette = sorted(colors.keys(), key=luma)
    if not palette:
        return mask, [], {}

    # Normalize luminance within mask
    lums = {xy: luma(px[xy[0], xy[1]]) for xy in mask}
    lo, hi = min(lums.values()), max(lums.values())
    span = max(1.0, hi - lo)
    lum_map = {xy: (v - lo) / span for xy, v in lums.items()}
    return mask, palette, lum_map


# ------------------------------------------------------------ 2D value noise

def value_noise_grid(width, height, cell_size, seed):
    """
    Generate a smooth [0, 1] noise field by interpolating between random
    lattice points. Deterministic for a given seed.
    """
    rng = random.Random(seed)
    gw = width // cell_size + 2
    gh = height // cell_size + 2
    lattice = [[rng.random() for _ in range(gw)] for _ in range(gh)]

    def smooth(t):  # smoothstep
        return t * t * (3 - 2 * t)

    field = [[0.0] * width for _ in range(height)]
    for y in range(height):
        gy = y / cell_size
        y0 = int(gy)
        ty = smooth(gy - y0)
        for x in range(width):
            gx = x / cell_size
            x0 = int(gx)
            tx = smooth(gx - x0)
            a = lattice[y0][x0]
            b = lattice[y0][x0 + 1]
            c = lattice[y0 + 1][x0]
            d = lattice[y0 + 1][x0 + 1]
            top = a * (1 - tx) + b * tx
            bot = c * (1 - tx) + d * tx
            field[y][x] = top * (1 - ty) + bot * ty
    return field


def octave_noise(width, height, seed):
    """
    Chunky low-frequency value noise for painterly blob patches, plus a
    small high-frequency octave for per-pixel grain. Renormalized to [0,1].
    """
    a = value_noise_grid(width, height, cell_size=7, seed=seed)  # big blobs
    b = value_noise_grid(width, height, cell_size=3, seed=seed ^ 0x9E3779B1)
    out = [[0.0] * width for _ in range(height)]
    lo, hi = 1.0, 0.0
    for y in range(height):
        for x in range(width):
            v = 0.75 * a[y][x] + 0.25 * b[y][x]
            out[y][x] = v
            if v < lo:
                lo = v
            if v > hi:
                hi = v
    span = max(1e-6, hi - lo)
    for y in range(height):
        for x in range(width):
            out[y][x] = (out[y][x] - lo) / span
    return out


# ----------------------------------------------------------------- compose

def build_cell(template_cell, variant, season_name):
    """
    Use template ONLY for the alpha mask. Palette, noise and accents are all
    custom so the output reads as a distinctly different art style.
    """
    mask, _, lum_map = cell_mask_and_palette(template_cell)
    out = Image.new("RGBA", (CELL_W, CELL_H), MAGENTA)
    if not mask:
        return out
    px = out.load()

    palette = PALETTES[season_name]
    n = len(palette)

    seed = hash((variant, season_name, "grass-v2", 0xBEEF)) & 0xFFFFFFFF
    noise = octave_noise(CELL_W, CELL_H, seed)

    # Identify the outline (pixels adjacent to non-mask) so we can darken them.
    outline = set()
    for (x, y) in mask:
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            if (x + dx, y + dy) not in mask:
                outline.add((x, y))
                break

    for (x, y) in mask:
        # Blend minimal source luminance with heavy noise — the diamond shape
        # comes from the mask, the "3D feel" comes from a gentle vertical
        # gradient we add explicitly below.
        lum = lum_map[(x, y)]
        # Vertical gradient inside the mask: darker toward the bottom tip,
        # brighter toward the top. Keeps the diamond reading as a surface.
        ys = [yy for (xx, yy) in mask if xx == x]
        ymin, ymax = min(ys), max(ys)
        span = max(1, ymax - ymin)
        vgrad = 1.0 - (y - ymin) / span  # 1 at top, 0 at bottom

        t = 0.55 * vgrad + 0.35 * noise[y][x] + 0.10 * lum
        t = max(0.0, min(1.0, t))
        idx = int(t * (n - 1) + 0.5)
        idx = max(0, min(n - 1, idx))
        px[x, y] = (*palette[idx], 255)

    # Darken outline pixels one palette step toward shadow for a crisp edge.
    for (x, y) in outline:
        r, g, b, a = px[x, y]
        # find current palette index
        cur = (r, g, b)
        try:
            ci = palette.index(cur)
        except ValueError:
            ci = 1
        ni = max(0, ci - 1)
        px[x, y] = (*palette[ni], 255)

    # Sprinkle accents inside (but not on) the outline.
    rng = random.Random(seed ^ 0xFACE)
    accent_list = ACCENTS[season_name]
    accent_count = 3 + rng.randint(0, 3)  # 3-6 accent pixels per cell
    interior = [xy for xy in mask if xy not in outline]
    rng.shuffle(interior)
    weights = [w for _, w in accent_list]
    total_w = sum(weights)
    for xy in interior[:accent_count]:
        r = rng.uniform(0, total_w)
        acc = accent_list[0][0]
        cum = 0
        for color, w in accent_list:
            cum += w
            if r <= cum:
                acc = color
                break
        px[xy[0], xy[1]] = (*acc, 255)

    return out


def main():
    src = Image.open(SRC).convert("RGBA")
    print(f"Source: {SRC.name} {src.size}")

    out = Image.new("RGBA", src.size, MAGENTA)

    built = 0
    for variant in VARIANTS:
        for season_name, col in SEASONS:
            template = extract_cell(src, variant, col)
            new_cell = build_cell(template, variant, season_name)
            out.paste(new_cell, (col * CELL_W, variant * CELL_H))
            built += 1

    DST.parent.mkdir(parents=True, exist_ok=True)
    out.save(DST)
    print(f"Wrote {DST}")
    print(f"Generated {built} cells ({len(VARIANTS)} variants × {len(SEASONS)} seasons)")


if __name__ == "__main__":
    main()
