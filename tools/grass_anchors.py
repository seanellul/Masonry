#!/usr/bin/env python3
"""
Build a 6×4 reference image of just the Grass season-anchor cells, scaled up
for visual inspection. 6 grass variants (rows in the source) × 4 seasons.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
SHEET = REPO / "content" / "tilesheet" / "seasonalgrass.png"
OUT = REPO / "tools" / "sprite_pipeline" / "inventory" / "seasonalgrass" / "anchors.png"

SEASONS = [("Spring", 0), ("Summer", 4), ("Autumn", 7), ("Winter", 10)]
ROWS = list(range(1, 7))  # Grass1..Grass6

CELL_W, CELL_H = 32, 36
ZOOM = 6
PAD = 8
HEADER = 32
LABEL = 18

src = Image.open(SHEET).convert("RGBA")

cw, ch = CELL_W * ZOOM, CELL_H * ZOOM
img_w = HEADER + len(SEASONS) * (cw + PAD) + PAD
img_h = HEADER + len(ROWS) * (ch + LABEL + PAD) + PAD
out = Image.new("RGBA", (img_w, img_h), (20, 20, 24, 255))
draw = ImageDraw.Draw(out)
try:
    font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 11)
    big = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 16)
except OSError:
    font = ImageFont.load_default()
    big = font

for ci, (name, _) in enumerate(SEASONS):
    x = HEADER + ci * (cw + PAD) + cw // 2 - 25
    draw.text((x, 6), name, fill=(220, 220, 220, 255), font=big)

for ri, row in enumerate(ROWS):
    y_top = HEADER + ri * (ch + LABEL + PAD)
    draw.text((4, y_top + ch // 2 - 8), f"Grass{row}", fill=(220, 220, 220, 255), font=big)
    for ci, (_, col) in enumerate(SEASONS):
        x = col * CELL_W
        y = row * CELL_H
        cell = src.crop((x, y, x + CELL_W, y + CELL_H))
        cell_z = cell.resize((cw, ch), Image.NEAREST)
        px = HEADER + ci * (cw + PAD)
        out.paste(cell_z, (px, y_top), cell_z)
        draw.text((px, y_top + ch + 1), f"Grass_{row}_{col}",
                  fill=(160, 200, 255, 255), font=font)

OUT.parent.mkdir(parents=True, exist_ok=True)
out.save(OUT)
print(f"Wrote {OUT}  ({out.size})")
