#!/usr/bin/env python3
"""
Track C — global cozy palette remap.

For every tilesheet in content/tilesheet/, remap every pixel to its nearest
neighbour in a hand-curated "cozy" target palette (color distance computed
in CIE Lab so the matches are perceptual, not naive RGB). Write the result
to content/tilesheet_ai/<name>.png.

Preserves:
  - The magenta #FF00FF key color (kept verbatim — the engine masks on it)
  - Alpha channel (per-pixel)
  - Every sprite's exact shape (no pixel moves, no resampling)

Skips tilesheets that already have a Track-A procedural output
(seasonalgrass.png) unless --force is passed.

Usage:
    python3 tools/cozy_palette_remap.py            # remap everything
    python3 tools/cozy_palette_remap.py --force    # include seasonalgrass
    python3 tools/cozy_palette_remap.py gnomes.png # just one sheet
"""
import sys
import shutil
from functools import lru_cache
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parent.parent
SRC_DIR = REPO / "content" / "tilesheet"
DST_DIR = REPO / "content" / "tilesheet_ai"
BUILD_DST = REPO / "build" / "content" / "tilesheet_ai"

# Tilesheets that have a better dedicated generator (Track A). Skip by default.
TRACK_A_SHEETS = {"seasonalgrass.png"}


# --------------------------------------------------------------- cozy palette
#
# 40 colors covering skin, hair, wood, stone, metal, vegetation, cloth,
# fire, water, snow, shadow, highlight. Warm, muted, slightly desaturated,
# storybook-cozy. No pure #000 or #FFF — shadows go plum, highlights go
# warm ivory.
#
COZY_PALETTE = [
    # Deep warm shadows (replaces blacks / near-blacks)
    (28, 20, 38),     # deep plum
    (46, 32, 54),     # plum shadow
    (64, 44, 60),     # umber plum
    # Warm darks (replaces dark browns / greys)
    (72, 44, 34),     # walnut
    (96, 58, 42),     # cocoa
    (118, 76, 48),    # umber
    # Skin / peach
    (222, 168, 128),  # peach
    (240, 198, 150),  # soft peach
    (252, 220, 178),  # cream skin
    # Hair — warm blonde to chestnut
    (172, 112, 58),   # chestnut
    (198, 146, 68),   # honey
    (228, 188, 110),  # blonde
    # Wood — honey oak ramp
    (134, 82, 48),    # walnut wood
    (176, 118, 64),   # oak
    (210, 156, 92),   # honey wood
    (238, 198, 136),  # cream wood
    # Stone / sandstone (replaces cool greys)
    (96, 86, 92),     # warm pewter
    (134, 120, 116),  # taupe
    (176, 160, 144),  # sandstone
    (216, 200, 176),  # warm off-white
    # Metal — brass / copper / iron
    (82, 72, 82),     # iron
    (140, 110, 72),   # bronze
    (196, 148, 78),   # brass
    (224, 172, 100),  # polished brass
    # Vegetation — warm mossy greens
    (42, 64, 34),     # pine shadow
    (70, 104, 48),    # moss
    (112, 148, 62),   # grass
    (168, 188, 84),   # lime
    (218, 222, 134),  # highlight lime
    # Cloth — sage, rose, plum, terracotta, teal
    (148, 162, 124),  # sage
    (196, 118, 112),  # rose
    (124, 78, 108),   # plum cloth
    (204, 124, 80),   # terracotta
    (96, 132, 138),   # muted teal
    # Fire / warm accents
    (230, 128, 70),   # amber
    (244, 176, 94),   # gold
    # Water
    (72, 108, 128),   # deep water
    (128, 172, 180),  # shallow water
    # Snow / frost (replaces pure white)
    (220, 224, 232),  # pale frost
    (244, 242, 232),  # warm cream / ivory highlight
]


# ------------------------------------------------- perceptual color distance

def srgb_to_linear(c: float) -> float:
    c /= 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def linear_to_xyz(r: float, g: float, b: float):
    x = 0.4124564 * r + 0.3575761 * g + 0.1804375 * b
    y = 0.2126729 * r + 0.7151522 * g + 0.0721750 * b
    z = 0.0193339 * r + 0.1191920 * g + 0.9503041 * b
    return x, y, z


# D65 reference white
_XN, _YN, _ZN = 0.95047, 1.0, 1.08883


def _f(t: float) -> float:
    return t ** (1 / 3) if t > 0.008856 else (7.787 * t + 16 / 116)


def rgb_to_lab(rgb):
    r, g, b = (srgb_to_linear(c) for c in rgb)
    x, y, z = linear_to_xyz(r, g, b)
    fx, fy, fz = _f(x / _XN), _f(y / _YN), _f(z / _ZN)
    L = 116 * fy - 16
    a = 500 * (fx - fy)
    b2 = 200 * (fy - fz)
    return L, a, b2


COZY_LAB = [rgb_to_lab(c) for c in COZY_PALETTE]


def is_magenta(rgb):
    r, g, b = rgb
    return r > 240 and b > 240 and g < 16


@lru_cache(maxsize=65536)
def nearest_cozy(rgb):
    if is_magenta(rgb):
        return rgb  # preserve magenta key
    L1, a1, b1 = rgb_to_lab(rgb)
    best, best_d = COZY_PALETTE[0], float("inf")
    for cand, (L2, a2, b2) in zip(COZY_PALETTE, COZY_LAB):
        d = (L1 - L2) ** 2 + (a1 - a2) ** 2 + (b1 - b2) ** 2
        if d < best_d:
            best_d = d
            best = cand
    return best


# ------------------------------------------------------------------ remap

def remap_image(src_path: Path, dst_path: Path) -> int:
    img = Image.open(src_path).convert("RGBA")
    w, h = img.size
    pixels = img.load()
    changed = 0
    # Build unique-color cache locally (still share the @lru_cache across sheets)
    for y in range(h):
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < 16:
                continue
            new = nearest_cozy((r, g, b))
            if new != (r, g, b):
                pixels[x, y] = (*new, a)
                changed += 1
    dst_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(dst_path)
    return changed


def main():
    args = sys.argv[1:]
    force = "--force" in args
    args = [a for a in args if a != "--force"]
    targets = args  # optional specific filenames

    sheets = sorted(SRC_DIR.glob("*.png"))
    if targets:
        sheets = [s for s in sheets if s.name in targets]

    if not sheets:
        print("No tilesheets matched.")
        return

    print(f"Cozy palette: {len(COZY_PALETTE)} colors")
    print(f"Source:       {SRC_DIR}")
    print(f"Destination:  {DST_DIR}")
    print()

    for src in sheets:
        if src.name in TRACK_A_SHEETS and not force:
            print(f"  skip {src.name}  (Track-A sheet; pass --force to override)")
            continue
        dst = DST_DIR / src.name
        changed = remap_image(src, dst)
        build_dst = BUILD_DST / src.name
        if BUILD_DST.exists():
            shutil.copy2(dst, build_dst)
        print(f"  {src.name:28}  {changed:>8} px remapped  →  {dst.name}")

    hits = nearest_cozy.cache_info()
    print(f"\nColor cache: {hits}")


if __name__ == "__main__":
    main()
