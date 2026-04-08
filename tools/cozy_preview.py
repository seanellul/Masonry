#!/usr/bin/env python3
"""Quick before/after preview for a few representative cozy-remapped sheets."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
ORIG = REPO / "content" / "tilesheet"
ALT = REPO / "content" / "tilesheet_ai"
OUT = REPO / "tools" / "sprite_pipeline" / "inventory" / "cozy_preview.png"

# Interesting crops: (label, sheet, x, y, w, h)
CROPS = [
    ("gnomes",    "gnomes.png",    0, 0, 192, 216),
    ("workshops", "workshops.png", 0, 0, 256, 180),
    ("furniture", "furniture.png", 0, 0, 192, 144),
    ("animals",   "animals.png",   0, 0, 256, 144),
    ("terrain",   "terrain.png",   0, 0, 192, 180),
]

ZOOM = 4
PAD = 14
HEADER = 30

def demagenta(img):
    out = img.copy().convert("RGBA")
    px = out.load()
    for y in range(out.size[1]):
        for x in range(out.size[0]):
            r, g, b, a = px[x, y]
            if r > 240 and b > 240 and g < 16:
                px[x, y] = (0, 0, 0, 0)
    return out


def main():
    panels = []
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 14)
    except OSError:
        font = ImageFont.load_default()

    for label, sheet, x, y, w, h in CROPS:
        a = demagenta(Image.open(ORIG / sheet)).crop((x, y, x + w, y + h))
        b = demagenta(Image.open(ALT / sheet)).crop((x, y, x + w, y + h))
        a = a.resize((w * ZOOM, h * ZOOM), Image.NEAREST)
        b = b.resize((w * ZOOM, h * ZOOM), Image.NEAREST)

        pw = w * ZOOM
        ph = h * ZOOM
        panel = Image.new("RGBA", (PAD + 2 * pw + PAD + PAD, HEADER + ph + PAD),
                          (22, 22, 26, 255))
        draw = ImageDraw.Draw(panel)
        draw.text((PAD, 6), f"{label}  — original", fill=(220, 220, 220, 255), font=font)
        draw.text((PAD + pw + PAD, 6), f"{label}  — cozy pack", fill=(255, 220, 150, 255), font=font)
        panel.paste(a, (PAD, HEADER), a)
        panel.paste(b, (PAD + pw + PAD, HEADER), b)
        panels.append(panel)

    # Stack vertically
    total_w = max(p.size[0] for p in panels)
    total_h = sum(p.size[1] for p in panels) + (len(panels) - 1) * 8
    out = Image.new("RGBA", (total_w, total_h), (16, 16, 20, 255))
    y = 0
    for p in panels:
        out.paste(p, (0, y), p)
        y += p.size[1] + 8

    OUT.parent.mkdir(parents=True, exist_ok=True)
    out.save(OUT)
    print(f"Wrote {OUT}  ({out.size})")


if __name__ == "__main__":
    main()
